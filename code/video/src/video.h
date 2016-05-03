#ifndef VIDEO_H
#define VIDEO_H
#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "portaudio.h"
#include <time.h>

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <string.h>

using namespace std;
using namespace cv;

typedef struct {
  double x;
  double y;
} point;

point meanxy2d(double** X, int rows, int cols);
double* polarsample(double** X, int rows, int cols,double theta, double x, double y, int r);
double** polar_transform(double** X, int NX, int MX,double* angle, int Nangle, int r, double cx, double cy);
double* read_channel_from_mat(Mat& src, int ch);
double* read_mat_from_channels(Mat& dst, double* r, double* g, double* b);
void mat2d_from_mat1d(double** dst, double* src, int rows, int cols);
void mat1d_from_mat2d(double* dst, double** src, int rows, int cols);
double** malloc2d(int rows, int cols);
void free2d(double** dst, int rows);
void detectAndDisplay( Mat frame, Mat background, double &out_freq);
#endif
