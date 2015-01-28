#ifndef __LOG_H__
#define __LOG_H__

#define MYDBM_LOG_FATAL_ERR         5
#define MYDBM_LOG_ERR               4
#define MYDBM_LOG_WARN              3
#define MYDBM_LOG_INFO              2
#define MYDBM_LOG_TRACE             1
#define MYDBM_LOG_DEBUG             0

#define LOG_FATAL_ERR(format, ...) dbm_print_log(__FILE__, __LINE__, MYDBM_LOG_FATAL_ERR, format, ## __VA_ARGS__)
#define LOG_ERR(format, ...)       dbm_print_log(__FILE__, __LINE__, MYDBM_LOG_ERR, format, ## __VA_ARGS__)
#define LOG_WARN(format, ...)      dbm_print_log(__FILE__, __LINE__, MYDBM_LOG_WARNING, format, ## __VA_ARGS__)
#define LOG_INFO(format, ...)      dbm_print_log(__FILE__, __LINE__, MYDBM_LOG_INFO, format, ## __VA_ARGS__)
#define LOG_TRACE(format, ...)     dbm_print_log(__FILE__, __LINE__, MYDBM_LOG_TRACE, format, ## __VA_ARGS__)
#define LOG_DEBUG(format, ...)     dbm_print_log(__FILE__, __LINE__, MYDBM_LOG_DEBUG, format, ## __VA_ARGS__)

#define MYDBM_FUNC_IN() LOG_INFO("[ %s ] in ...", __FUNCTION__)
#define MYDBM_FUNC_OUT() LOG_INFO("[ %s ] out ...", __FUNCTION__)


void dbm_log_init(const char *log_path_fmt, int level);
void dbm_destroy_log();
void dbm_set_log_fmt(const char *log_path_fmt);
void dbm_set_log_level(int level);
void dbm_close_log_stream();
void dbm_print_log(const char* file_path, int line_no, int log_level, const char* format, ...);

#endif
