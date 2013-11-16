ydb
===


a debug tool for php like gdb, you can look the variable value of running script, and it can profile your web script, it's better than the other tools like xdebug or xhprof

像gdb一样的php调试工具，在运行过程中查看变量值，也可以进行性能测试，就像xdebug和xhprof中提供的功能，但使用更方便




1. look the variable value 

  we can input the url in browser like this:
  
  "wenda.so.com/?c=IndexController&f=indexAction&v= guinness_qa"
  
  the c,v,v is your debug parameter,
  the ydb will dump the value of guinness_qa which's in the indexAction function  of class  IndexController on your webpage and
  stop the script,of course you can input the 'l' parameter to dump the variable value to a log file
  
  c: the class name;
  
  f: the functions name;
  
  v: the variable name,or if you do not provide this parameter,the ydb will dump all of the variable;
  
  
2. profile your script

   we can input the url in browser like this:
  
  "wenda.so.com/?c=IndexController&f=indexAction&t= 1"
  
   the parameter t stand for profile your script,the ydb will display a time rank desc for all the function in the indexAction function  of class  IndexController on your webpage and
  stop the script.
  
  
  

  
  
  



