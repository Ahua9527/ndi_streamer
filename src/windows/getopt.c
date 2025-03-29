/* getopt.c - GNU命令行选项解析器实现
   注意：getopt现在是C库的一部分，修改前请确保理解
   "保持文件命名空间清洁"的含义
   版权所有 (C) 1987-2001 自由软件基金会
   本文件是GNU C库的一部分

GNU C库是自由软件，您可以自由分发和修改它，
遵循GNU较宽松公共许可证条款，版本2.1或更高版本。

本库无任何担保，详情请参阅GNU较宽松公共许可证
您应该已经收到GNU较宽松公共许可证的副本，
如果没有，请写信给自由软件基金会：
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/* 防止Alpha OSF/1和AIX 3.2在<stdio.h>/<stdlib.h>中
定义getopt原型 */
#ifndef _NO_PROTO
# define _NO_PROTO  // 禁用原型定义
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if !defined __STDC__ || !__STDC__
/* This is a separate conditional since some stdc systems
reject `defined (const)'.  */
# ifndef const
#  define const
# endif
#endif

#include <stdio.h>

/* Comment out all this code if we are using the GNU C Library, and are not
actually compiling the library itself.  This code is part of the GNU C
Library, but also included in many other GNU distributions.  Compiling
and linking in this code is a waste when using the GNU C library
(especially if it is a shared library).  Rather than having every GNU
program understand `configure --with-gnu-libc' and omit the object files,
it is simpler to just do this in the source for each such file.  */

#define GETOPT_INTERFACE_VERSION 2
#if !defined _LIBC && defined __GLIBC__ && __GLIBC__ >= 2
# include <gnu-versions.h>
# if _GNU_GETOPT_INTERFACE_VERSION == GETOPT_INTERFACE_VERSION
#  define ELIDE_CODE
# endif
#endif

#ifndef ELIDE_CODE


/* This needs to come after some library #include
to get __GNU_LIBRARY__ defined.  */
#ifdef	__GNU_LIBRARY__
/* Don't include stdlib.h for non-GNU C libraries because some of them
contain conflicting prototypes for getopt.  */
# include <stdlib.h>
# include <unistd.h>
#endif	/* GNU C library.  */

#ifdef VMS
# include <unixlib.h>
# if HAVE_STRING_H - 0
#  include <string.h>
# endif
#endif

#ifndef _
/* This is for other GNU distributions with internationalized messages.  */
# if (HAVE_LIBINTL_H && ENABLE_NLS) || defined _LIBC
#  include <libintl.h>
#  ifndef _
#   define _(msgid)	gettext (msgid)
#  endif
# else
#  define _(msgid)	(msgid)
# endif
# if defined _LIBC && defined USE_IN_LIBIO
#  include <wchar.h>
# endif
#endif

/* This version of `getopt' appears to the caller like standard Unix `getopt'
but it behaves differently for the user, since it allows the user
to intersperse the options with the other arguments.

As `getopt' works, it permutes the elements of ARGV so that,
when it is done, all the options precede everything else.  Thus
all application programs are extended to handle flexible argument order.

Setting the environment variable POSIXLY_CORRECT disables permutation.
Then the behavior is completely standard.

GNU application programs can use a third alternative mode in which
they can distinguish the relative order of options and other arguments.  */

#include "getopt.h"

/* 全局变量声明 */

/* optarg - 存储选项参数的指针
   - 当getopt找到一个带参数的选项时，参数值存储在这里
   - 当ordering为RETURN_IN_ORDER时，非选项参数也存储在这里 */
char *optarg;

/* optind - 下一个要扫描的ARGV元素索引
   - 用于与调用者通信及连续调用间通信
   - 初始调用时为0表示第一次调用(需要初始化)
   - 返回-1时表示第一个非选项参数的位置
   - 其他情况下表示已扫描的进度
   - 根据POSIX 1003.2标准，首次调用前必须初始化为1 */
int optind = 1;

/* getopt初始化标志
   早期版本依赖optind==0进行初始化，但这会导致重调用问题 */
int __getopt_initialized;

/* nextchar - 指向当前选项元素中下一个要扫描的字符
   - 允许从上一次停止的位置继续扫描
   - 如果为NULL或空字符串，则前进到下一个ARGV元素 */
static char *nextchar;

/* opterr - 是否打印错误信息标志
   - 调用者可设为0来禁止打印未识别选项的错误 */
int opterr = 1;

/* optopt - 存储未识别的选项字符
   - 在某些系统上必须初始化为'?'以避免链接系统自带的getopt实现 */
int optopt = '?';

