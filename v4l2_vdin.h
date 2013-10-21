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

#include <utils/KeyedVector.h>
#include <cutils/properties.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace android {

#define NB_BUFFER 4

struct VideoInfo {
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    unsigned canvas[NB_BUFFER];
    bool isStreaming;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
};

class vdin_screen_source {
    public:
        vdin_screen_source();
        ~vdin_screen_source();
    	
        int start();
        int stop();
        int get_format();
        int set_format(int width = 640, int height = 480, int color_format = V4L2_PIX_FMT_NV21);
        int set_rotation(int degree);
        int aquire_buffer(unsigned* buff_info);
        int release_buffer(char* ptr);
        private:
            int mCurrentIndex;
            KeyedVector<int, int> mBufs;
            int mCameraHandle;
            struct VideoInfo *mVideoInfo;
};

}