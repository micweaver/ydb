

#include <unistd.h>
#include "php.h"
#include "main/php_version.h"
#include "ydb.h" 
#include "TSRM.h"
#include "SAPI.h"
#include "main/php_ini.h"
#include "ext/standard/head.h"
#include "ext/standard/html.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "php_globals.h"
#include "main/php_output.h"
#include "ext/standard/php_var.h"
#include "zend_extensions.h"

ZEND_DLEXPORT int ydb_zend_startup(zend_extension *extension)
{  

#ifdef PHP2_5
		CG(extended_info) = 1;
		// zend_printf("%s\n","ydb start PHP5.2");
#else
		TSRMLS_FETCH();
		CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
		// zend_printf("%s\n","ydb start PHP5.3");
#endif

		return SUCCESS;

}

ZEND_DLEXPORT void ydb_zend_shutdown(zend_extension *extension)
{
		/* Do nothing. */
}


ZEND_DLEXPORT void ydb_zend_activate(void)
{

		// zend_printf("cwd:%s\n",cwdydb);

		//memset
		YG(input_classname) = 0;
		YG(input_funname) = 0;
		YG(input_varname) = 0;


		YG(input_post_classname) = (char*)emalloc(sizeof(char)*100);
		YG(input_post_funname) =  (char*)emalloc(sizeof(char)*100);
		YG(input_post_varname) =  (char*)emalloc(sizeof(char)*100);

		YG(input_post_classname) [0]=0;
		YG(input_post_funname) [0] = 0;
		YG(input_post_varname) [0]= 0;

		YG(ydb_shouldreturn) = 0;
		YG(ydb_cur_op_array)= 0;
		YG(ydb_dst_op_array) = 0;

		YG(is_post) = 0;


		YG(parent_classname) = 0;
		YG(parent_funname)  = 0;


		///YG(start) = 0;//aaaaaaa
		//YG(end) = 0;

		YG(usetimer) = 0;
		YG(ydb_timer)= 0;

		YG(ydb_cur_classname) = 0;
		YG(ydb_cur_funname) = 0;
		YG(timer_op_array) = 0; 
		YG(timer_fun) = 0;
		YG(ydb_varn) = 0;

		YG(uselog) = 0;
		//     zend_printf("%s\n","ydb activate");

		ALLOC_HASHTABLE(YG(ydb_varn));
		zend_hash_init(YG(ydb_varn), 128, NULL, ZVAL_PTR_DTOR, 0);


		ALLOC_HASHTABLE(YG(timer_fun));
		zend_hash_init(YG(timer_fun), 128, NULL, NULL, 0);//need destructor


} 

static int ydb_timer_compare(const void* a,const void*b){

		Bucket *f = *((Bucket **) a);
		Bucket *s = *((Bucket **) b);

		int *ia = (int*)f->pData;
		int *ib = (int*)s->pData;

		if(*ia > *ib) return -1;
		else if (*ia < *ib) return 1;
		else return 0;
}

