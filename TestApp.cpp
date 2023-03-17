#include <iostream>
#include <direct.h>
#include <future>
#include <numeric>
#include <windows.h>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>

#include "RainbowCV/ContourDetection/EdgesSubPix.h"
#include "RainbowCV/GUI/cvui.h"
#include "RainbowCV/GUI/EnhancedWindow.h"

#include "RainbowCV/Matting/BackgroundMatting/bayesian.h"
#include "RainbowCV/ThreadPool/ThreadPool.h"

#include "RainbowCV/Matting/BackgroundMatting/globalmatting.h"
#include "RainbowCV/Matting/BackgroundMatting/guidedfilter.h"
#include "RainbowCV/Matting/BackgroundMatting/sharedmatting.h"

#include <onnxruntime_cxx_api.h>
#include <opencv2/ximgproc/edge_drawing.hpp>

#include "RainbowCV/AdaptiveThreshold/AdaptiveIntegralThresh.h"
#include "RainbowCV/EdgeDrawing//EDCircles.h"
#include "RainbowCV/OnnxRuntime/Helpers.h"
#include "RainbowCV/OnnxRuntime/yolov5/yolov5_seg_onnx.h"
#include "RainbowCV/OnnxRuntime/yolov8/yolov8_onnx.h"
#include "RainbowCV/OnnxRuntime/yolov8/yolov8_seg_onnx.h"

#include "RainbowCV/ImageBasic/ImgBasic.h"

using namespace std;
using namespace cv;

static string CURRENT_PATH;

void Bibble_sort(vector<double>& a, int n, int& change_times)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = i + 1; j < n; j++)
		{
			if (a[i] > a[j])
			{
				int temp = a[i];
				a[i] = a[j];
				a[j] = temp;
				change_times++;
			}
		}
	}
}


void insertionSort(vector<double>& a, int n, int& change_times)
{
	for (int i = 1; i < n; i++)
	{
		int j = i - 1;
		int temp = a[i];
		while (j >= 0 && a[j] > temp)
		{
			a[j + 1] = a[j];
			j--;
		}
		a[j + 1] = temp;
	}
}


void EdgesSubPix_TEST()
{
	vector<Contour> contours;
	vector<Vec4i> hierarchy;
	Mat colorImage;
	Mat src_img;
	src_img = imread(CURRENT_PATH + "\\images\\YL.BMP", IMREAD_GRAYSCALE);
	cvtColor(src_img, colorImage, COLOR_GRAY2BGR);

	int64 t0 = getCPUTickCount();
	if (src_img.data != nullptr)
	{
		EdgesSubPix(src_img, 1.0, 5, 20, contours, hierarchy, 1);
	}
	else
		return;

	int64 t1 = getCPUTickCount();
	cout << "execution time is " << (t1 - t0) / getTickFrequency() << " seconds" << endl;

	vector<vector<Point>> ocvContours;
	for (size_t i = 0; i < contours.size(); ++i)
	{
		Mat pts(contours[i].points);
		vector<Point> c;
		pts.convertTo(c, CV_32S);
		ocvContours.push_back(std::move(c));
	}
	sort(ocvContours.begin(), ocvContours.end(),
	     [&](vector<Point>& p1, vector<Point>& p2)
	     {
		     return (p1.size() > p2.size());
	     });


	drawContours(colorImage, ocvContours, -1, Scalar(0, 255, 0));

	Mat mask = Mat::zeros(src_img.rows, src_img.cols, CV_8U);
	for (int index = 0; index < MIN(1, ocvContours.size()); index++)
	{
		drawContours(mask, ocvContours, index, Scalar(255, 255, 255), -1);
	}
}

void GUI_TEST()
{
	string WINDOW_NAME = "CVUI Ehnanced UI Canny Edge";

	Mat fruits = imread(CURRENT_PATH + "\\images\\bear.jpg");
	Mat frame = fruits.clone();
	int low_threshold = 50, high_threshold = 150;
	bool use_canny = false;

	// Create a settings window using the EnhancedWindow class.
	EnhancedWindow settings(0, 0, fruits.size().width / 3, fruits.size().height / 3, "Settings");

	// Init cvui and tell it to create a OpenCV window, i.e. cv::namedWindow(WINDOW_NAME).
	cvui::init(WINDOW_NAME);

	while (true)
	{
		// Should we apply Canny edge?
		if (use_canny)
		{
			// Yes, we should apply it.
			cvtColor(fruits, frame, COLOR_BGR2GRAY);
			Canny(frame, frame, low_threshold, high_threshold, 3);
			cvtColor(frame, frame, COLOR_GRAY2BGR);
		}
		else
		{
			// No, so just copy the original image to the displaying frame.
			fruits.copyTo(frame);
		}

		// Render the settings window and its content, if it is not minimized.
		settings.begin(frame);
		if (!settings.isMinimized())
		{
			cvui::checkbox("Use Canny Edge", &use_canny);
			cvui::trackbar(settings.width() - 20, &low_threshold, 5, 150);
			cvui::trackbar(settings.width() - 20, &high_threshold, 80, 300);
			cvui::space(20); // add 20px of empty space
			cvui::text("Drag and minimize this settings window", 0.4, 0xff0000);
		}
		settings.end();

		// Update all cvui internal stuff, e.g. handle mouse clicks, and show
		// everything on the screen.
		cvui::imshow(WINDOW_NAME, frame);

		// Check if ESC was pressed
		if (waitKey(30) == 27)
		{
			break;
		}
	}
}

void EdgesSubPix_TEST_GUI()
{
	string WINDOW_NAME = "EdgesSubPix_TEST_GUI";

	vector<Contour> contours;
	vector<Vec4i> hierarchy;

	Mat fruits = imread(CURRENT_PATH + "\\images\\small_Missingangle\\sub\\left_bottom.png");
	resize(fruits, fruits, Size(1024, 1024));
	Mat frame = fruits.clone();

	bool use_EdgesSubPix = false;
	int low_threshold = 20, high_threshold = 40;

	EnhancedWindow settings(0, 0, fruits.size().width / 3,
	                        fruits.size().height / 3, "Settings");

	cvui::init(WINDOW_NAME);

	while (true)
	{
		if (use_EdgesSubPix)
		{
			// Yes, we should apply it.

			cvtColor(fruits, frame, COLOR_BGR2GRAY);
			EdgesSubPix(frame, 1.0, low_threshold, high_threshold, contours, hierarchy, 1);
			cvtColor(frame, frame, COLOR_GRAY2BGR);

			vector<vector<Point>> ocvContours;
			for (size_t i = 0; i < contours.size(); ++i)
			{
				Mat pts(contours[i].points);
				vector<Point> c;
				pts.convertTo(c, CV_32S);
				ocvContours.push_back(std::move(c));
			}

			drawContours(frame, ocvContours, -1, Scalar(0, 255, 0));
		}
		else
		{
			// No, so just copy the original image to the displaying frame.
			fruits.copyTo(frame);
		}


		settings.begin(frame);
		if (!settings.isMinimized())
		{
			cvui::checkbox("Use EdgesSubPix", &use_EdgesSubPix);
			cvui::trackbar(settings.width() - 20, &low_threshold, 0, 255);
			cvui::trackbar(settings.width() - 20, &high_threshold, 0, 255);
			cvui::space(20); // add 20px of empty space
			cvui::text("Drag and minimize this settings window", 0.4, 0xff0000);
		}
		settings.end();

		cvui::imshow(WINDOW_NAME, frame);

		// Check if ESC was pressed
		if (waitKey(30) == 27)
		{
			break;
		}
	}
}

