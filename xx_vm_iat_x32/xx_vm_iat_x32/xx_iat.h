#include <stdio.h>



#pragma pack(push)
#pragma pack(1)


#define MAX_INST  100
//#define INST_SYSTEM_CURT 4

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


 struct XX_HTHREAD
 {
	 struct XX_INST *xx_inst;
	 struct XX_CONTEXT *xx_context;
	 int inst_num;
	 char *plogdata;
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


