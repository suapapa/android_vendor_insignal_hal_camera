/*
 * Copyright (C) 2012 Insignal Co, Ltd.
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

#ifndef __ANDROID_SEC_V4L2_ADAPTER_H__
#define __ANDROID_SEC_V4L2_ADAPTER_H__

#include <sys/poll.h>
#include <linux/videodev2.h>
#include <videodev2_samsung.h>

#undef DEBUG_STR_PIXFMT

namespace android {

#define MAX_BUFFERS	(8)

struct v4l2Buffer {
    void   *start;
    size_t  length;
};

class SecV4L2Adapter
{
public:
    SecV4L2Adapter(const char *path, int ch);
    ~SecV4L2Adapter();

    int getFd(void);

    int setFmt(int w, int h, unsigned int fmt, int flag);
    int reqBufs(enum v4l2_buf_type t, int n);
    int queryBuf(struct v4l2Buffer *bufs, enum v4l2_buf_type type);
    int queryBufs(struct v4l2Buffer *bufs, enum v4l2_buf_type type, int n);
    int initBuf(struct v4l2Buffer *buf, int w, int h, int fmt);
    int initBufs(struct v4l2Buffer *bufs, int w, int h, int fmt);
    int closeBuf(struct v4l2Buffer *buf);
    int closeBufs(struct v4l2Buffer *bufs);
    int startStream(bool on);
    int qbuf(int idx);
    int dqbuf(void);
    int blk_dqbuf(void);
    int getCtrl(int id);
    int setCtrl(int id, int value);
    int getParm(struct sec_cam_parm *parm);
    int setParm(const struct sec_cam_parm *parm);
    int waitFrame(void);

    int getAddr(int idx, unsigned int *addrY, unsigned int *addrC);

    int enumSceneMode(const char *strScene);
    int enumFlashMode(const char *strFlash);
    int enumBrightness(int ev);
    int enumWB(const char *strWB);
    int enumEffect(const char *strEffect);
    int enumFocusMode(const char *strFocus);

    int nPixfmt(const char *strPixfmt);

    unsigned int frameSize(int v4l2_pixfmt, int width, int height);

private:
    int	_fd;
    int _index;
    struct pollfd _poll;

    int _openCamera(const char *path);
    int _setInputChann(int ch);

#ifdef DEBUG_STR_PIXFMT
    const char *_strPixfmt(int v4l2Pixfmt);

    static const char _strPixfmt_yuv420[];
    static const char _strPixfmt_nv12[];
    static const char _strPixfmt_nv12t[];
    static const char _strPixfmt_nv21[];
    static const char _strPixfmt_yuv422p[];
    static const char _strPixfmt_yuyv[];
    static const char _strPixfmt_rgb565[];
    static const char _strPixfmt_unknown[];
#endif
};

};
#endif
