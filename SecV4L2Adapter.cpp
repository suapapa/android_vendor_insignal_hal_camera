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
        v4l2Pixfmt = V4L2_PIX_FMT_NV21;	//for 3rd party

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

unsigned int SecV4L2Adapter::frameSize()
{
    LOGW_IF(_bufSize == 0, "Actual buffer size not available yet!");

    return _bufSize;
}

SecV4L2Adapter::SecV4L2Adapter(const char* path, int ch):
    _fd(0),
    _chIdx(-1),
    _bufCnt(0),
    _bufSize(0)
{
    LOGI("opening %s (ch=%d)...", path, ch);
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

    for (int i = 0; i < MAX_CAM_BUFFERS; i++) {
        _bufMapStart[i];
    }
    LOGI("opened %s (ch=%d)...", path, ch);
}

SecV4L2Adapter::~SecV4L2Adapter()
{
    LOGI("%s", __func__);
    if (_fd) {
        close(_fd);
        _fd = 0;
    }
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
    LOG_CAMERA_FUNC_ENTER;
    struct v4l2_input input;
    int ret;

    if (ch == _chIdx)
        return 0;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    LOGI("%s: enum chan", __func__);
    input.index = ch;
    if (ioctl(_fd, VIDIOC_ENUMINPUT, &input) != 0) {
        LOGE("ERR(%s):No matching index found\n", __func__);
        return -1;
    }

    LOGI("%s: set input", __func__);
    ret = ioctl(_fd, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    LOGI("Name of input channel[%d] is %d\n", input.index, _fd);

    _chIdx = input.index;

    return 0;
}

int SecV4L2Adapter::_setFmt(int w, int h, unsigned int fmt, int flag)
{
    LOG_CAMERA_FUNC_ENTER;
    int ret;

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
    struct v4l2_pix_format pixfmt;

    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = w;
    pixfmt.height = h;
    pixfmt.pixelformat = fmt;

    //pixfmt.sizeimage = frameSize(w, h, fmt);

    pixfmt.field = V4L2_FIELD_NONE;
    pixfmt.priv = flag;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    ret = ioctl(_fd, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("%s: VIDIOC_S_FMT failed!", __func__);
        return -1;
    }

    return ret;
}

int SecV4L2Adapter::_reqBufs(int n)
{
    LOG_CAMERA_FUNC_ENTER;
    struct v4l2_requestbuffers req;
    int ret;

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.count = n;

    ret = ioctl(_fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        LOGE("%s: VIDIOC_REQBUFS failed!", __func__);
        return -1;
    }

    LOGW_IF(n != 1 && req.count == 1, "insufficient buffer avaiable!");

    _bufCnt = req.count;

    return 0;
}

int SecV4L2Adapter::_queryBuf(int idx, int* length, int* offset)
{
    LOG_CAMERA_FUNC_ENTER;
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = idx;

    ret = ioctl(_fd, VIDIOC_QUERYBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("%s: VIDIOC_QUERYBUF failed!", __func__);
        return -1;
    }

    int l = v4l2_buf.length;
    int o = v4l2_buf.m.offset;
    LOGV("buffer-%d: length = %d, offset = %d", idx, l, o);

    if (_bufSize == 0)
        _bufSize = l;

    LOGE_IF(_bufSize != (size_t)l,
            "%s: Invaild length! _bufSize = %d, l = %d", __func__,
            _bufSize, l);

    if (length)
        *length = l;
    if (offset)
        *offset = o;

    return 0;
}

int SecV4L2Adapter::setupBufs(int w, int h, unsigned int fmt, unsigned int n,
                              int flag)
{
    LOG_CAMERA_FUNC_ENTER;
    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    int err;
    err = _setFmt(w, h, fmt, flag);
    if (err)
        return -1;

    _reqBufs(n);
    if (_bufCnt == 0)
        return -1;

    LOGW_IF(n > _bufCnt, "only %d buffers available! req = %d", _bufCnt, n);

    for (unsigned int i = 0; i < _bufCnt; i++) {
        _queryBuf(i, NULL, NULL);
    }

    // return available buffer count;
    return _bufCnt;
}

int SecV4L2Adapter::mapBuf(int idx)
{
    int length;
    int offset;
    int err = _queryBuf(idx, &length, &offset);
    if (err || length == 0) {
        LOGE("%s: aborted mmap-%d!", __func__, idx);
        return -1;
    }

    void* start = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED,
                       _fd, offset);
    if (start == MAP_FAILED) {
        LOGE("%s: mmap() failed. idx = %d, length = %d, offset = %d",
             __func__, idx, length, offset);
        return -1;
    }

    if (_bufMapStart[idx] != NULL) {
        LOGE("%s: mmaped alread in %d!", __func__, idx);
        return -1;
    }

    _bufMapStart[idx] = start;

    return 0;
}

