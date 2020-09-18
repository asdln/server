#include "webp_compress.h"

//#include "encode.h"
#include "webp/encode.h"
#include "png_buffer.h"

static int MyWriter(const uint8_t* data, size_t data_size,
    const WebPPicture* const pic) {

    PngBuffer* buffer = (PngBuffer*)pic->custom_ptr;
    std::vector<unsigned char>& p = buffer->GetVec();
    p.insert(p.end(), data, data + data_size);
    return 1;
}

BufferPtr WebpCompress::DoCompress(void* lpBmpBuffer, int nWidth, int nHeight)
{
    WebPConfig config;
    WebPConfigInit(&config);

    if (!WebPValidateConfig(&config))
        return nullptr;

    config.quality = 100.0;

    WebPPicture picture;
    WebPPictureInit(&picture);

    picture.use_argb = 1;
    picture.width = nWidth;
    picture.height = nHeight;

    int stride = nWidth * 4;
    WebPPictureImportRGBA(&picture, (const uint8_t*)lpBmpBuffer, stride);

    PngBuffer* pngBuffer = new PngBuffer;
    std::shared_ptr<PngBuffer> buffer(pngBuffer);

    FILE* out = nullptr;
    picture.writer = MyWriter;
    picture.custom_ptr = (void*)pngBuffer;

    if (!WebPEncode(&config, &picture))
        return nullptr;

    WebPFree(picture.extra_info);
    WebPPictureFree(&picture);

    return buffer;
}
