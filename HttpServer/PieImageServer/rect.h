#ifndef HTTPSERVER_RECT_H_
#define HTTPSERVER_RECT_H_


//���Ͻ�Ϊԭ�㣬���Һ�����Ϊ������
//������Ч��Χ��������Ϊright_,bottom_�ĵ�
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