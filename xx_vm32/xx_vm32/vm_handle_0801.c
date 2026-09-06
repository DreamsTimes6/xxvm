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
跟踪从最后开始截取200条，前面的指令取不到
mov edx,dword ptr ss:[ebp]
mov ecx,dword ptr ss:[ebp+0x4]
add ebp,0x4
xor eax,eax
test ecx,ecx
push esi

movzx esi,byte ptr ds:[edx]
xor esi,eax
and esi,0xFF
mov esi,dword ptr ds:[esi*4+0x18B90D8]
xor eax,esi
inc edx
xor eax,0x201D4679
dec ecx
jnz

pop esi
not eax
mov dword ptr ss:[ebp],eax
...
*/

int vm_handle_0801(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int outs_size,char *out_var)
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
	char t3[20];

	int index=0;
	char tmp[100];
	struct ST_VM_ITEM_DATA item_data;
	


	memset(&item_data, 0, sizeof(item_data));
	//_Addtolist(0,1,"vm_handle_0800");
	memset(outs,0,outs_size);

	/*inc edx*/
	/*inc t0*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_inc,sop_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size==INST_ITEM_SIZE_32)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	/*dec ecx*/
	/*dec t1*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_dec,sop_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size==INST_ITEM_SIZE_32)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	/*mov esi,dword ptr ds:[esi*4+0x7275F8]*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{
			
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGISDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,\
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_index_def)==0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].index_scale==0x04)
				{
					//_Addtolist(0,1,"second condition inst_num:[%d]",n);
					o_cnd=1;
				}
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

	return 0x0801;
}
