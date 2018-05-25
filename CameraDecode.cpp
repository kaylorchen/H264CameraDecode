//
// Created by root on 18-5-23.
//




#include "CameraDecode.h"


static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

CameraDecode::CameraDecode():mVideoName("/dev/video0"),mFramerate(30),mWidth(640),mHeight(480),
                               mFormat(V4L2_PIX_FMT_H264),mNumberOfRequestbuffers(N_BUFFER),
                               mFilename("test.H264"),mRecordTime(5),mSaveFlag(false),mH264DecodeFlag(false)
{
    std::cout << "Initializing Variables" << std::endl;
}

CameraDecode::~CameraDecode()
{
    std::cout << "Exit Camera Decode" <<std::endl;
    if (mH264DecodeFlag == true)
        H264DecodeUninit();
    StopCapture();
}

void CameraDecode::OpenCamera(std::string const &videoname)
{

    std::cout << "Opening" << mVideoName << std::endl;
    mDevice = open(videoname.c_str(),O_RDWR);
    if (mDevice == -1)
    {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
                videoname.c_str(), errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

}

void CameraDecode::CaptureAndProcess()
{
    StartCapture();
    time_t start;
    int count = 1;
    if (mSaveFlag == true)
        mH264File = fopen(mFilename.c_str(),"wb+");
    start = time(NULL);
    while( difftime(time(NULL),start) < mRecordTime || mRecordTime == 0) // default: 5 seconds
    {
        memset(&mBuf, 0, sizeof mBuf);
        mBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mBuf.memory = V4L2_MEMORY_MMAP;
        Dequeue();
        printf("Frame[%4u] inddex = %2d %u bytes %d.%06d \n", count, mBuf.index, mBuf.bytesused, (int)mBuf.timestamp.tv_sec, (int)mBuf.timestamp.tv_usec);
        if (mSaveFlag == true)
            fwrite(mMem[mBuf.index], mBuf.bytesused, 1, mH264File);
        if (mH264DecodeFlag == true)
            H264Decode();
        Requeue();
        count ++;
    }
    if (mSaveFlag == true)
        fclose(mH264File);
}

void CameraDecode::Dequeue()
{
    int ret = 0;
    do{
        ret = ioctl(mDevice, VIDIOC_DQBUF, &mBuf);
    }while (ret < 0);
}

void CameraDecode::Requeue()
{
    int ret = ioctl(mDevice, VIDIOC_QBUF, &mBuf);
    if(ret < 0)
    {
        printf("Unable to requeue buffer0\n");
        fclose(mH264File);
        exit(2);
    }
}

void CameraDecode::H264Decode()
{
    int frameFinished = 0;
    mPacket.size = mBuf.bytesused;
    mPacket.data = (uint8_t *)mMem[mBuf.index];
    while(mPacket.size > 0)
    {
        frameFinished = 0;
        int decodelen =avcodec_decode_video2(mCodec, mFrame, &frameFinished, &mPacket);
        mPacket.size -= decodelen;
        mPacket.data += decodelen;
        if (frameFinished > 0)
        {
            std::cout << "Docode successful" << std::endl;
        }
    }
}

bool CameraDecode::H264DecodeInit()
{
    std::cout << "H264 Deocode Init..." << std::endl;
    av_register_all();

    /** find the video encoder **/
    mVideoCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(!mVideoCodec)
    {
        printf("avcodec_find_decoder error\n");
        return false;
    }

    mCodec = avcodec_alloc_context3(mVideoCodec);
    if(!mCodec)
    {
        printf("avcodec_alloc_context3  error\n");
        return false;
    }

    mCodec->time_base.num = 1;
    mCodec->frame_number = 1;
    mCodec->codec_type = AVMEDIA_TYPE_VIDEO;
    mCodec->bit_rate = 0;
    mCodec->time_base.den = 30;
    mCodec->width = mWidth;
    mCodec->height = mHeight;
    if(avcodec_open2(mCodec, mVideoCodec, NULL) >= 0)
    {

        mFrame = av_frame_alloc();
        if (!mFrame) {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }
    }
    else
    {
        printf("avcodec_open2 error\n");
        return false;
    }
    av_init_packet(&mPacket);
    return true;
}

bool CameraDecode::H264DecodeUninit()
{
    std::cout << "H264 Deocode Uninit..." << std::endl;
    avcodec_close(mCodec);
    av_free(mCodec);
    av_free_packet(&mPacket);
    av_frame_free(&mFrame);
    return true;
}

void CameraDecode::DeviceInit()
{
    if (mH264DecodeFlag == true)
        H264DecodeInit();
    OpenCamera(mVideoName);
    struct v4l2_capability cap;
    std::cout << "Read camera's capability.\n";
    memset(&cap, 0, sizeof cap);
    int ret = 0;
    ret = ioctl(mDevice, VIDIOC_QUERYCAP, &cap);
    if (ret == -1)
    {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n",
                    mVideoName.c_str());
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n",
                mVideoName.c_str());
        exit(EXIT_FAILURE);
    }
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n",
                mVideoName.c_str());
        exit(EXIT_FAILURE);
    }

    struct v4l2_fmtdesc fmtdesc;
    printf("Enumerate supporting pixel format\n");
    memset(&fmtdesc, 0, sizeof fmtdesc);
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while ((ret = ioctl(mDevice, VIDIOC_ENUM_FMT, &fmtdesc)) == 0)
    {
        printf("index = %d, pixelformat = %c%c%c%c, description = %s\n", fmtdesc.index,
               fmtdesc.pixelformat & 0xFF, (fmtdesc.pixelformat >> 8) & 0xFF,
               (fmtdesc.pixelformat >> 16) & 0xFF, (fmtdesc.pixelformat >> 24) & 0xFF,
               fmtdesc.description);
        fmtdesc.index++;
    }

    struct v4l2_format fmt;
    printf("Configure format, width and height.\n");
    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = mWidth;
    fmt.fmt.pix.height = mHeight;
    fmt.fmt.pix.pixelformat = mFormat;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    ret = ioctl(mDevice, VIDIOC_S_FMT, &fmt);
    if(ret == -1)
    {
        errno_exit("VIDIOC_S_FMT");
    }

    printf("Configure framerate\n");
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof parm);
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(mDevice, VIDIOC_G_PARM, &parm);
    if (ret == -1)
    {
        errno_exit("VIDIOC_G_PARM");
    }
    printf("   Original: numerator = %d, denominator = %d\n",parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = mFramerate;
    printf("Expectation: numerator = %d, denominator = %d\n",parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);
    ret = ioctl(mDevice, VIDIOC_S_PARM, &parm);
    if (ret == -1)
    {
        errno_exit("VIDIOC_S_PARM");
    }
    ret = ioctl(mDevice, VIDIOC_G_PARM, &parm);
    if (ret == -1)
    {
        errno_exit("VIDIOC_G_PARM");
    }
    printf("     Actual: numerator = %d, denominator = %d\n",parm.parm.capture.timeperframe.numerator, parm.parm.capture.timeperframe.denominator);

    printf("Request video buffers\n");
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof rb);
    rb.count = mNumberOfRequestbuffers;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    ioctl(mDevice, VIDIOC_REQBUFS, &rb);
    mNumberOfRequestbuffers = rb.count;

    printf("Mmap video buffers\n");
    for (int i = 0; i < mNumberOfRequestbuffers; ++i) {
        memset(&mBuf, 0, sizeof mBuf);
        mBuf.index = i;
        mBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mBuf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(mDevice, VIDIOC_QUERYBUF, &mBuf);
        if (ret == -1)
        {
            errno_exit("VIDIOC_QUERYBUF");
        }
        mMem[i] = mmap(0, mBuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, mDevice, mBuf.m.offset);
    }

    printf("Insert video buffers into queue.\n");
    for (int i = 0; i < mNumberOfRequestbuffers; ++i) {
        memset(&mBuf, 0, sizeof mBuf);
        mBuf.index = i;
        mBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mBuf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(mDevice, VIDIOC_QBUF, &mBuf);
        if (ret == -1)
        {
            errno_exit("VIDIOC_QBUF");
        }
    }

}

void CameraDecode::StartCapture()
{
    printf("Start Capturing video stream.\n");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(mDevice, VIDIOC_STREAMON, &type);
    if (ret == -1)
    {
        errno_exit("VIDIOC_STREAMON");
    }
}

void CameraDecode::StopCapture()
{
    printf("Stop Capturing video stream.\n");
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(mDevice, VIDIOC_STREAMOFF, &type);
    if (ret == -1)
    {
        errno_exit("VIDIOC_STREAMOFF");
    }
}