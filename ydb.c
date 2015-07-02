#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "ydb.h"

void store_function_variable ();
void destroy_str (char *str);

static int replace_curl_fun ();
static int recover_curl_fun ();
static int dump_code ();

void ydb_curl_init (INTERNAL_FUNCTION_PARAMETERS);
void ydb_curl_setopt (INTERNAL_FUNCTION_PARAMETERS);
void ydb_curl_exec (INTERNAL_FUNCTION_PARAMETERS);

#define TEST_LOG_FILE "ydbtest.log"

#define YDB_LOG(fmt,arg...)   do {\
    FILE* fh = fopen(TEST_LOG_FILE,"a");\
    if(fh != NULL) {\
        fprintf(fh,"[%s:%d]" fmt "\n",__FILE__,__LINE__,##arg);\
        fclose(fh);\
    }\
} while(0)

#define INFO_USER(fmt,arg...)   do {\
    if(YG(depth) <= 1) {\
        zend_printf(fmt,##arg);\
    }\
} while(0)

ZEND_DLEXPORT int ydb_zend_startup (zend_extension * extension)
{

#if(PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 3)
	CG (compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
#else
	CG (extended_info) = 1;
#endif

	return SUCCESS;
}

ZEND_DLEXPORT void ydb_zend_shutdown (zend_extension * extension)
{
	/* Do nothing. */
}

static void init_curl ()
{

	YG (old_curl_url) = NULL;
	YG (new_curl_url)[0] = 0;

	YG (is_replace_new_fun) = false;
	YG (is_get_remote_res) = false;

	YG (is_send_header) = false;
	YG (is_net_timer) = false;

	YG (orig_curl_init) = NULL;
	YG (orig_curl_setopt) = NULL;
	YG (orig_curl_exec) = NULL;
}

static void init_dump_code ()
{

	YG (is_dump_code) = false;
	YG (is_get_code_info) = false;
	YG (file_name) = NULL;
	YG (line_start) = 0;
	YG (line_end) = 0;

}

ZEND_DLEXPORT void ydb_zend_activate (void)
{

	YG (is_look_variable) = false;
	YG (is_timer) = false;
	YG (is_get_debug_params) = false;
	YG (is_sum_time_begin) = false;

	YG (dst_symbol_table) = NULL;

	YG (input_classname) = NULL;
	YG (input_funname) = NULL;
	YG (input_varname) = NULL;

	YG (input_post_classname)[0] = 0;
	YG (input_post_funname)[0] = 0;
	YG (input_post_varname)[0] = 0;

	YG (ydb_shouldreturn) = false;
	YG (ydb_cur_op_array) = NULL;
	YG (ydb_dst_op_array) = NULL;

	YG (is_post) = false;

	YG (parent_classname) = NULL;
	YG (parent_funname) = NULL;

	YG (usetimer) = false;

	YG (ydb_cur_classname) = NULL;
	YG (ydb_cur_funname) = NULL;
	YG (timer_op_array) = NULL;
	YG (timer_fun) = NULL;
	YG (ydb_varn) = NULL;

	YG (uselog) = false;

	YG (is_set_new_var) = false;

	YG (depth) = 1;

	init_curl ();
	init_dump_code ();

	//     zend_printf("%s\n","ydb activate");

	ALLOC_HASHTABLE (YG (ydb_varn));
	zend_hash_init (YG (ydb_varn), INIT_HASH_SIZE, NULL, ZVAL_PTR_DTOR, 0);

	ALLOC_HASHTABLE (YG (timer_fun));
	zend_hash_init (YG (timer_fun), INIT_HASH_SIZE, NULL, NULL, 0);

	ALLOC_HASHTABLE (YG (input_vars));
	zend_hash_init (YG (input_vars), INIT_HASH_SIZE, NULL, NULL, 0);

	ALLOC_HASHTABLE (YG (key_count));
	zend_hash_init (YG (key_count), INIT_HASH_SIZE, NULL, NULL, 0);

	replace_curl_fun ();

}

void destroy_str (char *str)
{

	//  if(str) efree(str);
}

static void my_print_hash (HashTable * ht, int type)
{

	zval l_res;
	INIT_ZVAL (l_res);
	l_res.value.ht = ht;
	Z_TYPE (l_res) = IS_ARRAY;

	if (type) {
		zval *tmp = &l_res;
		php_var_dump (&tmp, 1);
	}
	else {
		zend_print_zval_r (&l_res, 0);
	}
}

static void my_print_zval (zval * zv)
{
	zend_print_zval_r (zv, 0);
}

static int ydb_timer_compare (const void *a, const void *b)
{

	Bucket *f = *((Bucket **) a);
	Bucket *s = *((Bucket **) b);

	int *ia = (int *) f->pData;
	int *ib = (int *) s->pData;

	if (*ia > *ib)
		return -1;
	else if (*ia < *ib)
		return 1;
	else
		return 0;
}

static int start_ydb ()
{

	return YG (is_look_variable) || YG (is_timer) || YG (is_net_timer);
}

static int add_ydb_header ()
{

	int status;
	zval function_name, retval;
	INIT_ZVAL (function_name);
	INIT_ZVAL (retval);

	zval param;
	zval *params = &param;
	INIT_PZVAL (params);

	ZVAL_STRING (params, CURL_RES_FLAG_HEADER, 0);
	ZVAL_STRING (&function_name, "header", 0);

	status = call_user_function (CG (function_table), NULL, &function_name, &retval, 1, &params);

	if (status != SUCCESS) {

	}

}

static void request_end ()
{

	zend_hash_destroy (YG (ydb_varn));
	zend_hash_destroy (YG (timer_fun));
	zend_hash_destroy (YG (input_vars));
	zend_hash_destroy (YG (key_count));
	FREE_HASHTABLE (YG (ydb_varn));
	FREE_HASHTABLE (YG (timer_fun));
	FREE_HASHTABLE (YG (input_vars));
	FREE_HASHTABLE (YG (key_count));

	recover_curl_fun ();

}

ZEND_DLEXPORT void ydb_zend_deactivate (void)
{

	if (!start_ydb () || YG (is_get_remote_res)) {
		request_end ();
		return;
	}

	php_end_ob_buffer (0, 0 TSRMLS_CC);

	int i;
	char *skey;
	ulong ikey;

	char cwdydb[DIR_MAX_LEN] = { 0 };
	zval *message;
	php_stream *stream = NULL;

	if (YG (uselog)) {
		getcwd (cwdydb, sizeof (cwdydb));
		strcat (cwdydb, YDB_LOG_FILE);
		stream = php_stream_open_wrapper (cwdydb, "a", IGNORE_URL_WIN | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL);
		//     zend_printf("cwd:%s\n",cwdydb);
		ALLOC_ZVAL (message);
		php_start_ob_buffer (NULL, 0, 1 TSRMLS_CC);
	}

	if (YG (is_look_variable) && YG (ydb_varn)) {	//打印变量值

		zval **data;
		zend_hash_internal_pointer_reset (YG (ydb_varn));
		int count = zend_hash_num_elements (YG (ydb_varn));

		if (count <= 0) {
			if (YG (input_varname)) {
				INFO_USER ("variable value %s::%s::%s not exist, you may supply the wrong param", YG (input_classname), YG (input_funname), YG (input_varname));
				goto EXIT;
			}
			else {
				INFO_USER ("variable value in %s::%s not exist,you  may supply the wrong param", YG (input_classname), YG (input_funname));
				goto EXIT;
			}
		}

		zend_printf ("=========the variable list================\n");

		for (i = 0; i < count; i++) {
			zend_hash_get_current_data (YG (ydb_varn), (void **) &data);
			zend_hash_get_current_key (YG (ydb_varn), &skey, &ikey, 0);
			zend_hash_move_forward (YG (ydb_varn));
			zend_printf ("$%s:", skey);
			zend_print_zval_r (*data, 0);
			zend_printf ("\n");
		}

		if (YG (is_dump_code)) {
			dump_code ();
		}

		if (YG (uselog)) {
			php_ob_get_buffer (message TSRMLS_CC);
			if (stream != NULL) {
				php_stream_write (stream, Z_STRVAL_P (message), Z_STRLEN_P (message));
			}
		}

		if (!YG (uselog)) {
			goto EXIT;
		}

	}
	else if (YG (is_timer) && YG (timer_fun)) {	//打印耗时排行
		skey = 0;
		int *data;
		zend_hash_internal_pointer_reset (YG (timer_fun));
		int count = zend_hash_num_elements (YG (timer_fun));

		if (count <= 0) {
			INFO_USER ("no function time in %s::%s, you may supply the wrong param", YG (input_classname), YG (input_funname));
			goto EXIT;
		}
		if (count > 0) {
			zend_hash_sort (YG (timer_fun), zend_qsort, ydb_timer_compare, 0 TSRMLS_CC);
			for (i = 0; i < count; i++) {
				zend_hash_get_current_data (YG (timer_fun), (void **) &data);
				zend_hash_get_current_key (YG (timer_fun), &skey, &ikey, 0);
				zend_hash_move_forward (YG (timer_fun));
				zend_printf ("%s=>", skey);
				zend_printf ("%.3f ms", (float) (*data) / 1000);
				zend_printf ("\n");

			}

			if (YG (is_dump_code)) {
				dump_code ();
			}

			if (YG (uselog)) {
				php_ob_get_buffer (message TSRMLS_CC);
				if (stream != NULL) {
					php_stream_write (stream, Z_STRVAL_P (message), Z_STRLEN_P (message));
				}
			}
		}
		if (!YG (uselog)) {
			goto EXIT;
		}
	}
	else if (YG (is_net_timer) && YG (timer_fun)) {	//打印网络请求耗时排行

		zend_printf ("=========the num of call times===============\n");

		skey = 0;
		int *data;
		zend_hash_internal_pointer_reset (YG (key_count));
		int count = zend_hash_num_elements (YG (key_count));

		if (count <= 0) {
			zend_printf ("no net request\n");
			goto EXIT;
		}
		if (count > 0) {
			zend_hash_sort (YG (key_count), zend_qsort, ydb_timer_compare, 0 TSRMLS_CC);
			for (i = 0; i < count; i++) {
				zend_hash_get_current_data (YG (key_count), (void **) &data);
				zend_hash_get_current_key (YG (key_count), &skey, &ikey, 0);
				zend_hash_move_forward (YG (key_count));
				zend_printf ("%s:%d\n", skey, *data);
			}

		}

		zend_printf ("=========the cost time rank list=============\n");

		zend_hash_internal_pointer_reset (YG (timer_fun));
		count = zend_hash_num_elements (YG (timer_fun));

		if (count <= 0) {
			zend_printf ("no net request\n");
			goto EXIT;
		}
		if (count > 0) {
			zend_hash_sort (YG (timer_fun), zend_qsort, ydb_timer_compare, 0 TSRMLS_CC);
			for (i = 0; i < count; i++) {
				zend_hash_get_current_data (YG (timer_fun), (void **) &data);
				zend_hash_get_current_key (YG (timer_fun), &skey, &ikey, 0);
				zend_hash_move_forward (YG (timer_fun));
				zend_printf ("%s=>", skey);
				zend_printf ("%.3f ms", (float) (*data) / 1000);
				zend_printf ("\n");

			}
			if (YG (uselog)) {
				php_ob_get_buffer (message TSRMLS_CC);
				if (stream != NULL) {
					php_stream_write (stream, Z_STRVAL_P (message), Z_STRLEN_P (message));
				}
			}
		}

	}

	if (YG (uselog)) {
		if (stream != NULL) {
			php_stream_close (stream);
		}
		php_end_ob_buffer (0, 0 TSRMLS_CC);
	}

  EXIT:

	request_end ();

}

static int dump_code ()
{

	if (!YG (is_dump_code) || !YG (file_name))
		return -1;

	zend_printf ("=========the code block of %s::%s=============\n", YG (input_classname), YG (input_funname));

#define LINE_LEN 3

	int cur_lineno = 0;
	char line_buf[LINE_LEN] = { 0 };

	FILE *fh = fopen (YG (file_name), "r");

	if (fh == NULL)
		return -1;

	int len;
	while (fgets (line_buf, LINE_LEN, fh) != NULL) {

		if (cur_lineno + 1 >= YG (line_start) && cur_lineno + 1 <= YG (line_end)) {
			zend_printf ("%s", line_buf);
		}

		if (cur_lineno + 1 > YG (line_end))
			break;

		len = strlen (line_buf);
		if (line_buf[len - 1] == '\n')
			cur_lineno++;
	}

	fclose (fh);
	efree (YG (file_name));
	return 0;

}

static int parse_new_var ()
{

	char *new_vars_str = estrdup (YG (input_new_var));
	char *src_str = new_vars_str;

	// zend_printf("new_vars_str:%s\n",new_vars_str);

	int len = strlen (new_vars_str);

	if (new_vars_str[0] == '{' && new_vars_str[len - 1] == '}') {
		new_vars_str[len - 1] = 0;
		new_vars_str++;

		int status;
		zval function_name, retval;

		INIT_ZVAL (function_name);
		INIT_ZVAL (retval);

		zval param1, param2;
		zval *params[2];

		params[0] = &param1;
		INIT_PZVAL (params[0]);
		params[1] = &param2;
		INIT_PZVAL (params[1]);

		ZVAL_STRING (params[0], ":", 0);
		ZVAL_STRING (params[1], new_vars_str, 0);

		ZVAL_STRING (&function_name, "explode", 0);
		status = call_user_function (CG (function_table), NULL, &function_name, &retval, 2, params);

		if (status != SUCCESS && Z_TYPE (retval) != IS_ARRAY) {
			zend_printf ("call explode fail");
			return -1;
		}

		//     my_print_hash(Z_ARRVAL(retval), 1);

		HashTable *h_vars = Z_ARRVAL (retval);
		zval **v_item;
		zval v_retval;
		INIT_ZVAL (v_retval);

		ZVAL_STRING (params[0], "=", 0);

		zend_hash_internal_pointer_reset (h_vars);
		while (zend_hash_has_more_elements (h_vars) == SUCCESS) {

			zend_hash_get_current_data (h_vars, (void **) &v_item);
			// zend_printf("v_item:%s\n",Z_STRVAL_PP(v_item));
			ZVAL_STRING (params[1], Z_STRVAL_PP (v_item), 0);
			status = call_user_function (CG (function_table), NULL, &function_name, &v_retval, 2, params);

			if (status != SUCCESS || Z_TYPE (v_retval) != IS_ARRAY) {
				zend_printf ("new variable \"%s\" parse fail\n", Z_STRVAL_PP (v_item));
				zend_hash_move_forward (h_vars);
				continue;
			}
			if (zend_hash_num_elements (Z_ARRVAL (v_retval)) != 2) {	//有些变态的数组以"="作为键，这样也解释为无效
				zend_printf ("new variable \"%s\" invalid\n", Z_STRVAL_PP (v_item));
				zend_hash_move_forward (h_vars);
				continue;
			}

			zval **s_key;
			zval **s_val;
			zend_hash_index_find (Z_ARRVAL (v_retval), 0, (void **) &s_key);
			zend_hash_index_find (Z_ARRVAL (v_retval), 1, (void **) &s_val);

			zend_hash_add (YG (input_vars), Z_STRVAL_PP (s_key), strlen (Z_STRVAL_PP (s_key)) + 1, s_val, sizeof (zval *), NULL);	//5.3 下 key = arr[cc] 时会崩溃,待查原因
			zend_hash_move_forward (h_vars);

		}

	}

	else {
		return -1;
	}

	// zend_printf("input_vars:\n");
	//    my_print_hash(YG(input_vars),1);
	efree (src_str);
	return 0;

}

static int is_num (char *s)
{
	while (*s) {
		if (!isdigit (*s))
			return 0;
		s++;
	}
	return 1;
}

static void get_debug_params ()
{

	zval *arrParam;
	zval **pvalue;

	char *refer;
	int i;
	HashTable *ht;

	char *ckey = "c",			//类名
		*fkey = "f",			//函数名
		*vkey = "v",			//变量名
		*okey = "o",			//标示post请求
		*tkey = "t",			//标示进行计时操作
		*lkey = "l",			//标示将结果打印进日志
		*akey = "a",			//新的变量值
		*rkey = "r",			//网络请求耗时排行
		*skey = "s",			//打印源代码
		*dkey = "d";			//表示远程调用深度

	arrParam = PG (http_globals)[TRACK_VARS_GET];
	if (zend_hash_find (Z_ARRVAL_P (arrParam), okey, strlen (okey) + 1, (void **) &pvalue) == SUCCESS) {
		YG (is_post) = true;
	}

	if (!YG (is_post)) {		// get 参数
		if (zend_hash_find (Z_ARRVAL_P (arrParam), ckey, strlen (ckey) + 1, (void **) &pvalue) == SUCCESS) {
			YG (input_classname) = Z_STRVAL_PP (pvalue);
		}
		else {
			YG (input_classname) = NULL;
		}

		if (zend_hash_find (Z_ARRVAL_P (arrParam), fkey, strlen (fkey) + 1, (void **) &pvalue) == SUCCESS) {
			YG (input_funname) = Z_STRVAL_PP (pvalue);
		}
		else {
			YG (input_funname) = NULL;
		}

		if (zend_hash_find (Z_ARRVAL_P (arrParam), vkey, strlen (vkey) + 1, (void **) &pvalue) == SUCCESS) {
			YG (input_varname) = Z_STRVAL_PP (pvalue);
		}
		else {
			YG (input_varname) = NULL;
		}

		if (zend_hash_find (Z_ARRVAL_P (arrParam), akey, strlen (akey) + 1, (void **) &pvalue) == SUCCESS) {
			YG (input_new_var) = Z_STRVAL_PP (pvalue);
			parse_new_var ();
		}
		else {
			YG (input_new_var) = NULL;
		}

		if (zend_hash_find (Z_ARRVAL_P (arrParam), dkey, strlen (dkey) + 1, (void **) &pvalue) == SUCCESS) {

			char *dep = Z_STRVAL_PP (pvalue);
			if (dep && is_num (dep)) {
				YG (depth) = atoi (dep);
				YDB_LOG ("depth:%d", YG (depth));
			}
		}

	}

	//if(!YG(input_classname) || !YG(input_funname)){ // 这样前一个请求可能影响后一个
	if (YG (is_post)) {			// post 参数

		zend_is_auto_global ("_SERVER", sizeof ("_SERVER") - 1 TSRMLS_CC);
		if (PG (http_globals)[TRACK_VARS_SERVER]) {
			ht = Z_ARRVAL_P (PG (http_globals)[TRACK_VARS_SERVER]);
			if (zend_hash_find (ht, "HTTP_REFERER", sizeof ("HTTP_REFERER"), (void **) & pvalue) == SUCCESS) {

				refer = Z_STRVAL_PP (pvalue);
				// zend_printf("refer:%s\n",refer);
				int cur = 0;
				int j = 0;
				int k = 0;
				while (refer[j]) {

					if (cur != 6) {

						switch (refer[j]) {
						case 'c':	//class name
							if (refer[j + 1] == '=') {
								j += 2;
								cur = 1;
							}
							break;
						case 'f':	// function name
							if (refer[j + 1] == '=') {
								j += 2;
								cur = 2;
							}
							break;
						case 'v':	// variable name
							if (refer[j + 1] == '=') {
								j += 2;
								cur = 3;
							}
							break;
						case 'l':	// use log?
							if (refer[j + 1] == '=') {
								j += 2;
								cur = 4;
								YG (uselog) = true;
							}
							break;
						case 't':	// use timer?
							if (refer[j + 1] == '=') {
								j += 2;
								cur = 5;
								YG (is_timer) = true;
							}
							break;
						case 'a':	// new var 
							if (refer[j + 1] == '=' && refer[j + 2] == '{') {
								j += 2;
								cur = 6;
							}
							break;
						case 'r':	// time rank list
							if (refer[j + 1] == '=') {
								j += 2;
								cur = 7;
								YG (is_net_timer) = true;
							}
							break;
						case '&':
						case '=':
							cur = 0;
							k = 0;
							break;

						}
					}

					if (cur == 1) {
						YG (input_post_classname)[k++] = refer[j];
						if (refer[j + 1] == '&' || refer[j + 1] == 0) {
							YG (input_post_classname)[k] = 0;
						}
					}
					else if (cur == 2) {
						YG (input_post_funname)[k++] = refer[j];
						if (refer[j + 1] == '&' || refer[j + 1] == 0) {
							YG (input_post_funname)[k] = 0;
						}
					}
					else if (cur == 3) {
						YG (input_post_varname)[k++] = refer[j];
						if (refer[j + 1] == '&' || refer[j + 1] == 0) {
							YG (input_post_varname)[k] = 0;
						}
					}
					else if (cur == 6) {
						YG (input_post_newvar)[k++] = refer[j];
						if (refer[j + 1] == '&' || refer[j + 1] == 0) {
							YG (input_post_newvar)[k] = 0;
						}

						if (refer[j] == '}')
							cur = 0;

					}
					j++;
				}

			}

			if (YG (input_post_classname)[0]) {
				YG (input_classname) = YG (input_post_classname);
			}

			if (YG (input_post_funname)[0]) {
				YG (input_funname) = YG (input_post_funname);
			}

			if (YG (input_post_varname)[0]) {
				YG (input_varname) = YG (input_post_varname);
			}

			if (YG (input_post_newvar)[0]) {
				YG (input_new_var) = YG (input_post_newvar);
				parse_new_var ();
			}

			// zend_printf("pc:%s,pf:%s,pv:%s,pnv:%s,pr:%d\n",YG(input_classname), YG(input_funname),YG(input_varname),YG(input_new_var),YG(is_net_timer));

		}
	}

	if (zend_hash_find (Z_ARRVAL_P (arrParam), rkey, strlen (rkey) + 1, (void **) &pvalue) == SUCCESS) {
		YG (is_net_timer) = true;
		YG (is_timer) = false;
		YG (is_look_variable) = false;
		YG (ydb_shouldreturn) = true;
		goto EXIT;
	}

	if (!YG (input_classname) || !YG (input_funname)) {
		YG (is_timer) = false;
		YG (is_look_variable) = false;
		return;
	}

	if (zend_hash_find (Z_ARRVAL_P (arrParam), skey, strlen (skey) + 1, (void **) &pvalue) == SUCCESS) {
		YG (is_dump_code) = true;
	}

	php_start_ob_buffer (NULL, 0, 1 TSRMLS_CC);
	if (zend_hash_find (Z_ARRVAL_P (arrParam), tkey, strlen (tkey) + 1, (void **) &pvalue) == SUCCESS) {
		YG (is_timer) = true;

	}
	else {
		YG (is_look_variable) = true;
	}

  EXIT:

	if (start_ydb () && zend_hash_find (Z_ARRVAL_P (arrParam), lkey, strlen (lkey) + 1, (void **) &pvalue) == SUCCESS) {
		YG (uselog) = true;
	}

}

ZEND_DLEXPORT void ydb_zend_fcall_begin (zend_op_array * op_array)
{
	if (!start_ydb ())
		return;
	if (YG (ydb_shouldreturn))
		return;

	YG (ydb_cur_op_array) = op_array;

	if (!YG (is_sum_time_begin)) {
		gettimeofday (&YG (start_sum), NULL);
	}

	if (YG (is_timer) && YG (timer_op_array) && op_array && op_array == YG (timer_op_array)) {
		gettimeofday (&YG (start), NULL);
	}
}

ZEND_DLEXPORT void ydb_zend_fcall_end (zend_op_array * op_array)
{

	if (!start_ydb ())
		return;
	if (YG (ydb_shouldreturn))
		return;

	if (YG (is_look_variable) && YG (ydb_varn)) {
		int count = zend_hash_num_elements (YG (ydb_varn));
		if (count > 0 && op_array && YG (ydb_dst_op_array) && op_array == YG (ydb_dst_op_array)) {
			if (!YG (uselog)) {
				zend_bailout ();
			}
		}
	}
	int time_use = 0;
	char timer_key[100] = { 0 };
	if (YG (is_timer) && YG (timer_op_array) && op_array && op_array == YG (timer_op_array) && YG (ydb_cur_classname) && YG (ydb_cur_funname) && (strcmp (YG (ydb_cur_classname), YG (input_classname)) || strcmp (YG (ydb_cur_funname), YG (input_funname)))) {	// why?

		int *data;
		gettimeofday (&YG (end), NULL);
		time_use = (YG (end).tv_sec - YG (start).tv_sec) * 1000000 + (YG (end).tv_usec - YG (start).tv_usec);
		sprintf (timer_key, "%s::%s", YG (ydb_cur_classname), YG (ydb_cur_funname));
		//zend_printf("key:%s,timeuse:%d\n",timer_key,time_use);
		if (zend_hash_find (YG (timer_fun), timer_key, strlen (timer_key) + 1, (void **) &data) == SUCCESS) {
			if (*data < time_use) {
				zend_hash_update (YG (timer_fun), timer_key, strlen (timer_key) + 1, &time_use, sizeof (int), NULL);
			}

		}
		else {
			zend_hash_update (YG (timer_fun), timer_key, strlen (timer_key) + 1, &time_use, sizeof (int), NULL);
		}

	}

	if (YG (is_timer)) {

		if (op_array && YG (ydb_dst_op_array) && op_array == YG (ydb_dst_op_array)) {

			gettimeofday (&YG (end_sum), NULL);
			time_use = (YG (end_sum).tv_sec - YG (start_sum).tv_sec) * 1000000 + (YG (end_sum).tv_usec - YG (start_sum).tv_usec);
			sprintf (timer_key, "sum_cost_time:  %s::%s ", YG (input_classname), YG (input_funname));
			zend_hash_update (YG (timer_fun), timer_key, strlen (timer_key) + 1, &time_use, sizeof (int), NULL);
			if (!YG (uselog)) {
				zend_bailout ();
			}
		}
	}
}

void store_function_variable ()
{

	HashTable *ht;
	//HashTable *test1,*test2;
	zval **data;
	zval *tmp;
	char *skey;
	ulong ikey;
	int i;
	ht = YG (dst_symbol_table);
	//test1= &EG(symbol_table);
	//test2= EG(active_symbol_table);

	if (!ht)
		return;

	if (YG (input_varname)) {
		if (zend_hash_find (ht, YG (input_varname), strlen (YG (input_varname)) + 1, (void **) &data) == SUCCESS) {
			ALLOC_ZVAL (tmp);
			MAKE_COPY_ZVAL (data, tmp);
			if (YG (ydb_varn)) {
				zend_hash_update (YG (ydb_varn), YG (input_varname), strlen (YG (input_varname)) + 1, &tmp, sizeof (zval *), NULL);
			}
		}
		return;
	}
	else {

		zend_hash_internal_pointer_reset (ht);
		int count = zend_hash_num_elements (ht);
		for (i = 0; i < count; i++) {
			zend_hash_get_current_data (ht, (void **) &data);
			zend_hash_get_current_key (ht, &skey, &ikey, 0);
			zend_hash_move_forward (ht);
			if (YG (ydb_varn)) {
				ALLOC_ZVAL (tmp);
				MAKE_COPY_ZVAL (data, tmp);
				zend_hash_update (YG (ydb_varn), skey, strlen (skey) + 1, &tmp, sizeof (zval *), NULL);
			}
		}

	}

}

static int cur_pos = 0;
// 0 1[2key3][key]
static int get_next_key (char *arglist, char *key)
{

	int status = 0;

	int ki = 0;

	while (arglist[cur_pos]) {

		if (arglist[cur_pos] == '[') {
			if (status == 0) {
				status = 1;
			}
			else {
				goto fail;
			}
		}
		else if (arglist[cur_pos] == ']') {

			if (status == 2) {
				status = 3;
				cur_pos++;
				break;
			}
			else {
				goto fail;
			}
		}
		else {

			if (status == 1 || status == 2) {
				if (ki >= MAX_KEY_LEN) {
					goto fail;
				}
				status = 2;
				key[ki++] = arglist[cur_pos];
			}
			else {
				goto fail;
			}
		}
		cur_pos++;
	}

	if (status != 3) {
		goto fail;
	}

	key[ki] = 0;

	if (arglist[cur_pos] == 0)
		return 0;

	return 1;

  fail:
	return -1;

}

static int replace_new_variable ()
{

	HashTable *h_vars = YG (input_vars);
	// zend_printf("in replace_new_variable\n");
	//  my_print_hash(h_vars, 1);

	zval **z_val;
	char *s_var;
	char *skey;
	ulong ikey;

//    my_print_hash(EG(active_symbol_table), 1);

	int count = zend_hash_num_elements (h_vars);
	if (count <= 0)
		return -1;

	zend_printf ("=======the replace variable list==========\n");
	zend_hash_internal_pointer_reset (h_vars);
	while (zend_hash_has_more_elements (h_vars) == SUCCESS) {
		zend_hash_get_current_data (h_vars, (void **) &z_val);
		zend_hash_get_current_key (h_vars, &skey, &ikey, 0);

		s_var = Z_STRVAL_PP (z_val);
		//      zend_printf("key:%s,val:%s\n",skey,s_var);

		if (strstr (skey, "[") && strstr (skey, "]")) {

			cur_pos = 0;

			char cur_key[MAX_KEY_LEN] = { 0 };
			char first_key[MAX_KEY_LEN] = { 0 };
			zval **cur_zv = NULL;
			zval **next_zv = NULL;
			zval *n_var;
			int i = 0;
			while (*skey != '[') {
				cur_key[i++] = *skey++;
			}

			strcpy (first_key, cur_key);
			//    zend_printf("first key:%s\n",cur_key);
			cur_key[i] = 0;
			int ret;

			if (zend_hash_find (EG (active_symbol_table), cur_key, strlen (cur_key) + 1, (void **) &cur_zv) == SUCCESS) {

				zend_printf ("$%s:\n", cur_key);
				my_print_zval (*cur_zv);
				zend_printf ("=>");

				if (Z_TYPE_PP (cur_zv) != IS_ARRAY) {

					MAKE_STD_ZVAL (n_var);
					array_init (n_var);
					zend_hash_update (EG (active_symbol_table), cur_key, strlen (cur_key) + 1, &n_var, sizeof (zval *), (void **) &cur_zv);
				}

				while (1) {
					ret = get_next_key (skey, cur_key);

					//  zend_printf("arr key:%s, ret:%d\n",cur_key,ret);

					if (ret == 0) {
						if (Z_TYPE_PP (cur_zv) == IS_ARRAY) {	// may replace the array value with string

							MAKE_STD_ZVAL (n_var);
							ZVAL_STRING (n_var, s_var, 1);	// all new var type is string 
								zend_hash_update (Z_ARRVAL_PP (cur_zv), cur_key, strlen (cur_key) + 1, &n_var, sizeof (zval *), NULL);
						}
						break;
					}
					else if (ret == 1) {

						if (zend_hash_find (Z_ARRVAL_PP (cur_zv), cur_key, strlen (cur_key) + 1, (void **) &next_zv) == FAILURE) {

							MAKE_STD_ZVAL (n_var);
							array_init (n_var);
							zend_hash_update (Z_ARRVAL_PP (cur_zv), cur_key, strlen (cur_key) + 1, &n_var, sizeof (zval *), (void **) &next_zv);
							// fix me,need efree it
						}
						else if ((Z_TYPE_PP (next_zv) != IS_ARRAY)) {	// may replace the string with array

							MAKE_STD_ZVAL (n_var);
							array_init (n_var);
							zend_hash_update (Z_ARRVAL_PP (cur_zv), cur_key, strlen (cur_key) + 1, &n_var, sizeof (zval *), (void **) &next_zv);
						}
						cur_zv = next_zv;
					}
					else {
						break;
					}
				}

				if (zend_hash_find (EG (active_symbol_table), first_key, strlen (first_key) + 1, (void **) &cur_zv) == SUCCESS) {
					my_print_zval (*cur_zv);
				}

				// my_print_hash(EG(active_symbol_table), 1);

			}
			else {

				zend_printf ("\"$%s\" not exist\n", cur_key);
			}

		}
		else {

			zval **old_val;
			if (zend_hash_find (EG (active_symbol_table), skey, strlen (skey) + 1, (void **) &old_val) == SUCCESS) {	//目前都是覆盖为字符串类型，未来考虑保持原类型

				zend_printf ("$%s:\n", skey);
				my_print_zval (*old_val);

				zval *n_var;
				MAKE_STD_ZVAL (n_var);
				ZVAL_STRING (n_var, s_var, 1);	// all new var type is string 
					ZEND_SET_SYMBOL (EG (active_symbol_table), skey, n_var);

				zend_printf ("=>");
				my_print_zval (n_var);
				zend_printf ("\n");
			}
			else {
				zend_printf ("\"$%s\" not exist\n", skey);
			}
			//  my_print_hash(EG(active_symbol_table), 1);
		}

		zend_hash_move_forward (h_vars);
	}

	return 0;
}

static int replace_curl_fun ()
{

	if (YG (is_replace_new_fun))
		return 0;

	zend_function *orig;

	zend_hash_find (EG (function_table), "curl_init", sizeof ("curl_init"), (void **) & orig);
	if (!YG (orig_curl_init)) {
		YG (orig_curl_init) = orig->internal_function.handler;
	}

	/*    if(YG(orig_curl_init) == ydb_curl_init) { // 完善这个地方
	   zend_printf("some big error\n");
	   zend_bailout();      
	   } */

	orig->internal_function.handler = ydb_curl_init;

	zend_hash_find (EG (function_table), "curl_setopt", sizeof ("curl_setopt"), (void **) & orig);
	if (!YG (orig_curl_setopt)) {
		YG (orig_curl_setopt) = orig->internal_function.handler;
	}
	orig->internal_function.handler = ydb_curl_setopt;

	zend_hash_find (EG (function_table), "curl_exec", sizeof ("curl_exec"), (void **) & orig);
	if (!YG (orig_curl_exec)) {
		YG (orig_curl_exec) = orig->internal_function.handler;
	}
	orig->internal_function.handler = ydb_curl_exec;

	YG (is_replace_new_fun) = true;

	return 0;

}

static int recover_curl_fun ()
{

	if (!YG (is_replace_new_fun))
		return 0;

	zend_function *orig;

	zend_hash_find (EG (function_table), "curl_init", sizeof ("curl_init"), (void **) & orig);
	orig->internal_function.handler = YG (orig_curl_init);

	zend_hash_find (EG (function_table), "curl_setopt", sizeof ("curl_setopt"), (void **) & orig);
	orig->internal_function.handler = YG (orig_curl_setopt);

	zend_hash_find (EG (function_table), "curl_exec", sizeof ("curl_exec"), (void **) & orig);
	orig->internal_function.handler = YG (orig_curl_exec);

	YG (is_replace_new_fun) = false;

	return 0;

}

static int replace_new_mysql_fun ()
{								// todo

}

static int is_cross_app ()
{								// will do is_net_timer in the future

	return YG (is_look_variable) || YG (is_timer);

}

static int is_need_curl_url ()
{

	return YG (is_net_timer);

}

void ydb_curl_init (INTERNAL_FUNCTION_PARAMETERS)
{

	if (is_cross_app () || is_need_curl_url ()) {
		char *url = NULL;
		int url_len = 0;
		if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "|s", &url, &url_len) == FAILURE) {
			return;
		}
		if (url) {
			YG (old_curl_url) = estrdup (url);
			//YDB_LOG("init set url:%s", YG(old_curl_url));
		}
	}

	YG (orig_curl_init) (INTERNAL_FUNCTION_PARAM_PASSTHRU);

}

