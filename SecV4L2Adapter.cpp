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

//#define LOG_NDEBUG 0
#define LOG_TAG "SecV4L2Adapter"
#include <utils/Log.h>
#include "CameraLog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <camera/CameraParameters.h>

#include "SecV4L2Adapter.h"

namespace android {

#ifdef DEBUG_STR_PIXFMT
const char SecV4L2Adapter::_strPixfmt_yuv420[]   = "YUV420";
const char SecV4L2Adapter::_strPixfmt_nv12[]     = "NV12";
const char SecV4L2Adapter::_strPixfmt_nv12t[]    = "NV12T";
const char SecV4L2Adapter::_strPixfmt_nv21[]     = "NV21";
const char SecV4L2Adapter::_strPixfmt_yuv422p[]  = "YUV422P";
const char SecV4L2Adapter::_strPixfmt_yuyv[]     = "YUYV";
const char SecV4L2Adapter::_strPixfmt_rgb565[]   = "RGB565";
const char SecV4L2Adapter::_strPixfmt_unknown[]  = "UNKNOWN";
#endif

int SecV4L2Adapter::nPixfmt(const char* strPixfmt)
{
    int v4l2Pixfmt;

    if (NULL == strPixfmt) {
        LOGE("%s: given NULL for strPixfmt!", __func__);
        return -1;
    }

    if (0 == strcmp(strPixfmt, CameraParameters::PIXEL_FORMAT_RGB565))
        v4l2Pixfmt = V4L2_PIX_FMT_RGB565;
    else if (0 == strcmp(strPixfmt, CameraParameters::PIXEL_FORMAT_YUV420SP))
        v4l2Pixfmt = V4L2_PIX_FMT_NV21;
    else if (0 == strcmp(strPixfmt, CameraParameters::PIXEL_FORMAT_YUV420P))	// YV12
        v4l2Pixfmt = V4L2_PIX_FMT_YUV420;
    else if (0 == strcmp(strPixfmt, "yuv420sp_custom"))
        v4l2Pixfmt = V4L2_PIX_FMT_NV12T;
    else if (0 == strcmp(strPixfmt, "yuv420sp_ycrcb"))
        v4l2Pixfmt = V4L2_PIX_FMT_NV21;
    else if (0 == strcmp(strPixfmt, "yuv422i"))
        v4l2Pixfmt = V4L2_PIX_FMT_YUYV;
    else if (0 == strcmp(strPixfmt, "yuv422p"))
        v4l2Pixfmt = V4L2_PIX_FMT_YUV422P;
    else if (0 == strcmp(strPixfmt, CameraParameters::PIXEL_FORMAT_JPEG))
        v4l2Pixfmt = V4L2_PIX_FMT_YUYV;
    else
        v4l2Pixfmt = V4L2_PIX_FMT_NV21; //for 3rd party

    LOGV("%s: converted %s to %d", __func__, strPixfmt, v4l2Pixfmt);

    return v4l2Pixfmt;
}

int SecV4L2Adapter::enumSceneMode(const char* strScene)
{
    int enumScene;

    if (NULL == strScene) {
        LOGE("%s: given NULL for strScene!", __func__);
        return -1;
    }

    if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_AUTO))
        enumScene = SCENE_MODE_NONE;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_ACTION))
        enumScene = SCENE_MODE_SPORTS;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_PORTRAIT))
        enumScene = SCENE_MODE_PORTRAIT;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_LANDSCAPE))
        enumScene = SCENE_MODE_LANDSCAPE;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_NIGHT))
        enumScene = SCENE_MODE_NIGHTSHOT;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_NIGHT_PORTRAIT))
        enumScene = SCENE_MODE_NIGHTSHOT;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_THEATRE))
        enumScene = SCENE_MODE_NIGHTSHOT;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_BEACH))
        enumScene = SCENE_MODE_BEACH_SNOW;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_SNOW))
        enumScene = SCENE_MODE_BEACH_SNOW;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_SUNSET))
        enumScene = SCENE_MODE_SUNSET;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_STEADYPHOTO))
        enumScene = SCENE_MODE_LANDSCAPE;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_FIREWORKS))
        enumScene = SCENE_MODE_FIREWORKS;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_SPORTS))
        enumScene = SCENE_MODE_SPORTS;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_PARTY))
        enumScene = SCENE_MODE_PARTY_INDOOR;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_CANDLELIGHT))
        enumScene = SCENE_MODE_CANDLE_LIGHT;
    else if (0 == strcmp(strScene, CameraParameters::SCENE_MODE_BARCODE))
        enumScene = SCENE_MODE_TEXT;
    else
        enumScene = SCENE_MODE_NONE;

    LOGV("%s: converted %s to %d", __func__, strScene, enumScene);
    return enumScene;
}

