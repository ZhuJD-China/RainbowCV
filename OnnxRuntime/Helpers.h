#pragma once
#include "opencv2/imgproc.hpp"

using namespace cv;

#define H_W_C 1
#define C_H_W 2

class Helpers
{
public:
	static std::vector<float> loadImage_lists;
	static std::vector<std::string> loadLabels_lists;

	static std::vector<float> loadImage(const std::string & filename, int width, int height, bool Normalize, int image_shape_Transpose);
	static std::vector<std::string> loadLabels(const std::string& filename);

};