void LinesDetector_TEST()
{
	int WAIT_TIME = 20;

	Mat imageRGB = imread(CURRENT_PATH + "\\images\\car_line.jpg");
	Mat image = imread(CURRENT_PATH + "\\images\\car_line.jpg", 0);
	auto lineHandler = EDLines(image);
	Mat outputImage;

	imshow("INPUT IMAGE", imageRGB);
	waitKey(WAIT_TIME);
	outputImage = lineHandler.getSmoothImage();
	imshow("SMOOTHING", outputImage);
	waitKey(WAIT_TIME);
	outputImage = lineHandler.getGradImage();
	imshow("GRADIENT AND THRESHOLDING", outputImage);
	waitKey(WAIT_TIME);
	outputImage = lineHandler.getAnchorImage();
	imshow("ANCHORING AND CONNECTING THEM", outputImage);
	waitKey(WAIT_TIME);
	outputImage = lineHandler.getEdgeImage();
	imshow("EDGES", outputImage);
	waitKey(WAIT_TIME);
	outputImage = lineHandler.getLineImage();
	imshow("ED LINES", outputImage);
	waitKey(WAIT_TIME);
	outputImage = lineHandler.drawOnImage();
	imshow("ED LINES OVER SOURCE IMAGE", outputImage);
	waitKey(0);
}

void LinesDetector_TEST_LJ()
{
	Mat colorImage;
	Mat src_img;

	src_img = imread(CURRENT_PATH + "\\images\\LJ_4096.BMP", IMREAD_COLOR);

	colorImage = src_img.clone();

	Mat gray_img = imread(CURRENT_PATH + "\\images\\LJ_4096.BMP", IMREAD_GRAYSCALE);

	vector<Mat> channels;
	Mat imageBlueChannel;
	Mat imageGreenChannel;
	Mat imageRedChannel;
	split(src_img, channels);
	imageBlueChannel = channels.at(0);
	imageGreenChannel = channels.at(1);
	imageRedChannel = channels.at(2);

	auto lineHandler = EDLines(imageBlueChannel);

	Mat getSmoothImage = lineHandler.getSmoothImage();
	Mat getGradImage = lineHandler.getGradImage();
	Mat getAnchorImage = lineHandler.getAnchorImage();
	Mat getEdgeImage = lineHandler.getEdgeImage();
	Mat getLineImage = lineHandler.getLineImage();
	Mat drawOnImage = lineHandler.drawOnImage();
}

void ColorSpaceConversion_TEST()
{
	Mat imageRGB = imread(CURRENT_PATH + "\\images\\car_line.jpg");

	clock_t start1;
	clock_t end1;
	double work_time1;
	start1 = clock();
	int64 t2 = getCPUTickCount();
	Mat gray_opencv;
	cvtColor(imageRGB, gray_opencv, COLOR_RGB2GRAY);
	int64 t4 = getCPUTickCount();
	cout << "COLOR_RGB2GRAY time is " << (t4 - t2) / getTickFrequency() << " seconds" << endl;
	end1 = clock();
	work_time1 = (end1 - start1) / static_cast<double>(CLOCKS_PER_SEC);
	cout << "Work Time COLOR_RGB2GRAY : " << fixed << work_time1 << "s" << endl;
}

int st(int i) //Thread function
{
	std::cout << "hello " << i << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "world " << i << std::endl;
	return i * i;
}

void ThreadPool_TEST()
{
	ThreadPool pool(4); //Create four threads in the pool
	//std::future Provides a mechanism to access the results of asynchronous operations
	std::vector<future<int>> results;

	// enqueue and store future
	//auto result = pool.enqueue([](int answer) { return answer; }, 42);
	for (int i = 3; i < 8; ++i)
	{
		results.emplace_back(
			pool.enqueue([i]
			{
				return st(i);
			})
		);
	}

	// get result from future
	for (auto&& result : results)
		std::cout << result.get() << ' ';
	std::cout << std::endl;
}

void ThreadPool_image_TEST()
{
	ThreadPool pool(4);

	std::vector<std::future<Mat>> results;

	static Mat image = imread(CURRENT_PATH + "\\images\\car_line.jpg");
	cvtColor(image, image, COLOR_RGB2GRAY);

	for (int i = 0; i < 8; ++i)
	{
		results.emplace_back(
			pool.enqueue([i]
			{
				Mat mat_mean, mat_stddev;
				meanStdDev(image, mat_mean, mat_stddev);
				double a = mat_mean.at<double>(1, 0);
				double d = mat_stddev.at<double>(1, 0);
				for (int y = 0; y < image.rows; y++)
				{
					for (int x = 0; x < image.cols; x++)
					{
						image.at<uchar>(y, x) = i * i;
					}
				}
				cout << "i=" << i << endl;

				return image;
			})
		);
	}

	for (auto&& result : results)
	{
		auto temp = result.get();
	}
}

void ThreadPool_one_var()
{
	const __int64 iter = 9999;
	static Mat image = imread(CURRENT_PATH + "\\images\\car_line.jpg");
	cvtColor(image, image, COLOR_RGB2GRAY);

	ThreadPool pool(4);
	std::vector<future<__int64>> results;
	static __int64 sum = 0;

	clock_t start;
	clock_t end;
	double work_time;
	start = clock();
	for (__int64 i = 0; i < iter; ++i)
	{
		results.emplace_back(
			pool.enqueue([i]
			{
				sum += 1;

				Mat mat_mean, mat_stddev;
				meanStdDev(image, mat_mean, mat_stddev);
				double a = mat_mean.at<double>(1, 0);
				double d = mat_stddev.at<double>(1, 0);

				return i;
			})
		);
	}
	end = clock();
	work_time = (end - start) / static_cast<double>(CLOCKS_PER_SEC);
	cout << "Work Time : " << fixed << work_time << "s" << endl;
	cout << "sum = " << sum << endl;

	for (auto&& result : results)
	{
		//std::cout << result.get() << ' ';
	}
	std::cout << "------------------------------------------------------" << std::endl;
}

