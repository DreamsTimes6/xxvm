#pragma once
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif
	/*
	读入日志文件，并优化
	参数1：日志目录
	参数2：调试器目录
	*/
	//HANDLE  xx_opmz_analyze(char *dbg_path,char *pinst_file, char *pinst_dfile, char *pexp_file, char *popmz_log);
	//void  xx_opmz_stop();


	/*
	检查更新还原数据文件
	*/


	/*
	两个文件
	指令信息文件
	指令数据文件
	*/
	struct ST_VM_INST
	{
		//char paddr[0x20];   //指令地址
		//int addr_size;      //地址大小

		int idef;           //指令定义
		char var[0x10];     //常数，寄存器定义
		int seq;            //带自身序号，函数传参方便，共标记节点原数据使用
		int src_seq;        //原指令序号，可能一个日志中包含多份日志
		int opmz_flag;      //优化标志
		//char data[0x100];     //存储数据
	};


	/*不使用*/
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





	int xx_opmz_data(char *pfile, int src_seq, int idef, char *var);


	/*
	写入指令数据项
	*/
	int xx_opmz_item_write(char *pdfile, struct ST_VM_ITEM_DATA *pitem_data);


	/*
	写入无效的指令和指令数据
	*/
	void xx_opmz_write_invalid(char *pfile, char *pdfile, int seq);




#ifdef __cplusplus
}
#endif