void ydb_curl_setopt (INTERNAL_FUNCTION_PARAMETERS)
{

	if (is_cross_app () || is_need_curl_url ()) {
		zval *zid, **zvalue;
		long options;

		if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "rlZ", &zid, &options, &zvalue) == FAILURE) {
			return;
		}
		zval curl_const;
		zend_get_constant ("CURLOPT_URL", strlen ("CURLOPT_URL"), &curl_const);
		if (Z_LVAL (curl_const) == options && zvalue && Z_TYPE_PP (zvalue) == IS_STRING) {
			YG (old_curl_url) = estrdup (Z_STRVAL_PP (zvalue));
			//YDB_LOG("setopt set url:%s", YG(old_curl_url));
			if (is_cross_app ()) {
				RETURN_TRUE;	//注意返回值
			}
		}
		YG (orig_curl_setopt) (INTERNAL_FUNCTION_PARAM_PASSTHRU);
	}
	else {
		YG (orig_curl_setopt) (INTERNAL_FUNCTION_PARAM_PASSTHRU);
	}

}

/*
* 
* 有类似如下的返回头
HTTP/1.1 100 Continue

HTTP/1.1 200 OK
Server: 360Server
Date: Mon, 14 Apr 2014 13:32:21 GMT

*/
int split_curl_res (char **source_res)
{								// no len?

	int res = 0;
	char *source = *source_res;
	if (!source || strncmp (source, CURL_RES_FIRST_LINE, strlen (CURL_RES_FIRST_LINE))) {
		while (*source) {

			if (source[0] == '\r' && source[1] == '\n', source[2] == '\r' && source[3] == '\n') {
				source += 4;
				if (strncmp (source, CURL_RES_FIRST_LINE, strlen (CURL_RES_FIRST_LINE))) {
					break;
				}
				else
					return 1;
			}
			source++;
		}

	}

	char res_line[MAX_HEAD_LINE_LEN] = { 0 };
	int i = 0;
	while (*source) {

		if (source[0] == '\r' && source[1] == '\n') {

			res_line[i] = 0;
			if (strcmp (res_line, CURL_RES_FLAG_HEADER) == 0)
				res = 1;

			if (source[2] == '\r' && source[3] == '\n') {
				source += 4;
				*source_res = source;
				return res;
			}

			i = 0;
			source += 2;
		}
		res_line[i++] = *source++;
	}

	*source_res = source;
	return res;

}

