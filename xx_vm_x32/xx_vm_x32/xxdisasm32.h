#include <stdio.h>
//#include "xx_inst.h"
#pragma once




#ifdef __cplusplus
extern "C" {
#endif






#define _export   \
	 __declspec(dllexport)

#define DLL_PUBLIC   \
	 __declspec(dllexport)

#define uchar \
	unsigned char


#define ulong \
	unsigned long

#pragma pack(push)
#pragma pack(1)


#pragma comment(lib,"xxdisasm32.lib")



#define INST_SYSTEM_CURT 4







struct XX_TBL_PREFIX
{
	uchar 	prefix_uuid[10 + 1];
	uchar 	prefix[2 + 1];
	uchar 	group[1 + 1];
};

struct XX_TBL_PREFIX_LEN
{
	uchar 	prefix_uuid[10 + 1];
	uchar 	prefix[10 + 1];
	uchar 	group[10 + 1];
};

struct XX_TBL_ONE_OPCODE
{
	uchar opcode1_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar opcode[2 + 1];
	uchar opcode_define_flag[1 + 1];
	uchar two_opcode_flag[1 + 1];
	uchar ext_opcode_flag[1 + 1];
	uchar fpu_opcode_flag[1 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_mic_2[50 + 1];
	uchar opcode_mic_3[50 + 1];
	uchar opcode_type[20 + 1];
	uchar segment[10 + 1];
	uchar op_items[1 + 1];
	uchar item_flag[20 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar citem1_type[10 + 1];
	uchar citem2_type[10 + 1];
	uchar citem3_type[10 + 1];
	uchar citem4_type[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[1 + 1];
	uchar rex_flag[1 + 1];
	uchar modify_address[1 + 1];
	uchar var_seq[40 + 1];
	uchar eflag[2 + 1];
};

struct XX_TBL_ONE_OPCODE_LEN
{
	uchar opcode1_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar opcode[10 + 1];
	uchar opcode_define_flag[10 + 1];
	uchar two_opcode_flag[10 + 1];
	uchar ext_opcode_flag[10 + 1];
	uchar fpu_opcode_flag[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_mic_2[10 + 1];
	uchar opcode_mic_3[10 + 1];
	uchar opcode_type[10 + 1];
	uchar segment[10 + 1];
	uchar op_items[10 + 1];
	uchar item_flag[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar citem1_type[10 + 1];
	uchar citem2_type[10 + 1];
	uchar citem3_type[10 + 1];
	uchar citem4_type[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[10 + 1];
	uchar rex_flag[10 + 1];
	uchar modify_address[10 + 1];
	uchar var_seq[10 + 1];
	uchar eflag[10 + 1];
};

struct XX_TBL_EXT_ONE_OPCODE
{
	uchar ext1_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar opcode[2 + 1];
	uchar opcode_define_flag[1 + 1];
	uchar ext_group[10 + 1];
	uchar mod_group[50 + 1];
	uchar prefix_group[2 + 1];
	uchar reg_opcode_group[2 + 1];
	uchar rm_group[50 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_mic_2[50 + 1];
	uchar opcode_mic_3[50 + 1];
	uchar opcode_type[20 + 1];
	uchar segment[10 + 1];
	uchar op_items[1 + 1];
	uchar item_flag[20 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[1 + 1];
	uchar modify_address[1 + 1];
	uchar var_seq[40 + 1];
	uchar eflag[2 + 1];
};

struct XX_TBL_EXT_ONE_OPCODE_LEN
{
	uchar ext1_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar opcode[10 + 1];
	uchar opcode_define_flag[10 + 1];
	uchar ext_group[10 + 1];
	uchar mod_group[10 + 1];
	uchar prefix_group[10 + 1];
	uchar reg_opcode_group[10 + 1];
	uchar rm_group[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_mic_2[10 + 1];
	uchar opcode_mic_3[10 + 1];
	uchar opcode_type[10 + 1];
	uchar segment[10 + 1];
	uchar op_items[10 + 1];
	uchar item_flag[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[10 + 1];
	uchar modify_address[10 + 1];
	uchar var_seq[10 + 1];
	uchar eflag[10 + 1];
};


struct XX_TBL_TWO_OPCODE
{
	uchar opcode2_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar first_opcode[2 + 1];
	uchar second_opcode[2 + 1];
	uchar prefix_group[50 + 1];
	uchar opcode_define_flag[1 + 1];
	uchar ext_opcode_flag[1 + 1];
	uchar three_opcode_flag[1 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_mic_2[50 + 1];
	uchar opcode_mic_3[50 + 1];
	uchar opcode_type[20 + 1];
	uchar segment[10 + 1];
	uchar op_items[1 + 1];
	uchar item_flag[20 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar citem1_type[10 + 1];
	uchar citem2_type[10 + 1];
	uchar citem3_type[10 + 1];
	uchar citem4_type[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[1 + 1];
	uchar modify_address[1 + 1];
	uchar var_seq[40 + 1];
	uchar eflag[2 + 1];
};

struct XX_TBL_TWO_OPCODE_LEN
{
	uchar opcode2_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar first_opcode[10 + 1];
	uchar second_opcode[10 + 1];
	uchar prefix_group[10 + 1];
	uchar opcode_define_flag[10 + 1];
	uchar ext_opcode_flag[10 + 1];
	uchar three_opcode_flag[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_mic_2[10 + 1];
	uchar opcode_mic_3[10 + 1];
	uchar opcode_type[10 + 1];
	uchar segment[10 + 1];
	uchar op_items[10 + 1];
	uchar item_flag[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar citem1_type[10 + 1];
	uchar citem2_type[10 + 1];
	uchar citem3_type[10 + 1];
	uchar citem4_type[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[10 + 1];
	uchar modify_address[10 + 1];
	uchar var_seq[10 + 1];
	uchar eflag[10 + 1];
};

struct XX_TBL_EXT_TWO_OPCODE
{
	uchar ext2_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar first_opcode[2 + 1];
	uchar second_opcode[2 + 1];
	uchar opcode_define_flag[1 + 1];
	uchar ext_group[10 + 1];
	uchar mod_group[50 + 1];
	uchar prefix_group[50 + 1];
	uchar reg_opcode_group[2 + 1];
	uchar rm_group[50 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_mic_2[50 + 1];
	uchar opcode_mic_3[50 + 1];
	uchar opcode_type[20 + 1];
	uchar segment[10 + 1];
	uchar op_items[1 + 1];
	uchar item_flag[20 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[1 + 1];
	uchar modify_address[1 + 1];
	uchar var_seq[40 + 1];
	uchar eflag[2 + 1];
};

struct XX_TBL_EXT_TWO_OPCODE_LEN
{
	uchar ext2_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar first_opcode[10 + 1];
	uchar second_opcode[10 + 1];
	uchar opcode_define_flag[10 + 1];
	uchar ext_group[10 + 1];
	uchar mod_group[10 + 1];
	uchar prefix_group[10 + 1];
	uchar reg_opcode_group[10 + 1];
	uchar rm_group[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_mic_2[10 + 1];
	uchar opcode_mic_3[10 + 1];
	uchar opcode_type[10 + 1];
	uchar segment[10 + 1];
	uchar op_items[10 + 1];
	uchar item_flag[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[10 + 1];
	uchar modify_address[10 + 1];
	uchar var_seq[10 + 1];
	uchar eflag[10 + 1];
};

struct XX_TBL_THREE_OPCODE
{
	uchar opcode3_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar first_opcode[2 + 1];
	uchar second_opcode[2 + 1];
	uchar third_opcode[2 + 1];
	uchar prefix_count[1 + 1];
	uchar prefix_group_1[2 + 1];
	uchar prefix_group_2[2 + 1];
	uchar opcode_define_flag[1 + 1];
	uchar ext_opcode_flag[1 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_mic_2[50 + 1];
	uchar opcode_mic_3[50 + 1];
	uchar opcode_type[20 + 1];
	uchar segment[10 + 1];
	uchar op_items[1 + 1];
	uchar item_flag[20 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[1 + 1];
	uchar modify_address[1 + 1];
	uchar var_seq[40 + 1];
	uchar eflag[2 + 1];
};

struct XX_TBL_THREE_OPCODE_LEN
{
	uchar opcode3_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar first_opcode[10 + 1];
	uchar second_opcode[10 + 1];
	uchar third_opcode[10 + 1];
	uchar prefix_count[10 + 1];
	uchar prefix_group_1[10 + 1];
	uchar prefix_group_2[10 + 1];
	uchar opcode_define_flag[10 + 1];
	uchar ext_opcode_flag[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_mic_2[10 + 1];
	uchar opcode_mic_3[10 + 1];
	uchar opcode_type[10 + 1];
	uchar segment[10 + 1];
	uchar op_items[10 + 1];
	uchar item_flag[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[10 + 1];
	uchar modify_address[10 + 1];
	uchar var_seq[10 + 1];
	uchar eflag[10 + 1];
};

struct XX_TBL_EXT_THREE_OPCODE
{
	uchar ext3_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar first_opcode[2 + 1];
	uchar second_opcode[2 + 1];
	uchar third_opcode[2 + 1];
	uchar opcode_define_flag[1 + 1];
	uchar ext_group[10 + 1];
	uchar mod_group[50 + 1];
	uchar prefix_group[2 + 1];
	uchar reg_opcode_group[2 + 1];
	uchar rm_group[2 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_mic_2[50 + 1];
	uchar opcode_mic_3[50 + 1];
	uchar opcode_type[20 + 1];
	uchar segment[10 + 1];
	uchar op_items[1 + 1];
	uchar item_flag[20 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[1 + 1];
	uchar modify_address[1 + 1];
	uchar var_seq[40 + 1];
	uchar eflag[2 + 1];
};

struct XX_TBL_EXT_THREE_OPCODE_LEN
{
	uchar ext3_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar first_opcode[10 + 1];
	uchar second_opcode[10 + 1];
	uchar third_opcode[10 + 1];
	uchar opcode_define_flag[10 + 1];
	uchar ext_group[10 + 1];
	uchar mod_group[10 + 1];
	uchar prefix_group[10 + 1];
	uchar reg_opcode_group[10 + 1];
	uchar rm_group[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_mic_2[10 + 1];
	uchar opcode_mic_3[10 + 1];
	uchar opcode_type[10 + 1];
	uchar segment[10 + 1];
	uchar op_items[10 + 1];
	uchar item_flag[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar superscripts[10 + 1];
	uchar modrm_exist_flag[10 + 1];
	uchar modify_address[10 + 1];
	uchar var_seq[10 + 1];
	uchar eflag[10 + 1];
};

struct XX_TBL_MODRM
{
	uchar modrm_uuid[10 + 1];
	uchar modrm[2 + 1];
	uchar mod[2 + 1];
	uchar mod_bitmic[10 + 1];
	uchar rm[2 + 1];
	uchar rm_bitmic[10 + 1];
	uchar sib_exist_flag[1 + 1];
	uchar rm_type[1 + 1];
	uchar rm_sign[1 + 1];
	uchar rm_displacement_type[1 + 1];
	uchar rm_def_p1[50 + 1];
	uchar rm_def_p2[50 + 1];
	uchar rm_def_p3[50 + 1];
	uchar rm_def_p4[50 + 1];
	uchar rm_def_cp1[50 + 1];
	uchar rm_def_cp2[50 + 1];
	uchar rm_def_cp3[50 + 1];
	uchar rm_def_cp4[50 + 1];
	uchar rm_def_mmx[50 + 1];
	uchar rm_def_xmm[50 + 1];
	uchar reg_opcode[2 + 1];
	uchar reg_opcode_bitmic[10 + 1];
	uchar reg_def_p1[50 + 1];
	uchar reg_def_p2[50 + 1];
	uchar reg_def_p3[50 + 1];
	uchar reg_def_p4[50 + 1];
	uchar reg_def_cp1[50 + 1];
	uchar reg_def_cp2[50 + 1];
	uchar reg_def_cp3[50 + 1];
	uchar reg_def_cp4[50 + 1];
	uchar reg_def_mmx[50 + 1];
	uchar reg_def_xmm[50 + 1];
	uchar reg_def_cr[50 + 1];
	uchar reg_def_dr[50 + 1];
	uchar reg_def_seg[50 + 1];
};

struct XX_TBL_MODRM_LEN
{
	uchar modrm_uuid[10 + 1];
	uchar modrm[10 + 1];
	uchar mod[10 + 1];
	uchar mod_bitmic[10 + 1];
	uchar rm[10 + 1];
	uchar rm_bitmic[10 + 1];
	uchar sib_exist_flag[10 + 1];
	uchar rm_type[10 + 1];
	uchar rm_sign[10 + 1];
	uchar rm_displacement_type[10 + 1];
	uchar rm_def_p1[10 + 1];
	uchar rm_def_p2[10 + 1];
	uchar rm_def_p3[10 + 1];
	uchar rm_def_p4[10 + 1];
	uchar rm_def_cp1[10 + 1];
	uchar rm_def_cp2[10 + 1];
	uchar rm_def_cp3[10 + 1];
	uchar rm_def_cp4[10 + 1];
	uchar rm_def_mmx[10 + 1];
	uchar rm_def_xmm[10 + 1];
	uchar reg_opcode[10 + 1];
	uchar reg_opcode_bitmic[10 + 1];
	uchar reg_def_p1[10 + 1];
	uchar reg_def_p2[10 + 1];
	uchar reg_def_p3[10 + 1];
	uchar reg_def_p4[10 + 1];
	uchar reg_def_cp1[10 + 1];
	uchar reg_def_cp2[10 + 1];
	uchar reg_def_cp3[10 + 1];
	uchar reg_def_cp4[10 + 1];
	uchar reg_def_mmx[10 + 1];
	uchar reg_def_xmm[10 + 1];
	uchar reg_def_cr[10 + 1];
	uchar reg_def_dr[10 + 1];
	uchar reg_def_seg[10 + 1];
};

struct XX_TBL_SIB
{
	uchar sib_uuid[10 + 1];
	uchar sib[2 + 1];
	uchar scale[2 + 1];
	uchar scale_bitmic[10 + 1];
	uchar index[2 + 1];
	uchar index_bitmic[10 + 1];
	uchar index_type[1 + 1];
	uchar index_sign[1 + 1];
	uchar index_scale_value[10 + 1];
	uchar index_mic[50 + 1];
	uchar index_cmic[50 + 1];
	uchar base[2 + 1];
	uchar base_bitmic[10 + 1];
	uchar base_special[1 + 1];
	uchar base_mic[50 + 1];
	uchar base_cmic[50 + 1];

};

struct XX_TBL_SIB_LEN
{
	uchar sib_uuid[10 + 1];
	uchar sib[10 + 1];
	uchar scale[10 + 1];
	uchar scale_bitmic[10 + 1];
	uchar index[10 + 1];
	uchar index_bitmic[10 + 1];
	uchar index_type[10 + 1];
	uchar index_sign[10 + 1];
	uchar index_scale_value[10 + 1];
	uchar index_mic[10 + 1];
	uchar index_cmic[10 + 1];
	uchar base[10 + 1];
	uchar base_bitmic[10 + 1];
	uchar base_special[10 + 1];
	uchar base_mic[10 + 1];
	uchar base_cmic[10 + 1];

};

struct XX_TBL_ITEM_TYPE
{
	uchar type_uuid[10 + 1];
	uchar index_func[10 + 1];
	uchar type_mic[10 + 1];
	uchar modrm_exist[1 + 1];
	uchar sib_exist[1 + 1];
	uchar item_type[2 + 1];
	uchar modrm_select[2 + 1];
	uchar mod_range[50 + 1];
	uchar reg_type[2 + 1];
	uchar reg_def[50 + 1];
	uchar type_size[2 + 1];
};

struct XX_TBL_ITEM_TYPE_LEN
{
	uchar type_uuid[10 + 1];
	uchar index_func[10 + 1];
	uchar type_mic[10 + 1];
	uchar modrm_exist[10 + 1];
	uchar sib_exist[10 + 1];
	uchar item_type[10 + 1];
	uchar modrm_select[10 + 1];
	uchar mod_range[10 + 1];
	uchar reg_type[10 + 1];
	uchar reg_def[10 + 1];
	uchar type_size[10 + 1];
};


struct XX_TBL_ITEM_LTYPE
{
	uchar ltype_uuid[10 + 1];
	uchar ltype_mic[10 + 1];
	uchar length_type[10 + 1];
	uchar item16_addr_size[10 + 1];
	uchar item16_data_size[10 + 1];
	uchar item32_addr_size[10 + 1];
	uchar item32_data_size[10 + 1];
	uchar item64_addr_size[10 + 1];
	uchar item64_data_size[10 + 1];
	uchar seg_flag[1 + 1];
	uchar seg_size[10 + 1];
};

struct XX_TBL_ITEM_LTYPE_LEN
{
	uchar ltype_uuid[10 + 1];
	uchar ltype_mic[10 + 1];
	uchar length_type[10 + 1];
	uchar item16_addr_size[10 + 1];
	uchar item16_data_size[10 + 1];
	uchar item32_addr_size[10 + 1];
	uchar item32_data_size[10 + 1];
	uchar item64_addr_size[10 + 1];
	uchar item64_data_size[10 + 1];
	uchar seg_flag[10 + 1];
	uchar seg_size[10 + 1];
};


struct XX_TBL_ITEM_TINDEX
{
	uchar item_tindex_uuid[10 + 1];
	uchar item_type[10 + 1];
	uchar type_index[10 + 1];
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
	uchar fpu_uuid[10 + 1];
	uchar search_index[50 + 1];
	uchar opcode[2 + 1];
	uchar modrm_flag[1 + 1];
	uchar mod_group[50 + 1];
	uchar reg_opcode_group[2 + 1];
	uchar rm_group[50 + 1];
	uchar modrm[10 + 1];
	uchar modrm_reg_opcode[10 + 1];
	uchar opcode_mic_1[50 + 1];
	uchar opcode_type[20 + 1];
	uchar op_items[2 + 1];
	uchar item1_type[50 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[50 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[50 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[50 + 1];
	uchar item4_ltype[10 + 1];
	uchar modrm_exist_flag[1 + 1];
};

struct XX_TBL_FPU_OPCODE_LEN
{
	uchar fpu_uuid[10 + 1];
	uchar search_index[10 + 1];
	uchar opcode[10 + 1];
	uchar modrm_flag[10 + 1];
	uchar mod_group[10 + 1];
	uchar reg_opcode_group[10 + 1];
	uchar rm_group[10 + 1];
	uchar modrm[10 + 1];
	uchar modrm_reg_opcode[10 + 1];
	uchar opcode_mic_1[10 + 1];
	uchar opcode_type[10 + 1];
	uchar op_items[10 + 1];
	uchar item1_type[10 + 1];
	uchar item1_ltype[10 + 1];
	uchar item2_type[10 + 1];
	uchar item2_ltype[10 + 1];
	uchar item3_type[10 + 1];
	uchar item3_ltype[10 + 1];
	uchar item4_type[10 + 1];
	uchar item4_ltype[10 + 1];
	uchar modrm_exist_flag[10 + 1];
};








//////////////// xx_inst //////////////////////

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
#define PREFIX_CS      "cs"
#define PREFIX_SS      "ss"
#define PREFIX_DS      "ds"
#define PREFIX_ES      "es"
#define PREFIX_FS      "fs"
#define PREFIX_GS      "gs"


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


struct XX_INST
{
	struct XX_INST_TABLE  xx_inst_table;
	struct XX_INST_CODE   xx_inst_code;
	struct XX_INST_MIC    xx_inst_mic;
	struct XX_INST_EXIST  xx_inst_exist;
	struct XX_INST_ITEMS  xx_inst_items;
};




















/*一部分接口的返回码*/
#define REG_SAME    1
#define REG_NSAME   2
#define REG_1_SUB   3
#define REG_2_SUB   4


#define INST_MAX_LEN 100
/////////////////////////////////////



#define  XREG_AL    0x00000100 
#define  XREG_AH    0x00000101 
#define  XREG_AX    0x00000202 
#define  XREG_EAX   0x00000404 
#define  XREG_RAX   0x00000806 

#define  XREG_CL    0x00010100 
#define  XREG_CH    0x00010101 
#define  XREG_CX    0x00010202 
#define  XREG_ECX   0x00010404 
#define  XREG_RCX   0x00010806 

#define  XREG_DL    0x00020100 
#define  XREG_DH    0x00020101 
#define  XREG_DX    0x00020202 
#define  XREG_EDX   0x00020404 
#define  XREG_RDX   0x00020806 

#define  XREG_BL    0x00030100 
#define  XREG_BH    0x00030101 
#define  XREG_BX    0x00030202 
#define  XREG_EBX   0x00030404 
#define  XREG_RBX   0x00030806 

#define  XREG_SPL   0x00040100 
#define  XREG_SP    0x00040202 
#define  XREG_ESP   0x00040404 
#define  XREG_RSP   0x00040806 

#define  XREG_BPL   0x00050100 
#define  XREG_BP    0x00050202 
#define  XREG_EBP   0x00050404 
#define  XREG_RBP   0x00050806 

#define  XREG_SIL   0x00060100 
#define  XREG_SI    0x00060202 
#define  XREG_ESI   0x00060404 
#define  XREG_RSI   0x00060806 

#define  XREG_DIL   0x00070100 
#define  XREG_DI    0x00070202 
#define  XREG_EDI   0x00070404 
#define  XREG_RDI   0x00070806 


#define  XREG_R8L   0x01000100 
#define  XREG_R8W   0x01000202 
#define  XREG_R8D   0x01000404 
#define  XREG_R8    0x01000806 

#define  XREG_R9L   0x01010100 
#define  XREG_R9W   0x01010202 
#define  XREG_R9D   0x01010404 
#define  XREG_R9    0x01010806 

#define  XREG_R10L   0x01020100 
#define  XREG_R10W   0x01020202 
#define  XREG_R10D   0x01020404 
#define  XREG_R10    0x01020806 

#define  XREG_R11L   0x01030100 
#define  XREG_R11W   0x01030202 
#define  XREG_R11D   0x01030404 
#define  XREG_R11    0x01030806 

#define  XREG_R12L   0x01040100 
#define  XREG_R12W   0x01040202 
#define  XREG_R12D   0x01040404 
#define  XREG_R12    0x01040806 

#define  XREG_R13L   0x01050100 
#define  XREG_R13W   0x01050202 
#define  XREG_R13D   0x01050404 
#define  XREG_R13    0x01050806 

#define  XREG_R14L   0x01060100 
#define  XREG_R14W   0x01060202 
#define  XREG_R14D   0x01060404 
#define  XREG_R14    0x01060806 

#define  XREG_R15L   0x01070100 
#define  XREG_R15W   0x01070202 
#define  XREG_R15D   0x01070404 
#define  XREG_R15    0x01070806 

#define  XREG_CS   0x02000202
#define  XREG_DS   0x02010202
#define  XREG_SS   0x02020202
#define  XREG_ES   0x02030202
#define  XREG_FS   0x02040202
#define  XREG_GS   0x02050202

#define  XREG_CR032  0x03000404
#define  XREG_CR232  0x03020404
#define  XREG_CR332  0x03030404
#define  XREG_CR432  0x03040404


#define  XREG_CR064  0x03000806
#define  XREG_CR264  0x03020806
#define  XREG_CR364  0x03030806
#define  XREG_CR464  0x03040806

#define  XREG_DR032  0x04000404
#define  XREG_DR132  0x04010404
#define  XREG_DR232  0x04020404
#define  XREG_DR332  0x04030404
#define  XREG_DR632  0x04060404
#define  XREG_DR732  0x04070404


#define  XREG_DR064  0x04000806
#define  XREG_DR164  0x04010806
#define  XREG_DR264  0x04020806
#define  XREG_DR364  0x04030806
#define  XREG_DR664  0x04060806
#define  XREG_DR764  0x04070806


#define  XREG_MM0  0x05000806
#define  XREG_MM1  0x05010806
#define  XREG_MM2  0x05020806
#define  XREG_MM3  0x05030806
#define  XREG_MM4  0x05040806
#define  XREG_MM5  0x05050806
#define  XREG_MM6  0x05060806
#define  XREG_MM7  0x05070806


#define  XREG_XMM0   0x0600100a
#define  XREG_XMM1   0x0601100a
#define  XREG_XMM2   0x0602100a
#define  XREG_XMM3   0x0603100a
#define  XREG_XMM4   0x0604100a
#define  XREG_XMM5   0x0605100a
#define  XREG_XMM6   0x0606100a
#define  XREG_XMM7   0x0607100a
#define  XREG_XMM8   0x0608100a
#define  XREG_XMM9   0x0609100a
#define  XREG_XMM10  0x060a100a
#define  XREG_XMM11  0x060b100a
#define  XREG_XMM12  0x060c100a
#define  XREG_XMM13  0x060d100a
#define  XREG_XMM14  0x060e100a
#define  XREG_XMM15  0x060f100a


#define  XREG_YMM0   0x0700140c
#define  XREG_YMM1   0x0701140c
#define  XREG_YMM2   0x0702140c
#define  XREG_YMM3   0x0703140c
#define  XREG_YMM4   0x0704140c
#define  XREG_YMM5   0x0705140c
#define  XREG_YMM6   0x0706140c
#define  XREG_YMM7   0x0707140c
#define  XREG_YMM8   0x0708140c
#define  XREG_YMM9   0x0709140c
#define  XREG_YMM10  0x070a140c
#define  XREG_YMM11  0x070b140c
#define  XREG_YMM12  0x070c140c
#define  XREG_YMM13  0x070d140c
#define  XREG_YMM14  0x070e140c
#define  XREG_YMM15  0x070f140c


#define  XREG_ST0   0x08000a08
#define  XREG_ST1   0x08010a08
#define  XREG_ST2   0x08020a08
#define  XREG_ST3   0x08030a08
#define  XREG_ST4   0x08040a08
#define  XREG_ST5   0x08050a08
#define  XREG_ST6   0x08060a08
#define  XREG_ST7   0x08070a08









	/*不在使用*/
	///////////////////////


	/*获取上一个api的错误代码值*/
	int xx_get_last_err();


	/*获取上一个api的错误代码描述b文本*/
	char *xx_get_last_err_des();


	/*反汇编接口
	 *参数
	 *	1：数据
	 *	2：指令位数
	 *	3：模块地址
	 *	4：指令地址
	 *	5：已初始化的指令索引序号，从0开始
	 *
	 *返回值
	 *	1-成功，0-失败
	 *
	 *
	 * */
	int xxdisasm(uchar *pdata, int inst_system, ulong mod_base, ulong inst_addr, ulong seq);


	/*反汇编接口2
	 * 	地址参数为指针，适用于64位
	 *参数
	 *	1：数据
	 *	2：指令位数
	 *	3：模块地址的指针
	 *	4：指令地址的指针
	 *	5：已初始化的指令索引序号，从0开始
	 *
	 *返回值
	 *	1-成功，0-失败
	 *
	 *
	 * */
	int xxdisasm2(uchar *pdata, int inst_system, uchar *pmod_base, uchar *pinst_addr, ulong seq);


	/*反汇编引擎的内存空间释放
	 *
	 *
	 * */
	void xx_free();


	/*初始化指令空间
	 *参数
	 *	1：需要初始化的指令空间个数
	 *
	 *返回值
	 *	1-成功，0-失败
	 *
	 * */
	int xx_inst_init(ulong inst_count);



	/*释放已初始化的指令空间
	 *
	 *
	 * */
	void xx_inst_free();


	/*获取已初始化的指令空间个数
	 *
	 *返回值
	 *	已初始化的指令空间个数
	 *
	 * */
	int xx_inst_init_count();



	/*指令接口*/
	/*获取指令二进制数据的长度
	 *参数
	 *	1：指令索引序号
	 *返回值
	 *	1-成功，0-失败
	 *
	 * */
	int xx_get_inst_blength(ulong seq);


	/*获取指令的二进制数
	 *参数
	 *	1：指令索引序号
	 *	2：接受数据的缓冲区
	 *	3：缓冲区大小
	 *返回值
	 *	1-成功，0-失败
	 *
	 * */
	int xx_get_inst_data(ulong seq, uchar *buf, int bufsize);


	/*获取指令助记符文本的长度
	 *参数
	 *	1：指令索引序号
	 *返回值
	 *	成功，返回助记符文本长度
	 *	0-失败
	 *
	 *
	 * */
	int xx_get_inst_slength(ulong seq);


	/*获取指令的助记符文本
	 *参数
	 *	1：指令索引序号
	 *	2：接受数据的缓冲区
	 *	3：缓冲区大小
	 *返回值
	 *	1-成功，0-失败
	 *
	 * */
	int xx_get_inst_mic(ulong seq, uchar *buf, int bufsize);


	/*获取操作码定义
	 *参数
	 *	1：指令索引序号
	 *	2：操作码定义的接收指针
	 *返回值
	 *	1-成功，0-失败
	 *
	 *
	 * */
	int xx_get_op_type(ulong seq, ulong *op_def);


	/*获取操作码助记符
	 *参数
	 *	1：指令索引序号
	 *	2：操作码助记符接收缓冲区
	 *	3：缓冲区大小
	 *
	 *返回值
	 *	1-成功，0-失败
	 * */
	int xx_get_op_mic(ulong seq, uchar *buf, int bufsize);


	/*获取某个指令的高32位地址
	 *参数
	 *	1：指令索引序号
	 *	2：高32位地址的接收指针
	 *
	 *返回值
	 *	1-成功，0-失败
	 * */
	int xx_get_inst_haddr(ulong seq, ulong *phaddr);


	/*获取某个指令的低32位地址
	 *参数
	 *	1：指令索引序号
	 *	2：低32位地址的接收指针
	 *
	 *返回值
	 *	1-成功，0-失败
	 * */
	int xx_get_inst_laddr(ulong seq, ulong *pladdr);


	/*获取某个指令的模块地址的高32位地址
	 *参数
	 *	1：指令索引序号
	 *	2：高32位地址的接收指针
	 *
	 *返回值
	 *	1-成功，0-失败
	 * */
	int xx_get_inst_hbase(ulong seq, ulong *phbase);


	/*获取某个指令的模块地址的低32位地址
	 *参数
	 *	1：指令索引序号
	 *	2：高32位地址的接收指针
	 *
	 *返回值
	 *	1-成功，0-失败
	 * */
	int xx_get_inst_lbase(ulong seq, ulong *plbase);


	/*操作项接口*/
	/*获取操作项个数
	 *参数
	 *	1：指令索引序号
	 *	2：操作项个数的接收指针
	 *返回值
	 *	1-成功，0-失败
	 *
	 * */
	int xx_get_item_count(ulong seq, ulong *pitem_count);


	/*获取操作项类型
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *返回值
	 *	操作项类型。失败返回0
	 *
	 * */
	int xx_get_item_type(ulong seq, ulong item_seq);


	/*获取操作项助记符文本
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *	3：操作项助记符文本接收缓冲区
	 *	4：缓冲区大小
	 *返回值
	 *	1-成功，0-失败
	 *
	 * */
	int xx_get_item_mic(ulong seq, ulong item_seq, uchar *buf, int bufsize);


	/*获取操作项操作的数据大小
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *返回值
	 *	成功：返回操作项的数据大小，以字节为单位
	 *	失败：0
	 * */
	int xx_get_item_data_size(ulong seq, ulong item_seq);


	/*获取操作项的地址大小
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *返回值
	 *	成功：返回操作项的地址大小，以字节为单位
	 *	失败：0
	 * */
	int xx_get_item_addr_size(ulong seq, ulong item_seq);


	/*获取操作项中的常数值
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *	3：常数值的接收指针
	 *返回值
	 *	成功：1
	 *	失败：0
	 * */
	int xx_get_item_const(ulong seq, ulong item_seq, uchar *pconst);


	/*获取操作项中的精度值
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *	3：精度值的接收指针
	 *返回值
	 *	成功：1
	 *	失败：0
	 * */
	int xx_get_item_scale(ulong seq, ulong item_seq, ulong *pscale);


	/*获取操作项的索引寄存器定义
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *	3：索引寄存器定义的接收指针
	 *返回值
	 *	成功：1
	 *	失败：0
	 * */
	int xx_get_item_indexdef(ulong seq, ulong item_seq, ulong *pindex_def);


	/*获取操作项的基地址寄存器定义
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *	3：基地址寄存器定义的接收指针
	 *返回值
	 *	成功：1
	 *	失败：0
	 * */
	int xx_get_item_basedef(ulong seq, ulong item_seq, ulong *pbase_def);


	/*获取操作项的寄存器定义
	 *参数
	 *	1：指令索引序号
	 *	2：操作项索引序号
	 *	3：寄存器定义的接收指针
	 *返回值
	 *	成功：1
	 *	失败：0
	 * */
	int xx_get_item_regdef(ulong seq, ulong item_seq, ulong *preg_def);



	/*比较两个寄存器
	 *参数
	 *	1：寄存器1的定义
	 *	2：寄存器2的定义
	 *返回值
	 *      两个寄存器一样，  REG_SAME(1)
	 *	两个寄存器不一样  REG_NSAME(2)
	 *	寄存器1是寄存器2的子寄存器  REG_1_SUB(3)
	 *	寄存器2是寄存器1的子寄存器  REG_2_SUB(4)
	 *
	 * */
	int xx_cmp_reg(ulong reg_def1, ulong reg_def2);


	/*获取寄存器的助记符
	 *参数
	 *	1：寄存器的定义
	 *	2：寄存器助记符的接收缓冲区
	 *	3：缓冲区大小
	 *返回值
	 *	成功：1
	 *	失败：0
	 * */
	int xx_reg_mic(ulong reg_def, char *reg_mic, int mic_size);
	int xx_get_reg_mic(unsigned char *reg_def, char* reg_mic);

	void xx_disasm(struct XX_INST* ,uchar *codes);

	int  print_inst_code(ulong seq);
	int  print_inst_exist(ulong seq);
	int print_inst_mic(ulong seq);
	int print_inst_item(ulong seq);





#pragma pack(pop)





#ifdef __cplusplus
}
#endif


