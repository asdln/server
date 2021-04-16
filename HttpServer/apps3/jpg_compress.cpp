#include "jpg_compress.h"

#include "jpeglib.h"

void error_exit(j_common_ptr cinfo) {
	fprintf(stderr, "JpgCompress error\n", "http_server");
}

void JpgCompress::DoCompress(void* srcbuffer, int nSrcWidth, int nSrcHeight, std::vector<unsigned char>& output)
{
	void* dstBuffer = nullptr;
	unsigned long pOutSize = 0;

	jpeg_compress_struct toWriteInfo;
	jpeg_error_mgr errorMgr;
	toWriteInfo.err = jpeg_std_error(&errorMgr);

	//注册失败的回调函数
	toWriteInfo.err->error_exit = error_exit;
	jpeg_create_compress(&toWriteInfo);

	//确定要用于输出压缩的jpeg的数据空间
	jpeg_mem_dest(&toWriteInfo, (unsigned char**)&dstBuffer, &pOutSize);

	toWriteInfo.image_width = nSrcWidth;
	toWriteInfo.image_height = nSrcHeight;

	//toWriteInfo.jpeg_width = nWidth / 2;
	//toWriteInfo.jpeg_height = nHeight / 2;

	toWriteInfo.input_components = 3;// 在此为1,表示灰度图， 如果是彩色位图，则为4
	toWriteInfo.in_color_space = JCS_EXT_RGB; //JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像 

	jpeg_set_defaults(&toWriteInfo);
	jpeg_set_quality(&toWriteInfo, 100, TRUE);	//设置压缩质量100表示100%
	jpeg_start_compress(&toWriteInfo, TRUE);

	int nRowStride = nSrcWidth * 3;	// 如果不是索引图,此处需要乘以4
	JSAMPROW row_pointer[1];	// 一行位图

	while (toWriteInfo.next_scanline < toWriteInfo.image_height)
	{
		row_pointer[0] = (JSAMPROW)((unsigned char*)srcbuffer + toWriteInfo.next_scanline * nRowStride);
		jpeg_write_scanlines(&toWriteInfo, row_pointer, 1);
	}

	jpeg_finish_compress(&toWriteInfo);
	jpeg_destroy_compress(&toWriteInfo);

	std::vector<unsigned char> buffer((unsigned char*)dstBuffer, (unsigned char*)dstBuffer + pOutSize);
	output.swap(buffer);

	if (dstBuffer != nullptr)
		free(dstBuffer);

}
