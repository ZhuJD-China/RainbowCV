#include "ImgBasic.h"

int set_rect_in_mat(Rect& r, const Mat& src)
{
	if (src.empty()) return -1;
	r &= Rect(0, 0, src.cols, src.rows);
	return 1;
}

void set_Corner(Mat& image, const Scalar& value, int corner_tl, int corner_tr, int corner_br, int corner_bl)
{
	for (int i = 0; i < corner_tl; i++)
		image(Range(i, i + 1), Range(0, corner_tl - i)).setTo(value);

	for (int i = 0; i < corner_tr; i++)
		image(Range(i, i + 1), Range(image.cols - corner_tr + i, image.cols)).setTo(value);

	for (int i = 0; i < corner_br; i++)
		image(Range(image.rows - corner_br + i, image.rows + 1 - corner_br + i),
		      Range(image.cols - 1 - i, image.cols)).setTo(value);

	for (int i = 0; i < corner_bl; i++)
		image(Range(image.rows - corner_bl + i, image.rows + 1 - corner_bl + i), Range(0, 1 + i)).setTo(value);
}

void set_Margin(Mat& image, const Scalar& value, int top, int bottom, int left, int right)
{
	if (top < 0 || top > image.rows || top > image.cols)
		return;

	if (bottom < 0 || bottom > image.rows || bottom > image.cols)
		return;

	if (left < 0 || left > image.rows || left > image.cols)
		return;

	if (right < 0 || right > image.rows || right > image.cols)
		return;

	image.rowRange(0, top).setTo(value);
	image.rowRange(image.rows - bottom, image.rows).setTo(value);
	image.colRange(0, left).setTo(value);
	image.colRange(image.cols - right, image.cols).setTo(value);
}

bool isPointInRect(Point P, Rect rect) {

	Point A = rect.tl();
	Point B(rect.tl().x + rect.width, rect.tl().y);
	Point C(rect.tl().x + rect.width, rect.tl().y + rect.height);
	Point D(rect.tl().x, rect.tl().y + rect.height);
	int x = P.x;
	int y = P.y;
	int a = (B.x - A.x)*(y - A.y) - (B.y - A.y)*(x - A.x);
	int b = (C.x - B.x)*(y - B.y) - (C.y - B.y)*(x - B.x);
	int c = (D.x - C.x)*(y - C.y) - (D.y - C.y)*(x - C.x);
	int d = (A.x - D.x)*(y - D.y) - (A.y - D.y)*(x - D.x);
	if ((a >= 0 && b >= 0 && c >= 0 && d >= 0) || (a <= 0 && b <= 0 && c <= 0 && d <= 0)) {
		return true;
	}

	//      AB X AP = (b.x - a.x, b.y - a.y) x (p.x - a.x, p.y - a.y) = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
	//      BC X BP = (c.x - b.x, c.y - b.y) x (p.x - b.x, p.y - b.y) = (c.x - b.x) * (p.y - b.y) - (c.y - b.y) * (p.x - b.x);
	return false;
}