ZEND_DLEXPORT void ydb_zend_deactivate(void)
{

		int i;
		char* skey;
		ulong ikey;

		char cwdydb[100] = {0};
		zval* message;
		php_stream *stream = NULL;

		if(YG(uselog)){
				getcwd(cwdydb,sizeof(cwdydb));
				strcat(cwdydb,"/log/ydbinfo.log");
				stream = php_stream_open_wrapper(cwdydb,"a",IGNORE_URL_WIN | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL);
				//     zend_printf("cwd:%s\n",cwdydb);
				ALLOC_ZVAL(message);
				php_start_ob_buffer (NULL, 0, 1 TSRMLS_CC);
		}

		if(YG(ydb_varn) && YG(ydb_timer) == 0){
				zval**data;

				zend_hash_internal_pointer_reset(YG(ydb_varn));
				int count = zend_hash_num_elements(YG(ydb_varn));
				for( i=0 ; i< count; i++){
						zend_hash_get_current_data(YG(ydb_varn), (void**)&data);
						zend_hash_get_current_key(YG(ydb_varn), &skey, &ikey, 0);
						zend_hash_move_forward(YG(ydb_varn));
						zend_printf("%s:",skey);
						zend_print_zval_r(*data,0);
						zend_printf("\n");
				} 

				if(YG(uselog)) {
						php_ob_get_buffer (message TSRMLS_CC);
						if(stream != NULL) {
								php_stream_write(stream, Z_STRVAL_P(message), Z_STRLEN_P(message));
						}
				}
				if(count>0) {
						if(!YG(uselog)) {
								zend_bailout();
						}
				}
		}
		if(YG(ydb_varn)){
				zend_hash_destroy(YG(ydb_varn));
				YG(ydb_varn) = 0;
		}

		efree(YG(input_post_classname));
		efree(YG(input_post_funname));
		efree(YG(input_post_varname));


		if(YG(ydb_timer) && YG(timer_fun)) {
				skey = 0;
				int*data;
				zend_hash_internal_pointer_reset(YG(timer_fun));
				int count = zend_hash_num_elements(YG(timer_fun));
				if(count > 0 ) {
						zend_hash_sort(YG(timer_fun), zend_qsort, ydb_timer_compare, 0 TSRMLS_CC);
						for( i=0 ; i< count; i++){
								zend_hash_get_current_data(YG(timer_fun), (void**)&data);
								zend_hash_get_current_key(YG(timer_fun), &skey, &ikey, 0);
								zend_hash_move_forward(YG(timer_fun));
								zend_printf("%s=>",skey);
								zend_printf("%.3f ms",(float)(*data)/1000);
								zend_printf("\n");

						} 
						if(YG(uselog)) {
								php_ob_get_buffer (message TSRMLS_CC);
								if(stream != NULL) {
										php_stream_write(stream, Z_STRVAL_P(message), Z_STRLEN_P(message));
								}
						}
				}
				if(!YG(uselog)) {
						zend_bailout();
				}
		}

		if(YG(uselog)){
				if(stream != NULL) {
						php_stream_close(stream);
				}
				php_end_ob_buffer (0, 0 TSRMLS_CC);
				FREE_ZVAL(message);
		}
}


