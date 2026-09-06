#include <stdio.h>

#include "xx_def.h"

#pragma once


struct XX_TBL_PREFIX
{
	uchar 	prefix_uuid[10+1];
	uchar 	prefix[2+1];
	uchar 	group[1+1];
};

struct XX_TBL_PREFIX_LEN
{
	uchar 	prefix_uuid[10+1];
	uchar 	prefix[10+1];
	uchar 	group[10+1];
};

struct XX_TBL_ONE_OPCODE
{
	uchar opcode1_uuid[10+1];
	uchar search_index[50+1];
	uchar opcode[2+1];
	uchar opcode_define_flag[1+1];
	uchar two_opcode_flag[1+1];
	uchar ext_opcode_flag[1+1];
	uchar fpu_opcode_flag[1+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_mic_2[50+1];
	uchar opcode_mic_3[50+1];
	uchar opcode_type[20+1];
	uchar segment[10+1];
	uchar op_items[1+1];
	uchar item_flag[20+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar citem1_type[10+1];
	uchar citem2_type[10+1];
	uchar citem3_type[10+1];
	uchar citem4_type[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[1+1];
	uchar rex_flag[1+1];
	uchar modify_address[1+1];
	uchar var_seq[40+1];
	uchar eflag[2+1];
};

struct XX_TBL_ONE_OPCODE_LEN
{
	uchar opcode1_uuid[10+1];
	uchar search_index[10+1];
	uchar opcode[10+1];
	uchar opcode_define_flag[10+1];
	uchar two_opcode_flag[10+1];
	uchar ext_opcode_flag[10+1];
	uchar fpu_opcode_flag[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_mic_2[10+1];
	uchar opcode_mic_3[10+1];
	uchar opcode_type[10+1];
	uchar segment[10+1];
	uchar op_items[10+1];
	uchar item_flag[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar citem1_type[10+1];
	uchar citem2_type[10+1];
	uchar citem3_type[10+1];
	uchar citem4_type[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[10+1];
	uchar rex_flag[10+1];
	uchar modify_address[10+1];
	uchar var_seq[10+1];
	uchar eflag[10+1];
};

struct XX_TBL_EXT_ONE_OPCODE
{
	uchar ext1_uuid[10+1];
	uchar search_index[50+1];
	uchar opcode[2+1];
	uchar opcode_define_flag[1+1];
	uchar ext_group[10+1];
	uchar mod_group[50+1];
	uchar prefix_group[2+1];
	uchar reg_opcode_group[2+1];	
	uchar rm_group[50+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_mic_2[50+1];
	uchar opcode_mic_3[50+1];
	uchar opcode_type[20+1];
	uchar segment[10+1];
	uchar op_items[1+1];
	uchar item_flag[20+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[1+1];
	uchar modify_address[1+1];
	uchar var_seq[40+1];
	uchar eflag[2+1];
};

struct XX_TBL_EXT_ONE_OPCODE_LEN
{
	uchar ext1_uuid[10+1];
	uchar search_index[10+1];
	uchar opcode[10+1];
	uchar opcode_define_flag[10+1];
	uchar ext_group[10+1];
	uchar mod_group[10+1];
	uchar prefix_group[10+1];
	uchar reg_opcode_group[10+1];	
	uchar rm_group[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_mic_2[10+1];
	uchar opcode_mic_3[10+1];
	uchar opcode_type[10+1];
	uchar segment[10+1];
	uchar op_items[10+1];
	uchar item_flag[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[10+1];
	uchar modify_address[10+1];
	uchar var_seq[10+1];
	uchar eflag[10+1];
};


struct XX_TBL_TWO_OPCODE
{
	uchar opcode2_uuid[10+1];
	uchar search_index[50+1];
	uchar first_opcode[2+1];
	uchar second_opcode[2+1];
	uchar prefix_group[50+1];
	uchar opcode_define_flag[1+1];
	uchar ext_opcode_flag[1+1];
	uchar three_opcode_flag[1+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_mic_2[50+1];
	uchar opcode_mic_3[50+1];
	uchar opcode_type[20+1];
	uchar segment[10+1];
	uchar op_items[1+1];
	uchar item_flag[20+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar citem1_type[10 + 1];
	uchar citem2_type[10 + 1];
	uchar citem3_type[10 + 1];
	uchar citem4_type[10 + 1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[1+1];
	uchar modify_address[1+1];
	uchar var_seq[40+1];
	uchar eflag[2+1];
};

struct XX_TBL_TWO_OPCODE_LEN
{
	uchar opcode2_uuid[10+1];
	uchar search_index[10+1];
	uchar first_opcode[10+1];
	uchar second_opcode[10+1];
	uchar prefix_group[10+1];
	uchar opcode_define_flag[10+1];
	uchar ext_opcode_flag[10+1];
	uchar three_opcode_flag[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_mic_2[10+1];
	uchar opcode_mic_3[10+1];
	uchar opcode_type[10+1];
	uchar segment[10+1];
	uchar op_items[10+1];
	uchar item_flag[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar citem1_type[10 + 1];
	uchar citem2_type[10 + 1];
	uchar citem3_type[10 + 1];
	uchar citem4_type[10 + 1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[10+1];
	uchar modify_address[10+1];
	uchar var_seq[10+1];
	uchar eflag[10+1];
};

struct XX_TBL_EXT_TWO_OPCODE
{
	uchar ext2_uuid[10+1];
	uchar search_index[50+1];
	uchar first_opcode[2+1];
	uchar second_opcode[2+1];
	uchar opcode_define_flag[1+1];
	uchar ext_group[10+1];
	uchar mod_group[50+1];
	uchar prefix_group[50+1];
	uchar reg_opcode_group[2+1];	
	uchar rm_group[50+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_mic_2[50+1];
	uchar opcode_mic_3[50+1];
	uchar opcode_type[20+1];
	uchar segment[10+1];
	uchar op_items[1+1];
	uchar item_flag[20+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[1+1];
	uchar modify_address[1+1];
	uchar var_seq[40+1];
	uchar eflag[2+1];
};

struct XX_TBL_EXT_TWO_OPCODE_LEN
{
	uchar ext2_uuid[10+1];
	uchar search_index[10+1];
	uchar first_opcode[10+1];
	uchar second_opcode[10+1];
	uchar opcode_define_flag[10+1];
	uchar ext_group[10+1];
	uchar mod_group[10+1];
	uchar prefix_group[10+1];
	uchar reg_opcode_group[10+1];	
	uchar rm_group[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_mic_2[10+1];
	uchar opcode_mic_3[10+1];
	uchar opcode_type[10+1];
	uchar segment[10+1];
	uchar op_items[10+1];
	uchar item_flag[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[10+1];
	uchar modify_address[10+1];
	uchar var_seq[10+1];
	uchar eflag[10+1];
};

struct XX_TBL_THREE_OPCODE
{
	uchar opcode3_uuid[10+1];
	uchar search_index[50+1];
	uchar first_opcode[2+1];
	uchar second_opcode[2+1];
	uchar third_opcode[2+1];
	uchar prefix_count[1+1];
	uchar prefix_group_1[2+1];
	uchar prefix_group_2[2+1];
	uchar opcode_define_flag[1+1];
	uchar ext_opcode_flag[1+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_mic_2[50+1];
	uchar opcode_mic_3[50+1];
	uchar opcode_type[20+1];
	uchar segment[10+1];
	uchar op_items[1+1];
	uchar item_flag[20+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[1+1];
	uchar modify_address[1+1];
	uchar var_seq[40+1];
	uchar eflag[2+1];
};

struct XX_TBL_THREE_OPCODE_LEN
{
	uchar opcode3_uuid[10+1];
	uchar search_index[10+1];
	uchar first_opcode[10+1];
	uchar second_opcode[10+1];
	uchar third_opcode[10+1];
	uchar prefix_count[10+1];
	uchar prefix_group_1[10+1];
	uchar prefix_group_2[10+1];
	uchar opcode_define_flag[10+1];
	uchar ext_opcode_flag[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_mic_2[10+1];
	uchar opcode_mic_3[10+1];
	uchar opcode_type[10+1];
	uchar segment[10+1];
	uchar op_items[10+1];
	uchar item_flag[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[10+1];
	uchar modify_address[10+1];
	uchar var_seq[10+1];
	uchar eflag[10+1];
};

struct XX_TBL_EXT_THREE_OPCODE
{
	uchar ext3_uuid[10+1];
	uchar search_index[50+1];
	uchar first_opcode[2+1];
	uchar second_opcode[2+1];
	uchar third_opcode[2+1];
	uchar opcode_define_flag[1+1];
	uchar ext_group[10+1];
	uchar mod_group[50+1];
	uchar prefix_group[2+1];
	uchar reg_opcode_group[2+1];	
	uchar rm_group[2+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_mic_2[50+1];
	uchar opcode_mic_3[50+1];
	uchar opcode_type[20+1];
	uchar segment[10+1];
	uchar op_items[1+1];
	uchar item_flag[20+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[1+1];
	uchar modify_address[1+1];
	uchar var_seq[40+1];
	uchar eflag[2+1];
};

struct XX_TBL_EXT_THREE_OPCODE_LEN
{
	uchar ext3_uuid[10+1];
	uchar search_index[10+1];
	uchar first_opcode[10+1];
	uchar second_opcode[10+1];
	uchar third_opcode[10+1];
	uchar opcode_define_flag[10+1];
	uchar ext_group[10+1];
	uchar mod_group[10+1];
	uchar prefix_group[10+1];
	uchar reg_opcode_group[10+1];	
	uchar rm_group[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_mic_2[10+1];
	uchar opcode_mic_3[10+1];
	uchar opcode_type[10+1];
	uchar segment[10+1];
	uchar op_items[10+1];
	uchar item_flag[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar superscripts[10+1];
	uchar modrm_exist_flag[10+1];
	uchar modify_address[10+1];
	uchar var_seq[10+1];
	uchar eflag[10+1];
};

struct XX_TBL_MODRM
{
	uchar modrm_uuid[10+1];
	uchar modrm[2+1];
	uchar mod[2+1];
	uchar mod_bitmic[10+1];
	uchar rm[2+1];
	uchar rm_bitmic[10+1];
	uchar sib_exist_flag[1+1];	
	uchar rm_type[1+1];
	uchar rm_sign[1+1];
	uchar rm_displacement_type[1+1];	
	uchar rm_def_p1[50+1];
	uchar rm_def_p2[50+1];
	uchar rm_def_p3[50+1];
	uchar rm_def_p4[50+1];
	uchar rm_def_cp1[50+1];
	uchar rm_def_cp2[50+1];
	uchar rm_def_cp3[50+1];
	uchar rm_def_cp4[50+1];
	uchar rm_def_mmx[50+1];
	uchar rm_def_xmm[50+1];
	uchar reg_opcode[2+1];
	uchar reg_opcode_bitmic[10+1];
	uchar reg_def_p1[50+1];
	uchar reg_def_p2[50+1];
	uchar reg_def_p3[50+1];
	uchar reg_def_p4[50+1];
	uchar reg_def_cp1[50+1];
	uchar reg_def_cp2[50+1];
	uchar reg_def_cp3[50+1];
	uchar reg_def_cp4[50+1];
	uchar reg_def_mmx[50+1];
	uchar reg_def_xmm[50+1];
	uchar reg_def_cr[50+1];
	uchar reg_def_dr[50+1];
	uchar reg_def_seg[50+1];
};

struct XX_TBL_MODRM_LEN
{
	uchar modrm_uuid[10+1];
	uchar modrm[10+1];
	uchar mod[10+1];
	uchar mod_bitmic[10+1];
	uchar rm[10+1];
	uchar rm_bitmic[10+1];
	uchar sib_exist_flag[10+1];	
	uchar rm_type[10+1];
	uchar rm_sign[10+1];
	uchar rm_displacement_type[10+1];	
	uchar rm_def_p1[10+1];
	uchar rm_def_p2[10+1];
	uchar rm_def_p3[10+1];
	uchar rm_def_p4[10+1];
	uchar rm_def_cp1[10+1];
	uchar rm_def_cp2[10+1];
	uchar rm_def_cp3[10+1];
	uchar rm_def_cp4[10+1];
	uchar rm_def_mmx[10+1];
	uchar rm_def_xmm[10+1];
	uchar reg_opcode[10+1];
	uchar reg_opcode_bitmic[10+1];
	uchar reg_def_p1[10+1];
	uchar reg_def_p2[10+1];
	uchar reg_def_p3[10+1];
	uchar reg_def_p4[10+1];
	uchar reg_def_cp1[10+1];
	uchar reg_def_cp2[10+1];
	uchar reg_def_cp3[10+1];
	uchar reg_def_cp4[10+1];
	uchar reg_def_mmx[10+1];
	uchar reg_def_xmm[10+1];
	uchar reg_def_cr[10+1];
	uchar reg_def_dr[10+1];
	uchar reg_def_seg[10+1];
};

struct XX_TBL_SIB
{
	uchar sib_uuid[10+1];
	uchar sib[2+1];
	uchar scale[2+1];
	uchar scale_bitmic[10+1];
	uchar index[2+1];
	uchar index_bitmic[10+1];
	uchar index_type[1+1];
	uchar index_sign[1+1];
	uchar index_scale_value[10+1];
	uchar index_mic[50+1];
	uchar index_cmic[50+1];
	uchar base[2+1];
	uchar base_bitmic[10+1];
	uchar base_special[1+1];
	uchar base_mic[50+1];
	uchar base_cmic[50+1];

};

struct XX_TBL_SIB_LEN
{
	uchar sib_uuid[10+1];
	uchar sib[10+1];
	uchar scale[10+1];
	uchar scale_bitmic[10+1];
	uchar index[10+1];
	uchar index_bitmic[10+1];
	uchar index_type[10+1];
	uchar index_sign[10+1];
	uchar index_scale_value[10+1];
	uchar index_mic[10+1];
	uchar index_cmic[10+1];
	uchar base[10+1];
	uchar base_bitmic[10+1];
	uchar base_special[10+1];
	uchar base_mic[10+1];
	uchar base_cmic[10+1];

};

struct XX_TBL_ITEM_TYPE
{
	uchar type_uuid[10+1];        
	uchar index_func[10+1];  
	uchar type_mic[10+1];  
	uchar modrm_exist[1+1];  
	uchar sib_exist[1+1];  
	uchar item_type[2+1];  
	uchar modrm_select[2+1]; 
	uchar mod_range[50+1]; 
	uchar reg_type[2+1];  
	uchar reg_def[50+1];  
	uchar type_size[2 + 1];
};

struct XX_TBL_ITEM_TYPE_LEN
{
	uchar type_uuid[10+1];  
	uchar index_func[10+1]; 
	uchar type_mic[10+1];   
	uchar modrm_exist[10+1]; 
	uchar sib_exist[10+1];   
	uchar item_type[10+1];  
	uchar modrm_select[10+1]; 
	uchar mod_range[10+1]; 
	uchar reg_type[10+1];  
	uchar reg_def[10+1];  
	uchar type_size[10 + 1];
};


struct XX_TBL_ITEM_LTYPE
{
	uchar ltype_uuid[10+1];    
	uchar ltype_mic[10+1];
	uchar length_type[10+1];
	uchar item16_addr_size[10+1];
	uchar item16_data_size[10+1];
	uchar item32_addr_size[10+1];
	uchar item32_data_size[10+1];
	uchar item64_addr_size[10+1];
	uchar item64_data_size[10+1];
	uchar seg_flag[1+1];
	uchar seg_size[10+1];
};

struct XX_TBL_ITEM_LTYPE_LEN
{
	uchar ltype_uuid[10+1];     
	uchar ltype_mic[10+1];
	uchar length_type[10+1];
	uchar item16_addr_size[10+1];
	uchar item16_data_size[10+1];
	uchar item32_addr_size[10+1];
	uchar item32_data_size[10+1];
	uchar item64_addr_size[10+1];
	uchar item64_data_size[10+1];
	uchar seg_flag[10+1];
	uchar seg_size[10+1];
};


struct XX_TBL_ITEM_TINDEX
{
	uchar item_tindex_uuid[10+1];
	uchar item_type[10+1];
	uchar type_index[10+1];
};



struct XX_INIT_TBL_LIST
{
	char tbl_name[100];
	char *ptbl;      
	char *pltbl;     
	int  item_size;
	int  litem_size;
	int  rows_num;
	int  lrows_num;
	char tbl_sql[100];
	char tbl_sql_length[100];
	char data_name[100];   
	char ldata_name[100]; 
};



struct XX_TBL_FPU_OPCODE
{
	uchar fpu_uuid[10+1];
	uchar search_index[50+1];
	uchar opcode[2+1];
	uchar modrm_flag[1+1];
	uchar mod_group[50+1];
	uchar reg_opcode_group[2+1];	
	uchar rm_group[50+1];
	uchar modrm[10+1];
	uchar modrm_reg_opcode[10+1];
	uchar opcode_mic_1[50+1];
	uchar opcode_type[20+1];
	uchar op_items[2+1];
	uchar item1_type[50+1];
	uchar item1_ltype[10+1];
	uchar item2_type[50+1];
	uchar item2_ltype[10+1];
	uchar item3_type[50+1];
	uchar item3_ltype[10+1];
	uchar item4_type[50+1];
	uchar item4_ltype[10+1];
	uchar modrm_exist_flag[1+1];
};

struct XX_TBL_FPU_OPCODE_LEN
{
	uchar fpu_uuid[10+1];
	uchar search_index[10+1];
	uchar opcode[10+1];
	uchar modrm_flag[10+1];
	uchar mod_group[10+1];
	uchar reg_opcode_group[10+1];	
	uchar rm_group[10+1];
	uchar modrm[10+1];
	uchar modrm_reg_opcode[10+1];
	uchar opcode_mic_1[10+1];
	uchar opcode_type[10+1];
	uchar op_items[10+1];
	uchar item1_type[10+1];
	uchar item1_ltype[10+1];
	uchar item2_type[10+1];
	uchar item2_ltype[10+1];
	uchar item3_type[10+1];
	uchar item3_ltype[10+1];
	uchar item4_type[10+1];
	uchar item4_ltype[10+1];
	uchar modrm_exist_flag[10+1];
};


