// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef UTIL_H
#define UTIL_H
#include <getopt.h>

/**
 * 程序选项结构体
 * @param name 选项名称(如"-h"或"--help")
 * @param description 选项描述文本
 * @param is_flag 标志位，表示是否为标志选项(不需要参数)
 */
typedef struct ProgramOption {
    char *name;
    char *description;
    int is_flag;
} ProgramOption;

// 选项解析器上下文(不透明结构体，具体实现在.c文件中)
typedef struct OptionParserCtx OptionParserCtx;

/**
 * 初始化事件处理器
 */
void
eh_init();

/**
 * 检查事件处理器是否存活
 * @return 1表示存活，0表示不存活
 */
int
eh_alive();

/**
 * 等待事件处理器完成
 */
void
eh_wait();

/**
 * 初始化选项解析器
 * @param options 程序选项数组
 * @return 初始化后的选项解析器上下文
 */
OptionParserCtx *
op_init(const ProgramOption *options);

/**
 * 释放选项解析器资源
 * @param ctx 指向选项解析器上下文指针的指针
 */
void
op_free(OptionParserCtx **ctx);

/**
 * 解析命令行参数
 * @param argc 参数个数
 * @param argv 参数数组
 * @param ctx 选项解析器上下文
 * @param res 用于存储解析结果的指针
 * @return 成功返回0，失败返回非0
 */
int
op_parse(int argc, char **argv, const OptionParserCtx *ctx,
         const ProgramOption **res);

/**
 * 打印帮助信息
 * @param name 程序名称
 * @param ctx 选项解析器上下文
 */
void
op_print_help(char *name, const OptionParserCtx *ctx);

#endif
