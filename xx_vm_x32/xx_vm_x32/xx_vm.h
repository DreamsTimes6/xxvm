#include <stdio.h>




#pragma pack(push)
#pragma pack(1)


#define MAX_INST  400

//REG_INDEX
#define REG_EAX    0
#define REG_ECX    1
#define REG_EDX    2
#define REG_EBX    3
#define REG_ESP    4
#define REG_EBP    5
#define REG_ESI    6
#define REG_EDI    7


int get_reg_index(char *reg_def);


struct XX_CONTEXT
{
	unsigned long r[8];
	unsigned long eflag;
	unsigned long ip;
};


struct VM_REG_STATUS
{
	int entry_flag;
	int push_num;           //有效个数
	int pop_num;
	int reg_status[100];     //寄存器当前在堆栈或者索引,0无效，1有效
	char reg_def[100][20];       //个数由push_num决定
	int reg_index[100];      //个数由pop_num决定
};


struct XX_HTHREAD
{
	struct XX_INST *xx_inst;
	struct XX_CONTEXT *xx_context;
	int inst_num;
	char *plogdata;
};

typedef struct 
{
	int            seq_handle;
	char 		   str_handle[100];
	int            (*func_handle)(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int, char*);
}ST_HANDLE;

typedef struct 
{
	int            seq_handle;
	char 		   str_handle[100];
	int            (*func_shandle)(struct XX_INST *xx_inst,int num,char *outs,int);
}ST_SHANDLE;

 struct  XX_FMEM
{
	unsigned int start;
	unsigned int end;
	char filename[500];
};

 struct  ITEM_VAR
{
	unsigned int addr;
	unsigned int var;
	int  size;
	int flag;
};


 struct XX_MOD
 {
	 unsigned  long base;
	 unsigned  long size;
	 unsigned  long entry;
	 int sectionCount;
	 char name[255];
	 char path[255];
 };



 char debug_buf[500];

#pragma pack(pop)