void NO_ThreadPool_one_vartt()
{
	const __int64 iter = 9999;
	static Mat image = imread(CURRENT_PATH + "\\images\\car_line.jpg");
	cvtColor(image, image, COLOR_RGB2GRAY);

	clock_t start2;
	clock_t end2;
	double work_time2;
	start2 = clock();
	__int64 sum2 = 0;
	for (__int64 k = 0; k < iter; ++k)
	{
		sum2 += 1;

		Mat mat_mean, mat_stddev;
		meanStdDev(image, mat_mean, mat_stddev);
		double a = mat_mean.at<double>(1, 0);
		double d = mat_stddev.at<double>(1, 0);
	}
	end2 = clock();
	work_time2 = (end2 - start2) / static_cast<double>(CLOCKS_PER_SEC);
	cout << "Work Time2 : " << fixed << work_time2 << "s" << endl;
	cout << "sum2 = " << sum2 << endl;
}


void sub_cell_TEST_single()
{
	vector<Contour> contours;
	vector<Vec4i> hierarchy;
	std::vector<float> angles;
	std::vector<float> slope_k;

	int low_threshold = 20, high_threshold = 40;

	int border_len = 10;

	Mat colorImage;
	Mat src_img;
	Mat result_Mat;

	src_img = imread(CURRENT_PATH + "\\images\\small_Missingangle\\NG\\right_bottom20230117_183201_845463900.png",
	                 IMREAD_GRAYSCALE);


	copyMakeBorder(src_img, src_img, border_len, border_len, border_len, border_len,
	               BORDER_CONSTANT, Scalar(255, 255, 255, 255));

	cvtColor(src_img, colorImage, COLOR_GRAY2BGR);
	cvtColor(src_img, result_Mat, COLOR_GRAY2BGR);

	int64 t0 = getCPUTickCount();
	if (src_img.data != nullptr)
	{
		EdgesSubPix(src_img, 1.0, low_threshold, high_threshold, contours, hierarchy, 1);
	}
	else
		return;


	//cal angel and slope_k
	if (contours.size() > 0)
	{
		for (int index = 0; index < contours[0].direction.size(); index++)
		{
			double fRad = contours[0].direction[index];
			angles.emplace_back(fRad * (180 / CV_PI));
			slope_k.emplace_back(tan(fRad));
		}
	}

	double angle_gradient = 5;
	int consecutive_point_long = 10;
	int step = consecutive_point_long / 2;


	vector<int> indexofpoints;
	for (int i = 0; i < angles.size() - 1; i++)
	{
		double gra = abs(angles[i] - angles[i + 1]);
		if (gra > angle_gradient)
		{
			indexofpoints.emplace_back(i - 1);
			i = i + step;
		}
	}

	Point t_int;

	//detection points long
	for (int index = 0; index < indexofpoints.size(); index++)
	{
		vector<double> p_long_v; //raw
		vector<double> copy_long_v; //copy -> sort by my sort function to cal change times
		vector<double> sort_p_long_v; //sort by std::sort

		for (int k = 0; k < consecutive_point_long; k++)
		{
			Point2f t = contours[0].points[indexofpoints[index] + k];
			p_long_v.emplace_back(angles[indexofpoints[index] + k]);
			t_int.x = static_cast<int>(t.x + 0.5);
			t_int.y = static_cast<int>(t.y + 0.5);

			//show
			Rect rrrr;
			rrrr.x = t_int.x;
			rrrr.y = t_int.y;
			rrrr.width = 1;
			rrrr.height = 1;
			rectangle(colorImage, rrrr, Scalar(0, 0, 255, 255), -1);
		}

		sort_p_long_v = p_long_v;
		sort(sort_p_long_v.begin(), sort_p_long_v.end());

		if (abs(sort_p_long_v[0] - sort_p_long_v[sort_p_long_v.size() - 1]) > 80)
		{
			continue;
		}

		int change_times = 0;
		copy_long_v = p_long_v;
		Bibble_sort(copy_long_v, copy_long_v.size(), change_times); //to cal change times!!!

		if (change_times > 10)
		{
			cout << "find" << endl;
		}
	}

	int64 t1 = getCPUTickCount();
	cout << "execution time is " << (t1 - t0) / getTickFrequency() << " seconds" << endl;

	cout << "------------------------------------------------------------------" << endl;
}

