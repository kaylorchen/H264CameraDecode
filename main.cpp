#include <iostream>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <zconf.h>


extern "C"
{
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "linux/videodev2.h"
#include "sys/mman.h"
}
#define WIDTH 640
#define HEIGHT 480

int getInt();

int main() {

    int count;
    FILE *myH264;
    return getInt();

    fseek(myH264,0,SEEK_SET);
    count = 1;
    int screen_w = WIDTH, screen_h = HEIGHT;
    const int pixel_w = WIDTH, pixel_h = HEIGHT;
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
    codec_->width = WIDTH;
    codec_->height = HEIGHT;
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


    int readFileLen = 1;
    unsigned char readBuf[1024*50];
    unsigned char *parseBuf = (unsigned char *)malloc(20*1024*50);
    int  parseBufLen = 0;

    while(readFileLen > 0)
    {
        readFileLen = fread(readBuf, 1, sizeof(readBuf), myH264);
        if (readFileLen <=0 )
        {
            std::cout << "readFilelen = " << readFileLen << ", Read file over\n";
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
       /*                 uint8_t *YUV = (uint8_t *)malloc(WIDTH*HEIGHT*3);
                        memcpy(YUV,mFrame->data[0],WIDTH*HEIGHT);
                        for (int num = 0; num < WIDTH*HEIGHT/2; ++num) {
                            *(YUV + 2*num + WIDTH*HEIGHT) = *(mFrame->data[1]+num);
                            *(YUV + 2*num + WIDTH*HEIGHT + 1) = *(mFrame->data[1]+num);
                            *(YUV + 2*num + WIDTH*HEIGHT*2) = *(mFrame->data[1]+num);
                            *(YUV + 2*num + WIDTH*HEIGHT*2 + 1) = *(mFrame->data[1]+num);
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
                        SDL_Delay(33);
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

int getInt() {
    void *mem0[32];
    unsigned int nbufs = 32;
    struct v4l2_buffer buf0;
    struct v4l2_capability cap;
    int dev;
    int count = 1;

    printf("Open camera\n");
    // dev = open("/dev/video1", O_RDWR | O_NONBLOCK);
    dev = open("/dev/video2", O_RDWR );
    memset(&cap, 0, sizeof cap);
    ioctl(dev, VIDIOC_QUERYCAP, &cap);
    printf("Configure Format\n");
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    ioctl(dev, VIDIOC_S_FMT, &fmt);
    printf("sizeimage = %d\n",fmt.fmt.pix.sizeimage);
    ioctl(dev, VIDIOC_G_FMT, &fmt);
    printf("sizeimage = %d\n",fmt.fmt.pix.sizeimage);

    printf("Configure framerate\n");
    struct v4l2_streamparm parm;

    memset(&parm, 0, sizeof parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ioctl(dev, VIDIOC_G_PARM, &parm);
    printf("   Original: numerator = %d, denominator = %d\n",parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = 30;

    printf("Expectation: numerator = %d, denominator = %d\n",parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);

    ioctl(dev, VIDIOC_S_PARM, &parm);
    ioctl(dev, VIDIOC_G_PARM, &parm);
    printf("     Actual: numerator = %d, denominator = %d\n",parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);

    printf("Request video buffers\n");
    struct v4l2_requestbuffers rb;

    memset(&rb, 0, sizeof rb);
    rb.count = nbufs;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;

    ioctl(dev, VIDIOC_REQBUFS, &rb);
    nbufs = rb.count;

    printf("Mmap video buffers\n");
    for (int i = 0; i < nbufs; ++i) {
        memset(&buf0, 0, sizeof buf0);
        buf0.index = i;
        buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf0.memory = V4L2_MEMORY_MMAP;
        ioctl(dev, VIDIOC_QUERYBUF, &buf0);

        mem0[i] = mmap(0, buf0.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev, buf0.m.offset);
    }
    printf("Insert video buffers into queue.\n");
    for (int i = 0; i < nbufs; ++i) {
        memset(&buf0, 0, sizeof buf0);
        buf0.index = i;
        buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf0.memory = V4L2_MEMORY_MMAP;
        ioctl(dev, VIDIOC_QBUF, &buf0);
    }

    printf("Capture video stream.\n");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(dev, VIDIOC_STREAMON, &type);

    printf("Record video \n");
    // system("rm test.H264");
    FILE *myH264 = fopen("test.H264", "wb+");
    if(myH264 == NULL)
    {
        perror("cant open 264 file\n");
        return -1;
    }
    count = 0;
    while(count ++ < 90)
    {
        int ret = 0;
        memset(&buf0, 0, sizeof buf0);
        buf0.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf0.memory = V4L2_MEMORY_MMAP;
        do{
            ret = ioctl(dev, VIDIOC_DQBUF, &buf0);
            //usleep(1000*5);
            //std::cout << "ret = " << ret <<std::endl;

        }while (ret < 0);
        if( ret < 0)
        {
            printf("Unable to dequeue buffer0\n");
            fclose(myH264);
            exit(1);
        }
        fwrite(mem0[buf0.index], buf0.bytesused, 1, myH264);
        printf("Frame[%4u] inddex = %2d %u bytes %d.%06d \n", count, buf0.index, buf0.bytesused, (int)buf0.timestamp.tv_sec, (int)buf0.timestamp.tv_usec);
        ret = ioctl(dev, VIDIOC_QBUF, &buf0);
        if(ret <0)
        {
            printf("Unable to requeue buffer0\n");
            fclose(myH264);
            exit(2);
        }
    }
    fclose(myH264);
    return 0;
}