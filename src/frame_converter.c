// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "frame_converter.h"

#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "common.h"

// 帧转换器模块，提供NDI视频/音频帧到FFmpeg AVFrame的转换功能

/**
 * 将NDI视频格式FourCC转换为FFmpeg像素格式
 * @param type NDI视频格式FourCC枚举值
 * @return 对应的FFmpeg像素格式，不支持的格式返回-1
 */
enum AVPixelFormat
ndi_fourcc_to_ffmpeg(NDIlib_FourCC_video_type_e type)
{
    switch (type) {
    case NDIlib_FourCC_video_type_UYVY:
        return AV_PIX_FMT_UYVY422;
    case NDIlib_FourCC_video_type_UYVA:
        return AV_PIX_FMT_YUVA422P12;
    case NDIlib_FourCC_video_type_BGRA:
        return AV_PIX_FMT_BGRA;
    case NDIlib_FourCC_video_type_BGRX:
        return AV_PIX_FMT_BGR0;
    case NDIlib_FourCC_video_type_RGBA:
        return AV_PIX_FMT_RGBA;
    case NDIlib_FourCC_video_type_I420:
        return AV_PIX_FMT_YUV420P;
    case NDIlib_FourCC_video_type_NV12:
        return AV_PIX_FMT_NV12;
    case NDIlib_FourCC_video_type_RGBX:
        return AV_PIX_FMT_RGB444;
    case NDIlib_FourCC_video_type_P216:
        return AV_PIX_FMT_P216;
    case NDIlib_FourCC_video_type_PA16:
        return AV_PIX_FMT_P016;
    case NDIlib_FourCC_video_type_YV12:
        return AV_PIX_FMT_UYVY422;
    default:
        return -1;
    }
}

/**
 * 创建新的帧转换器上下文
 * @return 新分配的FrameConverterCtx指针，包含初始化的视频帧、音频帧和错误字符串缓冲区
 * @note 需要调用free_frame_converter_ctx释放分配的资源
 */
FrameConverterCtx *
new_frame_converter_ctx()
{
    FrameConverterCtx *ctx = malloc(sizeof(FrameConverterCtx));
    memset(ctx, 0, sizeof(FrameConverterCtx));
    ctx->error_str = malloc(AV_ERROR_MAX_STRING_SIZE + 100);
    ctx->video_frame = av_frame_alloc();
    ctx->audio_frame = av_frame_alloc();
    ctx->frame_index = 0;
    ctx->start_ts = get_current_ts_usec();
    return ctx;
}

/**
 * 释放帧转换器上下文资源
 * @param ctx 指向帧转换器上下文指针的指针
 * @return 成功返回0
 * @note 会释放所有分配的资源并将指针置为NULL
 */
int
free_frame_converter_ctx(FrameConverterCtx **ctx)
{
    if ((*ctx)->sws_ctx)
        sws_freeContext((*ctx)->sws_ctx);
    if ((*ctx)->swr_context)
        swr_free(&(*ctx)->swr_context);
    if ((*ctx)->audio_frame)
        av_frame_free(&(*ctx)->audio_frame);
    if ((*ctx)->video_frame)
        av_frame_free(&(*ctx)->video_frame);

    free(*ctx);
    *ctx = NULL;
    return 0;
}

/**
 * 重置帧转换器上下文状态
 * @param ctx 帧转换器上下文指针
 * @note 重置帧索引和起始时间戳，用于重新开始计数
 */
void
fc_reset(FrameConverterCtx *ctx)
{
    ctx->frame_index = 0;
    ctx->start_ts = get_current_ts_usec();
}

/**
 * 将NDI视频帧转换为FFmpeg AVFrame
 * @param ctx 帧转换器上下文
 * @param codec_ctx FFmpeg编解码器上下文
 * @param in_frame 输入的NDI视频帧
 * @return 转换后的AVFrame指针，失败返回NULL
 * @note 执行以下操作:
 * 1. 准备可写的输出帧
 * 2. 设置帧格式和尺寸
 * 3. 获取图像缓冲区
 * 4. 创建/获取图像缩放上下文
 * 5. 填充源图像参数
 * 6. 执行图像格式转换
 * 7. 设置时间戳和帧索引
 */
