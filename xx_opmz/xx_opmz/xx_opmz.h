#pragma once

#include <stdio.h>


////////////////////////////////////////////////////////////////////////////////////////////////
#if 0

static ST_HANDLE64 st_handle_func[] =
{
	{0x0000,0,"entry   "            ,           vm_handle_0000},
	{0x0100,4,"push dreg           ",           vm_handle_0100},
	{0x0101,2,"push wreg           ",           vm_handle_0101},
	{0x0102,1,"push breg           ",           vm_handle_0102},
	{0x0103,8,"push qreg           ",           vm_handle_0103},
	{0x0200,4,"push dconst         ",           vm_handle_0200},
	{0x0201,2,"push wconst         ",           vm_handle_0201},
	{0x0202,1,"push bconst         ",           vm_handle_0202},
	{0x0203,8,"push qconst         ",           vm_handle_0203},
	{0x0300,4,"pop dreg            ",           vm_handle_0300},
	{0x0301,2,"pop wreg            ",           vm_handle_0301},
	{0x0302,1,"pop breg            ",           vm_handle_0302},
	{0x0303,8,"pop qreg            ",           vm_handle_0303},
	{0x0400,4,"add d[sp+4],d[sp]   ",           vm_handle_0400},
	{0x0401,2,"add w[sp+2],w[sp]   ",           vm_handle_0401},
	{0x0402,1,"add b[sp+2],b[sp]   ",           vm_handle_0402},
	{0x0403,8,"add q[sp+8],q[sp]   ",           vm_handle_0403},
	{0x0500,4,"mov d[sp],d[[sp]]   ",           vm_handle_0500},
	{0x0501,2,"mov w[sp],w[[sp]]   ",           vm_handle_0501},
	{0x0502,1,"mov w[sp],b[[sp]]   ",           vm_handle_0502},
	{0x0503,8,"mov q[sp],q[[sp]]   ",           vm_handle_0503},
	{0x0600,8,"mov d[[sp]],d[sp+4] ",           vm_handle_0600},
	{0x0610,4,"mov d[[sp]],d[sp+8] ",           vm_handle_0610},
	{0x0611,4,"mov w[[sp]],w[sp+8] ",           vm_handle_0611},
	{0x0613,4,"mov q[[sp]],q[sp+8] ",           vm_handle_0613},
	{0x0700,4,"push dsp            ",           vm_handle_0700},
	{0x0701,2,"push wsp            ",           vm_handle_0701},
	{0x0703,8,"push qsp            ",           vm_handle_0703},
	{0x0800,0,"calc                ",           vm_handle_0800},
	//{0x0801,"calc                ",           vm_handle_0801},
	{0x0900,4,"and ~d[sp+4],~d[sp] ",           vm_handle_0900},
	{0x0901,2,"and ~w[sp+2],~w[sp] ",           vm_handle_0901},
	{0x0902,1,"and ~b[sp+2],~b[sp] ",           vm_handle_0902},
	{0x0903,8,"and ~q[sp+8],~q[sp] ",           vm_handle_0903},
	{0x0a00,4,"div (d[sp+4],d[sp]),d[sp+8]   ", vm_handle_0a00},
	{0x0b00,4,"shr d[sp],b[sp+4]   ",           vm_handle_0b00},
	{0x0b01,2,"shr w[sp],b[sp+2]   ",           vm_handle_0b01},
	{0x0b02,1,"shr b[sp],b[sp+2]   ",           vm_handle_0b02},
	{0x0b03,8,"shr q[sp],b[sp+8]   ",           vm_handle_0b03},
	{0x0b10,4,"shrd d[sp],d[sp+4],b[sp+8]   ",  vm_handle_0b10},
	{0x0c00,"mul d[sp+4],d[sp]   ",           vm_handle_0c00},--
	{0x0c10,"imul d[sp+4],d[sp]  ",           vm_handle_0c10},--
	{0x0c23,8,"mul  q[sp+8],q[sp]  ",           vm_handle_0c23},--
	{0x0c33,8,"imul  q[sp+8],q[sp] ",           vm_handle_0c33},--
	{0x0d00,4,"or ~d[sp+4],~d[sp]  ",           vm_handle_0d00},
	{0x0d01,2,"or ~w[sp+2],~w[sp]  ",           vm_handle_0d01},
	{0x0d02,1,"or ~b[sp+2],~b[sp]  ",           vm_handle_0d02},
	{0x0d03,8,"or ~q[sp+8],~q[sp]  ",           vm_handle_0d03},
	{0x0e00,4,"shl d[sp],b[sp+4]   ",           vm_handle_0e00},
	{0x0e01,2,"shl w[sp],b[sp+2]   ",           vm_handle_0e01},
	{0x0e02,1,"shl b[sp],b[sp+2]   ",           vm_handle_0e02},
	{0x0e03,8,"shl q[sp],b[sp+8]   ",           vm_handle_0e03},
	{0x0e10,4,"shld d[sp],d[sp+4],b[sp+8]   ",  vm_handle_0e10},
	{0x0e11,8,"shld q[sp],q[sp+8],b[sp+10]   ",  vm_handle_0e11},
	{0x0f00,0,"jmp_1  ",                        vm_handle_0f00},
	{0x0f01,0,"jmp  ",                          vm_handle_0f01},
	{0x1000,0,"cpuid               ",           vm_handle_1000},
	{0x1001,0,"rdtsc               ",           vm_handle_1001},
	{0x1100,8,"mov sp,d[sp]         ",           vm_handle_1100},
	{0x1103,8,"mov sp,q[sp]         ",           vm_handle_1103},
	{0x1200,0,"ret   ",                         vm_handle_1200},
	{0x1300,8,"popfd               ",           vm_handle_1300},
	{0x1400,0,"call dreg           ",           vm_handle_1400},
};


