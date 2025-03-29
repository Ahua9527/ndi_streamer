// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

// FFmpeg输出上下文头文件
// 定义FFmpeg输出相关的结构体和接口函数
// 用于封装FFmpeg格式输出、编码器设置和帧写入等操作

#ifndef FFMPEG_OUTPUT_H
#define FFMPEG_OUTPUT_H

#include <libavcodec/avcodec.h>

typedef struct FFmpegOutputCtx {
    struct AVFormatContext *o_ctx;        // FFmpeg输出格式上下文
    struct AVCodecContext *audio_codec_ctx; // 音频编码器上下文
    struct AVCodecContext *video_codec_ctx; // 视频编码器上下文
    int audio_stream_index;              // 音频流索引
    int video_stream_index;              // 视频流索引
    const char *output;                  // 输出文件路径/URL
    char *error_str;                     // 错误信息字符串
} FFmpegOutputCtx;

// 创建新的FFmpeg输出上下文
// 返回值: 成功返回FFmpegOutputCtx指针，失败返回NULL
FFmpegOutputCtx *
new_ffmpeg_output_ctx();

// 释放FFmpeg输出上下文
// 参数: ctx - 指向FFmpegOutputCtx指针的指针
// 返回值: 成功返回0，失败返回负数错误码
int
free_ffmpeg_output_ctx(FFmpegOutputCtx **ctx);

// 初始化FFmpeg输出上下文
// 参数: 
//   ctx - FFmpeg输出上下文指针
//   format - 输出格式名称(如"flv","mp4"等)
//   output - 输出文件路径/URL
// 返回值: 成功返回0，失败返回负数错误码
int
ffmpeg_output_init(FFmpegOutputCtx *ctx, const char *format,
                   const char *output);

// 关闭FFmpeg输出上下文
// 参数: ctx - FFmpeg输出上下文指针
// 注意: 会释放内部资源但不会释放ctx本身
void
ffmpeg_output_close(FFmpegOutputCtx *ctx);

// 关闭FFmpeg编码器
// 参数: ctx - FFmpeg输出上下文指针
// 注意: 仅关闭音频/视频编码器，不关闭输出上下文
void
ffmpeg_output_close_codecs(FFmpegOutputCtx *ctx);

// 写入输出文件头
// 参数:
//   ctx - FFmpeg输出上下文指针
//   av_opts - 附加的AVDictionary选项
// 返回值: 成功返回0，失败返回负数错误码
int
ffmpeg_output_write_header(FFmpegOutputCtx *ctx, AVDictionary **av_opts);

// 设置视频编码器
// 参数:
//   ctx - FFmpeg输出上下文指针
//   encoder_name - 编码器名称(如"libx264")
//   width - 视频宽度(像素)
//   height - 视频高度(像素)
//   framerate - 帧率(AVRational结构)
//   bitrate - 视频比特率(比特/秒)
// 返回值: 成功返回0，失败返回负数错误码
int
ffmpeg_output_setup_video(FFmpegOutputCtx *ctx, const char *encoder_name,
                          int width, int height, AVRational framerate,
                          int64_t bitrate);

// 设置音频编码器
// 参数:
//   ctx - FFmpeg输出上下文指针
//   encoder_name - 编码器名称(如"aac")
//   bitrate - 音频比特率(比特/秒)
// 返回值: 成功返回0，失败返回负数错误码
int
ffmpeg_output_setup_audio(FFmpegOutputCtx *ctx, char *encoder_name,
                          int64_t bitrate);
// 发送视频帧到编码器
// 参数:
//   ctx - FFmpeg输出上下文指针
//   frame - 视频帧(AVFrame结构)
// 返回值: 成功返回0，失败返回负数错误码
int
ffmpeg_output_send_video_frame(FFmpegOutputCtx *ctx, AVFrame *frame);

// 发送音频帧到编码器
// 参数:
//   ctx - FFmpeg输出上下文指针
//   frame - 音频帧(AVFrame结构)
// 返回值: 成功返回0，失败返回负数错误码
int
ffmpeg_output_send_audio_frame(FFmpegOutputCtx *ctx, AVFrame *frame);

#endif