static char *get_new_url ()
{

	YG (new_curl_url)[0] = 0;

	char *n_url = YG (new_curl_url);
	strcat (n_url, YG (old_curl_url));

	if (strstr (n_url, "=")) {
		strcat (n_url, "&d=2");	//表示远程访问深度，后续升级
	}
	else {
		strcat (n_url, "?d=2");
	}

	if (YG (input_classname)) {
		strcat (n_url, "&c=");
		strcat (n_url, YG (input_classname));
	}

	if (YG (input_funname)) {
		strcat (n_url, "&f=");
		strcat (n_url, YG (input_funname));
	}

	if (YG (input_varname)) {
		strcat (n_url, "&v=");
		strcat (n_url, YG (input_varname));
	}

	if (YG (input_new_var)) {
		strcat (n_url, "&a=");
		strcat (n_url, YG (input_new_var));
	}

	if (YG (is_timer)) {
		strcat (n_url, "&t=1");
	}

	if (YG (uselog)) {
		strcat (n_url, "&l=1");
	}

	if (YG (is_dump_code)) {
		strcat (n_url, "&s=1");
	}

	YDB_LOG ("old_url:%s,new_url:%s\n", YG (old_curl_url), YG (new_curl_url));

	efree (YG (old_curl_url));

	return n_url;

}