/* Describe how to deal with options that follow non-option ARGV-elements.

If the caller did not specify anything,
the default is REQUIRE_ORDER if the environment variable
POSIXLY_CORRECT is defined, PERMUTE otherwise.

REQUIRE_ORDER means don't recognize them as options;
stop option processing when the first non-option is seen.
This is what Unix does.
This mode of operation is selected by either setting the environment
variable POSIXLY_CORRECT, or using `+' as the first character
of the list of option characters.

PERMUTE is the default.  We permute the contents of ARGV as we scan,
so that eventually all the non-options are at the end.  This allows options
to be given in any order, even with programs that were not written to
expect this.

RETURN_IN_ORDER is an option available to programs that were written
to expect options and other ARGV-elements in any order and that care about
the ordering of the two.  We describe each non-option ARGV-element
as if it were the argument of an option with character code 1.
Using `-' as the first character of the list of option characters
selects this mode of operation.

The special argument `--' forces an end of option-scanning regardless
of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
`--' can cause `getopt' to return -1 with `optind' != ARGC.  */

static enum
{
	REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

/* Value of POSIXLY_CORRECT environment variable.  */
static char *posixly_correct;

#ifdef	__GNU_LIBRARY__
/* We want to avoid inclusion of string.h with non-GNU libraries
because there are many ways it can cause trouble.
On some systems, it contains special magic macros that don't work
in GCC.  */
# include <string.h>
# define my_index	strchr
#else

#define HAVE_STRING_H 1
# if HAVE_STRING_H
#  include <string.h>
# else
#  include <strings.h>
# endif

/* Avoid depending on library functions or files
whose names are inconsistent.  */

#ifndef getenv
extern char *getenv();
#endif

static char *
my_index(str, chr)
const char *str;
int chr;
{
	while (*str)
	{
		if (*str == chr)
			return (char *)str;
		str++;
	}
	return 0;
}

/* If using GCC, we can safely declare strlen this way.
If not using GCC, it is ok not to declare it.  */
#ifdef __GNUC__
/* Note that Motorola Delta 68k R3V7 comes with GCC but not stddef.h.
That was relevant to code that was here before.  */
# if (!defined __STDC__ || !__STDC__) && !defined strlen
/* gcc with -traditional declares the built-in strlen to return int,
and has done so at least since version 2.4.5. -- rms.  */
extern int strlen(const char *);
# endif /* not __STDC__ */
#endif /* __GNUC__ */

#endif /* not __GNU_LIBRARY__ */

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
been skipped.  `first_nonopt' is the index in ARGV of the first of them;
`last_nonopt' is the index after the last of them.  */

static int first_nonopt;
static int last_nonopt;

#ifdef _LIBC
/* Stored original parameters.
XXX This is no good solution.  We should rather copy the args so
that we can compare them later.  But we must not use malloc(3).  */
extern int __libc_argc;
extern char **__libc_argv;

/* Bash 2.0 gives us an environment variable containing flags
indicating ARGV elements that should not be considered arguments.  */

# ifdef USE_NONOPTION_FLAGS
/* Defined in getopt_init.c  */
extern char *__getopt_nonoption_flags;

static int nonoption_flags_max_len;
static int nonoption_flags_len;
# endif

# ifdef USE_NONOPTION_FLAGS
#  define SWAP_FLAGS(ch1, ch2) \
if (nonoption_flags_len > 0)						      \
{									      \
	char __tmp = __getopt_nonoption_flags[ch1];			      \
	__getopt_nonoption_flags[ch1] = __getopt_nonoption_flags[ch2];	      \
	__getopt_nonoption_flags[ch2] = __tmp;				      \
}
# else
#  define SWAP_FLAGS(ch1, ch2)
# endif
#else	/* !_LIBC */
# define SWAP_FLAGS(ch1, ch2)
#endif	/* _LIBC */

/* 交换ARGV中两个相邻子序列
   一个子序列是[first_nonopt,last_nonopt) - 已跳过的非选项参数
   另一个是[last_nonopt,optind) - 已处理的选项参数
   交换后更新first_nonopt和last_nonopt指向新的非选项参数位置 */

#if defined __STDC__ && __STDC__
static void exchange(char **);
#endif

static void
exchange(argv)
char **argv;
{
	int bottom = first_nonopt;  // 非选项参数起始位置
	int middle = last_nonopt;   // 非选项参数结束位置
	int top = optind;           // 当前扫描位置
	char *tem;

	/* 将较短的段与较长段的远端交换
	   这样可以将较短段放到正确位置
	   较长段整体位置正确但内部需要进一步交换 */

#if defined _LIBC && defined USE_NONOPTION_FLAGS
	/* 确保__getopt_nonoption_flags字符串处理能正常工作
	   我们的top参数必须在字符串范围内 */
	if (nonoption_flags_len > 0 && top >= nonoption_flags_max_len)
	{
		/* 需要扩展数组 - 用户可能添加了新参数 */
		char *new_str = malloc(top + 1);
		if (new_str == NULL)
			nonoption_flags_len = nonoption_flags_max_len = 0;
		else
		{
			memset(__mempcpy(new_str, __getopt_nonoption_flags,
				nonoption_flags_max_len),
				'\0', top + 1 - nonoption_flags_max_len);
			nonoption_flags_max_len = top + 1;
			__getopt_nonoption_flags = new_str;
		}
	}
#endif

	while (top > middle && middle > bottom)
	{
		if (top - middle > middle - bottom)
		{
			/* 底部段较短 */
			int len = middle - bottom;
			register int i;

			/* 将其与顶部段的上部交换 */
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[top - (middle - bottom) + i];
				argv[top - (middle - bottom) + i] = tem;
				SWAP_FLAGS(bottom + i, top - (middle - bottom) + i);
			}
			/* 将已移动的底部段排除在后续交换外 */
			top -= len;
		}
		else
		{
			/* 顶部段较短 */
			int len = top - middle;
			register int i;

			/* 将其与底部段的下部交换 */
			for (i = 0; i < len; i++)
			{
				tem = argv[bottom + i];
				argv[bottom + i] = argv[middle + i];
				argv[middle + i] = tem;
				SWAP_FLAGS(bottom + i, middle + i);
			}
			/* 将已移动的顶部段排除在后续交换外 */
			bottom += len;
		}
	}

