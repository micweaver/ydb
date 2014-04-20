#ifndef PHP_YDB_H
#define PHP_YDB_H

#include "php.h"
#include "zend_extensions.h"

#define YDB_NAME       "ydb"
#define YDB_VERSION    "0.5"
#define YDB_AUTHOR     "lizhonghua"
#define YDB_COPYRIGHT  "Copyright (c) 2013-2014 by lizhonghua"
#define YDB_COPYRIGHT_SHORT "Copyright (c) 2013-2014"
#define YDB_URL        "http://ydb.org"
#define YDB_URL_FAQ    "http://ydb.org/docs/faq#api"

#if(PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 3)

#define MAKE_COPY_ZVAL(ppzv, pzv) \
		*(pzv) = **(ppzv);            \
zval_copy_ctor((pzv));        \
INIT_PZVAL((pzv));

#endif

#define true 1
#define false 0

#define INIT_HASH_SIZE 15

#define MAX_PARAMS_LEN 100
#define MAX_NEWVAR_LEN 1000
#define MAX_KEY_LEN 100
#define MAX_URL_LEN 1000

#define DIR_MAX_LEN 200

#define YDB_LOG_FILE "/log/ydbinfo.log"

#define CURL_RES_FIRST_LINE "HTTP/1.1 200 OK"
#define CURL_RES_FLAG_HEADER "YDB-YDB : 1"

#define MAX_HEAD_LINE_LEN 200

ZEND_BEGIN_MODULE_GLOBALS (ydb)

        int is_look_variable;			//是否查看变量值
        int is_timer;					//是否计时
        int is_net_timer;				//是否对网络请求进行计时
        char *ydb_cur_classname;		//当前执行位于的类
        char *ydb_cur_funname;			//当前执行位于的函数

        HashTable *dst_symbol_table;
        zend_op_array *ydb_cur_op_array;	//当前执行函数的op_array
        zend_op_array *ydb_dst_op_array;	// 要执行观察的函数op_array

        char *input_classname;			//输入的类名
        char *input_funname;			//输入的函数名
        char *input_varname;			//输入的变量名
        char *input_new_var;			//输入的新变量值
        HashTable *ydb_varn;			//存放变量值

        int ydb_shouldreturn;

        int is_post;					//是否是post请求
        char input_post_classname[MAX_PARAMS_LEN];
        char input_post_funname[MAX_PARAMS_LEN];
        char input_post_varname[MAX_PARAMS_LEN];
        char input_post_newvar[MAX_NEWVAR_LEN];

        char *parent_classname;
        char *parent_funname;

        zend_op_array *timer_op_array;

        struct timeval start, start_sum;	//计时开始时间
        struct timeval end, end_sum;	//计时结束时间

        int usetimer;
        int is_sum_time_begin;			//是否开始总计时
        int ydb_timer;

        HashTable *timer_fun;			//存放函数计时值

        int uselog;						//是否使用日志记录结果

        int is_get_debug_params;		//是否已经获得了调试参数

        HashTable *input_vars;			//输入的变量数组
        int is_set_new_var;				// 是否已覆盖了变量

        void (*orig_curl_init) (INTERNAL_FUNCTION_PARAMETERS);
        void (*orig_curl_setopt) (INTERNAL_FUNCTION_PARAMETERS);
        void (*orig_curl_exec) (INTERNAL_FUNCTION_PARAMETERS);

        HashTable *key_count;			// 相同key的计数

        int is_replace_new_fun;			//是否已替换了新函数handler
        int is_get_remote_res;			//是否从被调用端获得了结果

        char *old_curl_url;
        char new_curl_url[MAX_URL_LEN];

        char *curl_res;					// curl返回的结果,包括头部

        int is_send_header;				//是否已经发送了ydb头部

        int is_dump_code;				//是否打印执行代码
        int is_get_code_info;			//是否获得了执行代码段信息
        char *file_name;				//文件名
        int line_start;					//起始行
        int line_end;					//结束行

        int depth;						//远程访问深度

ZEND_END_MODULE_GLOBALS (ydb)

	ZEND_DECLARE_MODULE_GLOBALS (ydb)
#ifdef ZTS
#define YG(v) TSRMG(ydb_globals_id, zend_ydb_globals *, v)
#else
#define YG(v) (ydb_globals.v)
#endif
#endif
