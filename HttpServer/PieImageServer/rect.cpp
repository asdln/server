#include "rect.h"


bool Rect::Intersect(size_t x, size_t y)
{
	if (x >= left_ && x <= right_ && y >= top_ && y <= bottom_)
		return true;

	return false;
}