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

	//ע��ʧ�ܵĻص�����
	toWriteInfo.err->error_exit = error_exit;
	jpeg_create_compress(&toWriteInfo);

	//ȷ��Ҫ�������ѹ����jpeg�����ݿռ�
	jpeg_mem_dest(&toWriteInfo, (unsigned char**)&dstBuffer, &pOutSize);

	toWriteInfo.image_width = nSrcWidth;
	toWriteInfo.image_height = nSrcHeight;

	//toWriteInfo.jpeg_width = nWidth / 2;
	//toWriteInfo.jpeg_height = nHeight / 2;

	toWriteInfo.input_components = 3;// �ڴ�Ϊ1,��ʾ�Ҷ�ͼ�� ����ǲ�ɫλͼ����Ϊ4
	toWriteInfo.in_color_space = JCS_EXT_RGB; //JCS_GRAYSCALE��ʾ�Ҷ�ͼ��JCS_RGB��ʾ��ɫͼ�� 

	jpeg_set_defaults(&toWriteInfo);
	jpeg_set_quality(&toWriteInfo, 100, TRUE);	//����ѹ������100��ʾ100%
	jpeg_start_compress(&toWriteInfo, TRUE);

	int nRowStride = nSrcWidth * 3;	// �����������ͼ,�˴���Ҫ����4
	JSAMPROW row_pointer[1];	// һ��λͼ

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