void ydb_curl_exec (INTERNAL_FUNCTION_PARAMETERS)
{

	zval function_name, retval;
	INIT_ZVAL (function_name);
	INIT_ZVAL (retval);
	zval *zid;

	int status;

	if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "r", &zid) == FAILURE) {
		return;
	}

	recover_curl_fun ();

	if (is_cross_app ()) {

		get_new_url ();

		char *n_url = YG (new_curl_url);

		zval param2, param3;
		zval *params[3];

		params[0] = zid;

		params[1] = &param2;
		INIT_PZVAL (params[1]);

		params[2] = &param3;
		INIT_PZVAL (params[2]);

		zval curl_const;
		zend_get_constant ("CURLOPT_URL", strlen ("CURLOPT_URL"), &curl_const);

		ZVAL_LONG (params[1], Z_LVAL (curl_const));
		ZVAL_STRING (params[2], n_url, 0);

		ZVAL_STRING (&function_name, "curl_setopt", 0);
		status = call_user_function (CG (function_table), NULL, &function_name, &retval, 3, params);

		if (status != SUCCESS) {

		}

		zend_get_constant ("CURLOPT_HEADER", strlen ("CURLOPT_HEADER"), &curl_const);
		ZVAL_LONG (params[1], Z_LVAL (curl_const));
		ZVAL_BOOL (params[2], 1);

		status = call_user_function (CG (function_table), NULL, &function_name, &retval, 3, params);

		if (status != SUCCESS) {

		}

	}

	if (YG (is_net_timer)) {
		gettimeofday (&YG (start), NULL);
	}

	ZVAL_STRING (&function_name, "curl_exec", 0);	// 调用方需要设置CURLOPT_RETURNTRANSFER,这里需要加上判断有没有设置 fixme
	status = call_user_function (CG (function_table), NULL, &function_name, &retval, 1, &zid);

	YDB_LOG ("status:%d", status);
	if (status != SUCCESS) {
		YDB_LOG ("status:%d,curl_exec fail", status);
	}

	if (YG (is_net_timer)) {
		int time_use = 0;
		int *data;
		gettimeofday (&YG (end), NULL);
		time_use = (YG (end).tv_sec - YG (start).tv_sec) * 1000000 + (YG (end).tv_usec - YG (start).tv_usec);

		char *key = YG (old_curl_url);
		//  zend_printf("key:%s,timeuse:%d\n",key,time_use);

		int *kcnt;

		char new_key[MAX_URL_LEN + 10] = { 0 };
		strcpy (new_key, key);

		int cnt = 1;
		if (zend_hash_find (YG (key_count), key, strlen (key) + 1, (void **) &kcnt) == SUCCESS) {
			cnt = *kcnt + 1;
		}

		sprintf (new_key, "%s_%d", key, cnt);
		zend_hash_update (YG (key_count), key, strlen (key) + 1, &cnt, sizeof (int), NULL);

		if (zend_hash_find (YG (timer_fun), new_key, strlen (new_key) + 1, (void **) &data) == SUCCESS) {
			if (*data < time_use) {
				zend_hash_update (YG (timer_fun), new_key, strlen (new_key) + 1, &time_use, sizeof (int), NULL);
			}

		}
		else {
			zend_hash_update (YG (timer_fun), new_key, strlen (new_key) + 1, &time_use, sizeof (int), NULL);
		}

	}

	if (!Z_STRVAL (retval)) {	//结果可能为空
		YDB_LOG ("res is empty");
		replace_curl_fun ();
		RETURN_STRING ("", 1);
	}

	if (is_cross_app ()) {
		YG (curl_res) = Z_STRVAL (retval);
		char *src_res = YG (curl_res);
		//   zend_printf("\nresstr:\n%s     =========end\n",Z_STRVAL(retval));
		//YDB_LOG("full res===:\n%s", src_res);

		if (split_curl_res (&src_res)) {
			zend_printf ("%s", src_res);
			YG (is_get_remote_res) = true;
			zend_bailout ();	//need do some efree work
		}
		else {
			//YDB_LOG("res===:\n%s", src_res);
			replace_curl_fun ();
			RETURN_STRING (src_res, 1);
		}

	}
	else {
		// zend_printf("resstr:%s\n",Z_STRVAL(retval));
		replace_curl_fun ();
		RETURN_STRING (Z_STRVAL (retval), 1);
	}

}

