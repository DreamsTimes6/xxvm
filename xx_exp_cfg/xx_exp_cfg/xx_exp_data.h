#pragma once
#include <stdio.h>

#define DEF_EXP_SIZE  0x100


/*
操作定义
1 - 赋值
2 - 加法
3 - 取反
4 - 或
5 - 与
6 - shr a, b
7 - shl a, b
8 - mov *
9 - mov * a, b

10 - cwd
11 - cdq

12 - shrd
13 - shld

14 - cdw
15 - cqd


变量定义
普通变量：
		1-reg;
		2-const;
		3-sp;
		4-[sp];
		5-[[sp]]
		6-flag
		7-[const]
*/

struct XX_EXP_VAR
{
	int idef;            //变量类型
	int flag;            //变量实际名称定值标志，定值参与匹配，因为有常数的情况
	int calc_var;        //运算变量序号，从0开始，-1表示无
	int calc_type;       //运算类型,同指令操作定义
	char sname[0x10];     //变量命名
	char svar[0x30];      //变量实际名称
	unsigned char ivar[0x10];       //变量值，常量或者寄存器索引
};

/*
增加变量运算转换，
1，由变量（序号）转换得到
2，运算类型
*/

/*
表达式定义
需要存储，使用固定空间
*/
struct XX_EXP
{
	//char pin_exp[0x100];    //输入的表达式，减小体积
	//int  exp_size;    //输入的表达式空间大小，通用也使用此大小，减小体积
	char pcomm_exp[0x100];  //通用表达式
	char pasm_inst[0x100];  //汇编指令
	int  var_num;     //变量数
	int  idef;       //指令定义，中间指令
	unsigned long lv_flag;  //等级标志，位标志
	int   sys_flag;    //位数标志,32位第一位为1，64位第二位为1
	int mark_flag;             //标记标志，只对非中间指令生效，1-只标记输出，2-标记所有，3-标记输出，临时标记其他
								//4-标记所有，不输出，以前的不使用
								//标记是否忽略这个指令，1-忽略
	int mark_place;			   //标记位置，并在此序号处生成优化结果，1-开始，2-结束，只对非中间指令生效
	//int restart_flag;          //重新开始标志，部分指令识别后，重新分析此指令段
	struct XX_EXP_VAR exp_var[8];  //变量 
};









