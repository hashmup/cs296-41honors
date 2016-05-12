#include <opencv2/core/core.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include "portaudio.h"

using namespace std;
using namespace cv;

const int X_SIZE = 640;
const int Y_SIZE = 480;
Mat input_img;
Mat hsv_skin_img= Mat(Size(X_SIZE,Y_SIZE),CV_8UC3);
Mat hsv_img, smooth_img;
int _max, _min;
double out_freq = 400.0;
double out_ampl = 1.0;
double out_phase = 0.0;

pthread_mutex_t mtx;

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

	area* a = (area*)data;
	int cnt = 0;
	long long _x = 0, _y = 0;
	const clock_t begin = clock();
//	printf("area %d %d %d %d\n", a->sx, a->ex, a->sy, a->ey);
	for(int y=a->sy; y<a->ey;y++)
	{
		for(int x=a->sx; x<a->ex; x++)
		{
			//printf("x %d, y %d\n", x, y);
			int index = hsv_img.step*y+(x*3);
			if(hsv_img.data[index+2] <= 40 ) //HSVでの検出
			{
				hsv_skin_img.data[index] = 255;
				hsv_skin_img.data[index+1] = 255;
				hsv_skin_img.data[index+2] = 255;
				_x += x;
				_y += y;
				cnt++;
				//printf("%lld, %lld\n", _x, _y);
			}
		}
	}
	if(a->sx == 0) {
		pthread_mutex_lock(&mtx);
		out_freq = 800 * ((double)cnt / (X_SIZE * Y_SIZE / 2.));
		printf("Freq: %lf\n", out_freq);
		pthread_mutex_unlock(&mtx);
	}
	else {
		pthread_mutex_lock(&mtx);
		out_ampl = 1.5 * ((double)cnt / (X_SIZE * Y_SIZE / 2.));
		printf("Ampl: %lf\n", out_ampl);
		pthread_mutex_unlock(&mtx);
	}
//	printf("a->sx : %d %lld %lld %d\n",a->sx, _x, _y, cnt);
	hand* h = (hand*)calloc(sizeof(hand), 1);
	h->size = cnt;
	h->center.x = (cnt!=0) ? (int)(_x / cnt) : 0;
	h->center.y = (cnt!=0) ? (int)(_y / cnt) : 0;
	//cout << "skin:" << float(clock() - begin) / CLOCKS_PER_SEC << endl;
	return h;
}


#define SAMPLE_RATE   (44100)
#define NUM_SECONDS 100

int buffersize = 5000;

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    char message[20];
}

paTestData;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/

static int patestCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) timeInfo;  //Prevent unused variable warnings.
    (void) statusFlags;
    (void) inputBuffer;

 //   printf("sending %ld, %f %f\n", framesPerBuffer, timeInfo -> currentTime, timeInfo -> outputBufferDacTime);
 pthread_mutex_lock(&mtx);
    for( i=0; i<framesPerBuffer; i++ )
    {
        out_phase += out_freq * M_PI * 2.0 / SAMPLE_RATE;
        double data = sin(out_phase) * out_ampl;
        *out++ = data;
        *out++ = data;
    }
pthread_mutex_unlock(&mtx);
    return paContinue;
}

static void StreamFinished( void* userData )
{
   paTestData *data = (paTestData *) userData;
   printf( "Stream Completed: %s\n", data->message );
}


void audio(hand* left, hand* right){
	printf("left size: %d center x: %d, y: %d\n", left->size, left->center.x, left->center.y);
	printf("right size: %d center x: %d, y: %d\n", right->size, right->center.x, right->center.y);
	//out_freq = 440 * (left->size / (X_SIZE * Y_SIZE / 2));
	//out_ampl = 2* (right->size / (X_SIZE * Y_SIZE / 2)) - 1;

}

int main(int argc, char *argv[])
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;

	VideoCapture cap(0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, X_SIZE);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, Y_SIZE);
	pthread_mutex_init(&mtx, NULL);
	if(!cap.isOpened())
	{
		printf("カメラが検出できませんでした");
		return -1;
	}

	namedWindow("Video", CV_WINDOW_AUTOSIZE);
	area left = (area){X_SIZE/2, 0, X_SIZE, Y_SIZE};
	area right = (area){0, 0, X_SIZE/2, Y_SIZE};


	  // printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n", SAMPLE_RATE, FRAMES_PER_BUFFER);

	    err = Pa_Initialize();
	    if( err != paNoError ) goto error;

	    outputParameters.device = Pa_GetDefaultOutputDevice();
	    if (outputParameters.device == paNoDevice) {
	      fprintf(stderr,"Error: No default output device.\n");
	      goto error;
	    }
	    outputParameters.channelCount = 2;
	    outputParameters.sampleFormat = paFloat32;
	    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	    outputParameters.hostApiSpecificStreamInfo = NULL;

	    err = Pa_OpenStream(
		      &stream,
		      NULL,
		      &outputParameters,
		      SAMPLE_RATE,
		      buffersize,
		      paClipOff,
		      patestCallback,
		      &data );
	    if( err != paNoError ) goto error;

	    sprintf( data.message, "No Message" );
	    err = Pa_SetStreamFinishedCallback( stream, &StreamFinished );
	    if( err != paNoError ) goto error;

	    err = Pa_StartStream( stream );
	    if( err != paNoError ) goto error;

	    goto noerror;

	error:
	    Pa_Terminate();
	    fprintf( stderr, "An error occured while using the portaudio stream\n" );
	    fprintf( stderr, "Error number: %d\n", err );
	    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
	    return err;

	noerror:

		while(1) {
			hsv_skin_img = Scalar(0, 0, 0);
			cap >> input_img;
			const clock_t begin = clock();
			medianBlur(input_img,smooth_img,7);
			cvtColor(smooth_img,hsv_img,CV_BGR2HSV);
			pthread_t thread1, thread2;

			pthread_create(&thread1, NULL, worker, (void*)&left);
			pthread_create(&thread2, NULL, worker, (void*)&right);
			void* rres, *lres;
			pthread_join(thread1, &rres);
			pthread_join(thread2, &lres);
//			audio((hand*)rres, (hand*)lres);
			imshow("Video", hsv_skin_img);
//		    	cout << float(clock() - begin) / CLOCKS_PER_SEC << endl;
			if(waitKey(30) >=0)
			{
				break;
			}
		}


	    err = Pa_StopStream( stream );
	    if( err != paNoError ) goto error;

	    err = Pa_CloseStream( stream );
	    if( err != paNoError ) goto error;

	    Pa_Terminate();
	    printf("Test finished.\n");
			pthread_mutex_destroy(&mtx);
	    return 0;
}
