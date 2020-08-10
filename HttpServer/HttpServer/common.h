#pragma once
class common
{
};

enum PixelDataType
{
	PIXEL_TYPE_RGB = 0,
	PIXEL_TYPE_RGBA
}; 

int GetPixelDataTypeSize(PixelDataType type);