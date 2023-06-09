#ifndef SHAREDMSTTING_H
#define SHAREDMSTTING_H

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cmath>
#include <vector>

using namespace std;

struct labelPoint
{
    int x;
    int y;
    int label;
};

struct Tuple
{
    cv::Scalar f;
    cv::Scalar b;
    double   sigmaf;
    double   sigmab;

    int flag;

};

struct Ftuple
{
    cv::Scalar f;
    cv::Scalar b;
    double   alphar;
    double   confidence;
};

/*The program determines that x is row and y is column in cv:: Point, which may be wrong,
 *but has no effect on the program result*/
class SharedMatting
{
public:
    SharedMatting()
    {
		kI = 10;
		kC = 5.0;
		kG = 4;  //each unknown p gathers at most kG forground and background samples
		uT.clear();
		tuples.clear();
    }
    ~SharedMatting()
    {
		pImg.release();
		trimap.release();
		matte.release();
		uT.clear();
		tuples.clear();
		ftuples.clear();

		for (int i = 0; i < height; ++i)
		{
			delete[] tri[i];
			delete[] unknownIndex[i];
			delete[] alpha[i];
		}
		delete[] tri;
		delete[] unknownIndex;
		delete[] alpha;
    }

    void loadImage(string filename);
    void loadTrimap(string filename)
    {
		trimap = cv::imread(filename);
		if (!trimap.data)
		{
			cout << "Loading Trimap Failed!" << endl;
			exit(-1);
		}
    }
    void expandKnown();
    void sample(cv::Point p, vector<cv::Point>& f, vector<cv::Point>& b);
    void gathering();
    void refineSample();
    void localSmooth();
    void solveAlpha();
    void save(string filename);
    void Sample(vector<vector<cv::Point> > &F, vector<vector<cv::Point> > &B);
    void getMatte();
    void release();

    double mP(int i, int j, cv::Scalar f, cv::Scalar b);
    double nP(int i, int j, cv::Scalar f, cv::Scalar b);
    double eP(int i1, int j1, int i2, int j2);
    double pfP(cv::Point p, vector<cv::Point>& f, vector<cv::Point>& b);
    double aP(int i, int j, double pf, cv::Scalar f, cv::Scalar b);
    double gP(cv::Point p, cv::Point fp, cv::Point bp, double pf);
    double gP(cv::Point p, cv::Point fp, cv::Point bp, double dpf, double pf);
    double dP(cv::Point s, cv::Point d);
    double sigma2(cv::Point p);
    double distanceColor2(cv::Scalar cs1, cv::Scalar cs2);
    double comalpha(cv::Scalar c, cv::Scalar f, cv::Scalar b);



private:
//    IplImage * pImg;
//    IplImage * trimap;
//    IplImage * matte;
    cv::Mat pImg;
    cv::Mat trimap;
    cv::Mat matte;

    vector<cv::Point> uT;
    vector<struct Tuple> tuples;
    vector<struct Ftuple> ftuples;

    int height;
    int width;
    int kI;
    int kG;
    int ** unknownIndex;
    int ** tri;
    int ** alpha;
    double kC;

    int step;
    int channels;
    uchar* data;

};



#endif
