// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef FRAME_CONVERTER_H
#define FRAME_CONVERTER_H

// 引入必要的库头文件
#include <Processing.NDI.Lib.h>  // NDI库，用于网络设备接口视频传输
#include <libavcodec/avcodec.h>  // FFmpeg编解码库
#include <libswresample/swresample.h>  // FFmpeg音频重采样库

// 定义帧转换器上下文结构体
typedef struct FrameConverterCtx {
    SwrContext *swr_context;  // 音频重采样上下文
    struct SwsContext *sws_ctx;  // 视频缩放/像素格式转换上下文

    AVFrame *audio_frame;  // 存储转换后的音频帧
    AVFrame *video_frame;  // 存储转换后的视频帧

    int64_t frame_index;  // 帧索引计数器
    int64_t start_ts;  // 起始时间戳

    char *error_str;  // 错误信息字符串
} FrameConverterCtx;

// 函数声明

/**
 * 创建并初始化一个新的帧转换器上下文
 * @return 返回新创建的FrameConverterCtx指针
 */
FrameConverterCtx *
new_frame_converter_ctx();

/**
 * 释放帧转换器上下文
 * @param ctx 指向FrameConverterCtx指针的指针
 * @return 成功返回0，失败返回非0值
 */
int
free_frame_converter_ctx(FrameConverterCtx **ctx);

/**
 * 重置帧转换器上下文状态
 * @param ctx FrameConverterCtx指针
 */
void
fc_reset(FrameConverterCtx *);

/**
 * 将NDI视频帧转换为AVFrame
 * @param ctx 帧转换器上下文
 * @param codec_ctx FFmpeg编解码上下文
 * @param in_frame 输入的NDI视频帧
 * @return 返回转换后的AVFrame指针
 */
AVFrame *
fc_ndi_video_frame_to_avframe(FrameConverterCtx *ctx, AVCodecContext *codec_ctx,
                              NDIlib_video_frame_v2_t *in_frame);

/**
 * 将NDI音频帧转换为AVFrame
 * @param ctx 帧转换器上下文
 * @param codec_ctx FFmpeg编解码上下文
 * @param in_frame 输入的NDI音频帧
 * @return 返回转换后的AVFrame指针
 */
AVFrame *
fc_ndi_audio_frame_to_avframe(FrameConverterCtx *ctx, AVCodecContext *codec_ctx,
                              NDIlib_audio_frame_v2_t *in_frame);
#endif
