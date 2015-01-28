#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "log.h"
#include "const.h"

static const char *dbm_get_log_level(int level);
static int dbm_check_log(const struct tm *cur_tm);


static char             _dbm_log_path[MYDBM_FILE_PATH_LEN] = {0};
static char             _dbm_log_path_fmt[MYDBM_FILE_PATH_LEN] = {0};
static int              _dbm_log_level = MYDBM_LOG_ERR; 
static FILE             *_dbm_log_fp = NULL;
static pthread_mutex_t  _dbm_log_mutex = PTHREAD_MUTEX_INITIALIZER;


void dbm_set_log_level(int level)
{
    if (level >= MYDBM_LOG_DEBUG || level <= MYDBM_LOG_FATAL_ERR)
    {
        _dbm_log_level = level;
    }
}

void dbm_set_log_fmt(const char *log_path_fmt)
{
    if (log_path_fmt != NULL && log_path_fmt[0] != '\0')
    {
        snprintf(_dbm_log_path_fmt, sizeof(_dbm_log_path_fmt), "%s", log_path_fmt);
    }
}

void dbm_log_init(const char *log_path_fmt, int level)
{
    dbm_set_log_fmt(log_path_fmt);
    dbm_set_log_level(level);
}

void dbm_close_log_stream()
{
    if (_dbm_log_fp != NULL)
    {
        fclose(_dbm_log_fp);
        _dbm_log_fp = NULL;
    }
}

void dbm_destroy_log()
{
    dbm_close_log_stream();
    pthread_mutex_destroy(&_dbm_log_mutex);
}

void dbm_print_log(const char* file_path, int line_no, int log_level, const char* format, ...)
{
    if (log_level < _dbm_log_level || NULL == file_path || NULL == format)
        return;

    int n = 0;
    time_t cur;
    int ret = MYDBM_OK;
    struct tm cur_tm = {0};
    const char* file_name = NULL;
    char log_buf[MYDBM_MAX_BUF_LEN] = {0};

    file_name = strrchr(file_path, '/');
    file_name = (file_name == NULL) ? file_path : (file_name + 1);

    pthread_mutex_lock(&_dbm_log_mutex);

    time(&cur);
    localtime_r(&cur, &cur_tm);

    ret = dbm_check_log(&cur_tm);
    if (MYDBM_OK == ret)
    {
        n = snprintf(log_buf, sizeof(log_buf),
                     "%4d/%02d/%02d %02d:%02d:%02d - [pid: %d][%s][%s %d] - ",
                     cur_tm.tm_year + 1900,
                     cur_tm.tm_mon + 1,
                     cur_tm.tm_mday,
                     cur_tm.tm_hour,
                     cur_tm.tm_min,
                     cur_tm.tm_sec,
                     getpid(),
                     dbm_get_log_level(log_level),
                     file_name,
                     line_no);

        va_list arg_ptr;
        va_start(arg_ptr, format);
        vsnprintf(log_buf + n, sizeof(log_buf) - n - 2, format, arg_ptr);
        va_end(arg_ptr);
        strcat(log_buf, "\n");

        fwrite(log_buf, sizeof(char), strlen(log_buf), _dbm_log_fp);
        fflush(_dbm_log_fp);
    }

    pthread_mutex_unlock(&_dbm_log_mutex);
}

static int dbm_check_log(const struct tm *cur_tm)
{
    int ret = MYDBM_OK;
    size_t len = 0;
    char log_path[MYDBM_FILE_PATH_LEN] = {0};

    if ('\0' == _dbm_log_path_fmt[0])
    {
        _dbm_log_fp = _dbm_log_fp == NULL ? stderr : _dbm_log_fp;
        return MYDBM_OK;
    }

    len = strftime(log_path, sizeof(log_path), _dbm_log_path_fmt, cur_tm); 

    if (strncmp(log_path, _dbm_log_path, len) != 0)
    {
        if (_dbm_log_fp != NULL && _dbm_log_fp != stderr)
        {
            fclose(_dbm_log_fp);
            _dbm_log_fp = NULL;
        }

        snprintf(_dbm_log_path, sizeof(_dbm_log_path), "%s", log_path);
        _dbm_log_fp = fopen(_dbm_log_path, "a");
        ret = (_dbm_log_fp == NULL) ? MYDBM_ERR_OPEN : ret;
    }
    else
    {
        if (NULL == _dbm_log_fp)
        {
            _dbm_log_fp = fopen(_dbm_log_path, "a");
            ret = (_dbm_log_fp == NULL) ? MYDBM_ERR_OPEN : ret;
        }
    }

    return ret;
}

static const char *dbm_get_log_level(int level)
{
    switch (level)
    {
    case MYDBM_LOG_FATAL_ERR:
        return "FATAL ERROR";
    case MYDBM_LOG_ERR:
        return "ERROR";
    case MYDBM_LOG_WARN:
        return "WARNING";
    case MYDBM_LOG_INFO:
        return "INFO";
    case MYDBM_LOG_TRACE:
        return "TRACE";
    case MYDBM_LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}