#endif

/*
优化文件的数据结构
sdef,op_size可以不要，已知定义，这两个数据就已知，减小数据文件的大小
*/
struct ST_VM_INST
{
	//char paddr[0x20];   //指令地址
	//int addr_size;      //地址大小

	int idef;           //指令定义
	char var[0x10];     //常数，寄存器定义
	int seq;            //带自身序号，函数传参方便，共标记节点原数据使用
	int src_seq;        //原指令序号
	int opmz_flag;      //优化标志，0-未操作，1-已优化，2-不操作
	//char data[0x100];     //存储数据
};

struct ST_VM_INST_DATA
{
	//char paddr[0x20];   //指令地址
	char data[0x100];     //存储数据
};


/*
指令项文件的存储结构
每条指令只有一个结构，不论指令是否存在，都保留一个结构
指令项值结构
*/
struct ST_VM_ITEM_DATA
{
	//int seq; //指令序号，不用指令序号，结构索引就是序号
	int item_num; //项数目，最多3个项
	unsigned char flag_start[0x10];  //标志寄存器的值，执行前，如果有
	unsigned char flag_end[0x10];    //标志寄存器的值，执行后，如果有
	unsigned char var[0x3][0x10];    //变量，执行前的值
	unsigned char result[0x10];      //结果，执行后的值
};



/*变量的描述*/
struct VAR_DATA
{
	int idef;           //变量数字定义，类型的下级定义,1-reg;2-const;3-sp;4-[sp];5-[[sp]]
	char sdef[0x20];    //变量符号定义，
	//int  type;        //变量类型，，不使用，使用idef
	//char *exp;          //该节点生成的表达式 ，数据内部不使用 
	//int exp_size;       //表达式长度
	int op_size;
	char var[0x10];     //变量的值
	int offset;         //sp偏移值
	int start_offset;         //还原指令的sp偏移值，所有节点一样
	int vsize;          //变量的值大小，以字节为单位
	int sub_seq;        //如果是子节点，则有子节点的原指令序号，，从1开始，，0表示无，用来获取指令运算变量节点的信息
};

/*操作描述，操作的定义，操作的变量数目，操作的变量，操作的结果*/
struct VAR_OP
{
	int root_flag;      //root标志
	int  idef;          //操作数字定义，基本操作需要定义
	int src_seq;        //原指令序号
	int src_idef;       //原指令定义
	int src_item_flag;  //是由原指令的结果项还是操作项生成，结果项-1，操作项-2
	int src_item_seq;   //由原指令的哪一个操作项生成，操作项序号，0开始
	char sdef[0x20];    //操作符号定义
	//char sexp[0x20];    //操作表达式(数学)符号定义
	int ivar;           //变量数目，一般操作变量数不会太多
	int sub_flag;      //子指令标志，标记这个节点是否是子指令
	//int src_num;     //关联的原数据条数，最大6个
	//int src_seq[6];  //关联的原数据的序号
	//char vseq[0x20];    //变量的操作顺序，每一字节标识1个变量序号，会对应子节点的序号，默认到子节点序号
};

struct ST_OP_DATA
{
	struct VAR_DATA data;
	struct VAR_OP   op;
};



/*
还原后的指令结构
*/
struct XX_OPMZ_INST
{
	int idef;       //指令定义，如果是中间指令，则有该定义
	int start_seq;  //原指令起始序号
	int end_seq;    //原指令结束序号
	char asm[0x40]; //还原后的指令
};



/*
当前环境的位数大小，32位为4，64位为8
需要在启动前确定位数大小
*/
static int g_curt_sys = 4;



