int SecV4L2Adapter::enumFlashMode(const char* strFlash)
{
    int enumFlash;

    if (NULL == strFlash) {
        LOGE("%s: given NULL for strFlash!", __func__);
        return -1;
    }

    if (0 == strcmp(strFlash, CameraParameters::FLASH_MODE_OFF))
        enumFlash = FLASH_MODE_OFF;
    else if (0 == strcmp(strFlash, CameraParameters::FLASH_MODE_AUTO))
        enumFlash = FLASH_MODE_AUTO;
    else if (0 == strcmp(strFlash, CameraParameters::FLASH_MODE_ON))
        enumFlash = FLASH_MODE_ON;
    else if (0 == strcmp(strFlash, CameraParameters::FLASH_MODE_TORCH))
        enumFlash = FLASH_MODE_TORCH;
    else if (0 == strcmp(strFlash, CameraParameters::FLASH_MODE_RED_EYE))
        enumFlash = FLASH_MODE_OFF;
    else
        enumFlash = FLASH_MODE_OFF;

    LOGV("%s: converted %s to %d", __func__, strFlash, enumFlash);
    return enumFlash;
}

int SecV4L2Adapter::enumBrightness(int ev)
{
    int ev_enum = ev + EV_DEFAULT;
    if (0 > ev_enum || EV_MAX < ev_enum) {
        LOGE("%s: failed to convert given ev, %d to enum. rollback to default.",
             __func__, ev);
        ev_enum = EV_DEFAULT;
    }

    LOGV("%s: converted %d to %d", __func__, ev, ev_enum);
    return ev_enum;
}

int SecV4L2Adapter::enumWB(const char* strWB)
{
    int enumWB;

    if (NULL == strWB) {
        LOGE("%s: given NULL for strWB!", __func__);
        return -1;
    }

    if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_AUTO))
        enumWB = WHITE_BALANCE_AUTO;
#if 0
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_INCANDESCENT))
        enumWB = WHITE_BALANCE_INCANDESCENT;
#endif
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_FLUORESCENT))
        enumWB = WHITE_BALANCE_FLUORESCENT;
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT))
        enumWB = WHITE_BALANCE_FLUORESCENT;
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_DAYLIGHT))
        enumWB = WHITE_BALANCE_SUNNY;
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
        enumWB = WHITE_BALANCE_CLOUDY;
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_TWILIGHT))
        enumWB = -1;
    else if (0 == strcmp(strWB, CameraParameters::WHITE_BALANCE_SHADE))
        enumWB = WHITE_BALANCE_CLOUDY;
    else
        enumWB = WHITE_BALANCE_AUTO;

    LOGE_IF(0 > enumWB, "%s, %s unsupported!", __func__, strWB);
    LOGV("%s: converted %s to %d", __func__, strWB, enumWB);

    return enumWB;
}

int SecV4L2Adapter::enumEffect(const char* strEffect)
{
    int enumEffect;

    if (NULL == strEffect) {
        LOGE("%s: given NULL for strEffect!", __func__);
        return -1;
    }

    if (0 == strcmp(strEffect, CameraParameters::EFFECT_NONE))
        enumEffect = IMAGE_EFFECT_NONE;
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_MONO))
        enumEffect = IMAGE_EFFECT_BNW;
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_NEGATIVE))
        enumEffect = IMAGE_EFFECT_NEGATIVE;
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_SEPIA))
        enumEffect = IMAGE_EFFECT_SEPIA;
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_AQUA))
        enumEffect = IMAGE_EFFECT_AQUA;
#if 0
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_SOLARIZE))
        enumEffect = IMAGE_EFFECT_SOLARIZE;
#endif
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_POSTERIZE))
        enumEffect = -1;
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_WHITEBOARD))
        enumEffect = -1;
    else if (0 == strcmp(strEffect, CameraParameters::EFFECT_BLACKBOARD))
        enumEffect = -1;
    else
        enumEffect = -1;

    LOGE_IF(0 > enumEffect, "%s, %s unsupported!", __func__, strEffect);
    LOGV("%s: converted %s to %d", __func__, strEffect, enumEffect);

    return enumEffect;
}

