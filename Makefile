PHP_SRC=/usr/local/php/include/php   # php源代码目录

PHP_EXT_HOME=/usr/local/php/extensions  # 扩展部署目录


INCLUDE=-I$(PHP_SRC) -I$(PHP_SRC)/main -I$(PHP_SRC)/TSRM -I$(PHP_SRC)/Zend -I$(PHP_SRC)/SAPI
CC=gcc

ydb.so: ydb.o
	$(CC) -shared -rdynamic -o ydb.so ydb.o
ydb.o: ydb.c
	$(CC) -g -fpic  $(INCLUDE) -c ydb.c

clean:
	rm -fr *.so *.o

install: ydb.so
	sudo cp -fp ydb.so $(PHP_EXT_HOME)


