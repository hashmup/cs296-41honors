#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <pthread.h>
using namespace std;
using namespace cv;

String face_cascade_name = "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml";
CascadeClassifier face_cascade;
Ptr<BackgroundSubtractor> pMOG2;
vector<Rect> detectFace(Mat frame){
	vector<Rect> faces;
	Mat frame_gray;

	cvtColor(frame, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);
	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
	for( size_t i = 0; i < faces.size(); i++ )
  {
    Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
    ellipse( frame, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );
  }
  //-- Show what you got
  // imshow("Face", frame );
	return faces;
}

double* read_channel_from_mat(Mat& src, int ch) {
  double* ret = (double*)calloc(src.rows * src.cols, sizeof(double));
  unsigned char *raw = (unsigned char*)(src.data);
  int channel = src.channels();
  for(int i = 0; i < src.rows; i ++)
    for(int j = 0; j < src.cols; j ++)
      ret[i + j * src.rows] = raw[i * src.step + j * channel + ch];
  return ret;
}

double* read_mat_from_channels(Mat& dst, double* r, double* g, double* b) {
  unsigned char *raw = (unsigned char*)(dst.data);
  int channel = dst.channels();
  for(int i = 0; i < dst.rows; i ++)
    for(int j = 0; j < dst.cols; j ++) {
      raw[i * dst.step + j * channel + 0] = b[i + j * dst.rows];
      raw[i * dst.step + j * channel + 1] = g[i + j * dst.rows];
      raw[i * dst.step + j * channel + 2] = r[i + j * dst.rows];
    }
}

