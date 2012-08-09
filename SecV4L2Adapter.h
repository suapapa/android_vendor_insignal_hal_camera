/*
 * Copyright (C) 2012 Homin Lee <suapapa@insignal.co.kr>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ANDROID_SEC_V4L2_ADAPTER_H__
#define __ANDROID_SEC_V4L2_ADAPTER_H__

#include <sys/poll.h>
#include <linux/videodev2.h>
#include "videodev2_samsung.h"

#define MAX_CAM_BUFFERS         (8)

namespace android {

class SecV4L2Adapter {
public:
    SecV4L2Adapter(const char* path, int ch);
    ~SecV4L2Adapter();

    int getFd(void);
    int getChIdx(void);

    int setupBufs(int w, int h, unsigned int fmt, unsigned int n, int flag = 0);
    int mapBuf(int idx);
    int mapBufInfo(int idx, void** start, size_t* size);
    int closeBufs(void);
    int startStream(bool on);
    int qBuf(unsigned int idx);
    int dqBuf(void);
    int qAllBufs(void);
    int blk_dqbuf(void);
    int getCtrl(int id);
    int setCtrl(int id, int value);
    int getParm(struct sec_cam_parm* parm);
    int setParm(const struct sec_cam_parm* parm);
    int waitFrame(int timeout = 10000);

    int getAddr(int idx, unsigned int* addrY, unsigned int* addrC);

    int enumSceneMode(const char* strScene);
    int enumFlashMode(const char* strFlash);
    int enumBrightness(int ev);
    int enumWB(const char* strWB);
    int enumEffect(const char* strEffect);
    int enumFocusMode(const char* strFocus);

    int nPixfmt(const char* strPixfmt);

    unsigned int frameSize(void);

private:
    int	_fd;
    int _chIdx;
    struct pollfd _poll;

    unsigned int _bufCnt;
    size_t _bufSize;
    void* _bufMapStart[MAX_CAM_BUFFERS];

    int _openCamera(const char* path);
    int _setInputChann(int ch);

    int _setFmt(int w, int h, unsigned int fmt, int flag);
    int _reqBufs(int n);
    int _queryBuf(int idx, int* length, int* offset);
};

};
#endif