int SecV4L2Adapter::enumFocusMode(const char* strFocus)
{
    int enumFocus;

    if (NULL == strFocus) {
        LOGE("%s: given NULL for strFocus!", __func__);
        return -1;
    }

    if (0 == strcmp(strFocus, CameraParameters::FOCUS_MODE_AUTO))
        enumFocus = FOCUS_MODE_AUTO;
    else if (0 == strcmp(strFocus, CameraParameters::FOCUS_MODE_MACRO))
        enumFocus = FOCUS_MODE_MACRO;
    else if (0 == strcmp(strFocus, CameraParameters::FOCUS_MODE_INFINITY))
        enumFocus = FOCUS_MODE_INFINITY;
    else
        enumFocus = -1;

    LOGE_IF(0 > enumFocus, "%s, %s unsupported!", __func__, strFocus);
    LOGV("%s: converted %s to %d", __func__, strFocus, enumFocus);

    return enumFocus;
}

#define ALIGN(B, V)     (((V) + ((B) - 1)) & (~((B) - 1)))
unsigned int SecV4L2Adapter::frameSize(int width, int height, int v4l2Pixfmt)
{
    unsigned int bpp, size;

    switch (v4l2Pixfmt) {
    case V4L2_PIX_FMT_NV12T:
        size = ALIGN(8 * 1024, ALIGN(128, width) + ALIGN(32, height)) +
               ALIGN(8 * 1024, ALIGN(128, width) + ALIGN(32, height >> 1));
        break;

    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV420:
        size = (width * height * 3) >> 1;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
        size = width * height * 2;
        break;

    case V4L2_PIX_FMT_RGB32:
        size = width * height * 4;
        break;

    default :
        LOGE("%s: Invalid pixfmt, %d given!", __func__, v4l2Pixfmt);
        size = 0;
        break;
    }

#ifdef DEBUG_STR_PIXFMT
    LOGV("%s: frames size for %dx%d,%s is %d", __func__,
         width, height, _strPixfmt(v4l2Pixfmt), size);
#endif

    return size;
}

#ifdef DEBUG_STR_PIXFMT
const char* SecV4L2Adapter::_strPixfmt(int v4l2Pixfmt)
{
    const char* strPixfmt;
    switch (v4l2Pixfmt) {
    case V4L2_PIX_FMT_YUV420:
        strPixfmt = _strPixfmt_yuv420;
        break;
    case V4L2_PIX_FMT_NV12:
        strPixfmt = _strPixfmt_nv12;
        break;
    case V4L2_PIX_FMT_NV12T:
        strPixfmt = _strPixfmt_nv12t;
        break;
    case V4L2_PIX_FMT_NV21:
        strPixfmt = _strPixfmt_nv21;
        break;
    case V4L2_PIX_FMT_YUV422P:
        strPixfmt = _strPixfmt_yuv422p;
        break;
    case V4L2_PIX_FMT_YUYV:
        strPixfmt = _strPixfmt_yuyv;
        break;
    case V4L2_PIX_FMT_RGB565:
        strPixfmt = _strPixfmt_rgb565;
        break;
    default:
        LOGE("%s: unknown pixfmt, %d given!", __func__, v4l2Pixfmt);
        strPixfmt = _strPixfmt_unknown;
        break;
    }

    LOGV("%s: converted %d to %s", __func__, v4l2Pixfmt, strPixfmt);

    return strPixfmt;
}
#endif

SecV4L2Adapter::SecV4L2Adapter(const char* path, int ch) :
    _fd(0),
    _index(-1)
{
    LOGV("opeing %s (ch=%d)...", path, ch);
    int err = 0;
    err |= _openCamera(path);
    err |= _setInputChann(ch);

    if (err) {
        LOGE("!! V4L2 init failed !! path=%s, ch=%d", path, ch);

        if (_fd) {
            close(_fd);
            _fd = 0;
        }
    }
}

SecV4L2Adapter::~SecV4L2Adapter()
{
    LOGV("%s", __func__);
    if (_fd)
        close(_fd);
}

