#include <iostream>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL.h>


extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}


int main() {
    std::cout << "Hello, World!" << std::endl;

    int screen_w = 640, screen_h = 480;
    const int pixel_w = 640, pixel_h = 480;
    SDL_Window *sdlWindow;
    SDL_Renderer *sdlRenderer;
    SDL_Texture *sdlTexture;
    SDL_Rect sdlRect;

    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    sdlWindow = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                 screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (sdlWindow == 0)
    {
        printf("SDL: could not create SDL_Window - exiting:%s\n", SDL_GetError());
        return -1;
    }

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    if (sdlRenderer == NULL)
    {
        printf("SDL: could not create SDL_Renderer - exiting:%s\n", SDL_GetError());
        return -1;
    }
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);
    if (sdlTexture == NULL)
    {
        printf("SDL: could not create SDL_Texture - exiting:%s\n", SDL_GetError());
        return -1;
    }


    /**Decode H264 to YUV**/

    AVFrame *pFrame_;
    AVPacket packet = {0};
    int frameFinished;
    AVFrame *pFrameYUV;
    struct SwsContext *img_convert_ctx =NULL;
    pFrameYUV =av_frame_alloc();

    av_register_all();

    /** find the video encoder **/
    AVCodec *videoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!videoCodec)
    {
        printf("avcodec_find_decoder error\n");
        return -1;
    }

    AVCodecParserContext *avParserContext = av_parser_init(AV_CODEC_ID_H264);
    if(!avParserContext)
    {
        printf("av_parser_init  error\n");
        return -1;
    }
    AVCodecContext *codec_ = avcodec_alloc_context3(videoCodec);
    if(!codec_)
    {
        printf("avcodec_alloc_context3  error\n");
        return -1;
    }

    codec_->time_base.num = 1;
    codec_->frame_number = 1;
    codec_->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_->bit_rate = 0;
    codec_->time_base.den = 30;
    codec_->width = 640;
    codec_->height = 480;
    if(avcodec_open2(codec_, videoCodec, NULL) >= 0)//打开解码器
    {

        pFrame_ = av_frame_alloc();
        if (!pFrame_) {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }
    }
    else
    {
        printf("avcodec_open2 error\n");
        return -1;
    }
    av_init_packet(&packet);

    FILE *myH264 = fopen("1.H264", "rb");//解码的文件264
    if(myH264 == NULL)
    {
        perror("cant open 264 file\n");
        return -1;
    }
    int readFileLen = 1;
    unsigned char readBuf[1024*50];
    unsigned char *parseBuf = (unsigned char *)malloc(20*1024*50);
    int  parseBufLen = 0;
    int count = 1;
    while(readFileLen > 0)
    {
        readFileLen = fread(readBuf, 1, sizeof(readBuf), myH264);
        if (readFileLen <=0 )
        {
            std::cout << "Read file over\n";
            break;
        } else
        {
            int handleLen = 0;
            int handleFileLen = readFileLen;
            while (handleFileLen > 0)
            {
                int nLength = av_parser_parse2(avParserContext, codec_, &parseBuf, &parseBufLen, readBuf + handleLen, handleFileLen, 0, 0, 0);//查找264帧头

                handleFileLen -= nLength;
                handleLen += nLength;
                if(parseBufLen <= 0)//当parseBufLen大于0时，说明查找到了帧头
                {
                    continue;
                }
                packet.size = parseBufLen;//将查找到的帧长度送入
                packet.data = parseBuf;//将查找到的帧内存送入
                while (packet.size > 0)
                {
                    frameFinished = 0;
                    int decodelen =avcodec_decode_video2(codec_, pFrame_, &frameFinished, &packet);
                    packet.size -= decodelen;
                    packet.data += decodelen;

                    if(frameFinished > 0)
                    {
                        printf("count = %d\n", count++);
      //                  printf("%d %d\n",avpicture_get_size(AV_PIX_FMT_YUYV422, codec_->width,codec_->height), 640*480*2);
       /*                 uint8_t *YUV = (uint8_t *)malloc(640*480*3);
                        memcpy(YUV,pFrame_->data[0],640*480);
                        for (int num = 0; num < 320*480; ++num) {
                            *(YUV + 2*num + 640*480) = *(pFrame_->data[1]+num);
                            *(YUV + 2*num + 640*480 + 1) = *(pFrame_->data[1]+num);
                            *(YUV + 2*num + 640*480*2) = *(pFrame_->data[1]+num);
                            *(YUV + 2*num + 640*480*2 + 1) = *(pFrame_->data[1]+num);
                        }*/

                       enum AVPixelFormat FMT = AV_PIX_FMT_NV12;
                        uint8_t *yuv = (uint8_t *) av_malloc(avpicture_get_size(FMT, codec_->width,codec_->height)* sizeof(uint8_t));
                        avpicture_fill((AVPicture *)pFrameYUV, yuv, FMT, codec_->width, codec_->height);
                        img_convert_ctx = sws_getContext(codec_->width, codec_->height, codec_->pix_fmt, codec_->width, codec_->height, FMT, 2, NULL, NULL, NULL);
                        sws_scale(img_convert_ctx, (const uint8_t* const*) pFrame_->data,  pFrame_->linesize, 0, codec_->height, pFrameYUV->data,pFrameYUV->linesize);
                        SDL_UpdateTexture(sdlTexture, NULL, yuv, pixel_w);

                        sdlRect.x = 0;
                        sdlRect.y = 0;
                        sdlRect.w = screen_w;
                        sdlRect.h = screen_h;

                        SDL_RenderClear(sdlRenderer);
                        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
                        SDL_RenderPresent(sdlRenderer);

                        av_free(yuv);
                        SDL_Delay(50);
                /*        free(YUV);
                        YUV =NULL;*/

                    } else
                    {
                        printf("failed to decodec\n");
                    }

                }
            }
        }
    }
    avcodec_close(codec_);
    av_free(codec_);
    av_free_packet(&packet);
    av_frame_free(&pFrame_);
    av_frame_free(&pFrameYUV);
    fclose(myH264);

    return 0;
}