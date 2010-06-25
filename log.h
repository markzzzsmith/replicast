#ifndef __LOG_H_
#define __LOG_H_

#include <stdarg.h>


#ifdef LIBLOG_DEBUG_ALL
	#define LIBLOG_DEBUG
	#define LIBLOG_DEBUG_HIGH
	#define LIBLOG_DEBUG_MED
	#define LIBLOG_DEBUG_LOW
#endif /* LIBLOG_DEBUG_ALL */

enum log_destination {
	LOG_DEST_DEFAULT = 0x00,	/* log module default */
	LOG_DEST_FILE = 0x01,
	LOG_DEST_SYSLOG = 0x02,
	LOG_DEST_TTY = 0x04,		/* stdout, stderr */
	LOG_DEST_TTY_STDOUT = 0x08,	/* stdout only */
	LOG_DEST_LOCAL_FUNC = 0x10,	/* caller provided log function */
};


enum log_syslog_facility {		/* so we don't need syslog.h */
	LOG_SYSLOG_NONE,
	LOG_SYSLOG_KERN,
	LOG_SYSLOG_USER,
	LOG_SYSLOG_MAIL,
	LOG_SYSLOG_DAEMON,
	LOG_SYSLOG_AUTH,
	LOG_SYSLOG_SYSLOG,
	LOG_SYSLOG_LPR,
	LOG_SYSLOG_NEWS,
	LOG_SYSLOG_UUCP,
	LOG_SYSLOG_CRON,
	LOG_SYSLOG_AUTHPRIV,
	LOG_SYSLOG_FTP,
	LOG_SYSLOG_LOCAL0,
	LOG_SYSLOG_LOCAL1,
	LOG_SYSLOG_LOCAL2,
	LOG_SYSLOG_LOCAL3,
	LOG_SYSLOG_LOCAL4,
	LOG_SYSLOG_LOCAL5,
	LOG_SYSLOG_LOCAL6,
	LOG_SYSLOG_LOCAL7,
};


enum log_msg_severity {			/* syslog prios plus extra debug */	
	LOG_SEV_EMERG = 1,
	LOG_SEV_ALERT,
	LOG_SEV_CRIT,
	LOG_SEV_ERR,
	LOG_SEV_WARNING,
	LOG_SEV_NOTICE,
	LOG_SEV_INFO,
	LOG_SEV_DEBUG,
	LOG_SEV_DEBUG_HIGH = LOG_SEV_DEBUG,
	LOG_SEV_DEBUG_MED = LOG_SEV_DEBUG_HIGH+1,
	LOG_SEV_DEBUG_LOW = LOG_SEV_DEBUG_MED+1,	
};

void log_open(const enum log_destination log_dest,
              const char *syslog_ident,
              const enum log_syslog_facility syslog_facility,
              const char *log_file,
              void (*logf_open_fail)(const char *, void *),
              void *logf_open_fail_ptr,
              void (*log_msg_local_func)
                (const enum log_msg_severity, const char *, va_list));


void log_msg(const enum log_msg_severity msg_severity,
             const char *fmt,
             ...);

void log_msg_va(const enum log_msg_severity msg_severity,
		const char *fmt,
		va_list fmt_args);


void log_msg_exit(const enum log_msg_severity msg_severity,
                  void (*exit_func)(void *),
                  void *exit_ptr,
                  const char *fmt,
                  ...);

#ifdef LIBLOG_DEBUG
void log_debug(const char *fmt, ...);
#else
#define log_debug(...) {}
#endif /* LIBLOG_DEBUG */

#ifdef LIBLOG_DEBUG_HIGH
void log_debug_high(const char *fmt, ...);
#else
#define log_debug_high(...) {}
#endif /* LIBLOG_DEBUG_HIGH */

#ifdef LIBLOG_DEBUG_MED
void log_debug_med(const char *fmt, ...);
#else
#define log_debug_med(...) {}
#endif /* LIBLOG_DEBUG_MED */

#ifdef LIBLOG_DEBUG_LOW
void log_debug_low(const char *fmt, ...);
#else
#define log_debug_low(...) {}
#endif /* LIBLOG_DEBUG_LOW */

void log_msg_tty(const enum log_msg_severity msg_severity,
		 const char *fmt,
		 ...);

void log_close(void);

void log_set_detail_level(const enum log_msg_severity msg_severity);

enum log_msg_severity log_get_detail_level(void);

#endif /* __LOG_H_ */
