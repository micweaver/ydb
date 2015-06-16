ydb
===


a debug  tool for php like gdb, a php zend extension. you can view the variable value of running script, and it can profile your web script, it's better than the other tools like xdebug or xhprof

ydb 是像gdb一样的php调试工具，用扩展实现，可以在运行过程中查看变量值，也可以进行性能测试，而不用对源代码进行任何更改，就像xdebug和xhprof 中提供的功能，但使用更方便，ydb目前有如下几大功能：


    1.  查看任意变量值
    2.  查看函数执行耗时及排行
    3.  更改运行时变量值，可作为一种强大的单元测试工具
    4.  跨网络调试，对远程调用的模块进行分析
    5.  网络请求调用性能分析
    6.  dump代码到页面



所有信息查看都在url中输入参数进行，而不用更改源代码，更详细的文档介绍见 http://blog.csdn.net/micweaver/article/details/17891163




1. view the variable value 

  we can input the url in browser like this:
  
  "wenda.so.com/?c=IndexController&f=indexAction&v= guinness_qa"
  
  the c,v,v is your debug parameter,
  the ydb will dump the value of guinness_qa which's in the indexAction function  of class  IndexController on your webpage and
  stop the script,of course you can input the 'l' parameter to dump the variable value to a log file
  
  c: the class name;
  
  f: the functions name;
  
  v: the variable name,or if you do not provide this parameter,the ydb will dump all of the variable in the function
 
  
2. profile your script

   we can input the url in browser like this:
  
  "wenda.so.com/?c=IndexController&f=indexAction&t= 1"
  
   the parameter t stand for profiling your script,the ydb will display a time cost rank  for all the function in the indexAction function  of class  IndexController on your webpage and
  stop the script desc  descending order.
 
ydb未来将提供的功能：

1. 查看变量中间值
2. 多层次函数调用性能观察
3. 查看函数调用栈
4. 进行安全编码方面的检测调试
  
  


