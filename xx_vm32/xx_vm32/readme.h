/*
针对代码数据复用，完整性校验，
1，代码流重定向
2，修改校验的地方，原代码引用替换为定值，
3，处理所有虚拟函数，没有地方调用原虚拟区段代码
4，去掉原虚拟代码区段，增加重定向代码区段


可能的问题
体积增大，
代码写入的正确性
内存地址的表示方法
根据vm_opcode运算出的立即数的表示方法,
	虚拟代码块,vm_opcode作为输入或者立即数作为输入
指令块的连接


体积增大优化
去掉无效代码
去掉跳转，虚拟代码块的连接代码
必须去掉分支代码，否则会回到原代码流程


代码执行流平坦，无分支
所有原虚拟代码块互相独立无依赖


标记有效代码或者标记无效代码


单步执行，速度太慢,
使用trace功能获取指令块，分析后写入,
trace  条件,eip在模块范围内,  eip==edi,,


完整性校验干掉的必要性，，


需要处理的地方

返回到原代码的处理，原代码不记录(插件完成此功能)

原代码执行完返回到重建代码处，原代码返回时栈顶是原虚拟代码，

完整性校验值计算的下一块代码地址,1,返回原代码，2，返回虚拟代码
	(完整性校验值（两处）处理后，
	vm_retn后面插入一个自定义函数，如果返回地址在原代码处，不处理，如果在虚拟代码处，去掉retn)

cpuid校验值计算的jmp，关联指令片段连接（ebx）（手动处理）

可移植性，不能有新内存的绝对地址，只使用偏移 

执行前完整性校验（断点）


原代码的逻辑无法复制,代码块的连接
//////////////////////////////////////////
简易handle识别，还原vm代码

识别操作逻辑

1.push 
	sp-4
2.pop 
	sp+4
3.add [sp+4],[sp]
	sp,[sp+4]=[sp]+[sp+4],[sp]=eflag
4.mov [sp],[[sp]]
	sp,[sp]变化



xxdisasm 一字节对其#pragma pack(1)






//////////////////////// resource /////////////////////////
type_name_num  n=0    n<type_name_num
type_name[n]


type_name[n]_idnum  m=0   m<type_name[n]_idnum
type_name[n]_id[m]


type_int_num  n=0    n<type_int_num
type_int[n]


type_int[n]_idnum  m=0   m<type_int[n]_idnum
type_int[n]_id[m]


//////////////////////////////////////////////////////////////
获取type_name_num
获取所有type_name

获取type_name[n]的tyoe_name_num
获取type_name[n]的所有type_name[n]_id



获取type_int_num
获取所有type_int

获取type_int[n]的tyoe_int_num
获取type_int[n]的所有type_int[n]_id

///////////////////////////////////////////////////////////////
demo1
type id number   
index=number-1
a=a8c3   b=a8c7
    !(!(!b | !b) | !a) | !(!(!a | !a) | !b)
=!(b | !a) | !(a | !b)
=(a & !b) | (!a & b)
=a&!b | !a&b

[00430f24] = 01043d88
[01043d88 + 30] = 01043f48 

01043f48 + 4 = 01043f4c
[01043f4c]=41dda8c3

01043f48 + 2c = 01043f74
[01043f74] = 005d8924 
005d8924 + 4 = 005d8928
[005d8928] = a8c7

//////////////////
f(a,b)=a&!b | !a&b

typei_id_k1=w[[[res_base]+30]+4]
typei_id_k2=w[[[[res_base]+30]+2c]+4]
typei_id_n=f(typei_id_k1,typei_id_k2)


typei_sid_k1=w[[[res_base]+30]+4]
typei_sid_k2=w[[[[res_base]+30]+2c]+38+4]
typei_sid_n=f(typei_sid_k1,typei_sid_k2)


typei_id_addr=


typen_id_k1=w[[[res_base]+30]+4]
typen_id_k2=w[[[[res_base]+30]+2c]]
typen_id_n=f(typen_id_k1,typen_id_k2)


typen_sid_k1=w[[[res_base]+30]+4]
typen_sid_k2=w[[[[res_base]+30]+2c]+a0+4]
typen_sid_n=f(typen_sid_k1,typen_sid_k2)


typen_idn_addr_k1=w[[[res_base]+30]+4]
typen_idn_addr_k2=w[[[[res_base]+30]+2c]+m*2^3+8]
typen_idn_addr=f(typen_id_addr_k1,typen_id_addr_k2)+enc_base

*/