void sub_cell_TEST_multiple()
{
	std::vector<String> imgList;
	glob(CURRENT_PATH + "\\images\\small_Missingangle\\NG\\", imgList);

	for (int sss = 0; sss < imgList.size(); sss++)
	{
		vector<Contour> contours;
		vector<Vec4i> hierarchy;
		std::vector<float> angles;
		std::vector<float> slope_k;

		contours.clear();
		hierarchy.clear();
		angles.clear();
		slope_k.clear();

		int low_threshold = 20, high_threshold = 40;

		int border_len = 10;

		Mat colorImage;
		Mat src_img;
		Mat result_Mat;


		//src_img = imread(CURRENT_PATH + "\\images\\small_Missingangle\\sub\\left_bottom20230117_183536_060951000.png", IMREAD_GRAYSCALE);
		src_img = imread(imgList[sss], IMREAD_GRAYSCALE);

		copyMakeBorder(src_img, src_img, border_len, border_len, border_len, border_len,
		               BORDER_CONSTANT, Scalar(255, 255, 255, 255));

		cvtColor(src_img, colorImage, COLOR_GRAY2BGR);
		cvtColor(src_img, result_Mat, COLOR_GRAY2BGR);

		int64 t0 = getCPUTickCount();
		if (src_img.data != nullptr)
		{
			EdgesSubPix(src_img, 1.0, low_threshold, high_threshold, contours, hierarchy, 1);
		}
		else
			return;


		//cal angel and slope_k
		if (contours.size() <= 0)
		{
			continue;
		}

		for (int index = 0; index < contours[0].direction.size(); index++)
		{
			double fRad = contours[0].direction[index];
			angles.emplace_back(fRad * (180 / CV_PI));
			slope_k.emplace_back(tan(fRad));
		}


		double angle_gradient = 5;
		int consecutive_point_long = 10;
		int step = consecutive_point_long / 2;


		vector<int> indexofpoints;
		indexofpoints.clear();
		for (int i = 0; i < angles.size() - step - 1; i++)
		{
			double gra = abs(angles[i] - angles[i + 1]);
			if (gra > angle_gradient)
			{
				indexofpoints.emplace_back(i - 1);
				i = i + step;
			}
		}

		Point t_int;

		//detection points long
		for (int index = 0; index < indexofpoints.size(); index++)
		{
			vector<double> p_long_v; //raw
			vector<double> copy_long_v; //copy -> sort by my sort function to cal change times
			vector<double> sort_p_long_v; //sort by std::sort
			vector<double> Adjacentpointgradient;


			for (int k = 0; k < consecutive_point_long; k++)
			{
				if ((indexofpoints[index] + k) > contours[0].points.size() - 1)
				{
					break;
				}
				Point2f t = contours[0].points[indexofpoints[index] + k];
				p_long_v.emplace_back(angles[indexofpoints[index] + k]);
				t_int.x = static_cast<int>(t.x + 0.5);
				t_int.y = static_cast<int>(t.y + 0.5);

				//show
				Rect rrrr;
				rrrr.x = t_int.x;
				rrrr.y = t_int.y;
				rrrr.width = 1;
				rrrr.height = 1;
				rectangle(colorImage, rrrr, Scalar(255, 0, 0, 255), -1);
			}
			if (p_long_v.empty())
			{
				continue;
			}

			sort_p_long_v = p_long_v;
			sort(sort_p_long_v.begin(), sort_p_long_v.end());

			double max_sub = abs(sort_p_long_v[0] - sort_p_long_v[sort_p_long_v.size() - 1]);
			double countofverandzero = 0, countotan = 0;
			for (auto i : p_long_v)
			{
				if (abs(abs(i) - 90) < angle_gradient) //ver
				{
					countofverandzero++;
				}
				if (abs(abs(i) - 0) < angle_gradient) //hor
				{
					countofverandzero++;
				}
			}
			for (auto i : p_long_v)
			{
				if (abs(abs(i) - 135) < 2) //135
				{
					countotan++;
				}
				if (abs(abs(i) - 45) < 2) //45
				{
					countotan++;
				}
			}
			if (countofverandzero > 5)
			{
				continue;
			}
			if (countotan > 5)
			{
				continue;
			}

			if (max_sub > 80 && countofverandzero > 0)
			{
				continue;
			}

			int change_times = 0;
			/*		int change_times222 = 0;
					vector<double> copy_long_v222;
					copy_long_v222 = p_long_v;
					insertionSort(copy_long_v222, copy_long_v222.size(), change_times222);
						*/

			copy_long_v = p_long_v;
			Bibble_sort(copy_long_v, copy_long_v.size(), change_times); //to cal change times!!!


			for (int q = 0; q < p_long_v.size() - 1; q++)
			{
				double temp_g = abs(p_long_v[q + 1] - p_long_v[q]);
				if (temp_g > angle_gradient)
				{
					Adjacentpointgradient.emplace_back(temp_g);
				}
			}
			vector<double> sort_Adjacentpointgradient = Adjacentpointgradient;
			sort(sort_Adjacentpointgradient.begin(), sort_Adjacentpointgradient.end());
			double adj_max_min = abs(
				sort_Adjacentpointgradient[0] - sort_Adjacentpointgradient[sort_Adjacentpointgradient.size() - 1]);

			if (adj_max_min < 3 && change_times < 20)
			{
				continue;
			}

			if (change_times >= 8)
			{
				for (int k = 0; k < consecutive_point_long; k++)
				{
					Point2f t = contours[0].points[indexofpoints[index] + k];
					t_int.x = static_cast<int>(t.x + 0.5);
					t_int.y = static_cast<int>(t.y + 0.5);

					//show
					Rect rrrr;
					rrrr.x = t_int.x;
					rrrr.y = t_int.y;
					rrrr.width = 1;
					rrrr.height = 1;
					rectangle(colorImage, rrrr, Scalar(0, 0, 255, 255), -1);
				}
				//Obvious indication
				Rect obv_r;
				Point2f t = contours[0].points[indexofpoints[index] + consecutive_point_long];
				t_int.x = static_cast<int>(t.x + 0.5);
				t_int.y = static_cast<int>(t.y + 0.5);
				obv_r.x = t_int.x - 5;
				obv_r.y = t_int.y - 5;
				obv_r.width = 10;
				obv_r.height = 10;
				rectangle(colorImage, obv_r, Scalar(0, 0, 255, 255), 1);

				cout << "find" << endl;
				imwrite(imgList[sss] + ".png", colorImage);
			}
		}

		int64 t1 = getCPUTickCount();
		cout << "execution time is " << (t1 - t0) / getTickFrequency() << " seconds" << endl;
		cout << "------------------------------------------------------------------" << endl;
	}
}

void globalmatting_TEST()
{
	Mat image = imread(CURRENT_PATH + "\\images\\matting\\input.png", IMREAD_COLOR);
	Mat trimap = imread(CURRENT_PATH + "\\images\\matting\\trimap.png", IMREAD_GRAYSCALE);

	// (optional) exploit the affinity of neighboring pixels to reduce the 
	// size of the unknown region. please refer to the paper
	// 'Shared Sampling for Real-Time Alpha Matting'.
	expansionOfKnownRegions(image, trimap, 9);

	Mat foreground, alpha;
	globalMatting(image, trimap, foreground, alpha);

	// filter the result with fast guided filter
	alpha = guidedFilter(image, alpha, 10, 1e-5);
	for (int x = 0; x < trimap.cols; ++x)
		for (int y = 0; y < trimap.rows; ++y)
		{
			if (trimap.at<uchar>(y, x) == 0)
				alpha.at<uchar>(y, x) = 0;
			else if (trimap.at<uchar>(y, x) == 255)
				alpha.at<uchar>(y, x) = 255;
		}

	imwrite(CURRENT_PATH + "\\images\\matting\\alpha.png", alpha);
}

void sharedmatting_TEST2()
{
	SharedMatting sm;

	sm.loadImage(CURRENT_PATH + "\\images\\matting\\input.png");
	sm.loadTrimap(CURRENT_PATH + "\\images\\matting\\trimap.png");

	clock_t start;
	clock_t end;
	double work_time;
	start = clock();
	sm.solveAlpha();
	end = clock();
	work_time = (end - start) / static_cast<double>(CLOCKS_PER_SEC);
	cout << "solveAlpha Time : " << fixed << work_time << "s" << endl;

	sm.save(CURRENT_PATH + "\\images\\matting\\alpha2.png");
}

void bayesian_TEST()
{
	Mat img = imread(CURRENT_PATH + "\\images\\matting\\input.png", IMREAD_COLOR);
	Mat trimap = imread(CURRENT_PATH + "\\images\\matting\\trimap.png", IMREAD_GRAYSCALE);

	BayesianMatting bm(img, trimap);

	Mat result;
	bm.solve(result);
}


