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

//extern void    cdecl _Addtolist(long addr,int highlight,char *format,...);

extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];
extern char vm_oip[];



extern char opmz_dfile[];  //优化数据文件名

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
vm_pop_reg

mov t0,[sp+00]
add sp,4  ;lea sp,[sp+4]
mov [regbase+index],t0

*/
int vm_handle_0300(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int outs_size,char *out_var)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	char t0[20];
	int index=0;
	char tmp[100];

	int pop_index=0;
	struct ST_VM_ITEM_DATA item_data;


	memset(&item_data, 0, sizeof(item_data));


	//_Addtolist(0,1,"vm_handle_0300");
	memset(outs,0,outs_size);
	/*mov t0,[sp+00]*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{
			
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_sp)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x00 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_32)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);


					f_cnd=1;
				}
				else if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_sp)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_32)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);


					f_cnd=1;
				}
			}
		}
	}
	if(f_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	
	/*
	add sp,4;lea sp,[sp+4]
	*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_add,sop_len)==0)
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

						s_cnd=1;
					}
				}
			}
		}
	}

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
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x04)
						{

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

	
	/*mov [regbase+index],t0*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_MREGI &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_regbase)==0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].index_def_len!=0)
			{
				
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,t0)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_32)
				{
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_index_def,tmp);
					//sprintf(outs+strlen(outs),"index_%s=",tmp);
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_index_def);
					sprintf(outs+strlen(outs),"index=%08x  ",xx_contect[n].r[index]);
					pop_index=xx_contect[n].r[index]; //寄存器环境
					//sprintf(outs+strlen(outs),"  ");

					/*out_var，常数或者寄存器*/
					memcpy(out_var, &(xx_contect[n].r[index]), sizeof(xx_contect[n].r[index]));
					
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs+strlen(outs),"%08x ",xx_contect[n].r[index]);
					

					t_cnd=1;

					item_data.item_num = 1;
					memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
					memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

					memcpy(item_data.var[0], &xx_contect[n].r[REG_ESP], sizeof(xx_contect[n].r[REG_ESP]));

					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					memcpy(item_data.result, &xx_contect[n].r[index], sizeof(xx_contect[n].r[index]));

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
#if 0
	if(reg_status_stack.entry_flag==1)
	{
		reg_status_stack.reg_index[reg_status_stack.pop_num]=pop_index;
		//memcpy(reg_status_stack.reg_def[reg_status_stack.push_num],t0,xreg_len);
		reg_status_stack.reg_status[reg_status_stack.pop_num]=1;
		reg_status_stack.pop_num++;
	}
	else
	{
		reg_status_stack.reg_index[reg_status_stack.pop_num]=pop_index;
		//memcpy(reg_status_stack.reg_def[reg_status_stack.push_num],t0,xreg_len);
		reg_status_stack.reg_status[reg_status_stack.pop_num]=1;
		reg_status_stack.pop_num++;
	}
#endif

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x0300;
}





