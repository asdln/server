#ifndef HTTPSERVER_RECT_H_
#define HTTPSERVER_RECT_H_


//左上角为原点，向右和向下为正方向
//矩形有效范围包括坐标为right_,bottom_的点
class Rect
{
public:

	Rect(size_t left, size_t top, size_t right, size_t bottom)
		:left_(left), top_(top), right_(right), bottom_(bottom){}

	bool Intersect(size_t x, size_t y);

protected:

	size_t left_;
	size_t top_;
	size_t right_;
	size_t bottom_;
	
};

#endif //HTTPSERVER_RECT_H_