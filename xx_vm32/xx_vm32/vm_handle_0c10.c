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

/*
mov eax,dword ptr ss:[ebp+0x4]
mov edx,dword ptr ss:[ebp]
lea ebp,dword ptr ss:[ebp-0x4]
mul edx
mov dword ptr ss:[ebp+0x4],edx
mov dword ptr ss:[ebp+0x8],eax


mov t0,[sp+04]
mov t1,[sp+00]
sub sp,04;lea sp,[sp+fc]
mul t1
mov [sp+04],t1
mov [sp+08],t0
*/
int vm_handle_0c10(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int outs_size,char *out_var)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	int o_cnd=0;

	char t0[20];
	char t1[20];
	char t2[20];

	int index=0;
	char tmp[100];
	struct ST_VM_ITEM_DATA item_data;
	


	memset(&item_data, 0, sizeof(item_data));

	//_Addtolist(0,1,"vm_handle_0c00");
	memset(outs,0,outs_size);

	/*mov t0,[sp+04]*/
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
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x04)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
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


	/*mov t1,[sp+00]*/
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
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x00)
				{
					memset(t1,0,sizeof(t1));
					memcpy(t1,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					s_cnd=1;
				}
				else if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_sp)==0 )
				{
					memset(t1,0,sizeof(t1));
					memcpy(t1,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					s_cnd=1;
				}
			}
		}
	}
	if(s_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*mul t1*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_imul,sop_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,t1)==0)
			{

				//(sub,sp,4) d[sp]=00000206  d[sp+4]=00000000 d[sp+8]=00000006
				//sprintf(outs+strlen(outs),"(sub,sp,4) d[sp]=%08x  d[sp+4]=%08x d[sp+8]=%08x",\
				//	xx_contect[n+1].eflag,xx_contect[n+1].r[get_reg_index(xreg_edx)],xx_contect[n+1].r[get_reg_index(xreg_eax)]);
				sprintf(outs+strlen(outs),"(sub,sp,4) %08x * %08x = H%08x-L%08x",\
					xx_contect[n].r[get_reg_index(xreg_eax)],\
					xx_contect[n].r[get_reg_index(t1)],\
					xx_contect[n+1].r[get_reg_index(xreg_edx)],xx_contect[n+1].r[get_reg_index(xreg_eax)]);

/*
				memset(tmp,0,sizeof(tmp));
				xx_get_reg_mic(t0,tmp);
				sprintf(outs+strlen(outs),"%s=",tmp);
				index=get_reg_index(t0);
				sprintf(outs+strlen(outs),"%08x",xx_contect[n].r[index]);
				
				sprintf(outs+strlen(outs),"  ");
				
				memset(tmp,0,sizeof(tmp));
				xx_get_reg_mic(t1,tmp);
				sprintf(outs+strlen(outs),"%s=",tmp);
				index=get_reg_index(t1);
				sprintf(outs+strlen(outs),"%08x",xx_contect[n].r[index]);
				
				sprintf(outs+strlen(outs),"  ");

				memset(tmp,0,sizeof(tmp));
				xx_get_reg_mic(t1,tmp);
				sprintf(outs+strlen(outs),"mul %s",tmp);
				sprintf(outs+strlen(outs),"  ");

				memset(tmp,0,sizeof(tmp));
				xx_get_reg_mic(t0,tmp);
				sprintf(outs+strlen(outs),"%s=",tmp);
				index=get_reg_index(t0);
				sprintf(outs+strlen(outs),"%08x",xx_contect[n+1].r[index]);
				
				sprintf(outs+strlen(outs),"  ");
				
				memset(tmp,0,sizeof(tmp));
				xx_get_reg_mic(t1,tmp);
				sprintf(outs+strlen(outs),"%s=",tmp);
				index=get_reg_index(t1);
				sprintf(outs+strlen(outs),"%08x",xx_contect[n+1].r[index]);
				
				sprintf(outs+strlen(outs),"  ");

				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				
*/
				o_cnd=1;

				item_data.item_num = 2;
				memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
				memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

				memcpy(item_data.var[0], &xx_contect[n].r[get_reg_index(xreg_eax)], sizeof(xx_contect[n].r[get_reg_index(xreg_eax)]));

				memcpy(item_data.result, &xx_contect[n + 1].r[get_reg_index(xreg_eax)], sizeof(xx_contect[n + 1].r[get_reg_index(xreg_eax)]));

				/*忽略高位*/
				memcpy(item_data.var[1], &xx_contect[n].r[get_reg_index(t1)], sizeof(xx_contect[n].r[get_reg_index(t1)]));

				break;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x0c10;
}