int onnxruntime_resnet50()
{
	Ort::Env env;
	Ort::RunOptions runOptions;
	Ort::Session session(nullptr);

	constexpr int64_t numChannels = 3;
	constexpr int64_t width = 224;
	constexpr int64_t height = 224;
	constexpr int64_t numClasses = 1000;
	constexpr int64_t numInputElements = numChannels * height * width;


	const std::string imageFile = CURRENT_PATH + "\\images\\seashore.jpg";
	const std::string labelFile = CURRENT_PATH + "\\models\\imagenet_classes.txt";
	auto modelPath = L"D:\\RainbowCV\\RainbowCV\\TestApp\\models\\resnet50v2.onnx";


	//load labels
	std::vector<std::string> labels = Helpers::loadLabels(labelFile);
	if (labels.empty())
	{
		std::cout << "Failed to load labels: " << labelFile << std::endl;
		return 1;
	}

	// load image
	const std::vector<float> imageVec = Helpers::loadImage(imageFile, width, height,true, C_H_W);
	if (imageVec.empty())
	{
		std::cout << "Failed to load image: " << imageFile << std::endl;
		return 1;
	}

	if (imageVec.size() != numInputElements)
	{
		std::cout << "Invalid image format. Must be " << numChannels << "x" << width << "x" << height << "RGB image." <<
			std::endl;
		return 1;
	}

	// Use CUDA GPU----
	Ort::SessionOptions ort_session_options;
	OrtCUDAProviderOptions options;
	options.device_id = 0;
	options.arena_extend_strategy = 0;
	options.gpu_mem_limit = 2 * 1024 * 1024 * 1024;
	options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
	options.do_copy_in_default_stream = 1;
	OrtSessionOptionsAppendExecutionProvider_CUDA(ort_session_options, options.device_id);


	//OrtSessionOptions session_options;

	//OrtCUDAProviderOptions options;
	//options.device_id = 0;
	//options.arena_extend_strategy = 0;
	//options.gpu_mem_limit = 2 * 1024 * 1024 * 1024;
	//options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
	//options.do_copy_in_default_stream = 1;

	//OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, &options, options.device_id);

	//// create session
	session = Ort::Session(env, modelPath, ort_session_options);


	// Use CPU----
	//session = Ort::Session(env, modelPath, Ort::SessionOptions{ nullptr });

	// define shape
	const std::array<int64_t, 4> inputShape = {1, numChannels, height, width};
	const std::array<int64_t, 2> outputShape = {1, numClasses};

	// define array
	std::array<float, numInputElements> input;
	std::array<float, numClasses> results;

	// define Tensor
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	auto inputTensor = Ort::Value::CreateTensor<float>(memory_info, input.data(),
	                                                   input.size(), inputShape.data(), inputShape.size());
	auto outputTensor = Ort::Value::CreateTensor<float>(memory_info, results.data(),
	                                                    results.size(), outputShape.data(), outputShape.size());

	// copy image data to input array
	std::copy(imageVec.begin(), imageVec.end(), input.begin());


	// define names
	Ort::AllocatorWithDefaultOptions ort_alloc;
	Ort::AllocatedStringPtr inputName = session.GetInputNameAllocated(0, ort_alloc);
	Ort::AllocatedStringPtr outputName = session.GetOutputNameAllocated(0, ort_alloc);
	const std::array<const char*, 1> inputNames = {inputName.get()};
	const std::array<const char*, 1> outputNames = {outputName.get()};
	inputName.release();
	outputName.release();


	for (int i = 0; i < 5; i++)
	{
		clock_t start;
		clock_t end;
		double work_time;
		start = clock();
		// run inference
		try
		{
			session.Run(runOptions, inputNames.data(), &inputTensor, 1, outputNames.data(), &outputTensor, 1);
		}
		catch (Ort::Exception& e)
		{
			std::cout << e.what() << std::endl;
			return 1;
		}
		end = clock();
		work_time = (end - start) / static_cast<double>(CLOCKS_PER_SEC);
		cout << "session.Run Time : " << fixed << work_time * 1000 << "ms" << endl;
	}
	// sort results
	std::vector<std::pair<size_t, float>> indexValuePairs;
	for (size_t i = 0; i < results.size(); ++i)
	{
		indexValuePairs.emplace_back(i, results[i]);
	}
	std::sort(indexValuePairs.begin(), indexValuePairs.end(), [](const auto& lhs, const auto& rhs)
	{
		return lhs.second > rhs.second;
	});

	// show Top5
	for (size_t i = 0; i < 5; ++i)
	{
		const auto& result = indexValuePairs[i];
		std::cout << i + 1 << ": " << labels[result.first] << " " << result.second << std::endl;
	}

	return 0;
}

void EdgesSubPix_TEST_LJ()
{
	vector<Contour> contours;
	vector<Vec4i> hierarchy;
	std::vector<float> angles;
	std::vector<float> slope_k;

	int low_threshold = 20, high_threshold = 40;

	Mat colorImage;
	Mat src_img;
	Mat result_Mat;

	src_img = imread(CURRENT_PATH + "\\images\\LJ_4096.BMP", IMREAD_COLOR);

	colorImage = src_img.clone();

	Mat gray_img = imread(CURRENT_PATH + "\\images\\LJ_4096.BMP", IMREAD_GRAYSCALE);

	vector<Mat> channels;
	Mat imageBlueChannel;
	Mat imageGreenChannel;
	Mat imageRedChannel;
	split(src_img, channels);
	imageBlueChannel = channels.at(0);
	imageGreenChannel = channels.at(1);
	imageRedChannel = channels.at(2);

	Mat open_b;
	morphologyEx(imageBlueChannel, open_b, MORPH_OPEN,
	             Mat::ones(Size(3, 1), CV_8UC1));

	if (imageBlueChannel.data != nullptr)
	{
		EdgesSubPix(imageBlueChannel, 1.0, low_threshold, high_threshold, contours, hierarchy, 1);
	}
	else
		return;

	vector<vector<Point>> ocvContours;
	for (size_t i = 0; i < contours.size(); ++i)
	{
		Mat pts(contours[i].points);
		vector<Point> c;
		pts.convertTo(c, CV_32S);
		ocvContours.push_back(std::move(c));
	}
	sort(ocvContours.begin(), ocvContours.end(),
	     [&](vector<Point>& p1, vector<Point>& p2)
	     {
		     return (p1.size() > p2.size());
	     });
	drawContours(colorImage, ocvContours, -1, Scalar(0, 255, 0));
}


int yolov5_seg_onnx()
{
	string img_path = CURRENT_PATH + "\\images\\bus.jpg";
	string model_path = CURRENT_PATH + "\\models\\yolov5s-seg.onnx";
	YoloSegOnnx test;
	//Net net;
	if (test.ReadModel(model_path, true, 0, true))
	{
		cout << "read net ok!" << endl;
	}
	else
	{
		return -1;
	}

	vector<Scalar> color;
	srand(time(nullptr));
	for (int i = 0; i < 80; i++)
	{
		int b = rand() % 256;
		int g = rand() % 256;
		int r = rand() % 256;
		color.push_back(Scalar(b, g, r));
	}
	vector<OutputSeg> result;
	Mat img = imread(img_path);

	clock_t start;
	clock_t end;
	double work_time;
	start = clock();
	if (test.OnnxDetect(img, result))
	{
		DrawPred(img, result, _className, color);
	}
	else
	{
		cout << "Detect Failed!" << endl;
	}
	end = clock();
	work_time = (end - start) / static_cast<double>(CLOCKS_PER_SEC);
	cout << "yolov5_seg_onnx Time : " << fixed << work_time * 1000 << "ms" << endl;

	return 0;
}


