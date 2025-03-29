// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "util.h"

#include <stdatomic.h>
#ifdef _WIN32
#include <windows.h>  // Windows平台特定头文件
#else
#include <pthread.h>  // POSIX线程头文件
#endif
#include <signal.h>   // 信号处理头文件
#include <stdio.h>    // 标准输入输出
#include <stdlib.h>   // 标准库函数
#include <string.h>   // 字符串处理

#ifdef _WIN32
HANDLE mu;               // Windows互斥锁句柄
CONDITION_VARIABLE cv;   // Windows条件变量
CRITICAL_SECTION cvl;    // Windows临界区
#else
pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;  // POSIX互斥锁初始化
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;    // POSIX条件变量初始化
#endif

int eh_initialized = 0;           // 事件处理初始化标志
_Atomic(int) eh_got_signal = 0;   // 原子标志，表示是否收到信号

// 选项解析器内部上下文结构体
typedef struct OPInternalCtx {
    char *short_options;          // 短选项字符串
    struct option *r_options;     // 原始选项结构数组
    const ProgramOption *p_options; // 程序选项数组
} OPInternalCtx;

#ifdef _WIN32
// Windows信号处理函数
BOOL WINAPI
eh_signal_handler(DWORD param)
{
    atomic_store(&eh_got_signal, 1);  // 原子操作设置信号标志
    WakeAllConditionVariable(&cv);    // 唤醒所有等待条件变量的线程
    return 1;
}

// Windows平台事件处理初始化
void
eh_init()
{
    if (!eh_initialized) {
        atomic_store(&eh_got_signal, 0);  // 初始化信号标志
        mu = CreateMutex(NULL, FALSE, NULL);  // 创建互斥锁
        InitializeConditionVariable(&cv);     // 初始化条件变量
        InitializeCriticalSection(&cvl);      // 初始化临界区
        SetConsoleCtrlHandler(&eh_signal_handler, 1);  // 设置控制台控制处理器
        eh_initialized = 1;  // 设置初始化标志
    }
}

// Windows平台等待信号
void
eh_wait()
{
    if (!eh_initialized) {
        eh_init();  // 如果未初始化，先初始化
    }

    WaitForSingleObject(mu, INFINITE);  // 获取互斥锁

    while (eh_alive()) {  // 检查是否收到终止信号
        SleepConditionVariableCS (&cv, &cvl, INFINITE);  // 等待条件变量
    }
    ReleaseMutex(mu);  // 释放互斥锁
}

#else
// POSIX信号处理函数
void
eh_signal_handler(__attribute__((unused)) int _)
{
    atomic_store(&eh_got_signal, 1);  // 原子操作设置信号标志
    pthread_cond_signal(&cv);         // 发送条件变量信号
}

// POSIX平台事件处理初始化
void
eh_init()
{
    if (!eh_initialized) {
        atomic_store(&eh_got_signal, 0);  // 初始化信号标志
        struct sigaction action = {};     // 信号动作结构体
        action.sa_handler = &eh_signal_handler;  // 设置信号处理函数
        sigemptyset(&action.sa_mask);      // 清空信号掩码
        action.sa_flags = 0;              // 无特殊标志
        sigaction(SIGINT, &action, NULL); // 设置SIGINT信号处理
        eh_initialized = 1;               // 设置初始化标志
    }
}

// POSIX平台等待信号
void
eh_wait()
{
    pthread_mutex_lock(&mu);  // 获取互斥锁
    if (!eh_initialized) {
        eh_init();  // 如果未初始化，先初始化
    }

    while (eh_alive()) {  // 检查是否收到终止信号
        pthread_cond_wait(&cv, &mu);  // 等待条件变量
    }
    pthread_mutex_unlock(&mu);  // 释放互斥锁
}
#endif

// 检查是否存活(未收到终止信号)
int
eh_alive()
{
    return !atomic_load(&eh_got_signal);  // 原子读取信号标志
}

// 初始化选项解析器
OptionParserCtx *
op_init(const ProgramOption *options)
{
    int n, i, j, no_s_args = 0;
    struct option *raw_options;
    char *ptr, *short_options, *name;
    OPInternalCtx *res;

    // 计算短选项参数数量
    for (n = 0; options[n].name != NULL; ++n) {
        ptr = strchr(options[n].name, ',');
        if (ptr != NULL) {
            no_s_args += options[n].is_flag ? 1 : 2;
        }
    }
    
    // 分配内存
    res = malloc(sizeof(OPInternalCtx));
    raw_options = malloc(sizeof(struct option) * (n + 1));
    memset(raw_options, 0, sizeof(struct option) * (n + 1));
    short_options = malloc(no_s_args + 1);
    j = 0;

    // 填充选项结构
    for (i = 0; i < n; ++i) {
        name = options[i].name;
        ptr = strchr(name, ',');
        if (ptr == NULL) {
            raw_options[i].name = name;  // 只有长选项
        }
        else {
            raw_options[i].name = &ptr[1];  // 长选项部分
            if (options[i].is_flag) {
                short_options[j++] = name[0];  // 标志选项
            }
            else {
                short_options[j++] = name[0];  // 带参数选项
                short_options[j++] = ':';
            }
        }

        raw_options[i].has_arg
                = options[n].is_flag ? no_argument : required_argument;
        raw_options[i].val = i;
        raw_options[i].flag = NULL;
    }

    short_options[j] = '\0';  // 终止短选项字符串
    res->r_options = raw_options;
    res->p_options = options;
    res->short_options = short_options;

    return (OptionParserCtx *)res;
}

// 释放选项解析器资源
void
op_free(OptionParserCtx **ctx)
{
    OPInternalCtx *ctx_internal = (OPInternalCtx *)(*ctx);
    free(ctx_internal->r_options);      // 释放原始选项数组
    free(ctx_internal->short_options);  // 释放短选项字符串
    free(ctx_internal);                 // 释放上下文结构体
    *ctx = NULL;                        // 置空指针
}

// 解析命令行选项
int
op_parse(int argc, char **argv, const OptionParserCtx *ctx,
         const ProgramOption **res)
{
    OPInternalCtx *ctx_internal = (OPInternalCtx *)ctx;
    int option_index = -1;
    // 使用getopt_long解析选项
    int c = getopt_long(argc, argv, ctx_internal->short_options,
                        ctx_internal->r_options, &option_index);
    *res = NULL;

    if (c < 0)
        return c;

    const ProgramOption *options = ctx_internal->p_options;
    if (option_index == -1) {
        // 处理短选项
        for (int i = 0; options[i].name; ++i) {
            if (strchr(options[i].name, ',') != NULL
                && options[i].name[0] == c) {
                *res = &options[i];  // 设置匹配的选项
            }
        }

        return c;
    }

    *res = &options[option_index];  // 设置匹配的选项

    if (strchr(options[option_index].name, ',') != NULL) {
        return options[option_index].name[0];  // 返回短选项字符
    }

    return c;
}

// 打印帮助信息
void
op_print_help(char *name, const OptionParserCtx *ctx)
{
    OPInternalCtx *ctx_internal = (OPInternalCtx *)ctx;
    const ProgramOption *options = ctx_internal->p_options;

    printf("Usage of %s: \n", name);
    for (int i = 0; options[i].name; ++i) {
        char *ptr = strchr(options[i].name, ',');

        if (ptr == NULL) {
            // 只有长选项
            printf("    --%s: %s\n", options[i].name, options[i].description);
        }
        else {
            // 短选项和长选项
            printf("    -%c, --%s: %s\n", options[i].name[0], &ptr[1],
                   options[i].description);
        }
    }
}
