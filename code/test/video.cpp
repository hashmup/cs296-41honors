#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
using namespace std;
using namespace cv;

const int X_SIZE = 640;
const int Y_SIZE = 480;
Mat input_img;
Mat hsv_skin_img= Mat(Size(X_SIZE,Y_SIZE),CV_8UC3);

int _max, _min;

typedef struct _area {
	int sx, sy, ex, ey;
} area;

typedef struct _point {
	int x, y;
} point;

typedef struct _hand {
	int size;
	point center;
} hand;

void* worker(void* data){
	Mat hsv_img, smooth_img;
	area* a = (area*)data;
	int cnt = 0;
	long long _x = 0, _y = 0;
	const clock_t begin = clock();
	medianBlur(input_img,smooth_img,7);
	cvtColor(smooth_img,hsv_img,CV_BGR2HSV);
	printf("area %d %d %d %d\n", a->sx, a->ex, a->sy, a->ey);
	for(int y=a->sy; y<a->ey;y++)
	{
		for(int x=a->sx; x<a->ex; x++)
		{
			int a = hsv_img.step*y+(x*3);
			if(hsv_img.data[a] >=0 && hsv_img.data[a] <=15 &&hsv_img.data[a+1] >=50 && hsv_img.data[a+2] >= 50 ) //HSVでの検出
			{
				hsv_skin_img.data[a] = 255;
				hsv_skin_img.data[a+1] = 255;
				hsv_skin_img.data[a+2] = 255;
				_x += x;
				_y += y;
				cnt++;
			}
		}
	}
	hand* h = (hand*)calloc(sizeof(hand), 1);
	h->size = cnt;
	h->center.x = (cnt) ? (int)(_x / cnt) : 0;
	h->center.y = (cnt) ? (int)(_y / cnt) : 0;
	cout << "skin:" << float(clock() - begin) / CLOCKS_PER_SEC << endl;
	return h;
}

void audio(hand* left, hand* right){
	printf("left size: %d center x: %d, y: %d\n", left->size, left->center.x, left->center.y);
	printf("right size: %d center x: %d, y: %d\n", right->size, right->center.x, right->center.y);
}

int main(int argc, char *argv[])
{
	VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, X_SIZE);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, Y_SIZE);
	if(!cap.isOpened())
	{
		printf("カメラが検出できませんでした");
		return -1;
	}

	namedWindow("Video", CV_WINDOW_AUTOSIZE);
	area left = (area){X_SIZE/2, 0, X_SIZE, Y_SIZE};
	area right = (area){0, 0, X_SIZE/2, Y_SIZE};
	while(1)
	{
		hsv_skin_img = Scalar(0, 0, 0);
		cap >> input_img;
    const clock_t begin = clock();
		pthread_t thread1, thread2;

		pthread_create(&thread1, NULL, worker, (void*)&left);
		pthread_create(&thread2, NULL, worker, (void*)&right);
		void* rres, *lres;
		pthread_join(thread1, &rres);
		pthread_join(thread2, &lres);
		audio((hand*)rres, (hand*)lres);
		imshow("Video", hsv_skin_img);
    cout << float(clock() - begin) / CLOCKS_PER_SEC << endl;
		if(waitKey(30) >=0)
		{
			break;
		}
	}
}