ZEND_DLEXPORT void ydb_zend_fcall_begin(zend_op_array  *op_array)
{	

		//---------------------处理参数-------

		zval* arr;
		zval** temp;
		char* ckey = "c",*fkey="f",*vkey="v",*okey="o",*tkey="t",*lkey="l";
		char* r_str;
		int len = 2;
		char* refer;
		int i;
		HashTable* h;

		arr = PG(http_globals)[TRACK_VARS_GET];
		if(zend_hash_find(HASH_OF(arr), okey, len, (void **)&temp) ==SUCCESS){
				YG(is_post) = 1;
		} 

		if(!YG(is_post)){
				if(zend_hash_find(HASH_OF(arr), ckey, len, (void **)&temp) ==SUCCESS){
						r_str = Z_STRVAL_PP(temp);
						YG(input_classname) = r_str;
				} else {
						YG(input_classname) = 0;
				}

				if(zend_hash_find(HASH_OF(arr), fkey, len, (void **)&temp) == SUCCESS) {
						r_str = Z_STRVAL_PP(temp);
						YG(input_funname) = r_str;
				} else {
						YG(input_funname) = 0;
				}

				if(zend_hash_find(HASH_OF(arr), vkey, len, (void **)&temp) ==SUCCESS) {
						r_str = Z_STRVAL_PP(temp);
						YG(input_varname) = r_str;
				} else {
						YG(input_varname) = 0;
				}

		} 

		zend_is_auto_global("_SERVER", sizeof("_SERVER") - 1 TSRMLS_CC);
		if( YG(input_classname) == 0 &&  YG(input_funname)  == 0  &&   YG(input_varname) == 0 &&  PG(http_globals)[TRACK_VARS_SERVER]){
				arr = PG(http_globals)[TRACK_VARS_SERVER];
				h= HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);
				if(zend_hash_find(h, "HTTP_REFERER", sizeof("HTTP_REFERER"), (void **)&temp) == SUCCESS){

						refer = Z_STRVAL_PP(temp);
						// zend_printf("refer:%s\n",refer);
						int cur = 0;
						int j = 0;
						int k = 0;
						while (refer[j]){
								switch (refer[j]){
										case 'c':  //class name
												if(refer[j+1] == '='){ 
														j+=2;
														cur = 1;
												}
												break;
										case 'f':// function name
												if(refer[j+1] == '='){
														j+=2;
														cur = 2;
												}
												break;
										case 'v' : // variable name
												if(refer[j+1] == '='){
														j+=2;
														cur = 3;
												}
												break;
										case 'l' : // use log?
												if(refer[j+1] == '='){
														j+=2;
														cur = 4;
														YG(uselog) = 1;    
												}
												break;
										case 't' : // use timer?
												if(refer[j+1] == '='){
														j+=2;
														cur = 5;
														YG(ydb_timer) = 1;    
												}
												break;
										case '&' :
										case '=' :
												cur = 0;
												k = 0;
												break;				  

								}

								if(cur == 1) {
										YG(input_post_classname)[k++] = refer[j];
										if(refer[j+1] == '&' || refer[j+1] == 0) {
												YG(input_post_classname)[k] = 0;
										}
								} else if(cur == 2) {
										YG(input_post_funname)[k++] = refer[j];
										if(refer[j+1] == '&' || refer[j+1] == 0) {
												YG(input_post_funname)[k] = 0;
										}
								} else if(cur == 3) {
										YG(input_post_varname)[k++] = refer[j];
										if(refer[j+1] == '&' || refer[j+1] == 0) {
												YG(input_post_varname)[k] = 0;
										}
								} 
								j++;
						}

				}

				if(YG(input_post_classname)[0]){
						YG(input_classname) = YG(input_post_classname);
				}

				if(YG(input_post_funname)[0]){
						YG(input_funname) = YG(input_post_funname);
				}

				if(YG(input_post_varname)[0]){
						YG(input_varname) = YG(input_post_varname);
				}

				// zend_printf("pc:%s,pf:%s,pv:%s\n",YG(input_classname), YG(input_funname),YG(input_varname));

		}


		if(zend_hash_find(HASH_OF(arr), tkey, len, (void **)&temp) ==SUCCESS){
				YG(ydb_timer) = 1;
		}

		if(zend_hash_find(HASH_OF(arr), lkey, len, (void **)&temp) ==SUCCESS){
				YG(uselog) = 1;       
		}

		//---------------------------

		YG(ydb_cur_op_array) = op_array;


		if(YG(ydb_timer) && YG(timer_op_array)  &&  op_array &&  op_array ==  YG(timer_op_array) ) {
				gettimeofday(&YG(start),NULL);
		}

}

ZEND_DLEXPORT void ydb_zend_fcall_end(zend_op_array *op_array)
{

		if(YG(ydb_varn)){
				int count = zend_hash_num_elements(YG(ydb_varn));
				if(count>0 && op_array && YG(ydb_dst_op_array) && op_array == YG(ydb_dst_op_array)) { 
						if(!YG(uselog)) {
								zend_bailout();
						}
				}
		}

		if( YG(ydb_timer) &&  YG(timer_op_array)  &&  op_array &&  op_array ==  YG(timer_op_array)  && YG(ydb_cur_classname) && YG(ydb_cur_funname) &&  (strcmp(YG(ydb_cur_classname),YG(input_classname)) ||  strcmp(YG(ydb_cur_funname),YG(input_funname)))) {

				int time_use=0;
				char* timer_key = (char*)emalloc(sizeof(char)*100);
				memset(timer_key,0,100);
				int *data;
				gettimeofday(&YG(end),NULL);
				time_use=(YG(end).tv_sec-YG(start).tv_sec)*1000000+(YG(end).tv_usec-YG(start).tv_usec);
				sprintf(timer_key,"%s::%s",YG(ydb_cur_classname),YG(ydb_cur_funname));
				//zend_printf("key:%s,timeuse:%d\n",timer_key,time_use);
				if(zend_hash_find(YG(timer_fun), timer_key, strlen(timer_key)+1, (void**)&data) == SUCCESS){
						if(*data < time_use) {
								zend_hash_update(YG(timer_fun),timer_key,strlen(timer_key)+1,&time_use,sizeof(int),NULL);
						}

				} else {
						zend_hash_update(YG(timer_fun),timer_key,strlen(timer_key)+1,&time_use,sizeof(int),NULL);
				}

				efree(timer_key);
		}

		if(YG(timer_fun)){
				int count = zend_hash_num_elements(YG(timer_fun));
				if(count>0 && op_array && YG(ydb_dst_op_array) && op_array == YG(ydb_dst_op_array)) { 
						if(!YG(uselog)) {
								zend_bailout();
						}
				}
		}
}

