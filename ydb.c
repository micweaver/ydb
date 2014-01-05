#include <unistd.h>
#include "ydb.h" 


static void store_function_variable();

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
		
    YG(is_look_variable) = false;
		YG(is_timer)= false;
		YG(is_get_debug_params) = false;
		YG(is_sum_time_begin) = false;

		YG(dst_symbol_table) = NULL;
	
		YG(input_classname) = NULL;
		YG(input_funname) = NULL;
		YG(input_varname) = NULL;

		YG(input_post_classname) [0]= 0;
		YG(input_post_funname) [0] = 0;
		YG(input_post_varname) [0]= 0;

		YG(ydb_shouldreturn) = false;
		YG(ydb_cur_op_array)= NULL;
		YG(ydb_dst_op_array) = NULL;

		YG(is_post) = false;

		YG(parent_classname) = NULL;
		YG(parent_funname)  = NULL;



		YG(usetimer) = false;
		
		YG(ydb_cur_classname) = NULL;
		YG(ydb_cur_funname) = NULL;
		YG(timer_op_array) = NULL; 
		YG(timer_fun) = NULL;
		YG(ydb_varn) = NULL;

		YG(uselog) = false;

		//     zend_printf("%s\n","ydb activate");

		ALLOC_HASHTABLE(YG(ydb_varn));
		zend_hash_init(YG(ydb_varn), INIT_HASH_SIZE, NULL, ZVAL_PTR_DTOR, 0);


		ALLOC_HASHTABLE(YG(timer_fun));
		zend_hash_init(YG(timer_fun), INIT_HASH_SIZE, NULL, NULL, 0);


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

static int start_ydb(){

      return YG(is_look_variable) || YG(is_timer);
}

ZEND_DLEXPORT void ydb_zend_deactivate(void)
{
      
    if(!start_ydb()) {
			zend_hash_destroy(YG(ydb_varn));
		  zend_hash_destroy(YG(timer_fun));
			FREE_HASHTABLE(YG(ydb_varn));
			FREE_HASHTABLE(YG(timer_fun));
			return;
    }

        php_end_ob_buffer(0, 0 TSRMLS_CC);
		
		int i;
		char* skey;
		ulong ikey;

		char cwdydb[DIR_MAX_LEN] = {0};
		zval* message;
		php_stream *stream = NULL;

		if(YG(uselog)){
				getcwd(cwdydb,sizeof(cwdydb));
				strcat(cwdydb,YDB_LOG_FILE);
				stream = php_stream_open_wrapper(cwdydb,"a",IGNORE_URL_WIN | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL);
				//     zend_printf("cwd:%s\n",cwdydb);
				ALLOC_ZVAL(message);
				php_start_ob_buffer (NULL, 0, 1 TSRMLS_CC);
		}

		if(YG(is_look_variable) && YG(ydb_varn)){ //打印变量值
				zval**data;

				zend_hash_internal_pointer_reset(YG(ydb_varn));
				int count = zend_hash_num_elements(YG(ydb_varn));

                if(count <=0 ){
					if(YG(input_varname)) {
						zend_printf("variable value %s::%s::%s not exist, you may supply the wrong param",YG(input_classname),YG(input_funname),YG(input_varname));
						goto EXIT;
					} else {
	                    zend_printf("variable value in %s::%s not exist,you  may supply the wrong param",YG(input_classname),YG(input_funname));
						goto EXIT;
					}
                }	
				
				for(i=0 ; i< count; i++){
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

				zend_hash_destroy(YG(ydb_varn));
			
				if(!YG(uselog)) {
					goto EXIT;
				}
				
		} else if(YG(is_timer) && YG(timer_fun)){ //打印耗时排行
				skey = 0;
				int* data;
				zend_hash_internal_pointer_reset(YG(timer_fun));
				int count = zend_hash_num_elements(YG(timer_fun));

				if(count <=0 ){
                    zend_printf("no function time in %s::%s, you may supply the wrong param",YG(input_classname),YG(input_funname));
					goto EXIT;
                }
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
						goto EXIT;
				}
		}


		if(YG(uselog)){
				if(stream != NULL) {
						php_stream_close(stream);
				}
				php_end_ob_buffer (0, 0 TSRMLS_CC);
		}

EXIT:		

		zend_hash_destroy(YG(ydb_varn));
		zend_hash_destroy(YG(timer_fun));
		FREE_HASHTABLE(YG(ydb_varn));
		FREE_HASHTABLE(YG(timer_fun));
}