int SecV4L2Adapter::mapBufInfo(int idx, void** start, size_t* size)
{
    if (size)
        *size = _bufSize;
    if (start)
        *start = _bufMapStart[idx];
    return 0;
}

int SecV4L2Adapter::closeBufs()
{
    LOG_CAMERA_FUNC_ENTER;

    for (unsigned int i = 0; i < _bufCnt; i++) {
        if (_bufMapStart[i] == NULL)
            continue;

        munmap(_bufMapStart[i], _bufSize);
        LOGV("munmap-%d : addr = 0x%p size = %d\n",
             i, _bufMapStart[i], _bufSize);
        _bufMapStart[i] = NULL;
    }

    _bufCnt = 0;
    _bufSize = 0;

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
        LOGE("ERR(%s): Failed stream %s", __func__,
             on ? "on" : "off");
        return ret;
    }

    return ret;
}

int SecV4L2Adapter::qBuf(unsigned int idx)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    if (_fd == 0) {
        LOGE("%s: camera not opened!", __func__);
        return -1;
    }

    if (idx > (_bufCnt - 1)) {
        LOGE("%s: invalid index, %d!", __func__, idx);
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

int SecV4L2Adapter::dqBuf(void)
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
        LOGE("%s: VIDIOC_DQBUF failed\n", __func__);
        return ret;
    }

    if (v4l2_buf.index > (_bufCnt - 1)) {
        LOGE("%s: invalid index, %d!", __func__, v4l2_buf.index);
        return -1;
    }

    return v4l2_buf.index;
}

int SecV4L2Adapter::qAllBufs(void)
{
    int err;
    for (unsigned int i = 0; i < _bufCnt; i++) {
        err = qBuf(i);
        if (err)
            return err;
    }

    return 0;
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
        return dqBuf();
    } else {
        index = v4l2_buf.index;
        while (ret == 0) {
            ret = ioctl(_fd, VIDIOC_DQBUF, &v4l2_buf);
            if (ret == 0) {
                LOGV("VIDIOC_DQBUF is not still empty %d", v4l2_buf.index);
                qBuf(index);
                index = v4l2_buf.index;
            } else {
                LOGV("VIDIOC_DQBUF is empty now %d ",
                     index);
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
        LOGE("ERR(%s): VIDIOC_G_CTRL(id = 0x%x (%d)) failed, ret = %d\n", __func__, id, id - V4L2_CID_PRIVATE_BASE, ret);
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
        LOGE("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n", __func__, id, id - V4L2_CID_PRIVATE_BASE, value, ret);
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

int SecV4L2Adapter::waitFrame(int timeout)
{
    int ret;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
    ret = poll(&_poll, 1, timeout);
    if (ret < 0) {
        LOGE("ERR(%s):poll error, revent = %d\n", __func__,
             _poll.revents);
        return ret;
    }

    if (ret == 0) {
        LOGE("ERR(%s):No data in %d msecs..\n", __func__, timeout);
        return ret;
    }

    return 0;
}

int SecV4L2Adapter::getFd(void)
{
    return _fd;
}

int SecV4L2Adapter::getChIdx(void)
{
    return _chIdx;
}

};
