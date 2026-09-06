#pragma once
#include <stdio.h>
#include "xx_def.h"

/*
可能要对伪指令定义一些属性，直接写成case返回属性
	定义指令属性
	int idef;       指令数字定义
	char sdef[0x30];      指令文本
	int op_idef;    操作数字定义
	char op_sdef[0x30];   操作文本
	int tree_analyze;   是否开始分析标志，1-开始，0-否
	int tree_continue;  是否继续分析标志，（分析树存在，是否继续分析）1-继续，0-否
	int merge_flag;    合并后下一步操作标志，1-继续，0-否
	int sp_offset;  执行后sp变化
	int item_num;   操作项个数


	操作项属性
	int item_idef;   操作项数字定义
	char item_sdef[0x30];  操作项文本
	int item_size;   操作项大小
	int offset;      操作项内存偏移，如果为内存地址



操作定义
1-赋值
2-加法
3-取反
4-或
5-与
6-shr a,b
7-shl a,b
8-mov*
9-mov *a,b

10-cwd
11-cdq

12-shrd
13-shld

14-cdw1
15-cqd1

16-cdw2
17-cqd2

18-mul
19-imul

20-div
21-idiv

19-*
20-/

变量定义
	普通变量：
		1-reg;
		2-const;
		3-sp;
		4-[sp];
		5-[[sp]]
		6-flag
		7-[const]
		8-[sp]可忽略的虚值

	指令运算变量：
		0xa000:取反指令运算变量，大小4
		0xa001:取反指令运算变量，大小1
		0xa002:取反指令运算变量，大小2
		0xa003:取反指令运算变量，大小8
*/
///////////////////////////////////////////////////////////////////////////////////////////////
struct XX_DEF_ITEM
{
	int item_inst;         //操作项指令定义，用于子指令的生成

	int	chg_flag;			//操作项指令定义，修改为操作项可变化的标志，可拆分-第一位为1，可扩展-第二位为2
	int sep_idef;          //拆分指令定义，
	int expd_idef1;          //扩展指令定义1，
	int expd_idef2;          //扩展指令定义2，
	int item_idef;         //项数字定义
	char item_sdef[0x20];  //项文本
	int item_size;         //项大小
	int offset;            //项内存偏移，如果为内存地址，执行前的sp偏移，可以转化为执行后的sp偏移
};

struct XX_DEF_INST
{
	int idef;             //指令数字定义
	char sdef[0x30];      //指令文本
	int op_idef;          //操作数字定义
	char op_sdef[0x20];   //操作文本
	//char op_sexp[0x20];   //操作表达式(数学)符号定义

	int tree_op;          //是否参与分析树生成，1-是，0-否
	int tree_analyze;     //是否开始分析标志，1 - 开始，0 - 否
	int tree_continue;    //是否继续分析标志，（分析树存在，是否继续分析）1 - 继续;2-只操作sp，不生成树;3-退出
	int merge_flag;       //合并失败后下一步操作标志，1 - 忽略合并的错误，0-错误退出，2-释放当前树，下一条指令
	int rflag;            //指令有rflag标志
	//int mark_flag;        //优化标记标志，有中间指令时使用，1：保留该中间指令，2：

	int sp_offset;        //执行后的sp变化，要获得执行前的sp变化，加负号

	int expd1_idef;        //结果项扩展定义1，
	int expd2_idef;        //结果项扩展定义2，

	int mark_flag;        //该指令为必须标记指令

	int item_num;         //操作项个数

	struct XX_DEF_ITEM result; //结果项
	struct XX_DEF_ITEM item[4];   //操作项
};