int SecV4L2Adapter::_openCamera(const char* path)
{
    LOG_CAMERA_FUNC_ENTER;
    if (_fd) {
        LOGW("fd(%d) already opened. close it first!", _fd);
        close(_fd);
    }

    _fd = open(path, O_RDWR);
    if (0 >= _fd) {
        LOGE("ERR(%s):Cannot open %s (error : %s)\n",
             __func__, path, strerror(errno));
        return -1;
    }

    struct v4l2_capability cap;
    int ret = ioctl(_fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("ERR(%s):no capture devices\n", __func__);
        return -1;
    }

    memset(&_poll, 0, sizeof(_poll));
    _poll.fd = _fd;
    _poll.events = POLLIN | POLLERR;

    LOGI("%s opened. fd=%d", path, _fd);

    return 0;
}

int SecV4L2Adapter::_setInputChann(int ch)
{
    struct v4l2_input input;
    int ret;

    if (ch == _index)
        return 0;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    input.index = ch;
    if (ioctl(_fd, VIDIOC_ENUMINPUT, &input) != 0) {
        LOGE("ERR(%s):No matching index found\n", __func__);
        return -1;
    }

    ret = ioctl(_fd, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    LOGI("Name of input channel[%d] is %s\n", input.index, input.name);

    _index = input.index;

    return 0;
}

int SecV4L2Adapter::setFmt(int w, int h, unsigned int fmt, int flag)
{
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;

    int found = 0;
    while (ioctl(_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        if (fmtdesc.pixelformat == fmt) {
            LOGI("passed fmt = %s found pixel format[%d]: %s\n",
                 getStrFourCC(fmt), fmtdesc.index, fmtdesc.description);
            found = 1;
            break;
        }
        fmtdesc.index++;
    }

    if (!found) {
        LOGE("unsupported pixel format, %d\n", fmt);
        return -1;
    }

    struct v4l2_format v4l2_fmt;
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    struct v4l2_pix_format pixfmt;
    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = w;
    pixfmt.height = h;
    pixfmt.pixelformat = fmt;

    pixfmt.sizeimage = frameSize(w, h, fmt);

    pixfmt.field = V4L2_FIELD_NONE;
    pixfmt.priv = flag;
    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    ret = ioctl(_fd, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return -1;
    }

    return ret;
}

int SecV4L2Adapter::reqBufs(enum v4l2_buf_type t, int n)
{
    LOG_CAMERA_FUNC_ENTER;
    struct v4l2_requestbuffers req;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    req.count = n;
    req.type = t;
    req.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(_fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
        return -1;
    }

    return req.count;
}

int SecV4L2Adapter::queryBufs(struct v4l2Buffer* bufs, enum v4l2_buf_type type, int n)
{
    LOG_CAMERA_FUNC_ENTER;
    struct v4l2_buffer v4l2_buf;
    int i, ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    for (i = 0; i < n; i++) {
        v4l2_buf.type = type;
        v4l2_buf.memory = V4L2_MEMORY_MMAP;
        v4l2_buf.index = i;

        ret = ioctl(_fd, VIDIOC_QUERYBUF, &v4l2_buf);
        if (ret < 0) {
            LOGE("ERR(%s):VIDIOC_QUERYBUF failed\n", __func__);
            return -1;
        }

        bufs[i].length = v4l2_buf.length;

        if (n == 1) {
            bufs[i].length = v4l2_buf.length;
            if ((bufs[i].start = (char*)mmap(0, v4l2_buf.length,
                                             PROT_READ | PROT_WRITE, MAP_SHARED,
                                             _fd, v4l2_buf.m.offset)) < 0) {
                LOGE("%s %d] mmap() failed\n", __func__, __LINE__);
                return -1;
            }

            LOGV("bufs[%d].start = %p v4l2_buf.length = %d",
                 i, bufs[i].start, v4l2_buf.length);
        }
    }

    return 0;
}

int SecV4L2Adapter::startStream(bool on)
{
    LOG_CAMERA_FUNC_ENTER;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    ret = ioctl(_fd, on ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("ERR(%s): Failed stream %s", __func__, on ? "on" : "off");
        return ret;
    }

    return ret;
}

int SecV4L2Adapter::qbuf(int idx)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = idx;

    ret = ioctl(_fd, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QBUF failed\n", __func__);
        return ret;
    }

    return 0;
}

int SecV4L2Adapter::dqbuf(void)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(_fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_DQBUF failed\n", __func__);
        return ret;
    }

    return v4l2_buf.index;
}

