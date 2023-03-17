#include <filesystem>
#include <fstream>
#include <iostream>
#include <array>

#include "Helpers.h"

#include<opencv2/core.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>



std::vector<float> Helpers::loadImage(const std::string & filename, int width, int height,bool Normalize, int image_shape_mdoe)
{
	cv::Mat image = cv::imread(filename);
	if (image.empty()) {
		std::cout << "No image found.";
	}

	// convert from BGR to RGB
	cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

	// resize
	cv::resize(image, image, cv::Size(width, height));

	// reshape to 1D
	image = image.reshape(1, 1);

	// uint_8, [0, 255] -> float, [0, 1]
	// Normalize number to between 0 and 1
	// Convert to vector<float> from cv::Mat.
	std::vector<float> vec;
	if(Normalize)
	{
		image.convertTo(vec, CV_32FC1, 1. / 255);
	}
	else
	{
		image.convertTo(vec, CV_32FC1, 1);
	}

	if(image_shape_mdoe == H_W_C)
	{
		return vec;
	}
	else if(image_shape_mdoe == C_H_W)
	{
		// Transpose (Height, Width, Channel)(224,224,3) to (Chanel, Height, Width)(3,224,224)
		std::vector<float> output;
		for (size_t ch = 0; ch < 3; ++ch) {
			for (size_t i = ch; i < vec.size(); i += 3) {
				output.emplace_back(vec[i]);
			}
		}
		return output;
	}
	
}

std::vector<std::string> Helpers::loadLabels(const std::string & filename)
{
	std::vector<std::string> output;

	std::ifstream file(filename);
	if (file) {
		std::string s;
		while (getline(file, s)) {
			output.emplace_back(s);
		}
		file.close();
	}

	return output;
}