ZEND_DLEXPORT void ydb_statement_call (zend_op_array * op_array)
{

	if (!YG (is_get_debug_params)) {
		get_debug_params ();
		YG (is_get_debug_params) = true;
	}

	if (!start_ydb ())
		return;
	if (YG (ydb_shouldreturn))
		return;

	zend_class_entry *scope;
	scope = EG (scope);
	if (scope) {
		YG (ydb_cur_classname) = scope->name;
	}
	else {
		YG (ydb_cur_classname) = NULL;
	}

	zend_op_array *op;

	op = EG (active_op_array);
	if (op && op->function_name) {
		YG (ydb_cur_funname) = op->function_name;
	}
	else {
		YG (ydb_cur_funname) = NULL;
	}

	// zend_printf("sclass:%s\n",class_name);
	//  zend_printf("sfunc:%s\n",fun_name);

	if (!YG (ydb_cur_classname) || !YG (ydb_cur_funname)) {
		return;
	}

	if (strcmp (YG (input_classname), YG (ydb_cur_classname))) {
		return;
	}
	if (strcmp (YG (input_funname), YG (ydb_cur_funname))) {
		return;
	}

	if (YG (is_dump_code) && !YG (is_get_code_info) && op->filename) {

		YG (file_name) = estrdup (op->filename);
		YG (line_start) = op->line_start;
		YG (line_end) = op->line_end;
		YG (is_get_code_info) = true;
	}

	if (!YG (is_send_header) && !YG (uselog)) {
		add_ydb_header ();
		YG (is_send_header) = true;
	}

	if (!YG (ydb_dst_op_array))
		YG (ydb_dst_op_array) = YG (ydb_cur_op_array);

	if (YG (is_timer)) {
		if (op_array && !YG (timer_op_array))
			YG (timer_op_array) = op_array;
		YG (is_sum_time_begin) = true;
	}

	if (YG (is_timer))
		return;

	if (!EG (active_symbol_table)) {
		zend_rebuild_symbol_table (TSRMLS_C);
	}

	if (EG (active_symbol_table)) {
		YG (dst_symbol_table) = EG (active_symbol_table);
	}

	store_function_variable ();
	if (EG (active_symbol_table) && !YG (is_set_new_var)) {	//只在函数入口处进行一次变量覆盖
		replace_new_variable ();
		YG (is_set_new_var) = true;
	}

}

ZEND_DLEXPORT void ydb_oparray_init (zend_op_array * op_array)
{

}

#ifndef ZEND_EXT_API
#define ZEND_EXT_API    ZEND_DLEXPORT
#endif

ZEND_EXTENSION ();

ZEND_DLEXPORT zend_extension zend_extension_entry = {
	YDB_NAME,
	YDB_VERSION,
	YDB_AUTHOR,
	YDB_URL_FAQ,
	YDB_COPYRIGHT_SHORT,
	ydb_zend_startup,
	ydb_zend_shutdown,
	ydb_zend_activate,			/* activate_func_t */
	ydb_zend_deactivate,		/* deactivate_func_t */
	NULL,						/* message_handler_func_t */
	NULL,						/* op_array_handler_func_t */// we may use it
	ydb_statement_call,			/* statement_handler_func_t */
	ydb_zend_fcall_begin,		/* fcall_begin_handler_func_t */
	ydb_zend_fcall_end,			/* fcall_end_handler_func_t */
	ydb_oparray_init,			/* op_array_ctor_func_t */
	NULL,						/* op_array_dtor_func_t */
	STANDARD_ZEND_EXTENSION_PROPERTIES
};