static void get_debug_params(){

		zval* arrParam;
		zval** pvalue;
		
		char* refer;
		int i;
		HashTable* ht;
		
		char  *ckey="c", //类名
			  *fkey="f", //函数名
			  *vkey="v", //变量名
			  *okey="o", //标示post请求
			  *tkey="t", //标示进行计时操作
			  *lkey="l"; //标示将结果打印进日志
		
	
		arrParam = PG(http_globals)[TRACK_VARS_GET];
		if(zend_hash_find(Z_ARRVAL_P(arrParam), okey, strlen(okey)+1, (void **)&pvalue) ==SUCCESS){
				YG(is_post) = true;
		} 

		if(!YG(is_post)){ // get 参数
				if(zend_hash_find(Z_ARRVAL_P(arrParam), ckey, strlen(ckey)+1, (void **)&pvalue) ==SUCCESS){
						YG(input_classname) = Z_STRVAL_PP(pvalue);
				} else {
						YG(input_classname) = NULL;
				}

				if(zend_hash_find(Z_ARRVAL_P(arrParam), fkey, strlen(fkey)+1, (void **)&pvalue) == SUCCESS) {
						YG(input_funname) = Z_STRVAL_PP(pvalue);
				} else {
						YG(input_funname) = NULL;
				}

				if(zend_hash_find(Z_ARRVAL_P(arrParam), vkey, strlen(vkey)+1, (void **)&pvalue) ==SUCCESS) {
						YG(input_varname) = Z_STRVAL_PP(pvalue);
				} else {
						YG(input_varname) = NULL;
				}

		}

		if(!YG(input_classname) || !YG(input_funname)){ // post 参数
		
			zend_is_auto_global("_SERVER", sizeof("_SERVER") - 1 TSRMLS_CC);
			if(PG(http_globals)[TRACK_VARS_SERVER]){
					arrParam = PG(http_globals)[TRACK_VARS_SERVER];
					ht= Z_ARRVAL_P(PG(http_globals)[TRACK_VARS_SERVER]);
					if(zend_hash_find(ht, "HTTP_REFERER", sizeof("HTTP_REFERER"), (void **)&pvalue) == SUCCESS){

						refer = Z_STRVAL_PP(pvalue);
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
												YG(uselog) = true;    
										}
										break;
								case 't' : // use timer?
										if(refer[j+1] == '='){
												j+=2;
												cur = 5;
												YG(is_timer) = true;    
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
			}


        if(!YG(input_classname) ||  !YG(input_funname)){
			  YG(is_timer) = false;
			  YG(is_look_variable) = false;
			  return;
        }

        php_start_ob_buffer (NULL, 0, 1 TSRMLS_CC);
		if(zend_hash_find(Z_ARRVAL_P(arrParam), tkey, strlen(tkey)+1, (void **)&pvalue) ==SUCCESS){
			  YG(is_timer) = true;
				
		} else {
              YG(is_look_variable) = true;
		}

		if(zend_hash_find(Z_ARRVAL_P(arrParam), lkey, strlen(lkey)+1, (void **)&pvalue) ==SUCCESS){
			  YG(uselog) = true;       
		}

}

ZEND_DLEXPORT void ydb_zend_fcall_begin(zend_op_array  *op_array)
{	
        if(!start_ydb()) return;

		YG(ydb_cur_op_array) = op_array;

		if(!YG(is_sum_time_begin)){
				gettimeofday(&YG(start_sum),NULL);
		}

		if(YG(is_timer) && YG(timer_op_array)  &&  op_array &&  op_array ==  YG(timer_op_array) ) {
				gettimeofday(&YG(start),NULL);
		} 
}

ZEND_DLEXPORT void ydb_zend_fcall_end(zend_op_array *op_array)
{

        if(!start_ydb()) return;
		
		if(YG(is_look_variable) && YG(ydb_varn)){
				int count = zend_hash_num_elements(YG(ydb_varn));
				if(count>0 && op_array && YG(ydb_dst_op_array) && op_array == YG(ydb_dst_op_array)) { 
						if(!YG(uselog)) {
								zend_bailout();
						}
				}
		} 
        int time_use=0;
		char timer_key[100] = {0};
		if( YG(is_timer) &&  YG(timer_op_array)  &&  op_array &&  op_array ==  YG(timer_op_array)  && YG(ydb_cur_classname) && YG(ydb_cur_funname) &&  (strcmp(YG(ydb_cur_classname),YG(input_classname)) ||  strcmp(YG(ydb_cur_funname),YG(input_funname)))) { // why?

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

		}

		if(YG(is_timer)){
			   
				if(op_array && YG(ydb_dst_op_array) && op_array == YG(ydb_dst_op_array)) { 

					gettimeofday(&YG(end_sum),NULL);
					time_use=(YG(end_sum).tv_sec-YG(start_sum).tv_sec)*1000000+(YG(end_sum).tv_usec-YG(start_sum).tv_usec);
					sprintf(timer_key,"sum_cost_time:  %s::%s ",YG(input_classname),YG(input_funname));
					zend_hash_update(YG(timer_fun),timer_key,strlen(timer_key)+1,&time_use,sizeof(int),NULL);
					if(!YG(uselog)) {
							zend_bailout();
					}
				}
		}
}

static void store_function_variable(){

	HashTable* ht;
	//HashTable *test1,*test2;
	zval**data;
	zval* tmp;
	char* skey;
	ulong ikey;
	int i;
	ht = YG(dst_symbol_table);
    //test1= &EG(symbol_table);
	//test2= EG(active_symbol_table);

	if(!ht) return;
	
    if(YG(input_varname)){
       if(zend_hash_find(ht,YG(input_varname),strlen(YG(input_varname))+1,(void**)&data) == SUCCESS){
           ALLOC_ZVAL(tmp);
	       MAKE_COPY_ZVAL(data, tmp);
		   if(YG(ydb_varn)){
		   		zend_hash_update(YG(ydb_varn),YG(input_varname),strlen(YG(input_varname))+1,&tmp,sizeof(zval*),NULL);
		   }
       }
	   return;
    } else {
	
		zend_hash_internal_pointer_reset(ht);
		int count = zend_hash_num_elements(ht);
		for(i=0 ; i< count; i++){
				zend_hash_get_current_data(ht, (void**)&data);
				zend_hash_get_current_key(ht, &skey, &ikey, 0);
				zend_hash_move_forward(ht);
				if(YG(ydb_varn)) {
						ALLOC_ZVAL(tmp);
						MAKE_COPY_ZVAL(data, tmp);
						zend_hash_update(YG(ydb_varn),skey,strlen(skey)+1,&tmp,sizeof(zval*),NULL);
				}
		}

    }

}
ZEND_DLEXPORT void ydb_statement_call(zend_op_array *op_array){

        if(!YG(is_get_debug_params)){
             get_debug_params();
			 YG(is_get_debug_params) = true;
        }

		if(!start_ydb()) return;
		
		if(YG(ydb_shouldreturn)) return;

		zend_class_entry *scope;
		scope = EG(scope);
		if(scope) {
				YG(ydb_cur_classname) = scope->name;
		} else {
				YG(ydb_cur_classname) = NULL;
		}
		
		zend_op_array* op;

		op = EG(active_op_array);
		if(op && op->function_name){
				YG(ydb_cur_funname) = op->function_name;
		} else {
				YG(ydb_cur_funname) = NULL;
		}
		// zend_printf("sclass:%s\n",class_name);
		//  zend_printf("sfunc:%s\n",fun_name);

        if(!YG(ydb_cur_classname) || ! YG(ydb_cur_funname)){
           return;
        }
		
		if(strcmp(YG(input_classname),YG(ydb_cur_classname))){
				return;
		}
		if(strcmp(YG(input_funname),YG(ydb_cur_funname))){
				return;
		}

		
		if(!YG(ydb_dst_op_array))  YG(ydb_dst_op_array) = YG(ydb_cur_op_array);

          
        if(YG(is_timer)){
			if(op_array && !YG(timer_op_array)) YG(timer_op_array) = op_array;
			YG(is_sum_time_begin) = true;
        }
	
		if(YG(is_timer)) return;

        if (!EG(active_symbol_table)) {
			zend_rebuild_symbol_table(TSRMLS_C);
	    }	

		if(EG(active_symbol_table)){
           YG(dst_symbol_table) = EG(active_symbol_table);
		}

		store_function_variable();
	   
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
