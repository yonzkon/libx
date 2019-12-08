#ifndef _LIBY_SHOWMSG_H
#define _LIBY_SHOWMSG_H

#ifdef _WIN32
#define	CL_RESET	""
#define CL_NORMAL	CL_RESET
#define CL_NONE		CL_RESET
#define	CL_WHITE	""
#define	CL_GRAY		""
#define	CL_RED		""
#define	CL_GREEN	""
#define	CL_YELLOW	""
#define	CL_BLUE		""
#define	CL_MAGENTA	""
#define	CL_CYAN		""
#else
#define	CL_RESET	"\033[0;0m"
#define CL_NORMAL	CL_RESET
#define CL_NONE		CL_RESET
#define	CL_WHITE	"\033[1;29m"
#define	CL_GRAY		"\033[1;30m"
#define	CL_RED		"\033[1;31m"
#define	CL_GREEN	"\033[1;32m"
#define	CL_YELLOW	"\033[1;33m"
#define	CL_BLUE		"\033[1;34m"
#define	CL_MAGENTA	"\033[1;35m"
#define	CL_CYAN		"\033[1;36m"
#endif

enum msg_type {
	MSG_NONE,
	MSG_STATUS,
	MSG_SQL,

	MSG_FATAL = 100,
	MSG_ERROR,
	MSG_WARN,
	MSG_NOTICE,
	MSG_INFO,
	MSG_DEBUG,
};

extern char tmp_output[1024];
extern enum msg_type msglevel;

#ifdef __cplusplus
extern "C" {
#endif

int showmsg_level(int /*enum msg_type*/);

int show_message(const char *, ...);
int show_status(const char *, ...);
int show_sql(const char *, ...);

int show_fatal(const char *, ...);
int show_error(const char *, ...);
int show_warn(const char *, ...);
int show_notice(const char *, ...);
int show_info(const char *, ...);
int show_debug(const char *, ...);

#ifdef __cplusplus
}
#endif

#endif // _LIBYX_SHOWMSG_H
