//
// Created by root on 18-5-23.
//
#include "CameraDecode.h"
#include "iostream"
int main(int argc, char *argv[])
{
    std::cout << "H264 Carema Test----start" << std::endl;
    CameraDecode cameraDecode;
    cameraDecode.mVideoName = "/dev/video1";
    cameraDecode.mSaveFlag = false;
    cameraDecode.mH264DecodeFlag = false;
    cameraDecode.mRecordTime = 3;
    cameraDecode.mFilename = "60sec.H264";
    cameraDecode.DeviceInit();
    cameraDecode.CaptureAndProcess();
    std::cout << "H264 Carema Test----stop" << std::endl;
    return 0;
}
