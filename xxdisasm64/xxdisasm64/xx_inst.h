#include <stdio.h>
#include "g_sql.h"
#include "xx_def.h"

#define uchar \
	unsigned char
#define int8 \
	signed char

#define ulong32 \
	unsigned long 

#define ulong64 \
	unsigned long long


#define true 1
#define false 0

#define INST_SYSTEM_UNEXIST 0  
#define INST_SYSTEM_16      2  
#define INST_SYSTEM_32      4   
#define INST_SYSTEM_64      8   

#define INST_ITEM_SIZE_8        1  
#define INST_ITEM_SIZE_16       2   
#define INST_ITEM_SIZE_32       4   
#define INST_ITEM_SIZE_48       6  
#define INST_ITEM_SIZE_64       8  
#define INST_ITEM_SIZE_80       10  
#define INST_ITEM_SIZE_128      16  
#define INST_ITEM_SIZE_256      32  

#define INST_FLAG_NOTOP    0   
#define INST_FLAG_EXIST    1  
#define INST_FLAG_UNEXIST  2  

#define INST_NEXTOP_PREFIX           	      1
#define INST_NEXTOP_ONE_OPCODE        	      2
#define INST_NEXTOP_TWO_OPCODE        	      3
#define INST_NEXTOP_THREE_OPCODE              4
#define INST_NEXTOP_EXT_ONE_OPCODE            5
#define INST_NEXTOP_EXT_TWO_OPCODE            6
#define INST_NEXTOP_EXT_THREE_OPCODE          7
#define INST_NEXTOP_MODRM                     8
#define INST_NEXTOP_SIB                       9
#define INST_NEXTOP_ITEMS                     10
#define INST_NEXTOP_FINISH                    11
#define INST_NEXTOP_REX           	      12



#define INST_ITEM_TYPE_REG      1 
#define INST_ITEM_TYPE_MREG     2 
#define INST_ITEM_TYPE_MIMM     3 
#define INST_ITEM_TYPE_MREGDIS  4 
#define INST_ITEM_TYPE_MDIS     11 
#define INST_ITEM_TYPE_MREGI    12 
#define INST_ITEM_TYPE_MREGIS   13  
#define INST_ITEM_TYPE_MREGIDIS  14 
#define INST_ITEM_TYPE_MREGISDIS  15 
#define INST_ITEM_TYPE_IADDR    16 
#define INST_ITEM_TYPE_MISDIS  17 
#define INST_ITEM_TYPE_MIDIS   18 
#define INST_ITEM_TYPE_DIS      5 
#define INST_ITEM_TYPE_IMM      6 
#define INST_ITEM_TYPE_M_REG    7 
#define INST_ITEM_TYPE_M_MMX    8 
#define INST_ITEM_TYPE_M_XMM    9 
#define INST_ITEM_TYPE_CONST    10 


#define INST_MAX_ITEMS         5    


#define INST_ITEM_MODRM_REG    1    
#define INST_ITEM_MODRM_RM     2    

#define INST_ITEM_SIB_BASE      1
#define INST_ITEM_SIB_INDEX     2


#define INST_ITEM_MODRM_REG_P    	11    
#define INST_ITEM_MODRM_REG_MMX    	12    
#define INST_ITEM_MODRM_REG_XMM    	13    

#define INST_ITEM_MODRM_RM_M     	21    
#define INST_ITEM_MODRM_RM_REG_P     	22    
#define INST_ITEM_MODRM_RM_REG_XMM     	23    
#define INST_ITEM_MODRM_RM_REG_MMX     	24    

#define INST_ITEM_REGTYPE_P     1   
#define INST_ITEM_REGTYPE_C     2   
#define INST_ITEM_REGTYPE_D     3   
#define INST_ITEM_REGTYPE_F     4   
#define INST_ITEM_REGTYPE_S     5   
#define INST_ITEM_REGTYPE_MMX   6   
#define INST_ITEM_REGTYPE_XMM   7   


#define LEFT_S_BRACKET   "["
#define RIGHT_S_BRACKET   "]"
#define LENGTH_S_BRACKET  1

#define PREFIX_LOCK    "lock"
#define PREFIX_REPNE   "repne"
#define PREFIX_REPE    "repe"
#define PREFIX_CS      "cs:"
#define PREFIX_SS      "ss:"
#define PREFIX_DS      "ds:"
#define PREFIX_ES      "es:"
#define PREFIX_FS      "fs:"
#define PREFIX_GS      "gs:"


#define INST_ERROR_UNDEFINE     1

#define SUPS_1A       "1A"
#define SUPS_1B       "1B"
#define SUPS_1C       "1C"
#define SUPS_I64      "i64"
#define SUPS_O64      "o64"
#define SUPS_D64      "d64"
#define SUPS_F64      "f64"
#define SUPS_V        "v"
#define SUPS_V1       "v1"

