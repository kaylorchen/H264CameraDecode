//
// Created by Jiapeng on 18-5-23.
//

#ifndef DECODE_H264_RECORD_H
#define DECODE_H264_RECORD_H


#include "cstdio"
#include "sys/ioctl.h"
#include "iostream"
#include "fcntl.h"
#include "cstring"


extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "linux/videodev2.h"
#include "sys/mman.h"
}
#define N_BUFFER 32 //N_BUFFER shouldn't be greater than 32

class CameraDecode {
public:
    CameraDecode();
    ~CameraDecode();
    void DeviceInit();
    void CaptureAndProcess();
    void StartCapture();
    void StopCapture();
    bool H264DecodeInit();
    bool H264DecodeUninit();
    void H264Decode();

    /**
     * mVideoName is a video device name, default: /dev/video0
     * mFilename is a saved file name, default: test.H264
     * mWidth is number of horizontal pixels, default: 640
     * mHeight is number of vertical pixels, default: 480
     * mFramerate is frame rate of video, default: 30
     * mFormat is format of video, default: V4L2_PIX_FMT_H264 (linux/videodev2.h)
     * mNumberOfRequestbuffers is number of Requestbuffers, default: 32
     * mRecordTime is time of recording, default 5 (Unit: sec), 0 means infinity.
     * mSaveFlag is the flag of recording, default: false (H264 will not been saved when mSaveFlag is false)
     * mH264DecodeFlag is the flag of H264 Decoding, default: false (the frame will not decoded when mH264DecodeFlag is false )
     * ********************************************************************************
     * You can modify the above parameters before calling DeviceInit() and CaptureAndProcess();
     * ********************************************************************************
     * **/
    std::string mVideoName;
    std::string mFilename;
    uint32_t mWidth;
    uint32_t mHeight;
    uint16_t mFramerate;
    uint32_t mFormat;
    uint32_t mNumberOfRequestbuffers;
    uint32_t mRecordTime;
    AVFrame *mFrame;
    bool mSaveFlag;
    bool mH264DecodeFlag;

private:
    void OpenCamera(std::string const &videoname);
    void Dequeue();
    void Requeue();
    int mDevice;
    struct v4l2_buffer mBuf;
    void *mMem[N_BUFFER];
    FILE *mH264File;
    AVCodec *mVideoCodec;
    AVCodecContext *mCodec;
    AVPacket mPacket;

};


#endif //DECODE_H264_RECORD_H
