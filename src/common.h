// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>  // 标准整数类型

/**
 * 格式化FFmpeg错误信息
 * @param out 输出缓冲区，用于存储格式化后的错误信息
 * @param str 自定义错误描述
 * @param ec FFmpeg错误码
 */
void
av_error_fmt(char *out, char *str, int ec);

/**
 * 获取当前时间戳(微秒级)
 * @return 返回当前时间戳(从1970年1月1日开始的微秒数)
 */
int64_t
get_current_ts_usec();

#endif