#define REG_SAME    1
#define REG_NSAME   2
#define REG_1_SUB   3
#define REG_2_SUB   4


struct XX_INST_TABLE
{
	struct XX_TBL_PREFIX              *tbl_prefix;
	struct XX_TBL_ONE_OPCODE          *tbl_one_opcode;
	struct XX_TBL_TWO_OPCODE          *tbl_two_opcode;
	struct XX_TBL_THREE_OPCODE        *tbl_three_opcode;
	struct XX_TBL_EXT_ONE_OPCODE      *tbl_ext_one_opcode;
	struct XX_TBL_EXT_TWO_OPCODE      *tbl_ext_two_opcode;
	struct XX_TBL_EXT_THREE_OPCODE    *tbl_ext_three_opcode;
	struct XX_TBL_FPU_OPCODE          *tbl_fpu_opcode;
	struct XX_TBL_MODRM               *tbl_modrm;
	struct XX_TBL_SIB                 *tbl_sib;	
	struct XX_TBL_ITEM_TYPE           *tbl_item_type;
	struct XX_TBL_ITEM_LTYPE          *tbl_item_ltype;
};

struct XX_INST_CODE
{
	int   nprefix;                  
	uchar prefix[10];                
	int   prefix_len;            
	uchar rex[10];                
	int   rex_len;            
	uchar first_opcode[1];          
	uchar second_opcode[1];         
	uchar third_opcode[1];          
	uchar ext_first_opcode[1];      
	uchar ext_second_opcode[1];     
	uchar ext_third_opcode[1];      
	uchar modrm[1];                 
	uchar sib[1];                   
	uchar opcode_type[20];            
	uchar modify_address[20];    
	uchar direct_address[20];     
	uchar displacement[20];       
	uchar immediate[20];         
	uchar base[20];               
	uchar current_addr[20];      
	uchar disasm[50];             
	uchar item_flag[20];
	uchar var_seq[50];
	uchar eflag[10];
	uchar sups[10];
	int fix_item;
	int disasm_length;        
	int inst_system;          
};

/////////////////////////////////////////
struct XX_INST_EXIST
{
int prefix_flag;                
int rex_flag;               
int rexw_flag;                
int rexr_flag;               
int rexx_flag;                
int rexb_flag;                
int next_op_flag;               
int opcode_flag;               
int one_opcode_flag;           
int two_opcode_flag;           
int three_opcode_flag;         
int ext_one_opcode_flag;              
int ext_two_opcode_flag;              
int ext_three_opcode_flag;        
int fpu_opcode_flag;             
int modrm_flag;                 
int sib_flag;                  
int modify_address_flag;       
int items_flag;   
int fix_item;
int finish_flag;             
};

////////////////////////////////////////////////////////

struct XX_INST_MIC
{
uchar prefix[20];              
uchar opcode_mic[20];          
uchar disasm[50];              
};

struct XX_INST_ITEMS_MIC
{
	uchar item_prefix[30];   
	uchar item_segment[30];   
	uchar item_self_mic[30];    
	uchar item_mic[30];      
};


struct XX_INST_ITEMS_FLAG
{
	int item_type;       	 
	int item_modrm_select;    
	int item_op_flag;        
	int item_reg_flag;       
	int item_seg_flag;       
	int iaddress_flag;       
	int maddress_flag;       
	int immediate_flag;     
	int displacement_flag;   
	int item_sign;          
	int item_addr;         
};



struct XX_INST_ITEMS_VAR
{
	int   item_addr_size;          
	int   item_data_size;      
	int   item_seg_size;          
	uchar item_reg_def[30];              
	int   reg_def_len;            
	uchar item_base_def[30];               
	int   base_def_len;             
	uchar item_index_def[30];           
	int   index_def_len;             
	int   index_scale;             
	uchar item_segment[20];       
	uchar item_const[50];      
	int   const_size;         
};

struct XX_INST_TBL_ITEMS
{
	uchar item_type[100];  
	uchar item_ltype[100]; 
};


struct XX_INST_ITEMS
{
	int    nitem;                    
	struct XX_INST_ITEMS_FLAG xx_inst_items_flag[INST_MAX_ITEMS]; 
	struct XX_INST_ITEMS_VAR  xx_inst_items_var[INST_MAX_ITEMS];  
	struct XX_INST_ITEMS_MIC  xx_inst_items_mic[INST_MAX_ITEMS];  
};

//////////////BP51/////////////////////////

struct XX_INST
{
struct XX_INST_TABLE  xx_inst_table;
struct XX_INST_CODE   xx_inst_code;
struct XX_INST_MIC    xx_inst_mic; 
struct XX_INST_EXIST  xx_inst_exist;
struct XX_INST_ITEMS  xx_inst_items;
};

///////////////////////////////////////////








