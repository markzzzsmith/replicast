/*
 * Message logging routines
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <syslog.h>

#include "log.h"


enum {
	SYSLOG_VSNPRINTF_BUF_SZ = 4096,
};

static enum log_msg_severity log_severity = LOG_SEV_DEBUG_MED;
static int log_dests = LOG_DEST_TTY;
static void (*log_msg_local_function)
                (const enum log_msg_severity, const char *, va_list) = NULL;


static FILE *log_file_fd = NULL;
static const int log_default_syslog_facility = LOG_USER;


static void log_dest_file(FILE **fd,
			  const char *log_file,
			  void (*logf_open_fail)(const char *, void *),
			  void *logf_open_fail_ptr);


static int log_syslog_facil(const enum log_syslog_facility facil);

static void log_dest_syslog(const char *syslog_ident,
			    const enum log_syslog_facility facil);

static void log_dest_local_func(void (*log_msg_local_func)
				       (const enum log_msg_severity,
					const char *,
					va_list));

static void log_msg_file_va(const char *fmt,
			    va_list fmt_args);

static int log_msg_sev_to_sysl_prio(const enum log_msg_severity msg_severity);

static void log_msg_syslog_va(const enum log_msg_severity msg_severity,
			      const char *fmt,
			      va_list fmt_args);

static void log_msg_tty_va(const enum log_msg_severity msg_severity,
			   const char *fmt,
			   va_list fmt_args);

static void log_file_close(void);

static void log_syslog_close(void);

void log_open(const enum log_destination log_dest,
              const char *syslog_ident,
	      const enum log_syslog_facility syslog_facility,		
              const char *log_file,
              void (*logf_open_fail)(const char *, void *),
              void *logf_open_fail_ptr,
	      void (*log_msg_local_func)
		(const enum log_msg_severity, const char *, va_list))
{


	if (log_dest != LOG_DEST_DEFAULT) {

		log_dests = log_dest;	

		if (log_dest & LOG_DEST_FILE) {
			log_dest_file(&log_file_fd, log_file,
				      logf_open_fail, logf_open_fail_ptr);
		}

		if ((log_dest & LOG_DEST_SYSLOG) &&
		    (syslog_facility != LOG_SYSLOG_NONE)) {
			log_dest_syslog(syslog_ident, syslog_facility);
		}

		if (log_dest & LOG_DEST_LOCAL_FUNC) {
			log_dest_local_func(log_msg_local_func);
		}
	}

}


void log_msg(const enum log_msg_severity msg_severity,
             const char *fmt,
             ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_va(msg_severity, fmt, fmt_args);

	va_end(fmt_args);

}


void log_msg_exit(const enum log_msg_severity msg_severity,
                  void (*exit_func)(void *),
                  void *exit_ptr,
                  const char *fmt,
                  ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_va(msg_severity, fmt, fmt_args);

	va_end(fmt_args);

	if (exit_func != NULL) {
		(*exit_func)(exit_ptr);
	}

}


#ifdef LIBLOG_DEBUG
void log_debug(const char *fmt, ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_va(LOG_SEV_DEBUG, fmt, fmt_args);

	va_end(fmt_args);

}
#endif /* LIBLOG_DEBUG */


#ifdef LIBLOG_DEBUG_HIGH
void log_debug_high(const char *fmt, ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_va(LOG_SEV_DEBUG_HIGH, fmt, fmt_args);

	va_end(fmt_args);

}
#endif /* LIBLOG_DEBUG_HIGH */


#ifdef LIBLOG_DEBUG_MED
void log_debug_med(const char *fmt, ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_va(LOG_SEV_DEBUG_MED, fmt, fmt_args);

	va_end(fmt_args);

}
#endif /* LIBLOG_DEBUG_MED */


#ifdef LIBLOG_DEBUG_LOW
void log_debug_low(const char *fmt, ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_va(LOG_SEV_DEBUG_LOW, fmt, fmt_args);

	va_end(fmt_args);

}
#endif /* LIBLOG_DEBUG_LOW */


