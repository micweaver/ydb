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


#ifdef PHP2_5

#define MAKE_COPY_ZVAL(ppzv, pzv) \
		*(pzv) = **(ppzv);            \
zval_copy_ctor((pzv));        \
INIT_PZVAL((pzv));

#endif

#define true 1
#define false 0

#define INIT_HASH_SIZE 15

#define MAX_PARAMS_LEN 100

#define DIR_MAX_LEN 200

#define YDB_LOG_FILE "/log/ydbinfo.log"

ZEND_BEGIN_MODULE_GLOBALS(ydb)

		int is_look_variable; //是否查看变量值
		int is_timer; //是否计时
		char* ydb_cur_classname; //当前执行位于的类
		char* ydb_cur_funname; //当前执行位于的函数

		HashTable* dst_symbol_table;
		zend_op_array  *  ydb_cur_op_array; //当前执行函数的op_array
		zend_op_array  *  ydb_dst_op_array; // 要执行观察的函数op_array

		char* input_classname; //输入的类名
		char* input_funname; //输入的函数名
		char* input_varname; //输入的变量名

		HashTable* ydb_varn; //存放变量值

		int ydb_shouldreturn;

		int   is_post; //是否是post请求
		char  input_post_classname[MAX_PARAMS_LEN];
		char  input_post_funname[MAX_PARAMS_LEN];
		char  input_post_varname[MAX_PARAMS_LEN];

		char* parent_classname;
		char* parent_funname;

		zend_op_array* timer_op_array;

		struct timeval start,start_sum; //计时开始时间
		struct timeval end,end_sum; //计时结束时间

		int usetimer;
		int is_sum_time_begin; //是否开始总计时
		int ydb_timer;

		HashTable* timer_fun; //存放函数计时值

		int uselog; //是否使用日志记录结果

		int is_get_debug_params; //是否已经获得了调试参数

ZEND_END_MODULE_GLOBALS(ydb)

ZEND_DECLARE_MODULE_GLOBALS(ydb)

#ifdef ZTS
#define YG(v) TSRMG(ydb_globals_id, zend_ydb_globals *, v)
#else
#define YG(v) (ydb_globals.v)
#endif

#endif