ZEND_DLEXPORT void ydb_statement_call(zend_op_array *op_array){


		if(YG(ydb_shouldreturn)) return;

		char* fun_name = 0,*class_name = 0;
		zend_class_entry *scope;
		scope = EG(scope);
		if(scope) {
				class_name = scope->name;
				YG(ydb_cur_classname) = class_name;
		} else {
				YG(ydb_cur_classname) = 0;
		}
		zend_op_array* op;

		op = EG(active_op_array);
		if(op && op->function_name){
				fun_name = op->function_name;
				YG(ydb_cur_funname) = fun_name;
		} else {
				YG(ydb_cur_funname) = 0;
		}
		// zend_printf("sclass:%s\n",class_name);
		//  zend_printf("sfunc:%s\n",fun_name);

		if(!YG(input_funname)) return;

		if(YG(input_classname) &&   (!class_name  || strcmp(YG(input_classname),class_name))){
				return;
		}
		if(YG(input_funname) &&  (!fun_name || strcmp(YG(input_funname),fun_name))){
				return;
		}


		if(!YG(ydb_dst_op_array))  YG(ydb_dst_op_array) = YG(ydb_cur_op_array);

		if(YG(input_classname)  &&  class_name && strcmp(YG(input_classname),class_name) == 0 && YG(input_funname) && fun_name && strcmp(YG(input_funname) ,fun_name) ==0){
				if(op_array && !YG(timer_op_array)) YG(timer_op_array) = op_array;
		}


		if(YG(ydb_timer)) return;

		//zend_printf("%s\n","ydb statement call");
		if (!EG(active_symbol_table)) {
				zend_rebuild_symbol_table(TSRMLS_C);
		}	
		HashTable* h;
		zval **data;
		zval* tmp;
		char  * skey;
		ulong ikey;
		int i;
		h = EG(active_symbol_table);
		zend_hash_internal_pointer_reset(h);
		int count = zend_hash_num_elements(h);
		for( i=0 ; i< count; i++){
				zend_hash_get_current_data(h, (void**)&data);
				zend_hash_get_current_key(h, &skey, &ikey, 0);
				zend_hash_move_forward(h);

				if(YG(ydb_varn)) {
						if(YG(input_varname) && strcmp(YG(input_varname),skey)) continue;
						if(YG(input_varname) && strcmp(YG(input_varname),skey) == 0) YG(ydb_shouldreturn) = 1;

						ALLOC_ZVAL(tmp);
						MAKE_COPY_ZVAL(data, tmp);
						zend_hash_update(YG(ydb_varn),skey,strlen(skey)+1,&tmp,sizeof(zval*),NULL);
				}
		}
}

ZEND_DLEXPORT void ydb_oparray_init(zend_op_array *op_array){

}
#ifndef ZEND_EXT_API
#define ZEND_EXT_API    ZEND_DLEXPORT
#endif

ZEND_EXTENSION();

ZEND_DLEXPORT zend_extension zend_extension_entry = {
		YDB_NAME,
		YDB_VERSION,
		YDB_AUTHOR,
		YDB_URL_FAQ,
		YDB_COPYRIGHT_SHORT,
		ydb_zend_startup,
		ydb_zend_shutdown,
		ydb_zend_activate,           /* activate_func_t */
		ydb_zend_deactivate,           /* deactivate_func_t */
		NULL,           /* message_handler_func_t */
		NULL,           /* op_array_handler_func_t */
		ydb_statement_call, /* statement_handler_func_t */
		ydb_zend_fcall_begin,           /* fcall_begin_handler_func_t */
		ydb_zend_fcall_end,           /* fcall_end_handler_func_t */
		ydb_oparray_init,   /* op_array_ctor_func_t */
		NULL,           /* op_array_dtor_func_t */
		STANDARD_ZEND_EXTENSION_PROPERTIES
};

// it just a draft , i will improve it when i have time, of course , it  can be used now