void log_msg_tty(const enum log_msg_severity msg_severity,
		 const char *fmt,
		 ...)
{
	va_list fmt_args;


	va_start(fmt_args, fmt);

	log_msg_tty_va(msg_severity, fmt, fmt_args);

	va_end(fmt_args);

}


void log_close(void)
{


	if (log_dests & LOG_DEST_FILE) {
		log_file_close();
	}

	if (log_dests & LOG_DEST_SYSLOG) {
		log_syslog_close();
	}

}


void log_set_detail_level(const enum log_msg_severity msg_severity)
{


	log_severity = msg_severity;

}


enum log_msg_severity log_get_detail_level(void)
{


	return log_severity;

}



static void log_dest_file(FILE **fd,
			  const char *log_file,
			  void (*logf_open_fail)(const char *, void *),
			  void *logf_open_fail_ptr)
{


	if (log_file != NULL) {
		*fd = fopen(log_file, "w");
		if (*fd == NULL)  {
			log_dests &= ~LOG_DEST_FILE;
			if (logf_open_fail != NULL) {
				(*logf_open_fail)(log_file, logf_open_fail_ptr);
			}
		}
	}

}


static int log_syslog_facil(const enum log_syslog_facility facil)
{

	switch (facil) {
	case LOG_SYSLOG_KERN:
		return LOG_KERN;
	case LOG_SYSLOG_USER:
		return LOG_USER;
	case LOG_SYSLOG_MAIL:
		return LOG_MAIL;
	case LOG_SYSLOG_DAEMON:
		return LOG_DAEMON;
	case LOG_SYSLOG_AUTH:
		return LOG_AUTH;
	case LOG_SYSLOG_SYSLOG:
		return LOG_SYSLOG;
	case LOG_SYSLOG_LPR:
		return LOG_LPR;
	case LOG_SYSLOG_NEWS:
		return LOG_NEWS;
	case LOG_SYSLOG_UUCP:
		return LOG_UUCP;
	case LOG_SYSLOG_CRON:
		return LOG_CRON;
	case LOG_SYSLOG_AUTHPRIV:
		return LOG_AUTHPRIV;
	case LOG_SYSLOG_FTP:
		return LOG_FTP;
	case LOG_SYSLOG_LOCAL0:
		return LOG_LOCAL0;
	case LOG_SYSLOG_LOCAL1:
		return LOG_LOCAL1;
	case LOG_SYSLOG_LOCAL2:
		return LOG_LOCAL2;
	case LOG_SYSLOG_LOCAL3:
		return LOG_LOCAL3;
	case LOG_SYSLOG_LOCAL4:
		return LOG_LOCAL4;
	case LOG_SYSLOG_LOCAL5:
		return LOG_LOCAL5;
	case LOG_SYSLOG_LOCAL6:
		return LOG_LOCAL6;
	case LOG_SYSLOG_LOCAL7:
		return LOG_LOCAL7;
	default:
		return LOG_USER;
	}
}


static void log_dest_syslog(const char *syslog_ident,
			    const enum log_syslog_facility facil)
{


	openlog(syslog_ident, LOG_NDELAY, log_syslog_facil(facil));

}


static void log_dest_local_func(void (*log_msg_local_func)
				       (const enum log_msg_severity,
					const char *,
				        va_list))
{


	if (log_msg_local_func != NULL) {
		log_msg_local_function = log_msg_local_func;
	} else {
		log_dests &= ~LOG_DEST_LOCAL_FUNC;
	}

}


