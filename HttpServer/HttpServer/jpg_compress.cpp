#include "jpg_compress.h"

#include "cdjpeg.h"             /* Common decls for cjpeg/djpeg applications */
#include "jversion.h"           /* for version message */
#include "jconfigint.h"

static const char* const cdjpeg_message_table[] = {
#include "cderror.h"
  NULL
};

void error_exit(j_common_ptr cinfo) {

}

int Bmp2Jpeg_Compress(void* lpBmpBuffer, int nWidth, int nHeight, void** ppJpegBuffer, unsigned long* pOutSize)
{
	jpeg_compress_struct toWriteInfo;
	jpeg_error_mgr errorMgr;
	toWriteInfo.err = jpeg_std_error(&errorMgr);
	//注册失败的回调函数
	toWriteInfo.err->error_exit = error_exit;
	jpeg_create_compress(&toWriteInfo);
	//保存压缩后的图片
	//FILE* fp = NULL;
	//_wfopen_s(&fp, L"c:\\output.jpg", L"wb+");
	//jpeg_stdio_dest(&toWriteInfo, fp);
	//确定要用于输出压缩的jpeg的数据空间
	jpeg_mem_dest(&toWriteInfo, (unsigned char**)ppJpegBuffer, pOutSize);
	toWriteInfo.image_width = nWidth;
	toWriteInfo.image_height = nHeight;
	//toWriteInfo.jpeg_width = nWidth / 2;
	//toWriteInfo.jpeg_height = nHeight / 2;
	toWriteInfo.input_components = 4;// 在此为1,表示灰度图， 如果是彩色位图，则为4
	toWriteInfo.in_color_space = JCS_EXT_BGRA; //JCS_GRAYSCALE表示灰度图，JCS_RGB表示彩色图像 
	jpeg_set_defaults(&toWriteInfo);
	jpeg_set_quality(&toWriteInfo, 100, TRUE);	//设置压缩质量100表示100%
	jpeg_start_compress(&toWriteInfo, TRUE);
	int nRowStride = nWidth * 4;	// 如果不是索引图,此处需要乘以4
	JSAMPROW row_pointer[1];	// 一行位图
	while (toWriteInfo.next_scanline < toWriteInfo.image_height)
	{
		row_pointer[0] = (JSAMPROW)((unsigned char*)lpBmpBuffer + toWriteInfo.next_scanline * nRowStride);
		jpeg_write_scanlines(&toWriteInfo, row_pointer, 1);
	}
	jpeg_finish_compress(&toWriteInfo);
	jpeg_destroy_compress(&toWriteInfo);
	return 0;
}

bool JpgCompress::Compress(void* srcbuffer, int nSrcWidth, int nSrcHeight, void** dstBuffer, unsigned long* pOutSize)
{
	//struct jpeg_compress_struct cinfo;
	//struct jpeg_error_mgr jerr;

	///* Initialize the JPEG compression object with default error handling. */
	//cinfo.err = jpeg_std_error(&jerr);
	//jpeg_create_compress(&cinfo);

	///* Add some application-specific error messages (from cderror.h) */
	//jerr.addon_message_table = cdjpeg_message_table;
	//jerr.first_addon_message = JMSG_FIRSTADDONCODE;
	//jerr.last_addon_message = JMSG_LASTADDONCODE;

	///* Initialize JPEG parameters.
	// * Much of this may be overridden later.
	// * In particular, we don't yet know the input file's color space,
	// * but we need to provide some value for jpeg_set_defaults() to work.
	// */

	//cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
	//jpeg_set_defaults(&cinfo);

	//unsigned char* outbuffer = NULL;
	//unsigned long outsize = 0;

	//jpeg_mem_dest(&cinfo, &outbuffer, &outsize);

	//jpeg_start_compress(&cinfo, TRUE);

	//int nWidth = 100;
	//int nHeight = 100;

	//while (cinfo.next_scanline < cinfo.image_height) {
	//	num_scanlines = (*src_mgr->get_pixel_rows) (&cinfo, src_mgr);
	//	(void)jpeg_write_scanlines(&cinfo, src_mgr->buffer, num_scanlines);
	//}

	//jpeg_finish_compress(&cinfo);
	//jpeg_destroy_compress(&cinfo);

	//jpeg_compress_struct toWriteInfo;


	//free(outbuffer);

	return true;
}