int SecV4L2Adapter::blk_dqbuf(void)
{
    struct v4l2_buffer v4l2_buf;
    int index;
    int ret = 0;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(_fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        // LOGV("VIDIOC_DQBUF first is empty") ;
        waitFrame();
        return dqbuf();
    } else {
        index = v4l2_buf.index;
        while (ret == 0) {
            ret = ioctl(_fd, VIDIOC_DQBUF, &v4l2_buf);
            if (ret == 0) {
                LOGV("VIDIOC_DQBUF is not still empty %d", v4l2_buf.index);
                qbuf(index);
                index = v4l2_buf.index;
            } else {
                LOGV("VIDIOC_DQBUF is empty now %d ", index);
                return index;
            }
        }
    }
    return v4l2_buf.index;
}

int SecV4L2Adapter::getCtrl(int id)
{
    struct v4l2_control ctrl;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    ctrl.id = id;

    ret = ioctl(_fd, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s): VIDIOC_G_CTRL(id = 0x%x (%d)) failed, ret = %d\n",
             __func__, id, id - V4L2_CID_PRIVATE_BASE, ret);
        return ret;
    }

    return ctrl.value;
}

int SecV4L2Adapter::setCtrl(int id, int value)
{
    struct v4l2_control ctrl;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    ctrl.id = id;
    ctrl.value = value;

    ret = ioctl(_fd, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id - V4L2_CID_PRIVATE_BASE, value, ret);
        return ret;
    }

    return ctrl.value;
}

int SecV4L2Adapter::getParm(struct sec_cam_parm* parm)
{
    struct v4l2_streamparm streamparm;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    if (parm == NULL) {
        LOGE("%s: given NULL for sec_cam_parm *!!", __func__);
        return -1;
    }

    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(_fd, VIDIOC_G_PARM, &streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_G_PARM failed\n", __func__);
        return -1;
    }

    memcpy(parm, &streamparm.parm.raw_data, sizeof(sec_cam_parm));

    return 0;
}

int SecV4L2Adapter::setParm(const struct sec_cam_parm* parm)
{
    struct v4l2_streamparm streamparm;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    if (parm == NULL) {
        LOGE("%s: given NULL for sec_cam_parm *!!", __func__);
        return -1;
    }

    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    memcpy(&streamparm.parm.raw_data, parm, sizeof(sec_cam_parm));

    ret = ioctl(_fd, VIDIOC_S_PARM, &streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_PARM failed\n", __func__);
        return ret;
    }

    return 0;
}

int SecV4L2Adapter::getAddr(int idx, unsigned int* addrY, unsigned int* addrC)
{
    if (addrY) {
        *addrY = setCtrl(V4L2_CID_PADDR_Y, idx);
    }
    if (addrC) {
        *addrC = setCtrl(V4L2_CID_PADDR_CBCR, idx);
    }

    return 0;
}

int SecV4L2Adapter::waitFrame(void)
{
    int ret;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
    ret = poll(&_poll, 1, 10000);
    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return ret;
    }

    if (ret == 0) {
        LOGE("ERR(%s):No data in 10 secs..\n", __func__);
        return ret;
    }

    return 0;
}

int SecV4L2Adapter::initBufs(struct v4l2Buffer* bufs, int w, int h, int fmt, int n)
{
    LOG_CAMERA_FUNC_ENTER;
    int i, len;

    len = frameSize(w, h, fmt);

    for (i = 0; i < n; i++) {
        bufs[i].start = NULL;
        bufs[i].length = len;
    }

    return 0;
}

int SecV4L2Adapter::closeBufs(struct v4l2Buffer* bufs, int n)
{
    LOG_CAMERA_FUNC_ENTER;
    int i;

    for (i = 0; i < n; i++) {
        if (bufs[i].start) {
            munmap(bufs[i].start, bufs[i].length);
            LOGV("munmap():virt. addr[%d]: 0x%x size = %d\n",
                 i, (unsigned int) bufs[i].start, bufs[i].length);
            bufs[i].start = NULL;
        }
    }

    return 0;
}

int SecV4L2Adapter::getFd(void)
{
    return _fd;
}

int SecV4L2Adapter::getChIdx(void)
{
    return _index;
}

};