	/* 更新非选项参数的新位置记录 */
	first_nonopt += (optind - last_nonopt);
	last_nonopt = optind;
}

/* 第一次调用时初始化内部数据 */

#if defined __STDC__ && __STDC__
static const char *_getopt_initialize(int, char *const *, const char *);
#endif
static const char *
_getopt_initialize(argc, argv, optstring)
int argc;
char *const *argv;
const char *optstring;
{
	/* 从ARGV元素1开始处理选项(元素0是程序名)
	   之前跳过的非选项ARGV元素序列为空 */

	first_nonopt = last_nonopt = optind;

	nextchar = NULL;

	posixly_correct = getenv("POSIXLY_CORRECT");

	/* 确定如何处理选项和非选项的顺序 */
	if (optstring[0] == '-')
	{
		ordering = RETURN_IN_ORDER;  // 按原始顺序返回
		++optstring;
	}
	else if (optstring[0] == '+')
	{
		ordering = REQUIRE_ORDER;    // 严格顺序(选项必须在非选项前)
		++optstring;
	}
	else if (posixly_correct != NULL)
		ordering = REQUIRE_ORDER;
	else
		ordering = PERMUTE;         // 默认: 置换顺序(选项可与非选项混合)

#if defined _LIBC && defined USE_NONOPTION_FLAGS
	/* 处理非选项标志的特殊逻辑 */
	if (posixly_correct == NULL
		&& argc == __libc_argc && argv == __libc_argv)
	{
		if (nonoption_flags_max_len == 0)
		{
			if (__getopt_nonoption_flags == NULL
				|| __getopt_nonoption_flags[0] == '\0')
				nonoption_flags_max_len = -1;
			else
			{
				const char *orig_str = __getopt_nonoption_flags;
				int len = nonoption_flags_max_len = strlen(orig_str);
				if (nonoption_flags_max_len < argc)
					nonoption_flags_max_len = argc;
				__getopt_nonoption_flags =
					(char *)malloc(nonoption_flags_max_len);
				if (__getopt_nonoption_flags == NULL)
					nonoption_flags_max_len = -1;
				else
					memset(__mempcpy(__getopt_nonoption_flags, orig_str, len),
					'\0', nonoption_flags_max_len - len);
			}
		}
		nonoption_flags_len = nonoption_flags_max_len;
	}
	else
		nonoption_flags_len = 0;
#endif

	return optstring;
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
given in OPTSTRING.

If an element of ARGV starts with '-', and is not exactly "-" or "--",
then it is an option element.  The characters of this element
(aside from the initial '-') are option characters.  If `getopt'
is called repeatedly, it returns successively each of the option characters
from each of the option elements.

If `getopt' finds another option character, it returns that character,
updating `optind' and `nextchar' so that the next call to `getopt' can
resume the scan with the following option character or ARGV-element.

If there are no more option characters, `getopt' returns -1.
Then `optind' is the index in ARGV of the first ARGV-element
that is not an option.  (The ARGV-elements have been permuted
so that those that are not options now come last.)

OPTSTRING is a string containing the legitimate option characters.
If an option character is seen that is not listed in OPTSTRING,
return '?' after printing an error message.  If you set `opterr' to
zero, the error message is suppressed but we still return '?'.

If a char in OPTSTRING is followed by a colon, that means it wants an arg,
so the following text in the same ARGV-element, or the text of the following
ARGV-element, is returned in `optarg'.  Two colons mean an option that
wants an optional arg; if there is text in the current ARGV-element,
it is returned in `optarg', otherwise `optarg' is set to zero.

If OPTSTRING starts with `-' or `+', it requests different methods of
handling the non-option ARGV-elements.
See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

Long-named options begin with `--' instead of `-'.
Their names may be abbreviated as long as the abbreviation is unique
or is an exact match for some defined option.  If they have an
argument, it follows the option name in the same ARGV-element, separated
from the option name by a `=', or else the in next ARGV-element.
When `getopt' finds a long-named option, it returns 0 if that option's
`flag' field is nonzero, the value of the option's `val' field
if the `flag' field is zero.

The elements of ARGV aren't really const, because we permute them.
But we pretend they're const in the prototype to be compatible
with other systems.

LONGOPTS is a vector of `struct option' terminated by an
element containing a name which is zero.

LONGIND returns the index in LONGOPT of the long-named option found.
It is only valid when a long-named option has been found by the most
recent call.

If LONG_ONLY is nonzero, '-' as well as '--' can introduce
long-named options.  */

/* getopt核心实现函数
   参数:
   - argc/argv: 命令行参数
   - optstring: 合法选项字符串
   - longopts: 长选项结构体数组
   - longind: 返回找到的长选项索引
   - long_only: 是否允许'-'开头长选项
   返回值:
   - 选项字符
   - '?' 表示无效选项
   - -1 表示选项处理结束 */
int
_getopt_internal(argc, argv, optstring, longopts, longind, long_only)
int argc;
char *const *argv;
const char *optstring;
const struct option *longopts;
int *longind;
int long_only;
{
	int print_errors = opterr;  // 是否打印错误信息
	if (optstring[0] == ':')   // 以':'开头表示静默模式
		print_errors = 0;

	if (argc < 1)  // 无参数直接返回结束
		return -1;

	optarg = NULL;  // 重置选项参数指针

	// 初始化处理
	if (optind == 0 || !__getopt_initialized)
	{
		if (optind == 0)
			optind = 1;	// 跳过ARGV[0](程序名)
		optstring = _getopt_initialize(argc, argv, optstring);
		__getopt_initialized = 1;
	}

	// 检查当前ARGV元素是否为非选项参数
#if defined _LIBC && defined USE_NONOPTION_FLAGS
# define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0' \
	|| (optind < nonoption_flags_len && __getopt_nonoption_flags[optind] == '1'))
#else
# define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0')
#endif

	// 需要前进到下一个ARGV元素的情况
	if (nextchar == NULL || *nextchar == '\0')
	{
		// 确保first_nonopt和last_nonopt值合理
		if (last_nonopt > optind)
			last_nonopt = optind;
		if (first_nonopt > optind)
			first_nonopt = optind;

		// 处理参数置换逻辑
		if (ordering == PERMUTE)
		{
			// 将选项移到非选项前面
			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char **)argv);
			else if (last_nonopt != optind)
				first_nonopt = optind;

			// 跳过连续的非选项参数
			while (optind < argc && NONOPTION_P)
				optind++;
			last_nonopt = optind;
		}

		// 处理"--"特殊参数(结束选项解析)
		if (optind != argc && !strcmp(argv[optind], "--"))
		{
			optind++;
			if (first_nonopt != last_nonopt && last_nonopt != optind)
				exchange((char **)argv);
			else if (first_nonopt == last_nonopt)
				first_nonopt = optind;
			last_nonopt = argc;
			optind = argc;
		}

		// 所有参数处理完成
		if (optind == argc)
		{
			if (first_nonopt != last_nonopt)
				optind = first_nonopt;
			return -1;
		}

		// 遇到非选项参数时的处理
		if (NONOPTION_P)
		{
			if (ordering == REQUIRE_ORDER)
				return -1;
			optarg = argv[optind++];
			return 1;  // 返回1表示非选项参数
		}

		// 找到新的选项参数，跳过开头的'-'或'--'
		nextchar = (argv[optind] + 1 + (longopts != NULL && argv[optind][1] == '-'));
	}

	// 解析当前选项ARGV元素
	// 首先检查是否为长选项(--option或-long)
	if (longopts != NULL && (argv[optind][1] == '-' || 
		(long_only && (argv[optind][2] || !my_index(optstring, argv[optind][1])))))
	{
		char *nameend;
		const struct option *p;
		const struct option *pfound = NULL;
		int exact = 0;    // 是否精确匹配
		int ambig = 0;    // 是否模糊匹配
		int indfound = -1;
		int option_index;

		// 获取选项名(到'='或字符串结尾)
		for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
			/* Do nothing.  */;

		// 在所有长选项中查找匹配项
		for (p = longopts, option_index = 0; p->name; p++, option_index++)
		if (!strncmp(p->name, nextchar, nameend - nextchar))
		{
			if ((unsigned int)(nameend - nextchar) == (unsigned int)strlen(p->name))
			{
				// 找到精确匹配
				pfound = p;
				indfound = option_index;
				exact = 1;
				break;
			}
			else if (pfound == NULL)
			{
				// 第一个非精确匹配
				pfound = p;
				indfound = option_index;
			}
			else if (long_only
				|| pfound->has_arg != p->has_arg
				|| pfound->flag != p->flag
				|| pfound->val != p->val)
				// 第二个或更多非精确匹配
				ambig = 1;
		}

		// 处理模糊匹配错误
		if (ambig && !exact)
		{
			if (print_errors)
			{
#if defined _LIBC && defined USE_IN_LIBIO
				char *buf;
				__asprintf(&buf, _("%s: option `%s' is ambiguous\n"),
					argv[0], argv[optind]);

				if (_IO_fwide(stderr, 0) > 0)
					__fwprintf(stderr, L"%s", buf);
				else
					fputs(buf, stderr);

				free(buf);
#else
				fprintf(stderr, _("%s: option `%s' is ambiguous\n"),
					argv[0], argv[optind]);
#endif
			}
			nextchar += strlen(nextchar);
			optind++;
			optopt = 0;
			return '?';
		}

		// 处理找到的长选项
		if (pfound != NULL)
		{
			option_index = indfound;
			optind++;
			if (*nameend)
			{
				// 处理带'='的参数
				if (pfound->has_arg)
					optarg = nameend + 1;
				else
				{
					// 选项不允许参数但提供了参数
					if (print_errors)
					{
#if defined _LIBC && defined USE_IN_LIBIO
						char *buf;
#endif
						if (argv[optind - 1][1] == '-')
						{
							// --option格式
#if defined _LIBC && defined USE_IN_LIBIO
							__asprintf(&buf, _("%s: option `--%s' doesn't allow an argument\n"),
								argv[0], pfound->name);
#else
							fprintf(stderr, _("%s: option `--%s' doesn't allow an argument\n"),
								argv[0], pfound->name);
#endif
						}
						else
						{
							// +option或-option格式
#if defined _LIBC && defined USE_IN_LIBIO
							__asprintf(&buf, _("%s: option `%c%s' doesn't allow an argument\n"),
								argv[0], argv[optind - 1][0], pfound->name);
#else
							fprintf(stderr, _("%s: option `%c%s' doesn't allow an argument\n"),
								argv[0], argv[optind - 1][0], pfound->name);
#endif
						}

#if defined _LIBC && defined USE_IN_LIBIO
						if (_IO_fwide(stderr, 0) > 0)
							__fwprintf(stderr, L"%s", buf);
						else
							fputs(buf, stderr);

						free(buf);
#endif
					}

					nextchar += strlen(nextchar);
					optopt = pfound->val;
					return '?';
				}
			}
			else if (pfound->has_arg == 1)
			{
				// 选项需要参数但未提供
				if (optind < argc)
					optarg = argv[optind++];
				else
				{
					if (print_errors)
					{
#if defined _LIBC && defined USE_IN_LIBIO
						char *buf;
						__asprintf(&buf, _("%s: option `%s' requires an argument\n"),
							argv[0], argv[optind - 1]);

						if (_IO_fwide(stderr, 0) > 0)
							__fwprintf(stderr, L"%s", buf);
						else
							fputs(buf, stderr);

						free(buf);
#else
						fprintf(stderr, _("%s: option `%s' requires an argument\n"),
							argv[0], argv[optind - 1]);
#endif
					}
					nextchar += strlen(nextchar);
					optopt = pfound->val;
					return optstring[0] == ':' ? ':' : '?';
				}
			}
			nextchar += strlen(nextchar);
			if (longind != NULL)
				*longind = option_index;
			if (pfound->flag)
			{
				// 选项有flag字段，设置flag值
				*(pfound->flag) = pfound->val;
				return 0;
			}
			return pfound->val;
		}

		// 不是长选项，检查是否是短选项
		if (!long_only || argv[optind][1] == '-'
			|| my_index(optstring, *nextchar) == NULL)
		{
			// 无效选项处理
			if (print_errors)
			{
#if defined _LIBC && defined USE_IN_LIBIO
				char *buf;
#endif
				if (argv[optind][1] == '-')
				{
					// --option格式
#if defined _LIBC && defined USE_IN_LIBIO
					__asprintf(&buf, _("%s: unrecognized option `--%s'\n"),
						argv[0], nextchar);
#else
					fprintf(stderr, _("%s: unrecognized option `--%s'\n"),
						argv[0], nextchar);
#endif
				}
				else
				{
					// +option或-option格式
#if defined _LIBC && defined USE_IN_LIBIO
					__asprintf(&buf, _("%s: unrecognized option `%c%s'\n"),
						argv[0], argv[optind][0], nextchar);
#else
					fprintf(stderr, _("%s: unrecognized option `%c%s'\n"),
						argv[0], argv[optind][0], nextchar);
#endif
				}

#if defined _LIBC && defined USE_IN_LIBIO
				if (_IO_fwide(stderr, 0) > 0)
					__fwprintf(stderr, L"%s", buf);
				else
					fputs(buf, stderr);

				free(buf);
#endif
			}
			nextchar = (char *) "";
			optind++;
			optopt = 0;
			return '?';
		}
	}

	/* Decode the current option-ARGV-element.  */

	/* Check whether the ARGV-element is a long option.

	If long_only and the ARGV-element has the form "-f", where f is
	a valid short option, don't consider it an abbreviated form of
	a long option that starts with f.  Otherwise there would be no
	way to give the -f short option.

	On the other hand, if there's a long option "fubar" and
	the ARGV-element is "-fu", do consider that an abbreviation of
	the long option, just like "--fu", and not "-f" with arg "u".

	This distinction seems to be the most useful approach.  */

	if (longopts != NULL
		&& (argv[optind][1] == '-'
		|| (long_only && (argv[optind][2] || !my_index(optstring, argv[optind][1])))))
	{
		char *nameend;
		const struct option *p;
		const struct option *pfound = NULL;
		int exact = 0;
		int ambig = 0;
		int indfound = -1;
		int option_index;

		for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
			/* Do nothing.  */;

		/* Test all long options for either exact match
		or abbreviated matches.  */
		for (p = longopts, option_index = 0; p->name; p++, option_index++)
		if (!strncmp(p->name, nextchar, nameend - nextchar))
		{
			if ((unsigned int)(nameend - nextchar)
				== (unsigned int)strlen(p->name))
			{
				/* Exact match found.  */
				pfound = p;
				indfound = option_index;
				exact = 1;
				break;
			}
			else if (pfound == NULL)
			{
				/* First nonexact match found.  */
				pfound = p;
				indfound = option_index;
			}
			else if (long_only
				|| pfound->has_arg != p->has_arg
				|| pfound->flag != p->flag
				|| pfound->val != p->val)
				/* Second or later nonexact match found.  */
				ambig = 1;
		}

		if (ambig && !exact)
		{
			if (print_errors)
			{
#if defined _LIBC && defined USE_IN_LIBIO
				char *buf;

				__asprintf(&buf, _("%s: option `%s' is ambiguous\n"),
					argv[0], argv[optind]);

				if (_IO_fwide(stderr, 0) > 0)
					__fwprintf(stderr, L"%s", buf);
				else
					fputs(buf, stderr);

				free(buf);
#else
				fprintf(stderr, _("%s: option `%s' is ambiguous\n"),
					argv[0], argv[optind]);
#endif
			}
			nextchar += strlen(nextchar);
			optind++;
			optopt = 0;
			return '?';
		}

		if (pfound != NULL)
		{
			option_index = indfound;
			optind++;
			if (*nameend)
			{
				/* Don't test has_arg with >, because some C compilers don't
				allow it to be used on enums.  */
				if (pfound->has_arg)
					optarg = nameend + 1;
				else
				{
					if (print_errors)
					{
#if defined _LIBC && defined USE_IN_LIBIO
						char *buf;
#endif

						if (argv[optind - 1][1] == '-')
						{
							/* --option */
#if defined _LIBC && defined USE_IN_LIBIO
							__asprintf(&buf, _("\
											   %s: option `--%s' doesn't allow an argument\n"),
											   argv[0], pfound->name);
#else
							fprintf(stderr, _("\
											  %s: option `--%s' doesn't allow an argument\n"),
											  argv[0], pfound->name);
#endif
						}
						else
						{
							/* +option or -option */
#if defined _LIBC && defined USE_IN_LIBIO
							__asprintf(&buf, _("\
											   %s: option `%c%s' doesn't allow an argument\n"),
											   argv[0], argv[optind - 1][0],
											   pfound->name);
#else
							fprintf(stderr, _("\
											  %s: option `%c%s' doesn't allow an argument\n"),
											  argv[0], argv[optind - 1][0], pfound->name);
#endif
						}

#if defined _LIBC && defined USE_IN_LIBIO
						if (_IO_fwide(stderr, 0) > 0)
							__fwprintf(stderr, L"%s", buf);
						else
							fputs(buf, stderr);

						free(buf);
#endif
					}

					nextchar += strlen(nextchar);

					optopt = pfound->val;
					return '?';
				}
			}
			else if (pfound->has_arg == 1)
			{
				if (optind < argc)
					optarg = argv[optind++];
				else
				{
					if (print_errors)
					{
#if defined _LIBC && defined USE_IN_LIBIO
						char *buf;

						__asprintf(&buf,
							_("%s: option `%s' requires an argument\n"),
							argv[0], argv[optind - 1]);

						if (_IO_fwide(stderr, 0) > 0)
							__fwprintf(stderr, L"%s", buf);
						else
							fputs(buf, stderr);

						free(buf);
#else
						fprintf(stderr,
							_("%s: option `%s' requires an argument\n"),
							argv[0], argv[optind - 1]);
#endif
					}
					nextchar += strlen(nextchar);
					optopt = pfound->val;
					return optstring[0] == ':' ? ':' : '?';
				}
			}
			nextchar += strlen(nextchar);
			if (longind != NULL)
				*longind = option_index;
			if (pfound->flag)
			{
				*(pfound->flag) = pfound->val;
				return 0;
			}
			return pfound->val;
		}

		/* Can't find it as a long option.  If this is not getopt_long_only,
		or the option starts with '--' or is not a valid short
		option, then it's an error.
		Otherwise interpret it as a short option.  */
		if (!long_only || argv[optind][1] == '-'
			|| my_index(optstring, *nextchar) == NULL)
		{
			if (print_errors)
			{
#if defined _LIBC && defined USE_IN_LIBIO
				char *buf;
#endif

				if (argv[optind][1] == '-')
				{
					/* --option */
#if defined _LIBC && defined USE_IN_LIBIO
					__asprintf(&buf, _("%s: unrecognized option `--%s'\n"),
						argv[0], nextchar);
#else
					fprintf(stderr, _("%s: unrecognized option `--%s'\n"),
						argv[0], nextchar);
#endif
				}
				else
				{
					/* +option or -option */
#if defined _LIBC && defined USE_IN_LIBIO
					__asprintf(&buf, _("%s: unrecognized option `%c%s'\n"),
						argv[0], argv[optind][0], nextchar);
#else
					fprintf(stderr, _("%s: unrecognized option `%c%s'\n"),
						argv[0], argv[optind][0], nextchar);
#endif
				}

#if defined _LIBC && defined USE_IN_LIBIO
				if (_IO_fwide(stderr, 0) > 0)
					__fwprintf(stderr, L"%s", buf);
				else
					fputs(buf, stderr);

				free(buf);
#endif
			}
			nextchar = (char *) "";
			optind++;
			optopt = 0;
			return '?';
		}
	}

	// 处理短选项(-a, -b等)
	{
		char c = *nextchar++;  // 获取当前选项字符
		char *temp = my_index(optstring, c);  // 在optstring中查找选项

		// 如果这是当前ARGV元素的最后一个字符，前进到下一个ARGV元素
		if (*nextchar == '\0')
			++optind;

		// 无效选项处理
		if (temp == NULL || c == ':')
		{
			if (print_errors)
			{
#if defined _LIBC && defined USE_IN_LIBIO
				char *buf;
#endif
				if (posixly_correct)
				{
					// POSIX标准错误格式
#if defined _LIBC && defined USE_IN_LIBIO
					__asprintf(&buf, _("%s: illegal option -- %c\n"),
						argv[0], c);
#else
					fprintf(stderr, _("%s: illegal option -- %c\n"), argv[0], c);
#endif
				}
				else
				{
					// GNU扩展错误格式
#if defined _LIBC && defined USE_IN_LIBIO
					__asprintf(&buf, _("%s: invalid option -- %c\n"),
						argv[0], c);
#else
					fprintf(stderr, _("%s: invalid option -- %c\n"), argv[0], c);
#endif
				}

#if defined _LIBC && defined USE_IN_LIBIO
				if (_IO_fwide(stderr, 0) > 0)
					__fwprintf(stderr, L"%s", buf);
				else
					fputs(buf, stderr);

				free(buf);
#endif
			}
			optopt = c;
			return '?';
		}
		// 特殊处理POSIX -W选项(等同于长选项--)
		if (temp[0] == 'W' && temp[1] == ';')
		{
			char *nameend;
			const struct option *p;
			const struct option *pfound = NULL;
			int exact = 0;
			int ambig = 0;
			int indfound = 0;
			int option_index;

			// 处理-W选项的参数
			if (*nextchar != '\0')
			{
				optarg = nextchar;
				optind++;
			}
			else if (optind == argc)
			{
				if (print_errors)
				{
#if defined _LIBC && defined USE_IN_LIBIO
					char *buf;
					__asprintf(&buf, _("%s: option requires an argument -- %c\n"),
						argv[0], c);

					if (_IO_fwide(stderr, 0) > 0)
						__fwprintf(stderr, L"%s", buf);
					else
						fputs(buf, stderr);

					free(buf);
#else
					fprintf(stderr, _("%s: option requires an argument -- %c\n"),
						argv[0], c);
#endif
				}
				optopt = c;
				if (optstring[0] == ':')
					c = ':';
				else
					c = '?';
				return c;
			}
			else
				optarg = argv[optind++];

			// 在长选项表中查找匹配项
			for (nextchar = nameend = optarg; *nameend && *nameend != '='; nameend++)
				/* Do nothing.  */;

			for (p = longopts, option_index = 0; p->name; p++, option_index++)
			if (!strncmp(p->name, nextchar, nameend - nextchar))
			{
				if ((unsigned int)(nameend - nextchar) == strlen(p->name))
				{
					pfound = p;
					indfound = option_index;
					exact = 1;
					break;
				}
				else if (pfound == NULL)
				{
					pfound = p;
					indfound = option_index;
				}
				else
					ambig = 1;
			}

			if (ambig && !exact)
			{
				if (print_errors)
				{
#if defined _LIBC && defined USE_IN_LIBIO
					char *buf;
					__asprintf(&buf, _("%s: option `-W %s' is ambiguous\n"),
						argv[0], argv[optind]);

					if (_IO_fwide(stderr, 0) > 0)
						__fwprintf(stderr, L"%s", buf);
					else
						fputs(buf, stderr);

					free(buf);
#else
					fprintf(stderr, _("%s: option `-W %s' is ambiguous\n"),
						argv[0], argv[optind]);
#endif
				}
				nextchar += strlen(nextchar);
				optind++;
				return '?';
			}

			if (pfound != NULL)
			{
				option_index = indfound;
				if (*nameend)
				{
					if (pfound->has_arg)
						optarg = nameend + 1;
					else
					{
						if (print_errors)
						{
#if defined _LIBC && defined USE_IN_LIBIO
							char *buf;
							__asprintf(&buf, _("%s: option `-W %s' doesn't allow an argument\n"),
								argv[0], pfound->name);

							if (_IO_fwide(stderr, 0) > 0)
								__fwprintf(stderr, L"%s", buf);
							else
								fputs(buf, stderr);

							free(buf);
#else
							fprintf(stderr, _("%s: option `-W %s' doesn't allow an argument\n"),
								argv[0], pfound->name);
#endif
						}
						nextchar += strlen(nextchar);
						return '?';
					}
				}
				else if (pfound->has_arg == 1)
				{
					if (optind < argc)
						optarg = argv[optind++];
					else
					{
						if (print_errors)
						{
#if defined _LIBC && defined USE_IN_LIBIO
							char *buf;
							__asprintf(&buf, _("%s: option `%s' requires an argument\n"),
								argv[0], argv[optind - 1]);

							if (_IO_fwide(stderr, 0) > 0)
								__fwprintf(stderr, L"%s", buf);
							else
								fputs(buf, stderr);

							free(buf);
#else
							fprintf(stderr, _("%s: option `%s' requires an argument\n"),
								argv[0], argv[optind - 1]);
#endif
						}
						nextchar += strlen(nextchar);
						return optstring[0] == ':' ? ':' : '?';
					}
				}
				nextchar += strlen(nextchar);
				if (longind != NULL)
					*longind = option_index;
				if (pfound->flag)
				{
					*(pfound->flag) = pfound->val;
					return 0;
				}
				return pfound->val;
			}
			nextchar = NULL;
			return 'W';	// 让应用程序处理
		}
		// 处理带参数的选项
		if (temp[1] == ':')
		{
			if (temp[2] == ':')
			{
				// 可选参数选项
				if (*nextchar != '\0')
				{
					optarg = nextchar;
					optind++;
				}
				else
					optarg = NULL;
				nextchar = NULL;
			}
			else
			{
				// 必需参数选项
				if (*nextchar != '\0')
				{
					optarg = nextchar;
					optind++;
				}
				else if (optind == argc)
				{
					if (print_errors)
					{
#if defined _LIBC && defined USE_IN_LIBIO
						char *buf;
						__asprintf(&buf, _("%s: option requires an argument -- %c\n"),
							argv[0], c);

						if (_IO_fwide(stderr, 0) > 0)
							__fwprintf(stderr, L"%s", buf);
						else
							fputs(buf, stderr);

						free(buf);
#else
						fprintf(stderr, _("%s: option requires an argument -- %c\n"),
							argv[0], c);
#endif
					}
					optopt = c;
					if (optstring[0] == ':')
						c = ':';
					else
						c = '?';
				}
				else
					optarg = argv[optind++];
				nextchar = NULL;
			}
		}
		return c;
	}
}