//string img_path = CURRENT_PATH + "\\images\\bus.jpg";
//string model_path = CURRENT_PATH + "\\models\\yolov8n-seg.onnx";
template <typename _Tp>
int yolov8_onnx(_Tp& cls, Mat& img, string& model_path)
{
	if (cls.ReadModel(model_path, true))
	{
		cout << "read net ok!" << endl;
	}
	else
	{
		cout << "read net fail!" << endl;
		return -1;
	}

	vector<Scalar> color;
	srand(time(nullptr));
	for (int i = 0; i < 80; i++)
	{
		int b = rand() % 256;
		int g = rand() % 256;
		int r = rand() % 256;
		color.push_back(Scalar(b, g, r));
	}
	vector<OutputSeg> result;

	for (int i = 0; i < 10; i++)
	{
		clock_t start = clock();
		cls.OnnxDetect(img, result);
		clock_t end = clock();
		double work_time = (end - start) / static_cast<double>(CLOCKS_PER_SEC);
		cout << "work Time : " << fixed << work_time * 1000 << "ms" << endl;
	}

	if (cls.OnnxDetect(img, result))
	{
		DrawPred(img, result, _className, color);
	}
	else
	{
		cout << "Detect Failed!" << endl;
	}


	return 0;
}


void thresholdIntegral_Test()
{
	Mat colorImage;
	Mat src_img;
	//Mat result_Mat;

	src_img = imread(CURRENT_PATH + "\\images\\LJ_4096.BMP", IMREAD_COLOR);

	colorImage = src_img.clone();

	Mat gray_img = imread(CURRENT_PATH + "\\images\\LJ_4096.BMP", IMREAD_GRAYSCALE);

	vector<Mat> channels;
	Mat imageBlueChannel;
	Mat imageGreenChannel;
	Mat imageRedChannel;
	split(src_img, channels);
	imageBlueChannel = channels.at(0);
	imageGreenChannel = channels.at(1);
	imageRedChannel = channels.at(2);


	Mat bw1;
	adaptiveThreshold(imageBlueChannel, bw1, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, -2);

	Mat bw2 = Mat::zeros(imageBlueChannel.size(), CV_8UC1);
	thresholdIntegral(imageBlueChannel, bw2);
}

void ED_TEST()
{
	string filename = CURRENT_PATH + "\\IMAGES\\MZSP.BMP";
	Mat testImg = imread(filename, 0);
	TickMeter tm;

	for (int i = 1; i < 2; i++)
	{
		cout << "\n#################################################";
		cout << "\n####### ( " << i << " ) ORIGINAL & OPENCV COMPARISON ######";
		cout << "\n#################################################\n";
		Ptr<ximgproc::EdgeDrawing> ed = ximgproc::createEdgeDrawing();
		ed->params.EdgeDetectionOperator = ximgproc::EdgeDrawing::SOBEL;
		ed->params.GradientThresholdValue = 36;
		ed->params.AnchorThresholdValue = 8;
		vector<Vec6d> ellipses;
		vector<Vec4f> lines;

		//Detection of edge segments from an input image    
		tm.start();
		//Call ED constructor
		auto testED = ED(testImg, SOBEL_OPERATOR, 36, 8, 1, 10, 1.0, true);
		tm.stop();
		std::cout << "\ntestED.getEdgeImage()  (Original)  : " << tm.getTimeMilli() << endl;

		tm.reset();
		tm.start();
		ed->detectEdges(testImg);
		tm.stop();
		std::cout << "detectEdges()            (OpenCV)  : " << tm.getTimeMilli() << endl;

		Mat edgeImg0 = testED.getEdgeImage();
		Mat anchImg0 = testED.getAnchorImage();
		Mat gradImg0 = testED.getGradImage();
		Mat edgeImg1, diff;
		ed->getEdgeImage(edgeImg1);
		absdiff(edgeImg0, edgeImg1, diff);

		Mat gradImg0_close;
		morphologyEx(gradImg0, gradImg0_close, MORPH_CLOSE,
		             Mat::ones(Size(3, 3), CV_8UC1));

		Mat bw1;
		adaptiveThreshold(gradImg0_close, bw1, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, -2);


		Mat bw2 = Mat::zeros(gradImg0_close.size(), CV_8UC1);
		thresholdIntegral(gradImg0_close, bw2);

		cout << "different pixel count              : " << countNonZero(diff) << endl;
		imwrite("gradImg0.png", gradImg0);
		imwrite("anchImg0.png", anchImg0);
		imwrite("edgeImg0.png", edgeImg0);
		imwrite("edgeImg1.png", edgeImg1);
		imwrite("diff0.png", diff);

		//***************************** EDLINES Line Segment Detection *****************************
		//Detection of lines segments from edge segments instead of input image
		//Therefore, redundant detection of edge segmens can be avoided
		tm.reset();
		tm.start();
		auto testEDLines = EDLines(testED);
		tm.stop();
		cout << "-------------------------------------------------\n";
		cout << "testEDLines.getLineImage()         : " << tm.getTimeMilli() << endl;
		Mat lineImg0 = testEDLines.getLineImage(); //draws on an empty image
		imwrite("lineImg0.png", lineImg0);

		tm.reset();
		tm.start();
		ed->detectLines(lines);
		tm.stop();
		cout << "detectLines()            (OpenCV)  : " << tm.getTimeMilli() << endl;

		auto lineImg1 = Mat(lineImg0.rows, lineImg0.cols, CV_8UC1, Scalar(255));

		for (int i = 0; i < lines.size(); i++)
			line(lineImg1, Point2d(lines[i][0], lines[i][1]), Point2d(lines[i][2], lines[i][3]), Scalar(0), 1, LINE_AA);

		absdiff(lineImg0, lineImg1, diff);
		cout << "different pixel count              : " << countNonZero(diff) << endl;
		imwrite("lineImg1.png", lineImg1);
		imwrite("diff1.png", diff);

		//***************************** EDCIRCLES Circle Segment Detection *****************************
		//Detection of circles from already available EDPF or ED image
		tm.reset();
		tm.start();
		auto testEDCircles = EDCircles(testEDLines);
		tm.stop();
		cout << "-------------------------------------------------\n";
		cout << "EDCircles(testEDLines)             : " << tm.getTimeMilli() << endl;

		tm.reset();
		tm.start();
		ed->detectEllipses(ellipses);
		tm.stop();
		cout << "detectEllipses()         (OpenCV)  : " << tm.getTimeMilli() << endl;
		cout << "-------------------------------------------------\n";

		vector<mCircle> found_circles = testEDCircles.getCircles();
		vector<mEllipse> found_ellipses = testEDCircles.getEllipses();
		auto ellipsImg0 = Mat(lineImg0.rows, lineImg0.cols, CV_8UC3, Scalar::all(0));
		auto ellipsImg1 = Mat(lineImg0.rows, lineImg0.cols, CV_8UC3, Scalar::all(0));

		for (int i = 0; i < found_circles.size(); i++)
		{
			Point center(found_circles[i].center.x, found_circles[i].center.y);
			Size axes(found_circles[i].r, found_circles[i].r);
			double angle(0.0);
			auto color = Scalar(0, 255, 0);

			ellipse(ellipsImg0, center, axes, angle, 0, 360, color, 1, LINE_AA);
		}

		for (int i = 0; i < found_ellipses.size(); i++)
		{
			Point center(found_ellipses[i].center.x, found_ellipses[i].center.y);
			Size axes(found_ellipses[i].axes.width, found_ellipses[i].axes.height);
			double angle = found_ellipses[i].theta * 180 / CV_PI;
			auto color = Scalar(255, 255, 0);

			ellipse(ellipsImg0, center, axes, angle, 0, 360, color, 1, LINE_AA);
		}

		for (size_t i = 0; i < ellipses.size(); i++)
		{
			Point center(ellipses[i][0], ellipses[i][1]);
			Size axes(static_cast<int>(ellipses[i][2]) + static_cast<int>(ellipses[i][3]),
			          static_cast<int>(ellipses[i][2]) + static_cast<int>(ellipses[i][4]));
			double angle(ellipses[i][5]);
			Scalar color = ellipses[i][2] == 0 ? Scalar(255, 255, 0) : Scalar(0, 255, 0);

			ellipse(ellipsImg1, center, axes, angle, 0, 360, color, 1, LINE_AA);
		}

		imwrite("ellipsImg0.png", ellipsImg0);
		imwrite("ellipsImg1.png", ellipsImg1);

		//************************** EDPF Parameter-free Edge Segment Detection **************************
		// Detection of edge segments with parameter free ED (EDPF)
		tm.reset();
		tm.start();
		auto testEDPF = EDPF(testImg);
		tm.stop();
		cout << "testEDPF.getEdgeImage()            : " << tm.getTimeMilli() << endl;

		Ptr<ximgproc::EdgeDrawing> ed1 = ximgproc::createEdgeDrawing();
		ed1->params.EdgeDetectionOperator = ximgproc::EdgeDrawing::PREWITT;
		ed1->params.GradientThresholdValue = 11;
		ed1->params.AnchorThresholdValue = 3;
		ed1->params.PFmode = true;

		tm.reset();
		tm.start();
		ed1->detectEdges(testImg);
		tm.stop();
		std::cout << "detectEdges()  PF        (OpenCV)  : " << tm.getTimeMilli() << endl;

		edgeImg0 = testEDPF.getEdgeImage();
		ed->getEdgeImage(edgeImg1);
		absdiff(edgeImg0, edgeImg1, diff);
		cout << "different pixel count              : " << countNonZero(diff) << endl;
		imwrite("edgePFImage0.png", edgeImg0);
		imwrite("edgePFImage1.png", edgeImg1);
		imwrite("diff2.png", diff);
		//*********************** EDCOLOR Edge Segment Detection from Color Images **********************

		Mat colorImg = imread(filename);
		tm.reset();
		tm.start();
		auto testEDColor = EDColor(colorImg, 36);
		tm.stop();
		cout << "-------------------------------------------------\n";
		cout << "testEDColor                        : " << tm.getTimeMilli() << endl;

		tm.reset();
		tm.start();
		// get lines from color image
		auto colorLine = EDLines(testEDColor);
		tm.stop();
		cout << "get lines from color image         : " << tm.getTimeMilli() << endl;

		tm.reset();
		tm.start();
		// get circles from color image
		auto colorCircle = EDCircles(testEDColor);
		tm.stop();
		cout << "get circles from color image       : " << tm.getTimeMilli() << endl;
	}

	return;
}


