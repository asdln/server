#include "png_compress.h"
#include "png_buffer.h"

#include <png.h>
#include <vector>
#include <iostream>
#include <stdlib.h>
//encode and write PNG to memory (std::vector) with libpng on C++
typedef unsigned char ui8;
#define ASSERT_EX(cond, error_message) do { if (!(cond)) { std::cerr << error_message; exit(1);} } while(0)

static void PngWriteCallback(png_structp  png_ptr, png_bytep data, png_size_t length) {
	//std::vector<ui8>* p = (std::vector<ui8>*)png_get_io_ptr(png_ptr);
	//p->insert(p->end(), data, data + length);

	PngBuffer* buffer = (PngBuffer*)png_get_io_ptr(png_ptr);
	std::vector<ui8>& p = buffer->GetVec();
	p.insert(p.end(), data, data + length);
}
struct TPngDestructor {
	png_struct* p;
	TPngDestructor(png_struct* p) : p(p) {}
	~TPngDestructor() { if (p) { png_destroy_write_struct(&p, NULL); } }
};
void WritePngToMemory(size_t w, size_t h, const ui8* dataRGBA, PngBuffer* out) {
	
	png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	ASSERT_EX(p, "png_create_write_struct() failed");
	TPngDestructor destroyPng(p);
	png_infop info_ptr = png_create_info_struct(p);
	ASSERT_EX(info_ptr, "png_create_info_struct() failed");
	ASSERT_EX(0 == setjmp(png_jmpbuf(p)), "setjmp(png_jmpbuf(p) failed");
	png_set_IHDR(p, info_ptr, w, h, 8,
		PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	//png_set_compression_level(p, 1);
	std::vector<ui8*> rows(h);
	for (size_t y = 0; y < h; ++y)
		rows[y] = (ui8*)dataRGBA + y * w * 4;
	png_set_rows(p, info_ptr, &rows[0]);
	png_set_write_fn(p, out, PngWriteCallback, NULL);
	png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
}

BufferPtr PngCompress::DoCompress(void* lpBmpBuffer, int nWidth, int nHeight)
{
	unsigned char* ppJpegBuffer;
	unsigned long pOutSize = 0;
	PngBuffer* pngBuffer = new PngBuffer;
	std::shared_ptr<PngBuffer> buffer(pngBuffer);

	WritePngToMemory(nWidth, nHeight, (const ui8*)lpBmpBuffer, pngBuffer);

	return buffer;
}