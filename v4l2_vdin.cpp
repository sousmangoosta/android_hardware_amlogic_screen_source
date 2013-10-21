/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


//reinclude because of a bug with the log macros
//#define LOG_NDEBUG 0
#define LOG_TAG "V4L2VINSOURCE"
#include <utils/Log.h>
#include <utils/String8.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <linux/videodev.h>
#include <sys/time.h>

#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "v4l2_vdin.h"

namespace android {

vdin_screen_source::vdin_screen_source()
                  : mCameraHandle(-1),
                    mVideoInfo(NULL)
{
    mCameraHandle = open("/dev/video11", O_RDWR| O_NONBLOCK);
    if (mCameraHandle < 0)
    {
        ALOGE("[%s %d] mCameraHandle:%x", __FUNCTION__, __LINE__, mCameraHandle);
    }
    mVideoInfo = (struct VideoInfo *) calloc (1, sizeof (struct VideoInfo));
    if (mVideoInfo == NULL)
    {
        ALOGE("[%s %d] no memory for mVideoInfo", __FUNCTION__, __LINE__);
    }
}

vdin_screen_source::~vdin_screen_source()
{
    if (mCameraHandle >= 0)
    {
        close(mCameraHandle);
    }
    if (mVideoInfo)
    {
        free (mVideoInfo);
    }
}

int vdin_screen_source::start()
{
    int ret = -1;
	
    ALOGV("[%s %d] mCameraHandle:%x", __FUNCTION__, __LINE__, mCameraHandle);
	
    ioctl(mCameraHandle, VIDIOC_QUERYCAP, &mVideoInfo->cap);
	
    mVideoInfo->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->rb.memory = V4L2_MEMORY_MMAP;
    mVideoInfo->rb.count = NB_BUFFER;

    ret = ioctl(mCameraHandle, VIDIOC_REQBUFS, &mVideoInfo->rb);
    
    if (ret < 0) {
    	ALOGE("[%s %d] VIDIOC_REQBUFS:%d mCameraHandle:%x", __FUNCTION__, __LINE__, ret, mCameraHandle);
        return ret;
    }

    for (int i = 0; i < NB_BUFFER; i++) {
        memset (&mVideoInfo->buf, 0, sizeof (struct v4l2_buffer));

        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (mCameraHandle, VIDIOC_QUERYBUF, &mVideoInfo->buf);
        if (ret < 0) {
            ALOGE("[%s %d]VIDIOC_QUERYBUF %d failed", __FUNCTION__, __LINE__, i);
            return ret;
        }
        mVideoInfo->canvas[i] = mVideoInfo->buf.reserved;
        mVideoInfo->mem[i] = mmap (0, mVideoInfo->buf.length, PROT_READ | PROT_WRITE,
               MAP_SHARED, mCameraHandle, mVideoInfo->buf.m.offset);

        if (mVideoInfo->mem[i] == MAP_FAILED) {
            ALOGE("[%s %d] MAP_FAILED", __FUNCTION__, __LINE__);
            return -1;
        }

        mBufs.add((int)mVideoInfo->mem[i], i);
    }
    ALOGV("[%s %d] VIDIOC_QUERYBUF successful", __FUNCTION__, __LINE__);

    for (int i = 0; i < NB_BUFFER; i++) {
        mVideoInfo->buf.index = i;
        mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;
        ret = ioctl(mCameraHandle, VIDIOC_QBUF, &mVideoInfo->buf);
        if (ret < 0) {
            ALOGE("VIDIOC_QBUF Failed");
            return -1;
        }
    }
    enum v4l2_buf_type bufType;
    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
    ret = ioctl (mCameraHandle, VIDIOC_STREAMON, &bufType);

    ALOGV("[%s %d] VIDIOC_STREAMON:%x", __FUNCTION__, __LINE__, ret);
	
    return ret;
}

int vdin_screen_source::stop()
{
    int ret;
    enum v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl (mCameraHandle, VIDIOC_STREAMOFF, &bufType);
    if (ret < 0) {
        ALOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
    }
    for (int i = 0; i < NB_BUFFER; i++)
    {
        if (munmap(mVideoInfo->mem[i], mVideoInfo->buf.length) < 0)
            ALOGE("Unmap failed");
    }
    return ret;
}

int vdin_screen_source::get_format()
{
    return 0;
}

int vdin_screen_source::set_format(int width, int height, int color_format)
{
    ALOGV("[%s %d]", __FUNCTION__, __LINE__);
    int ret;
    
    mVideoInfo->width = width;
    mVideoInfo->height = height;
    mVideoInfo->framesizeIn = (width * height << 1);      //note color format
    mVideoInfo->formatIn = color_format;

    mVideoInfo->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->format.fmt.pix.width = width;
    mVideoInfo->format.fmt.pix.height = height;
    mVideoInfo->format.fmt.pix.pixelformat = color_format;

    ret = ioctl(mCameraHandle, VIDIOC_S_FMT, &mVideoInfo->format);
    if (ret < 0) {		
        ALOGE("[%s %d]VIDIOC_S_FMT %d", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

int vdin_screen_source::set_rotation(int degree)
{
    ALOGV("[%s %d]", __FUNCTION__, __LINE__);
    int ret = 0;
    return ret;
}


int vdin_screen_source::aquire_buffer(unsigned *buff_info)
{   
    int ret = -1;

    mVideoInfo->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    mVideoInfo->buf.memory = V4L2_MEMORY_MMAP;
	
    ret = ioctl(mCameraHandle, VIDIOC_DQBUF, &mVideoInfo->buf);
    if (ret < 0) {
        if(EAGAIN == errno){
            ret = -EAGAIN;
        }else{
            ALOGE("[%s %d]aquire_buffer %d", __FUNCTION__, __LINE__, ret);
        }
        buff_info[0] = 0;
        buff_info[1] = 0;
        return ret;
    }
    buff_info[0] = (unsigned)mVideoInfo->mem[mVideoInfo->buf.index];
    buff_info[1] = (unsigned)mVideoInfo->canvas[mVideoInfo->buf.index];
    return ret;
}

int vdin_screen_source::release_buffer(char* ptr)
{
    int ret = -1;
    int CurrentIndex;
    v4l2_buffer hbuf_query;
    memset(&hbuf_query,0,sizeof(v4l2_buffer));
    
    CurrentIndex = mBufs.valueFor(( unsigned int )ptr);
    hbuf_query.index = CurrentIndex;
    hbuf_query.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    hbuf_query.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(mCameraHandle, VIDIOC_QBUF, &hbuf_query);
    
    return 0;
}

}
