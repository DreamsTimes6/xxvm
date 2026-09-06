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
/*
给定指令块，逐个特征识别
指令块用xx_inst数组表示
*/
//////////////////////////////////


/*
定义sp寄存器
*/

extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];
extern char vm_oip[];


extern char opmz_dfile[];  //优化数据文件名

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
mov t0,b[sp+00]
mov t1,b[sp+4]
not t0
not t1
or t0,t1
mov [sp+4],t0
*/
int vm_handle_0d01(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int outs_size,char *out_var)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	int o_cnd=0;

	char t0[20];
	char t1[20];

	int index=0;
	char tmp[100];
	struct ST_VM_ITEM_DATA item_data;


	memset(&item_data, 0, sizeof(item_data));

	//_Addtolist(0,1,"vm_handle_0d01");
	memset(outs,0,outs_size);

	
	/*mov t1,[sp+02]*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0 ||
			memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_movzx,sop_len)==0)
		{
			
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_sp)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x02 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_16)
				{
					memset(t1,0,sizeof(t1));
					memcpy(t1,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
					sprintf(outs+strlen(outs),"(sub sp,2) %08x  ",xx_contect[n+1].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"second condition inst_num:[%d]",n);
					s_cnd=1;
							break;
				}
			}
		}
	}
	if(s_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*mov t0,[sp+00]*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0 ||
			memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_movzx,sop_len)==0)
		{
			
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_sp)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x00 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_16)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
					sprintf(outs+strlen(outs),"%08x  ",xx_contect[n+1].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					f_cnd=1;
							break;
				}
				else if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_sp)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_16)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
					sprintf(outs+strlen(outs),"%08x  ",xx_contect[n+1].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					f_cnd=1;
							break;
				}
			}
		}
	}
	if(f_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*not t1*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_not,sop_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_incmp(t1,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def)==0)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);

				memcpy(item_data.var[0], &xx_contect[n].r[get_reg_index(t1)], 2);

				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*not t0*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_not,sop_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_incmp(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def)==0)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);

				memcpy(item_data.var[1], &xx_contect[n].r[get_reg_index(t0)], 2);

				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*or t0,t1*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_or,sop_len)==0)
		{
			
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_incmp(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xreg_incmp(t1,xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def)==0)
				{
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs+strlen(outs),"%08x",xx_contect[n].r[index]);
					
					sprintf(outs+strlen(outs)," | ");
					
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
					sprintf(outs+strlen(outs),"%08x",xx_contect[n].r[index]);

					sprintf(outs+strlen(outs),"=");

					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
					sprintf(outs+strlen(outs),"%08x",xx_contect[n+1].r[index]);

					sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					t_cnd=1;

					item_data.item_num = 2;
					memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
					memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
					memcpy(item_data.result, &xx_contect[n + 1].r[index], 2);

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

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x0d01;
}