void ED_circle()
{
	string filename = CURRENT_PATH + "\\IMAGES\\billiard.jpg";
	Mat testImg = imread(filename, 0);

	Ptr<ximgproc::EdgeDrawing> ed = ximgproc::createEdgeDrawing();
	ed->params.EdgeDetectionOperator = ximgproc::EdgeDrawing::SOBEL;
	ed->params.GradientThresholdValue = 36;
	ed->params.AnchorThresholdValue = 8;
	vector<Vec6d> ellipses;
	vector<Vec4f> lines;

	ed->detectEdges(testImg);

	Mat getEdgeImage;
	ed->getEdgeImage(getEdgeImage);

	ed->detectEllipses(ellipses);
}

void single_c_2_three_c()
{
	std::vector<String> imgList;
	glob("G:\\BigDebrisClassification\\data\\4_zw", imgList);

	int count = 0;
	for (auto f : imgList)
	{
		count++;
		cout << count << " " << f << endl;

		Mat img = imread(f);
		if (img.empty())
			continue;
		if (img.channels() == 1)
		{
			vector<Mat> channels;
			for (int i = 0; i < 3; i++)
			{
				channels.emplace_back(img);
			}
			Mat result;
			cv::merge(channels, result);

			imwrite("G:\\BigDebrisClassification\\data\\3c_4_zw\\" + std::to_string(count) + "_zw.png"
			        , result);
		}
		else
		{
			imwrite("G:\\BigDebrisClassification\\data\\3c_4_zw\\" + std::to_string(count) + "_zw.png"
			        , img);
		}
	}
}

