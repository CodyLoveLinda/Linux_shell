#coding: utf-8

import _thread

# 写个斐波那契数列计算函数，用于消耗cpu资源
def fibbo(number):
    if number <= 2:
        return 1
    else:
        return fibbo(number - 1) + fibbo(number - 2)

# 运行函数消耗cpu资源
#fibbo(80)
# 创建两个线程
try:
   _thread.start_new_thread( fibbo, ( 80, ) )
   _thread.start_new_thread( fibbo, ( 80, ) )
except:
   print ("Error: 无法启动线程")

while 1:
   pass 
