#include <opencv2/opencv.hpp>
using namespace cv;


// set rect in mat
int set_rect_in_mat(Rect& r, const Mat& src);

void set_Corner(Mat& image, const Scalar& value, int corner_tl, int corner_tr, int corner_br, int corner_bl);

void set_Margin(Mat& image, const Scalar& value, int top, int bottom, int left, int right);

bool isPointInRect(Point P, Rect rect);