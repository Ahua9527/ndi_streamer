/* getopt.h - 命令行选项解析器头文件
   版权所有 (C) 1989-1994,1996-1999,2001,2003,2004,2009,2010
   自由软件基金会
   本文件是GNU C库的一部分

   GNU C库是自由软件，您可以自由分发和修改它，
   遵循GNU较宽松公共许可证条款，版本2.1或更高版本。

   本库无任何担保，详情请参阅GNU较宽松公共许可证
   您应该已经收到GNU较宽松公共许可证的副本，
   如果没有，请写信给自由软件基金会：
   59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */
#ifndef _GETOPT_H

#ifndef __need_getopt
# define _GETOPT_H 1  // 定义主头文件标记

/* 如果未定义__GNU_LIBRARY__，可能是独立使用或首次包含头文件
   对于glibc需要包含<features.h>，但独立使用时不存在。
   因此：如果未定义__GNU_LIBRARY__，包含<ctype.h>，
   它会为我们引入<features.h>（如果来自glibc）。
   选择ctype.h是因为它保证存在且不会污染命名空间 */
#if !defined __GNU_LIBRARY__
# include <ctype.h>  // 用于获取基础定义
#endif

/* 异常处理宏定义 */
#ifndef __THROW
# ifndef __GNUC_PREREQ
#  define __GNUC_PREREQ(maj, min) (0)  // GCC版本检查宏
# endif
# if defined __cplusplus && __GNUC_PREREQ (2,8)
#  define __THROW	throw ()  // C++异常规范
# else
#  define __THROW  // C语言无异常
# endif
#endif

/* 全局变量声明 */

/* 当前选项的参数值 - 由getopt设置 */
extern char *optarg;

/* 下一个要扫描的ARGV元素索引
   - 用于与调用者通信及连续调用间通信
   - 初始调用时为0表示第一次调用(需要初始化)
   - 返回-1时表示第一个非选项参数的位置
   - 其他情况下表示已扫描的进度 */
extern int optind;

/* 是否打印错误信息 (0=不打印未识别选项的错误) */
extern int opterr;

/* 未识别的选项字符 */
extern int optopt;

#ifndef __need_getopt
/* 长选项结构定义 - 用于getopt_long和getopt_long_only
   结构数组以name为NULL的元素结尾 */

struct option
{
  const char *name;   // 选项长名称
  int has_arg;        // 是否有参数: 
                      // 0=无(no_argument), 
                      // 1=必需(required_argument),
                      // 2=可选(optional_argument)
  int *flag;          // 标志变量指针(为NULL时返回val)
  int val;            // 返回值或设置到flag的值
};

/* 选项参数类型常量 */
# define no_argument		0       // 无参数
# define required_argument	1       // 必需参数
# define optional_argument	2       // 可选参数
#endif	/* need getopt */


/* 函数功能说明:
   处理ARGV中的参数(除去程序名)，根据OPTS定义的选项进行解析

   返回值:
   - 成功时返回选项字符
   - 无更多选项时返回-1
   - 未识别选项或缺少参数时设置optopt并返回'?'

   OPTS格式:
   - 单字符选项字母
   - 后跟:表示需要参数
   - 后跟::表示参数可选(GNU扩展)
   - --表示选项结束
   - OPTS以--开头时非选项参数视为'\0'选项的参数(GNU扩展) */

/* 函数原型声明 */
#ifdef __GNU_LIBRARY__
/* 为避免与其他库冲突，仅在GNU C库中声明原型 */
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __THROW;

# if defined __need_getopt && defined __USE_POSIX2 \
  && !defined __USE_POSIX_IMPLICITLY && !defined __USE_GNU
/* GNU getopt扩展功能运行时控制 */
#  ifdef __REDIRECT
  extern int __REDIRECT_NTH (getopt, (int ___argc, char *const *___argv,
				      const char *__shortopts),
			     __posix_getopt);
#  else
extern int __posix_getopt (int ___argc, char *const *___argv,
			   const char *__shortopts) __THROW;
#   define getopt __posix_getopt
#  endif
# endif
#else /* 非GNU库 */
extern int getopt ();  // 标准声明
#endif /* __GNU_LIBRARY__ */

#ifndef __need_getopt
extern int getopt_long (int ___argc, char *const *___argv,
			const char *__shortopts,
		        const struct option *__longopts, int *__longind)
       __THROW;
extern int getopt_long_only (int ___argc, char *const *___argv,
			     const char *__shortopts,
		             const struct option *__longopts, int *__longind)
       __THROW;

#endif

#ifdef	__cplusplus
}
#endif

/* 确保后续可以获取所有定义和声明 */
#undef __need_getopt

#endif /* _GETOPT_H */
#endif /* getopt.h */
