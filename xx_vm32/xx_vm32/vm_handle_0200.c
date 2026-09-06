#include "stdio.h"
//#include "Plugin.h"
#include "xx_opdef3.h"
#include "xx_vm.h"
#include "xxdisasm32.h"
#include "xx_opmz.h"


extern char xreg_rax[];
extern char xreg_rbx[];
extern char xreg_rcx[];
extern char xreg_rdx[];
extern char xreg_rbp[];
extern char xreg_rsp[];
extern char xreg_rsi[];
extern char xreg_rdi[];

extern char xreg_eax[];
extern char xreg_ebx[];
extern char xreg_ecx[];
extern char xreg_edx[];
extern char xreg_ebp[];
extern char xreg_esp[];
extern char xreg_esi[];
extern char xreg_edi[];

extern char xreg_ax[];
extern char xreg_bx[];
extern char xreg_cx[];
extern char xreg_dx[];
extern char xreg_bp[];
extern char xreg_sp[];
extern char xreg_si[];
extern char xreg_di[];

extern char xreg_al[];
extern char xreg_bl[];
extern char xreg_cl[];
extern char xreg_dl[];

extern char xreg_ah[];
extern char xreg_bh[];
extern char xreg_ch[];
extern char xreg_dh[];


extern int xreg_cmp(char* buf1,char* buf2);
extern int xreg_incmp(char* buf1,char* buf2);

extern void    cdecl _Addtolist(long addr,int highlight,char *format,...);

extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];
extern char vm_oip[];

extern char opmz_dfile[];  //优化数据文件名

/*
mov t0,dconst
sub sp,4 ; lea sp,[sp+fc]
mov [sp+00],t0

=============
*/
int vm_handle_0200(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int outs_size,char *out_var)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	int o_cnd=0;
	int index=0;

	char t0[20];
	char tmp[100];
	struct ST_VM_ITEM_DATA item_data;
	


	memset(&item_data, 0, sizeof(item_data));
	//_Addtolist(0,1,"vm_handle_0200");
	memset(outs,0,outs_size);
	/*sub sp,4 ; lea sp,[sp+fc]*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_sub,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_sp)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM )
				{
					/*检查第二个操作项寄存器*/
					if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x04)
					{
						//_Addtolist(0,1,"second condition inst_num:[%d]",n);
						s_cnd=1;
					}
				}
			}
		}
	}

	/*lea sp,[sp+fc]*/
	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_lea,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_sp)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS )
				{
					/*检查第二个操作项寄存器*/
					if(xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_sp)==0)
					{
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0xfc)
						{
							//_Addtolist(0,1,"second condition inst_num:[%d]",n);
							s_cnd=1;
						}
					}
				}
			}
		}
	}
	if(s_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	/*mov [sp+00],t0*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_MREGDIS &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_sp)==0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const[0]==0x00 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size==INST_ITEM_SIZE_32)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_32)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[1].reg_def_len);
					
					memset(tmp,0,sizeof(tmp));
					sprintf(outs+strlen(outs),"dconst=");
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs+strlen(outs),"%08x  ",xx_contect[n].r[index]);

					//sprintf(outs+strlen(outs),"  ");
					/*out_var，常数或者寄存器*/
					memcpy(out_var, &(xx_contect[n].r[index]), sizeof(xx_contect[n].r[index]));

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					t_cnd=1;

					item_data.item_num = 1;
					memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
					memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					memcpy(item_data.var[0], &xx_contect[n].r[index], sizeof(xx_contect[n].r[index]));

					memcpy(item_data.result, &xx_contect[n].r[index], sizeof(xx_contect[n].r[index]));

					break;
				}
			}
			else if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_MREG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_sp)==0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size==INST_ITEM_SIZE_32)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_32)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[1].reg_def_len);

					memset(tmp,0,sizeof(tmp));
					sprintf(outs+strlen(outs),"dconst=");
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs+strlen(outs),"%08x  ",xx_contect[n].r[index]);

					//sprintf(outs+strlen(outs),"  ");
					/*out_var，常数或者寄存器*/
					memcpy(out_var, &(xx_contect[n].r[index]), sizeof(xx_contect[n].r[index]));

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					t_cnd=1;

					item_data.item_num = 1;
					memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
					memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					memcpy(item_data.var[0], &xx_contect[n].r[index], sizeof(xx_contect[n].r[index]));

					memcpy(item_data.result, &xx_contect[n].r[REG_ESP], sizeof(xx_contect[n].r[REG_ESP]));

					break;
				}
			}
		}
	}
	if(t_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*有两个 mov t0,d[vm_opcode+00]*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0 ||
			memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_movzx,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG )
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_opcode)==0)
				{
					/*检查第二个操作项寄存器*/
					//_Addtolist(0,1,"other condition inst_num:[%d]",n);
					o_cnd++;
				}
				else if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_opcode)==0)
				{
					/*检查第二个操作项寄存器*/
					//_Addtolist(0,1,"other condition inst_num:[%d]",n);
					o_cnd++;
				}
			}
		}
	}
	//_Addtolist(0,1,"o_cnd:[%d]",o_cnd);
	if(o_cnd!=2)
	{
		outs[0]=0;
		return -1;
	}

	/*没有  mov t0,[vm_regbase+index]*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0 ||
			memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_movzx,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGI)
				{
					/*检查第二个操作项寄存器*/
					if(xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_regbase)==0 )
					{
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].index_def_len!=0)
						{	
							//_Addtolist(0,1,"other condition inst_num:[%d]",n);
							o_cnd=1;
						}
					}
				}
			}
		}
	}
	if(o_cnd==1)
	{
		outs[0]=0;
		return -1;
	}

#if 0
	/*没有  mov t0,sp*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,t0)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_sp)==0)
				{	
					//_Addtolist(0,1,"other condition inst_num:[%d]",n);
					o_cnd=1;
				}
			}
		}
	}
	if(o_cnd==1)
	{
		outs[0]=0;
		return -1;
	}
#endif

	/*没有mov vm_key,vm_opcode*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{	
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_key)==0)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG )
				{
					//_Addtolist(0,1,"other condition inst_num:[%d]",n);
					o_cnd=1;
				}
			}
		}
	}
	if(o_cnd==1)
	{
		outs[0]=0;
		return -1;
	}

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x0200;
}