AVFrame *
fc_ndi_video_frame_to_avframe(FrameConverterCtx *ctx, AVCodecContext *codec_ctx,
                              NDIlib_video_frame_v2_t *in_frame)
{

    AVFrame *out_frame = ctx->video_frame;

    av_frame_make_writable(out_frame);
    out_frame->format = codec_ctx->pix_fmt;
    out_frame->width = codec_ctx->width;
    out_frame->height = codec_ctx->height;
    av_frame_get_buffer(out_frame, 0);

    enum AVPixelFormat src_pix_fmt = ndi_fourcc_to_ffmpeg(in_frame->FourCC);

    ctx->sws_ctx = sws_getCachedContext(
            ctx->sws_ctx, in_frame->xres, in_frame->yres, src_pix_fmt,
            out_frame->width, out_frame->height, out_frame->format, SWS_BICUBIC,
            NULL, NULL, NULL);

    int src_stride[4] = {};
    uint8_t *src[4] = {};

    av_image_fill_linesizes(src_stride, src_pix_fmt, in_frame->xres);
    av_image_fill_pointers(src, src_pix_fmt, in_frame->yres, in_frame->p_data,
                           src_stride);

    sws_scale(ctx->sws_ctx, (const uint8_t *const *)src, src_stride, 0, in_frame->yres, out_frame->data,
              out_frame->linesize);

    out_frame->pkt_dts = get_current_ts_usec() - ctx->start_ts;
    out_frame->pts = ctx->frame_index * AV_TIME_BASE * in_frame->frame_rate_D
                     / in_frame->frame_rate_N;
    ctx->frame_index++;

    // AV_PICTURE_TYPE_I; // force infra
    out_frame->pict_type = AV_PICTURE_TYPE_NONE;

    return out_frame;
}

/**
 * 将NDI音频帧转换为FFmpeg AVFrame
 * @param ctx 帧转换器上下文
 * @param codec_ctx FFmpeg编解码器上下文
 * @param in_frame 输入的NDI音频帧
 * @return 转换后的AVFrame指针，失败返回NULL
 * @note 执行以下操作:
 * 1. 检查剩余样本数
 * 2. 准备可写的输出帧
 * 3. 设置音频参数(采样数、格式、采样率、声道布局)
 * 4. 获取音频缓冲区
 * 5. 处理刷新情况(无输入帧)
 * 6. 设置重采样参数
 * 7. 初始化重采样上下文
 * 8. 填充音频数据
 * 9. 执行重采样
 * 10. 设置时间戳
 */
AVFrame *
fc_ndi_audio_frame_to_avframe(FrameConverterCtx *ctx, AVCodecContext *codec_ctx,
                              NDIlib_audio_frame_v2_t *in_frame)
{
    int ret;
    int nb_samples = codec_ctx->frame_size;

    int remaining_pre
            = ctx->swr_context ? swr_get_out_samples(ctx->swr_context, 0) : 0;

    if (!in_frame && remaining_pre < nb_samples) { // wait
        return NULL;
    }

    AVFrame *out_frame = ctx->audio_frame;

    av_frame_make_writable(out_frame);
    out_frame->nb_samples = nb_samples;
    out_frame->format = codec_ctx->sample_fmt;
    out_frame->sample_rate = codec_ctx->sample_rate;
    out_frame->ch_layout = codec_ctx->ch_layout;
    av_frame_get_buffer(out_frame, 0);

    if (!in_frame) { // flush
        out_frame->pkt_dts = get_current_ts_usec() - ctx->start_ts;
        out_frame->pts = swr_next_pts(ctx->swr_context, INT64_MIN);

        ret = swr_convert(ctx->swr_context, out_frame->data,
                          out_frame->nb_samples, NULL, 0);

        if (ret < 0) {
            av_error_fmt(ctx->error_str, "error converting frame!", ret);
            return NULL;
        }
        return out_frame;
    }

    AVChannelLayout in_layout;
    av_channel_layout_default(&in_layout, in_frame->no_channels);

    ret = swr_alloc_set_opts2(&ctx->swr_context, &out_frame->ch_layout,
                              out_frame->format, out_frame->sample_rate,
                              &in_layout, AV_SAMPLE_FMT_FLTP,
                              in_frame->sample_rate, 0, NULL);
    if (ret < 0) {
        av_error_fmt(ctx->error_str, "error converting frame!", ret);
        return NULL;
    }

    if (!ctx->swr_context) {
        sprintf(ctx->error_str, "%s", "swr_alloc_set_opts returned null");
        return NULL;
    }

    if (!swr_is_initialized(ctx->swr_context)) {
        swr_init(ctx->swr_context);
    }

    uint8_t *in[8] = {};

    av_samples_fill_arrays(in, NULL, (const uint8_t *)in_frame->p_data,
                           in_frame->no_channels, in_frame->no_samples,
                           AV_SAMPLE_FMT_FLTP, 0);

    ret = swr_convert(ctx->swr_context, NULL, 0, (const uint8_t **)in,
                      in_frame->no_samples);
    if (ret < 0) {
        av_error_fmt(ctx->error_str, "error converting frame!", ret);
        return NULL;
    }

    int remaining = swr_get_out_samples(ctx->swr_context, 0);
    if (remaining < nb_samples) {
        return NULL;
    }

    out_frame->pkt_dts = get_current_ts_usec() - ctx->start_ts;
    out_frame->pts = swr_next_pts(ctx->swr_context, INT64_MIN);

    ret = swr_convert(ctx->swr_context, out_frame->data, out_frame->nb_samples,
                      NULL, 0);
    if (ret < 0) {
        av_error_fmt(ctx->error_str, "error converting frame!", ret);
        return NULL;
    }

    return out_frame;
}
