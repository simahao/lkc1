# 关于initcode的区别

|                          | xv6-k210                                                     | lkc                                                          | lkc1                                                         |
| ------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| initcode的位置           | xv6-user/initcode.S                                          | user/src/runtest.c                                           | xv6-user/init.c                                              |
| 实现的思路               | initcode.S以汇编代码形式编写逻辑                             | 以c语言的方式编写                                            | 以c语言的方式编写                                            |
| shutdown如何调用         | 不支持                                                       | user/lib封装了一层_syscall，方便在runtest.c中使用shutdown    | 没有任何封装，所以将__syscall汇编代码移植到init.c中          |
| 如何生成16进制           | makefile中通过od命令将xv6-user/initcode.S编译好的代码生成16进制，然后输出到initcode.txt文件中，之后手工将内容拷贝到kernerl/proc.c中的initcode[]数组中 | 通过咱们自己编写的scripts/runtst.sh，将user/src/runtest.c生成的代码以16进制输出，并且直接输出到include/initcode.h中，在src/proc/pcb_life.c中引入initcode.h文件，放置在initcode[]数组中 | 通过咱们自己编写的runtst.sh，将xv6-user/init.c生成的代码以16进制输出，并且直接输出到kernel/include/initcode.h中，在kernel/proc.c中引入initcode.h文件，放置在initcode[]数组中 |
| 如何自动化生成initcode.h | make geninit<br />16进制会保存在initcode.txt中               | make runtest<br />16进制会保存在include/initcode.h中         | make test<br />16进制会保存在kernel/include/initcode.h中     |