int onnxruntime_mobilenet()
{
	cout << "onnxruntime_mobilenet" << endl;
	Ort::Env env;
	Ort::RunOptions runOptions;
	Ort::Session session(nullptr);

	constexpr int64_t numChannels = 3;
	constexpr int64_t width = 128;
	constexpr int64_t height = 128;
	constexpr int64_t numClasses = 2;
	constexpr int64_t numInputElements = numChannels * height * width;


	//const std::string imageFile = CURRENT_PATH + "\\images\\test\\4_zw\\9_zw.png";

	std::vector<String> imgList;
	glob(CURRENT_PATH + "\\images\\test\\1_yl\\", imgList);
	//glob(CURRENT_PATH + "\\images\\test\\4_zw\\", imgList);

	vector<Mat> imag_mat_list;
	for (auto f : imgList)
	{
		Mat img = imread(f);
		imag_mat_list.emplace_back(img);
	}
	//const std::string labelFile = CURRENT_PATH + "\\models\\imagenet_classes.txt";
	auto modelPath = L"D:\\RainbowCV\\RainbowCV\\TestApp\\models\\Mobilenet_YL_ZW_bsize_3_128_128_20230308_None_.onnx";

	vector<vector<float>> image_vec_vector;
	for(auto file : imgList)
	{
		image_vec_vector.emplace_back(Helpers::loadImage(file, width, height,false,H_W_C));
	}


	// Use CUDA GPU----
	Ort::SessionOptions ort_session_options;
	OrtCUDAProviderOptions options;
	options.device_id = 0;
	options.arena_extend_strategy = 0;
	options.gpu_mem_limit = 2 * 1024 * 1024 * 1024;
	options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
	options.do_copy_in_default_stream = 1;
	OrtSessionOptionsAppendExecutionProvider_CUDA(ort_session_options, options.device_id);

	//// create session
	session = Ort::Session(env, modelPath, ort_session_options);

	// define shape
	const std::array<int64_t, 4> inputShape = {1, width, height, numChannels};
	const std::array<int64_t, 2> outputShape = {1, numClasses};

	// define array
	std::array<float, numInputElements> input;
	std::array<float, numClasses> results;

	// define Tensor
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	auto inputTensor = Ort::Value::CreateTensor<float>(memory_info, input.data(),
		input.size(), inputShape.data(), inputShape.size());
	auto outputTensor = Ort::Value::CreateTensor<float>(memory_info, results.data(),
		results.size(), outputShape.data(), outputShape.size());

	// define names
	Ort::AllocatorWithDefaultOptions ort_alloc;
	Ort::AllocatedStringPtr inputName = session.GetInputNameAllocated(0, ort_alloc);
	Ort::AllocatedStringPtr outputName = session.GetOutputNameAllocated(0, ort_alloc);
	const std::array<const char*, 1> inputNames = { inputName.get() };
	const std::array<const char*, 1> outputNames = { outputName.get() };
	inputName.release();
	outputName.release();

	int YL_sum = 0;
	int ZW_sum = 0;
	for(int idx =0 ;idx<image_vec_vector.size();idx++)
	{

		// copy image data to input array
		std::copy(image_vec_vector[idx].begin(), image_vec_vector[idx].end(), input.begin());

		for (int i = 0; i < 1; i++)
		{
			clock_t start;
			clock_t end;
			double work_time;
			start = clock();
			// run inference
			try
			{
				session.Run(runOptions, inputNames.data(), &inputTensor, 1, outputNames.data(), &outputTensor, 1);
			}
			catch (Ort::Exception& e)
			{
				std::cout << e.what() << std::endl;
				return 1;
			}
			end = clock();
			work_time = (end - start) / static_cast<double>(CLOCKS_PER_SEC);
			cout << "session.Run Time : " << fixed << work_time * 1000 << "ms" << endl;
		}
		// sort results
		std::vector<std::pair<size_t, float>> indexValuePairs;
		for (size_t i = 0; i < results.size(); ++i)
		{
			indexValuePairs.emplace_back(i, results[i]);
		}
		std::sort(indexValuePairs.begin(), indexValuePairs.end(), [](const auto& lhs, const auto& rhs)
		{
			return lhs.second > rhs.second;
		});

		// show 
		const auto& result = indexValuePairs[0];
		std::cout << result.first << " " << result.second << std::endl;
		if (result.first == 0)
		{
			imwrite(CURRENT_PATH + "\\images\\test\\res\\" + std::to_string(idx)+".png", imag_mat_list[idx]);
			YL_sum++;
		}
		else if (result.first == 1)
		{
			ZW_sum++;
		}
	}

	cout << "RES--------------" << endl;
	cout << "YL_sum=" << YL_sum <<" |"<< float(YL_sum/ (image_vec_vector.size()*1.0))<< endl;
	cout << "ZW_sum=" << ZW_sum << " |" << float(ZW_sum / (image_vec_vector.size()*1.0)) << endl;
	

	return 0;
}

void MZSP_TEST()
{
	string filename = CURRENT_PATH + "\\IMAGES\\MZSP.BMP";
	Mat testImg = imread(filename, 0);
	set_Margin(testImg, 255, 10, 10, 200, 200);

	Ptr<ximgproc::EdgeDrawing> ed = ximgproc::createEdgeDrawing();
	ed->params.EdgeDetectionOperator = ximgproc::EdgeDrawing::SOBEL;
	ed->params.GradientThresholdValue = 36;
	ed->params.AnchorThresholdValue = 8;
	vector<Vec6d> ellipses;
	vector<Vec4f> lines;

	ed->detectEdges(testImg);

	Mat getEdgeImage;
	ed->getEdgeImage(getEdgeImage);

	return;
}

int main()
{
	//get current project path
	char buffer[MAX_PATH];
	_getcwd(buffer, MAX_PATH);
	CURRENT_PATH = buffer;
	cout << "CURRENT_PATH:" << CURRENT_PATH << endl;

	//EdgesSubPix_TEST();

	//GUI_TEST();

	//EdgesSubPix_TEST_GUI();

	//LinesDetector_TEST();
	//LinesDetector_TEST_LJ();

	//ColorSpaceConversion_TEST();

	//ThreadPool_TEST(); // you can also chose openOMP!!!!
	//ThreadPool_image_TEST();

	//ThreadPool_one_var(); //one var shock
	//NO_ThreadPool_one_vartt();

	//sub_cell_TEST_single();
	//sub_cell_TEST_multiple();

	//globalmatting_TEST();
	//sharedmatting_TEST2(); //best fast!!
	//bayesian_TEST();


	//EdgesSubPix_TEST_LJ();

	//yolov5_seg();
	//yolov5_seg_onnx();

	/*Rect r(-99, -1, 21686, 1281);
	set_rect_in_mat(r, img);
	set_Corner(img, 0, 11, 11, 11, 11);
	*/

	//thresholdIntegral_Test();

	//ED_TEST();
	//ED_circle();

	//MZSP_TEST();

	//single_c_2_three_c();

	string img_path = CURRENT_PATH + "\\images\\bus.jpg";
	string seg_model_path = CURRENT_PATH + "\\models\\yolov8n-seg.onnx";
	string detect_model_path = CURRENT_PATH + "\\models\\yolov8n.onnx";
	Mat img = imread(img_path);
	Yolov8Onnx task_detect_onnx;
	Yolov8SegOnnx task_segment_onnx;
	//yolov8_onnx(task_detect_onnx,img,detect_model_path);  //onnxruntime detect
	//yolov8_onnx(task_segment_onnx, img, seg_model_path); //onnxruntime segment


	//onnxruntime_resnet50();
	//onnxruntime_mobilenet();


	return 0;
}

/*@@@@@@@@@@@@@ TIME-CAL
 clock_t start = clock();

clock_t end = clock();
double work_time = (end - start) / (double)CLOCKS_PER_SEC;
cout << "work Time : " << fixed << work_time * 1000 << "ms" << endl;
 */
