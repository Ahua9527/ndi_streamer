// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "common.h"

// 平台相关头文件
#include <libavutil/avutil.h>  // FFmpeg工具库
#ifdef _WIN32
#include <windows.h>  // Windows系统API
#else
#include <sys/time.h>  // Unix时间函数
#endif

/**
 * 格式化FFmpeg错误信息
 * @param out 输出缓冲区，用于存储格式化后的错误信息
 * @param str 自定义错误描述
 * @param ec FFmpeg错误码
 */
void
av_error_fmt(char *out, char *str, int ec)
{
    char error_buf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(error_buf, AV_ERROR_MAX_STRING_SIZE, ec);
    sprintf(out, "%s, (%s)\n", str, error_buf);
}

/**
 * 获取当前时间戳(微秒级)
 * @return 返回当前时间戳(从1970年1月1日开始的微秒数)
 */
int64_t
get_current_ts_usec()
{
#ifdef _WIN32
    FILETIME               filetime; /* 64位值，表示从1601年1月1日开始的100纳秒间隔数 */
    ULARGE_INTEGER         x;
    static const ULONGLONG epoch_offset_us = 11644473600000000ULL; /* 1601年1月1日到1970年1月1日的微秒数 */

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime(&filetime);  // 高精度时间获取(Windows 8+)
#else
    GetSystemTimeAsFileTime(&filetime);  // 标准时间获取
#endif
    x.LowPart =  filetime.dwLowDateTime;
    x.HighPart = filetime.dwHighDateTime;
    return (int64_t)(x.QuadPart / 10 - epoch_offset_us);
#else
    struct timeval now;
    gettimeofday(&now, NULL);  // Unix系统获取时间
    return (int64_t)now.tv_sec * 1000000 + (int64_t)now.tv_usec;
#endif
}