void log_msg_va(const enum log_msg_severity msg_severity,
		const char *fmt,
		va_list fmt_args)
{
	va_list orig_fmt_args;


	if (msg_severity <= log_severity) {

		va_copy(orig_fmt_args, fmt_args);

		if (log_dests & LOG_DEST_FILE) {
			log_msg_file_va(fmt, fmt_args);
			va_copy(fmt_args, orig_fmt_args);
		}

		if (log_dests & LOG_DEST_SYSLOG) {
			log_msg_syslog_va(msg_severity, fmt, fmt_args);	
			va_copy(fmt_args, orig_fmt_args);
		}

		if ((log_dests & LOG_DEST_TTY) ||
		    (log_dests & LOG_DEST_TTY_STDOUT)) {
			log_msg_tty_va(msg_severity, fmt, fmt_args);
			va_copy(fmt_args, orig_fmt_args);
		}

		if (log_dests & LOG_DEST_LOCAL_FUNC) {
			(*log_msg_local_function)(msg_severity, fmt, fmt_args);
			va_copy(fmt_args, orig_fmt_args);
		}

	}

}


static void log_msg_file_va(const char *fmt,
			    va_list fmt_args)
{


	if (log_file_fd != NULL) {
		vfprintf(log_file_fd, fmt, fmt_args);
	}

}


static int log_msg_sev_to_sysl_prio(const enum log_msg_severity msg_severity)
{


	switch (msg_severity) {
	case LOG_SEV_EMERG:
		return LOG_EMERG;
	case LOG_SEV_ALERT:
		return LOG_ALERT;
	case LOG_SEV_CRIT:
		return LOG_CRIT;
	case LOG_SEV_ERR:
		return LOG_ERR;
	case LOG_SEV_WARNING:
		return LOG_WARNING;
	case LOG_SEV_NOTICE:
		return LOG_NOTICE;
	case LOG_SEV_INFO:
		return LOG_INFO;
	default:
		return LOG_DEBUG;
	}

}


static void log_msg_syslog_va(const enum log_msg_severity msg_severity,
			      const char *fmt,
			      va_list fmt_args)
{
	static char syslog_str[SYSLOG_VSNPRINTF_BUF_SZ];
	static unsigned int i = 0;
	char tmp_str[SYSLOG_VSNPRINTF_BUF_SZ];
	char *nl_ptr;
	char *remain_ln_ptr;
	unsigned int tmp_str_sz;


	i += vsnprintf(&syslog_str[i], SYSLOG_VSNPRINTF_BUF_SZ - i, fmt,
			  fmt_args);

	/* always make syslog_str[] safe for processing below */
	syslog_str[SYSLOG_VSNPRINTF_BUF_SZ - 1] = '\0';

	nl_ptr = strrchr(syslog_str, '\n');
	if (nl_ptr != NULL) {
		tmp_str_sz = nl_ptr - syslog_str;
		memcpy(tmp_str, syslog_str, tmp_str_sz);			
		tmp_str[tmp_str_sz] = '\0';
		syslog(log_msg_sev_to_sysl_prio(msg_severity), "%s",
		       tmp_str);	

		if ((*(nl_ptr + 1)) != '\0') {
			remain_ln_ptr = strdup(nl_ptr + 1);
			strcpy(syslog_str, remain_ln_ptr);
			free(remain_ln_ptr);
			i = strlen(syslog_str);
		} else {
			i = 0;
		}
	} else {
		if (i >= (SYSLOG_VSNPRINTF_BUF_SZ - 1)) {
			syslog(log_msg_sev_to_sysl_prio(msg_severity), "%s",
				tmp_str);	
			i = 0;
		}
	}
}


static void log_msg_tty_va(const enum log_msg_severity msg_severity,
		           const char *fmt,
			   va_list fmt_args)
{



	if (log_dests & LOG_DEST_TTY_STDOUT) {
		vprintf(fmt, fmt_args);
	} else if (log_dests & LOG_DEST_TTY) {
		if (msg_severity <= LOG_SEV_WARNING) {
			vfprintf(stderr, fmt, fmt_args);
		} else {
			vfprintf(stdout, fmt, fmt_args);
		}
	}

}


static void log_file_close(void)
{


	if (log_file_fd != NULL) {
		fclose(log_file_fd);
	}

}


static void log_syslog_close(void)
{


	closelog();

}
