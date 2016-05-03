#include "video.h"

point meanxy2d(double** X, int rows, int cols) {
  point ret;
  ret.x = 0;
  ret.y = 0;
  double sum = 0;
  for(int i = 0; i < rows; i ++)
    for(int j = 0; j < cols; j ++)
      sum += X[i][j];
  for(int i = 0; i < rows; i ++)
    for(int j = 0; j < cols; j ++) {
      ret.y += i * X[i][j];
      ret.x += j * X[i][j];
    }
  ret.x /= sum;
  ret.y /= sum;
  return ret;
}

double* polarsample(double** X, int rows, int cols,
    double theta, double x, double y, int r) {
  double* v = (double*)calloc(r, sizeof(double));
  double dx = sin(theta);
  double dy = cos(theta);
  for(int i = 0; i < r; i ++) {
    int ix = round(x + dx * i);
    int iy = round(y - dy * i);
    if(ix >= 0 && ix < cols && iy >= 0 && iy < rows)
      v[i] = X[iy][ix];
    else
      v[i] = 0;
  }
  return v;
}

double** polar_transform(double** X, int NX, int MX,
    double* angle, int Nangle, int r, double cx, double cy) {
  double** ret = (double**)calloc(Nangle, sizeof(double*));
  double maxval = 0;
  for(int i = 0; i < Nangle; i ++) {
    ret[i] = polarsample(X, NX, MX, angle[i], cx, cy, r);
    for(int j = 0; j < r; j ++) {
      ret[i][j] *= (double)j * j / r / r;
      if(ret[i][j] > maxval)
        maxval = ret[i][j];
    }
  }
  for(int i = 0; i < Nangle; i ++) {
    for(int j = 0; j < r; j ++) {
      ret[i][j] /= maxval;
      if(ret[i][j] < 0.3)
        ret[i][j] = 0;
    }
  }
  return ret;
}


string string_from_int(int x) {
    std::stringstream ss;
    ss << x;
    std::string str = ss.str();
    return str;
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

void mat2d_from_mat1d(double** dst, double* src, int rows, int cols) {
  for(int i = 0; i < rows; i ++)
    for(int j = 0; j < cols; j ++)
      dst[i][j] = src[i + j * rows];
}

void mat1d_from_mat2d(double* dst, double** src, int rows, int cols) {
  for(int i = 0; i < rows; i ++)
    for(int j = 0; j < cols; j ++)
      dst[i + j * rows] = src[i][j];
}

double** malloc2d(int rows, int cols) {
  double** ret = (double**)calloc(rows, sizeof(double*));
  for(int i = 0; i < rows; i ++)
    ret[i] = (double*)calloc(cols, sizeof(double));
  return ret;
}

void free2d(double** dst, int rows) {
  for(int i = 0; i < rows; i ++)
    free(dst[i]);
  free(dst);
}

/** @function main */

/** @function detectAndDisplay */
void detectAndDisplay( Mat frame, Mat background, double &out_freq)
{
    Mat smooth;
    Mat HSV;

    GaussianBlur(frame, smooth, Size(11,11), 3, 3);
    double* bg_r = read_channel_from_mat(background, 2);
    double* bg_g = read_channel_from_mat(background, 1);
    double* bg_b = read_channel_from_mat(background, 0);

    int rc = background.rows * background.cols;

    double* frame_r = read_channel_from_mat(smooth, 2);
    double* frame_g = read_channel_from_mat(smooth, 1);
    double* frame_b = read_channel_from_mat(smooth, 0);

    for(int i = 0; i < rc; i ++) {
      double diff = (fabs(frame_r[i] - bg_r[i]) + fabs(frame_g[i] - bg_g[i]) + fabs(frame_b[i] - bg_b[i])) / 3 / 255;
      frame_r[i] = (diff > 0.2) * diff * 255;
      frame_g[i] = (diff > 0.2) * diff * 255;
      frame_b[i] = (diff > 0.2) * diff * 255;
    }

    int rows = background.rows;
    int cols = background.cols;
    // printf("row%d col%d\n", rows, cols);
    double** object = (double**)malloc2d(rows, cols);
    mat2d_from_mat1d(object, frame_r, rows, cols);
    point center = meanxy2d(object, rows, cols);

    int nangle = 100;
    double* angle_left = (double*)calloc(nangle, sizeof(double));
    for(int i = 0; i < nangle; i ++)
      angle_left[i] = 0.15 * M_PI + (0.7 - 0.15) * M_PI * i / nangle;
    double** polar_left = polar_transform(object, rows, cols, angle_left, nangle, rows / 2, center.x, center.y);
    point hand_left = meanxy2d(polar_left, nangle, rows / 2);

    // printf("left: angle = %f rad, radius = %f px\n", 0.15 * M_PI + (0.7 - 0.15) * M_PI * hand_left.y / nangle, hand_left.x);
    out_freq = 440 * pow(2, (hand_left.y / nangle) * 2);
    //printf("%f\n", out_freq);

    free(angle_left);
    free2d(polar_left, nangle);
//    imwrite("capture.png", frame_2);
    read_mat_from_channels(smooth, frame_r, frame_g, frame_b);

    free2d(object, background.rows);
    free(bg_r);
    free(bg_g);
    free(bg_b);
    free(frame_r);
    free(frame_g);
    free(frame_b);
    //-- Show what you got
    imshow("Video", smooth);
}
