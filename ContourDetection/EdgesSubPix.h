#ifndef __EDGES_SUBPIX_H__
#define __EDGES_SUBPIX_H__
#include <opencv2/opencv.hpp>
#include <vector>

struct Contour
{
	std::vector<cv::Point2f> points;  // edge location
	std::vector<float> direction;     // direction of the gradient in edge point, 
									  // starting from y axis, counter-clockwise

	//std::vector<float> angles;
	//std::vector<float> slope_k;

	std::vector<float> response;      // amptitude of the gradient in edge point


};
// gray             - only support 8-bit grayscale
// hierarchy, mode  - have the same meanings as in cv::findContours
CV_EXPORTS void EdgesSubPix(cv::Mat &gray, double alpha, int low, int high,
	std::vector<Contour> &contours, cv::OutputArray hierarchy,
	int mode);

// mode = RETR_LIST
CV_EXPORTS void EdgesSubPix(cv::Mat &gray, double alpha, int low, int high,
	std::vector<Contour> &contours);

#endif // __EDGES_SUBPIX_H__
