#ifndef PHP_YDB_H
#define PHP_YDB_H

#include "main/SAPI.h"


#define YDB_NAME       "ydb"
#define YDB_VERSION    "0.01"
#define YDB_AUTHOR     "lizhonghua"
#define YDB_COPYRIGHT  "Copyright (c) 2002-2013 by lizhonghua"
#define YDB_COPYRIGHT_SHORT "Copyright (c) 2002-2013"
#define YDB_URL        "http://ydb.org"
#define YDB_URL_FAQ    "http://ydb.org/docs/faq#api"


#ifdef PHP2_5

#define MAKE_COPY_ZVAL(ppzv, pzv) \
		*(pzv) = **(ppzv);            \
zval_copy_ctor((pzv));        \
INIT_PZVAL((pzv));

#endif


ZEND_BEGIN_MODULE_GLOBALS(ydb)
		char* ydb_cur_classname;
		char* ydb_cur_funname; 

		zend_op_array  *  ydb_cur_op_array;
		zend_op_array  *  ydb_dst_op_array;

		char* input_classname;
		char* input_funname;
		char* input_varname;

		HashTable* ydb_varn;

		int ydb_shouldreturn;

		int  is_post;
		char* input_post_classname;
		char* input_post_funname;
		char* input_post_varname;

		char* parent_classname;
		char* parent_funname;

		zend_op_array* timer_op_array;

		struct timeval start;
		struct timeval end;

		int usetimer;

		int ydb_timer;

		HashTable* timer_fun;
		char* timer_key;

		int uselog;

ZEND_END_MODULE_GLOBALS(ydb)

ZEND_DECLARE_MODULE_GLOBALS(ydb)

#ifdef ZTS
#define YG(v) TSRMG(ydb_globals_id, zend_ydb_globals *, v)
#else
#define YG(v) (ydb_globals.v)
#endif

#endif