int
getopt(argc, argv, optstring)
int argc;
char *const *argv;
const char *optstring;
{
	return _getopt_internal(argc, argv, optstring,
		(const struct option *) 0,
		(int *)0,
		0);
}




int
getopt_long(int argc, char *const *argv, const char *options,
const struct option *long_options, int *opt_index)
{
	return _getopt_internal(argc, argv, options, long_options, opt_index, 0, 0);
}

int
getopt_long_only(int argc, char *const *argv, const char *options,
const struct option *long_options, int *opt_index)
{
	return _getopt_internal(argc, argv, options, long_options, opt_index, 1, 0);
}





#endif	/* Not ELIDE_CODE.  */

#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
the above definition of `getopt'.  */

int
main(argc, argv)
int argc;
char **argv;
{
	int c;
	int digit_optind = 0;

	while (1)
	{
		int this_option_optind = optind ? optind : 1;

		c = getopt(argc, argv, "abc:d:0123456789");
		if (c == -1)
			break;

		switch (c)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (digit_optind != 0 && digit_optind != this_option_optind)
					printf("digits occur in two different argv-elements.\n");
				digit_optind = this_option_optind;
				printf("option %c\n", c);
				break;

			case 'a':
				printf("option a\n");
				break;

			case 'b':
				printf("option b\n");
				break;

			case 'c':
				printf("option c with value `%s'\n", optarg);
				break;

			case '?':
				break;

			default:
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (optind < argc)
	{
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
	}

	exit(0);
}

#endif /* TEST */