// Mat detectAndDisplay( Mat frame, Mat background)
Mat detectAndDisplay( Mat frame, Mat background)
{
    Mat smooth;
    Mat diff, dst, gray;
		float th = 30.0f;
    // GaussianBlur(frame, smooth, Size(11,11), 3, 3);
    absdiff(frame, background ,dst);
		cvtColor(dst, gray, CV_RGB2GRAY);
		// Ptr<BackgroundSubtractor> tmp = Ptr<BackgroundSubtractor>(pMOG2);
		// pMOG2->apply(frame, gray);
		// pMOG2 = tmp;
		// Mat bg;
		// pMOG2->getBackgroundImage(bg);
		// imshow("diff", bg);
    //-- Show what you got
		threshold(gray, dst, th, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
		return dst;
}

Mat getBackground(vector<Mat> bg, int numbg){
	Mat background, bgblur;
	int rc = bg[0].rows * bg[0].cols;
	double* bg_avg_r = (double*)calloc(rc, sizeof(double));
	double* bg_avg_g = (double*)calloc(rc, sizeof(double));
	double* bg_avg_b = (double*)calloc(rc, sizeof(double));
	for(int i = 0; i < numbg; i ++) {
		double* bg_ir = read_channel_from_mat(bg[i], 2);
		double* bg_ig = read_channel_from_mat(bg[i], 1);
		double* bg_ib = read_channel_from_mat(bg[i], 0);
		for(int j = 0; j < rc; j ++) {
			bg_avg_r[j] += bg_ir[j];
			bg_avg_g[j] += bg_ig[j];
			bg_avg_b[j] += bg_ib[j];
		}
		free(bg_ib);
		free(bg_ig);
		free(bg_ir);
	}

	for(int j = 0; j < rc; j ++) {
		bg_avg_r[j] /= numbg;
		bg_avg_g[j] /= numbg;
		bg_avg_b[j] /= numbg;
	}
	background = bg[0];
	read_mat_from_channels(background, bg_avg_r, bg_avg_g, bg_avg_b);
	free(bg_avg_r);
	free(bg_avg_g);
	free(bg_avg_b);

	// GaussianBlur(background, bgblur, Size(11,11), 3, 3);
	// imwrite("bg.png", bgblur);
	return background;
}

// Mat merge(Mat hsv_skin_img, Mat diff,vector<Rect> faces){
Mat merge(Mat hsv_skin_img,vector<Rect> faces){
	for(int i=0;i<faces.size();i++){
//		Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
//	  ellipse( hsv_skin_img, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 0, 0, 0 ), CV_FILLED, 8, 0 );
//			Point topRight(faces[i].x + faces[i].width, faces[i].y + faces[i].height);
//			Point bottomLeft(faces[i].x + faces[i].width, faces[i].y + faces[i].height;)
			rectangle(hsv_skin_img, Point (faces[i].x - 0.15*faces[i].width, faces[i].y - 0.15*faces[i].height), Point (faces[i].x + faces[i].width + 0.15*faces[i].width, faces[i].y + faces[i].height + 0.15*faces[i].height), Scalar(0,0,0), CV_FILLED, 8, 0 );
	}
	return hsv_skin_img;
}

pthread_mutex_t mtx;
vector<Rect> faces;
Mat input_img;
Mat hsv_skin_img= Mat(Size(640,480),CV_8UC3);
void* faceWorker(void* data){
	const clock_t begin = clock();
	Mat frame_gray;
	pthread_mutex_lock(&mtx);
	cvtColor(input_img, frame_gray, CV_BGR2GRAY);
	pthread_mutex_unlock(&mtx);
	equalizeHist(frame_gray, frame_gray);
	face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
	cout << "face" << float(clock() - begin) / CLOCKS_PER_SEC << endl;
}

void* skinWorker(void* data){
	const clock_t begin = clock();
	Mat hsv_img, smooth_img;
	pthread_mutex_lock(&mtx);
	medianBlur(input_img,smooth_img,7);
	pthread_mutex_unlock(&mtx);
	cvtColor(smooth_img,hsv_img,CV_BGR2HSV);
	for(int y=0; y<480;y++)
	{
		for(int x=0; x<640; x++)
		{
			int a = hsv_img.step*y+(x*3);
			if(hsv_img.data[a] >=0 && hsv_img.data[a] <=15 &&hsv_img.data[a+1] >=50 && hsv_img.data[a+2] >= 50 ) //HSVでの検出
			{
				hsv_skin_img.data[a] = 255;
				hsv_skin_img.data[a+1] = 255;
				hsv_skin_img.data[a+2] = 255;
			}
		}
	}
	cout << "skin:" << float(clock() - begin) / CLOCKS_PER_SEC << endl;
}

int main(int argc, char *argv[])
{
	VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	if(!cap.isOpened())
	{
		printf("カメラが検出できませんでした");
		return -1;
	}

	if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
	// int numbg = 30;
	// vector<Mat> bg = vector<Mat>(numbg);
	//
	// for(int i=0;i<numbg;i++){
	// 	cap >> bg[i];
	// }
	// cout << "test" << endl;
	// Mat background = getBackground(bg, numbg);
	// // pMOG2 = createBackgroundSubtractorMOG2();
	// cout << "test" << endl;
	// namedWindow("input_img", CV_WINDOW_AUTOSIZE);
	// namedWindow("hsv_skin_img", CV_WINDOW_AUTOSIZE);
	namedWindow("Video", CV_WINDOW_AUTOSIZE);
	// namedWindow("diff", CV_WINDOW_AUTOSIZE);
	// namedWindow("Face", CV_WINDOW_AUTOSIZE);
	pthread_mutex_init(&mtx, NULL);
	while(1)
	{
		hsv_skin_img = Scalar(0, 0, 0);
		faces.clear();
		cap >> input_img;
    const clock_t begin = clock();
		pthread_t thread1, thread2;
		pthread_create(&thread1, NULL, faceWorker, NULL);
		pthread_create(&thread2, NULL, skinWorker, NULL);
		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);
		// imshow("input_img",input_img);
		// imshow("hsv_skin_img",hsv_skin_img);
		// Mat diff = detectAndDisplay(input_img, bgblur);
		// Mat diff = detectAndDisplay(input_img, background);
		Mat merged = merge(hsv_skin_img, faces);
		imshow("Video", merged);
    cout << float(clock() - begin) / CLOCKS_PER_SEC << endl;
		if(waitKey(30) >=0)
		{
			break;
		}
	}
	pthread_mutex_destroy(&mtx);
}
