#pragma once

#include <stdio.h>
#include "xx_def.h"

#pragma pack(push)
#pragma pack(1)



#define MAX_INST  400
#define INST_SYSTEM_CURT 8

//REG_INDEX
#define REG_RAX    0
#define REG_RCX    1
#define REG_RDX    2
#define REG_RBX    3
#define REG_RSP    4
#define REG_RBP    5
#define REG_RSI    6
#define REG_RDI    7
#define REG_R8     8
#define REG_R9     9
#define REG_R10    10
#define REG_R11    11
#define REG_R12    12
#define REG_R13    13
#define REG_R14    14
#define REG_R15    15
///////////////////////////////////

int get_reg_index(char *reg_def);


struct XX_CONTEXT64
{
	unsigned long long r[16];
	unsigned long long eflag;
	unsigned long long ip;
};


struct VM_REG_STATUS
{
	int entry_flag;
	int push_num;          
	int pop_num;
	int reg_status[100];    
	char reg_def[100][20];    
	int reg_index[100];    
};


struct XX_HTHREAD64
{
	struct XX_INST *xx_inst;
	struct XX_CONTEXT64 *xx_context;
	int inst_num;
	char *plogdata;
};

typedef struct 
{
	int            seq_handle;
	int            data_size;  //δʹ��
	char 		   str_handle[100];
	int(*func_handle)(struct XX_INST *, int, struct XX_CONTEXT64 *, char *, int, char*);
}ST_HANDLE64;

typedef struct 
{
	int            seq_handle;
	char 		   str_handle[100];
	int            (*func_shandle)(struct XX_INST *xx_inst,int num,char *outs,int);
}ST_SHANDLE64;

 struct  XX_FMEM64
{
	unsigned long long start;
	unsigned long long end;
	char filename[500];
};

 struct  ITEM_VAR64
{
	unsigned long long addr;
	unsigned long long var;
	unsigned long long  size;
	unsigned long long flag;
};

 struct  OUT_VAR64
 {
	 unsigned long long o_var[4];
	 unsigned long long o_flag;
 };
	 


 struct XX_MOD
 {
	 unsigned long long base;
	 unsigned long long size;
	 unsigned long long entry;
	 int sectionCount;
	 char name[255];
	 char path[255];
 };

 extern char debug_buf[500];


#pragma pack(pop)







