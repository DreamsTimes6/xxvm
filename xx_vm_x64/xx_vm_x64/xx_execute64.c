#include <stdio.h>
#include <time.h>
//#include "xx_plugin.h"
#include "xx_vm.h"
#include "xx_comm64.h"
#include "xxdisasm64.h"
#include "xx_opdef2.h"
#include "xx_opdef3.h"
#include "xx_link_list.h"
#include "func_op.h"

#include <_plugins.h>
#include <bridgemain.h>



////////////////////////////////////////////////////////////////////////////
int init_mem_file(char *);
int free_mem_file();
int xx_gen_fmem(unsigned long long addr, struct XX_FMEM64 *xx_fmem, int flag);
int xx_get_fmem(unsigned long long addr, char *outname, unsigned long long *out_offset);
int xx_write_fmem(unsigned long long addr, char* pdata, unsigned int datasize);
int xx_read_fmem(unsigned long long addr, char* pdata, unsigned int datasize);
int init_file_path(char *ppath, char *pname, char *file);
int xx_init64(char* mainname);
int debug_log(char *tmp);
int xx_start_execute(struct XX_CONTEXT64 *init_context);
int get_inst_context(struct XX_INST* g_inst, struct XX_CONTEXT64 *g_context, int *num);
int calc_item(struct XX_INST *, int , struct ITEM_VAR64 *, struct XX_CONTEXT64 *, unsigned long long, int);
int conv_item(struct XX_INST *, unsigned int , int , struct XX_CONTEXT64 *, struct ITEM_VAR64 *);
int xx_execute(struct XX_INST *inst, struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context);
int xdbg_gen_fmem();
////////////////////////////////////////////////////////////////////////////
/*global   */
HANDLE saly_semaphore;


struct XX_LINK_NODE* xx_link = 0;
char debug_buf[500];  /* xx_vm.h declares extern */
//char fmempath[] = "xx_fmem\\";
char *fmempath=0;
int fmempath_size = 0;


ulong64 seg_gs = 0;
char not_handle[] = "stoped,can not handle it\r\n";

/*使用到的文件名*/
char inst_file_name[] = "vm_opmz.x";
char inst_dfile_name[] = "vm_opmz_data.x";
char opmz_log_name[] = "xx_opmz.txt";
char exp_file_name[] = "xx_exp.x";

char log_file[0x400];  //普通日志文件名   
char opmz_cmd[0x400];  //普通日志文件目录
char opmz_log[0x400];  //优化日志文件名
char opmz_file[0x400];  //优化指令信息
char opmz_dfile[0x400];  //优化指令数据
char exp_file[0x400];    //优化还原数据

struct XX_MOD xx_mod;
struct XX_CONTEXT64 start_context;

struct XX_INST g_inst[MAX_INST + 2];
struct XX_CONTEXT64 g_context[MAX_INST + 3];
//////////////////////////////////////////////////////////////////////

 HINSTANCE hinst;
extern char xdbg_path[];
//extern int init_vmsreg(struct XX_INST *xx_inst, int num);
extern int init_vmsreg2(struct XX_INST *xx_inst, int num);
extern int vm_handle(struct XX_HTHREAD64 *x_thread64);
extern int vm_handle_0000(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);


///////////////////////////////////////////////////////////////////////////////////
 int pause_flag=0;


extern  char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];

extern char vm_osp[];
extern char vm_oregbase[];
extern char vm_oopcode[];
extern char vm_oip[];
extern char vm_okey[];

extern int g_inst_num;


extern int vm_handle_1200(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int, char*);
//////////////////////////////////////////////////////////////////////////////
HANDLE curt_hthread = 0;

/*
分析函数中的信号必须释放
*/
int xx_start_saly()
{
	int iret = 0;
	//struct XX_CONTEXT64 in_context;

	char tmp[200];
	int threadid = 0;


	curt_hthread = GetCurrentThread();

	if (start_context.ip == 0)
	{
		MessageBox(0, "ip error ", "info", MB_OK);
		goto err_ret;
	}

	if (strlen(log_file) == 0)
	{
		MessageBox(0, "Init first", "info", MB_OK);
		goto err_ret;
	}

	g_inst_num = 0;

	/*获取当前context*/

	//memcpy(&in_context.r,pt_thread->reg.r,sizeof(pt_thread->reg.r));
	//in_context.eflag=pt_thread->reg.flags;
	//in_context.ip=pt_thread->reg.ip;


	threadid = DbgGetThreadId();
	if (threadid == 0)
	{
		//debug_log("error ip  ");
		//xx_append_file(log_file,"ip error\r\n",strlen("ip error\r\n"));
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "threadid error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		goto err_ret;
	}

	seg_gs = DbgGetTebAddress(threadid);
	if (seg_gs == 0)
	{
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "seg_fs error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		goto err_ret;
	}



	pause_flag = 0;




	//start_context.eflag = start_context.eflag & 0xfffffffffffffeff;

	iret = xx_start_execute(&start_context);
	if (iret == 1)
	{
		//MessageBox(0, "Finish", "success", MB_OK);
	}
	else
	{
		//MessageBox(0, "Finish", "info", MB_OK);
		goto err_ret;
	}


	//return 0;



	pause_flag = 0;
	//_plugin_logputs("finish");

	/*释放信号*/
	if (saly_semaphore)
	{
		ReleaseSemaphore(saly_semaphore, 1, 0);
		saly_semaphore = 0;
	}

	return 1;

err_ret:


	pause_flag = 0;
	//_plugin_logputs("finish");

	/*释放信号*/
	if (saly_semaphore)
	{
		ReleaseSemaphore(saly_semaphore, 1, 0);
		saly_semaphore = 0;
	}

	return 0;
}


void xx_saly_stop()
{
	int iret = 0;
	unsigned long status = 0;

	//pause_flag = 1;

	//Sleep(1000);

	iret = GetExitCodeThread(curt_hthread, &status);
	if (iret == 0)
	{
		return ;
	}

	curt_hthread = 0;
	if (status == STILL_ACTIVE)
	{
		//TerminateThread(curt_hthread, 0);
		pause_flag = 1;
		Sleep(1000);
	}

	if (saly_semaphore)
	{
		ReleaseSemaphore(saly_semaphore, 1, 0);
		saly_semaphore = 0;
	}

}


void xx_saly_semaphore(HANDLE sem)
{
	saly_semaphore = sem;
}



/**/
int is_vm_entry()
{
	int iret = 0;
	int n = 0;
	int inst_num = 0;
	char tmp[0x200];
	char cmdbuf[100];
	unsigned long long base = 0;
	unsigned long long ip = 0;
	struct XX_HTHREAD64 x_thread;
	char out_var[0x10];
	int cmd_len = 0;

	/*检查是否是vm_entry*/
	/*线程结构赋值*/
	memset(&g_inst, 0, sizeof(g_inst));
	memset(&g_context, 0, sizeof(g_context));

	/*数据赋值*/
	memcpy(&g_context[0], &start_context, sizeof(start_context));

	
	inst_num = 0;

	base = xx_mod.base;

	ip = g_context[0].ip;
	n = 0;
	while (1)
	{

		if (ip == 0)
		{
			//debug_log("error ip  ");
			//xx_append_file(log_file,"ip error\r\n",strlen("ip error\r\n"));
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "ip error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}

#if 0
		memset(cmdbuf, 0, sizeof(cmdbuf));
		iret = DbgMemRead(ip, cmdbuf, INST_MAX_LEN);
		if (iret == 0)
		{
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "_Readmemory error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}
#else
		cmd_len = 0x40;
		memset(cmdbuf, 0, sizeof(cmdbuf));
		iret = xx_read_fmem(ip, cmdbuf, cmd_len);
		if (iret == 0)
		{
			cmd_len = 0x0ff - (ip & 0x0ff);
			iret = xx_read_fmem(ip, cmdbuf, cmd_len);
			if (iret == 0)
			{
				return 0;
			}
		}
#endif

		memset(&g_inst[n], 0, sizeof(struct XX_INST));
		g_inst[n].xx_inst_code.inst_system = INST_SYSTEM_CURT;
		memcpy(g_inst[n].xx_inst_code.base, (char*)&base, g_inst[n].xx_inst_code.inst_system);
		memcpy(g_inst[n].xx_inst_code.current_addr, (char*)&ip, g_inst[n].xx_inst_code.inst_system);
		xx_disasm(&g_inst[n], cmdbuf);

		if (g_inst[n].xx_inst_exist.finish_flag != INST_FLAG_EXIST)
		{
			//debug_log("xx_disasm error ");
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "xx_disasm error.inst_system:[%x] base:[%llx] ip:[%llx]", \
				g_inst[n].xx_inst_code.inst_system, \
				*(ulong64*)g_inst[n].xx_inst_code.base, \
				*(ulong64*)g_inst[n].xx_inst_code.current_addr);
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}


		if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_jmp, sop_len) == 0 &&
			g_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_DIS)
		{
			ip = *(ulong64*)g_inst[n].xx_inst_items.xx_inst_items_var[0].item_const;
			//_Addtolist(0,1,"jmp ip:[%08x]",ip);
		}
		else if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_ja, sop_len) == 0 &&
			g_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_DIS)
		{
			ip = *(ulong64*)g_inst[n].xx_inst_items.xx_inst_items_var[0].item_const;
			//_Addtolist(0,1,"jmp ip:[%08x]",ip);
		}
		else if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_call, sop_len) == 0 && \
			g_inst[n].xx_inst_items.xx_inst_items_var[0].const_size != 0)
		{
			ip = *(ulong64*)g_inst[n].xx_inst_items.xx_inst_items_var[0].item_const;
		}
		else
		{
			ip = ip + g_inst[n].xx_inst_code.disasm_length;
		}

		inst_num++;

		/*结束条件*/
		if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_ret, sop_len) == 0)
		{
			//debug_log("not vm");
			break;
			//return 0;
		}
		else if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_ret, sop_len) == 0 &&
			memcmp(g_inst[n - 1].xx_inst_code.opcode_type, sop_push, sop_len) == 0 &&
			g_inst[n - 1].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
		{
			//debug_log("break;push ret");
			break;
		}
		else if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_jmp, sop_len) == 0 &&
			g_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
		{
			//debug_log("break;ret");
			break;
		}
		

		n++;

		if (n >= MAX_INST)
		{
			break;
		}

	}
	/*配置环境*/
	init_vmsreg2(g_inst, inst_num);

#if 0
	memset(&x_thread, 0, sizeof(x_thread));
	x_thread.xx_inst = g_inst;
	x_thread.xx_context = g_context;
	x_thread.inst_num = inst_num;

	memset(tmp, 0, sizeof(tmp));
	x_thread.plogdata = tmp;

	iret = vm_handle(&x_thread);
	if (iret != 0x0000)
	{
		return 0;
	}
#else
	/*调用内部函数，防止重复生成VM_ENTRY*/
	memset(tmp, 0, sizeof(tmp));
	memset(out_var, 0, sizeof(out_var));
	iret = vm_handle_0000(g_inst, inst_num, g_context, tmp, sizeof(tmp), out_var);
	if (iret != 0x0000)
	{
		return 0;
	}
#endif
	return 1;
}

int xx_start_execute(struct XX_CONTEXT64 *init_context)
{
	int iret = 0;
	struct XX_CONTEXT64 in_context;

	struct XX_HTHREAD64 x_thread;
	int n = 0;
	int inst_num = 0;
	char tmp[0x200];
	//struct XX_INST g_inst[MAX_INST+2];
	//struct XX_CONTEXT64 g_context[MAX_INST + 3];
	int env_flag = 0;
	int unknow_num = 0;
	ulong64 basesize = 0;
	ulong64 baseaddr = 0;
	char buf1[100];
	char buf2[100];
	char buf3[100];

	env_flag = 0;

	memcpy(&in_context, init_context, sizeof(in_context));

	while (1)
	{
		/*线程结构赋值*/
		memset(&g_inst, 0, sizeof(g_inst));
		memset(&g_context, 0, sizeof(g_context));

		/*数据赋值*/
		memcpy(&g_context[0], &in_context, sizeof(in_context));

		/*获取数据*/
		inst_num = 0;
		iret = get_inst_context(g_inst, g_context, &inst_num);
		if (iret == 0)
		{
			//debug_log("get_inst_context  ");
			//xx_append_file(log_file,"get_inst_context error\r\n",strlen("get_inst_context error\r\n"));
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s   %s", g_inst[inst_num - 1].xx_inst_mic.disasm, "inst_info error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));

			_plugin_logputs(debug_buf);
			memset(&start_context, 0, sizeof(struct XX_CONTEXT64));
			return 0;
		}



		/*配置环境*/
		//env_flag = 0;
		iret = init_vmsreg2(g_inst, inst_num);
		if (iret !=0)
		{
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "iret=%d\r\n",iret);
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
	
			env_flag ++;
		}
		else
		{
			env_flag = 0;
		}


		memset(&x_thread, 0, sizeof(x_thread));
		x_thread.xx_inst = g_inst;
		x_thread.xx_context = g_context;
		x_thread.inst_num = inst_num;

		memset(tmp, 0, sizeof(tmp));
		x_thread.plogdata = tmp;

		iret = vm_handle(&x_thread);
		if (iret == -1)
		{
			//debug_log("vm_handle  ");
			//xx_append_file(log_file,"vm_handle error\r\n",strlen("vm_handle error\r\n"));
			//memset(debug_buf,0,sizeof(debug_buf));
			//sprintf(debug_buf,"%s","vm_handle error\r\n");
			//xx_append_file(log_file,debug_buf,strlen(debug_buf));
			//break;
#ifdef DEBUG_LOG
/*测试*/
			for (n = 0; n < inst_num; n++)
			{
				memset(debug_buf, 0, sizeof(debug_buf));
				sprintf(debug_buf, "%llx  %s\r\n", g_context[n].ip, g_inst[n].xx_inst_mic.disasm);
				xx_append_file(log_file, debug_buf, strlen(debug_buf));
			}
			

			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "===============================================================\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
			unknow_num++;
		}
		else
		{
			unknow_num = 0;
		}
#if 0
		/*测试*/
		memset(buf1, 0, sizeof(buf1));
		xx_get_reg_mic(vm_sp, buf1);
		memset(buf2, 0, sizeof(buf2));
		xx_get_reg_mic(vm_opcode, buf2);
		memset(buf3, 0, sizeof(buf3));
		xx_get_reg_mic(vm_ip, buf3);
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "sp:[%s] opcode:[%s] ip:[%s]\r\n", buf1, buf2, buf3);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));

		memset(buf1, 0, sizeof(buf1));
		xx_get_reg_mic(vm_osp, buf1);
		memset(buf2, 0, sizeof(buf2));
		xx_get_reg_mic(vm_oopcode, buf2);
		memset(buf3, 0, sizeof(buf3));
		xx_get_reg_mic(vm_oip, buf3);
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "osp:[%s] oopcode:[%s] oip:[%s]\r\n", buf1, buf2, buf3);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif

		if (unknow_num >= 5)
		{
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "vm_handle error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			MessageBox(0,"More Invalid ","Worning",MB_OK);
			return 0;
		}

		xx_append_file(log_file, x_thread.plogdata, strlen(x_thread.plogdata));
		if (iret == 0x1200)
		{
			//debug_log("ret  ");
			//xx_append_file(log_file,"vm_handle error\r\n",strlen("vm_handle error\r\n"));
			memcpy(&start_context, &g_context[inst_num], sizeof(in_context));
			break;
		}

		if (env_flag > 2)
		{
			//debug_log("init_vmsreg2");
			//xx_append_file(log_file,"env error\r\n",strlen("env error\r\n"));
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "env error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			MessageBox(0, "More Invalid ", "Worning", MB_OK);
			return 0;
		}
		


		memset(&in_context, 0, sizeof(struct XX_CONTEXT64));
		memcpy(&in_context, &g_context[inst_num], sizeof(in_context));

	}



	return 1;
}






int get_inst_context(struct XX_INST* g_inst, struct XX_CONTEXT64 *g_context, int *num)
{
	int n = 0;
	int iret = 0;
	int inst_num = 0;
	unsigned long long base = 0;
	unsigned long long ip = 0;
	char cmdbuf[200];
	char tmp[200];
	int cmd_len = 0;

	n = 0;
	inst_num = 0;

	base = xx_mod.base;

	

	while (1)
	{
#ifdef DEBUG_LOG
		debug_log("===========================xx_execute=============================");
#endif
		ip = g_context[n].ip;
		
		if (ip == 0)
		{
			//debug_log("error ip  ");
			//xx_append_file(log_file,"ip error\r\n",strlen("ip error\r\n"));
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "ip error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}

		if (pause_flag == 1)
		{
			pause_flag = 0;
			//MessageBox(0, "Finish...", "info", MB_OK);
			return 0;
		}

		/*API则退出，有标签则退出，vm代码内一定没有标签
		x32,x64可能有错误的标签
		*/
		if (ip<xx_mod.base || ip>(xx_mod.base + xx_mod.size))
		{
			memset(tmp, 0, sizeof(tmp));
			iret = DbgGetLabelAt(ip, SEG_CS, tmp);
			if (iret && strcmp(tmp, "EntryPoint") != 0)
			{
				memset(debug_buf, 0, sizeof(debug_buf));
				sprintf(debug_buf, "%llx API:%s", ip, tmp);
				xx_append_file(log_file, debug_buf, strlen(debug_buf));
				return 0;
			}
		}
		
#if 0
		memset(cmdbuf, 0, sizeof(cmdbuf));
		iret = DbgMemRead(ip, cmdbuf, INST_MAX_LEN);
		if (iret == 0)
		{
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "_Readmemory error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}
#else
		cmd_len = 0x40;
		memset(cmdbuf, 0, sizeof(cmdbuf));
		iret = xx_read_fmem(ip, cmdbuf, cmd_len);
		if (iret == 0)
		{
			cmd_len = 0x0ff - (ip & 0x0ff);
			iret = xx_read_fmem(ip, cmdbuf, cmd_len);
			if (iret == 0)
			{
				return 0;
			}
		}
#endif

		memset(&g_inst[n], 0, sizeof(struct XX_INST));
		g_inst[n].xx_inst_code.inst_system = INST_SYSTEM_CURT;
		memcpy(g_inst[n].xx_inst_code.base, (char*)&base, g_inst[n].xx_inst_code.inst_system);
		memcpy(g_inst[n].xx_inst_code.current_addr, (char*)&ip, g_inst[n].xx_inst_code.inst_system);
		xx_disasm(&g_inst[n], cmdbuf);

		if (g_inst[n].xx_inst_exist.finish_flag != INST_FLAG_EXIST)
		{
			//debug_log("xx_disasm error ");
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "xx_disasm error.inst_system:[%x] base:[%llx] ip:[%llx]",\
				g_inst[n].xx_inst_code.inst_system,\
				*(ulong64*)g_inst[n].xx_inst_code.base,\
				*(ulong64*)g_inst[n].xx_inst_code.current_addr);
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}

		

		memset(&g_context[n + 1], 0, sizeof(struct XX_CONTEXT64));
		iret = xx_execute(&g_inst[n], &g_context[n], &g_context[n + 1]);
		if (iret == 0)
		{
			//debug_log("xx_execute2 error"); 
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "ip:[%llx]  %s", g_context[n].ip, "xx_execute error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));

			*num = inst_num+1;
			return 0;
		}


		/*过滤*/
		if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_jmp, sop_len) == 0 &&  \
			g_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type != INST_ITEM_TYPE_REG)
		{
			memcpy(&g_context[n], &g_context[n+1], sizeof(struct XX_CONTEXT64));
			continue;
		}

		inst_num++;

		/*结束条件*/
		if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_ret, sop_len) == 0)
		{
			//debug_log("not vm");
			break;
			//return 0;
		}
		else if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_ret, sop_len) == 0 &&
			memcmp(g_inst[n - 1].xx_inst_code.opcode_type, sop_push, sop_len) == 0 &&
			g_inst[n - 1].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
		{
			//debug_log("break;push ret");
			break;
		}
		else if (memcmp(g_inst[n].xx_inst_code.opcode_type, sop_jmp, sop_len) == 0 &&
			g_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
		{
			//debug_log("break;ret");
			break;
		}
		

		n++;

		if (n >= MAX_INST)
		{
			//memcpy(&g_context[0], &g_context[n - 1], sizeof(struct XX_CONTEXT64));
			memcpy(&g_context[0], &g_context[n], sizeof(struct XX_CONTEXT64));
			n = 0;
			inst_num = 0;
		}
	}


	*num = inst_num;
	return 1;
}


int get_inst_context_test(struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context)
{
	int n = 0;
	int iret = 0;
	int inst_num = 0;
	unsigned long long base = 0;
	unsigned long long ip = 0;
	char cmdbuf[200];
	char tmp[200];
	struct XX_INST xx_inst;
	ulong threadid = 0;
	int cmd_len = 0;

	n = 0;
	inst_num = 0;

#ifdef DEBUG_LOG
	debug_log("===========================xx_execute=============================");
#endif

	threadid = DbgGetThreadId();
	if (threadid == 0)
	{
		//debug_log("error ip  ");
		//xx_append_file(log_file,"ip error\r\n",strlen("ip error\r\n"));
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "threadid error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	seg_gs = DbgGetTebAddress(threadid);
	if (seg_gs == 0)
	{
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "seg_fs error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	base = xx_mod.base;

	ip = in_context->ip;
	if (ip == 0)
	{
		//debug_log("error ip  ");
		//xx_append_file(log_file,"ip error\r\n",strlen("ip error\r\n"));
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "ip error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}


#if 0
	memset(cmdbuf, 0, sizeof(cmdbuf));
	iret = DbgMemRead(ip, cmdbuf, INST_MAX_LEN);
	if (iret == 0)
	{
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "_Readmemory error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
#else
	cmd_len = 0x40;
	memset(cmdbuf, 0, sizeof(cmdbuf));
	iret = xx_read_fmem(ip, cmdbuf, cmd_len);
	if (iret == 0)
	{
		cmd_len = 0x0ff - (ip & 0x0ff);
		iret = xx_read_fmem(ip, cmdbuf, cmd_len);
		if (iret == 0)
		{
			return 0;
		}
	}
#endif

	memset(&xx_inst, 0, sizeof(xx_inst));
	xx_inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
	memcpy(xx_inst.xx_inst_code.base, (char*)&base, xx_inst.xx_inst_code.inst_system);
	memcpy(xx_inst.xx_inst_code.current_addr, (char*)&ip, xx_inst.xx_inst_code.inst_system);
	xx_disasm(&xx_inst, cmdbuf);

	if (xx_inst.xx_inst_exist.finish_flag != INST_FLAG_EXIST)
	{
		//debug_log("xx_disasm error ");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "xx_disasm error.inst_system:[%x] base:[%llx] ip:[%llx]", \
			xx_inst.xx_inst_code.inst_system, \
			*(ulong64*)xx_inst.xx_inst_code.base, \
			*(ulong64*)xx_inst.xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
#if 1

	iret = xx_execute(&xx_inst,in_context, out_context);
	if (iret == 0)
	{
		//debug_log("xx_execute2 error"); 
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "ip:[%llx]  %s", ip, "xx_execute error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
#endif

	return 1;
}


/*
执行前生成所有的内存文件
*/
int xx_execute(struct XX_INST *inst, struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context)
{
	unsigned int op_type = 0;
	struct XX_INST xx_inst;
	char tmp[200];
	int n = 0;
	int iret = 0;
	struct ITEM_VAR64 item_var[4];

	if (xx_link == 0)
	{
		return 0;
	}

#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "rax:%llx rcx:%llx rdx:%llx rbx:%llx rsp:%llx rbp:%llx rsi:%llx rdi:%llx ip:%llx rflag:%llx", \
		in_context->r[REG_RAX], in_context->r[REG_RCX], in_context->r[REG_RDX], in_context->r[REG_RBX]\
		, in_context->r[REG_RSP], in_context->r[REG_RBP], in_context->r[REG_RSI], in_context->r[REG_RDI]\
		, in_context->ip, in_context->eflag);
	debug_log(debug_buf);

	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "r8:%llx r9:%llx r10:%llx r11:%llx r12:%llx r13:%llx r14:%llx r15:%llx ", \
		in_context->r[REG_R8], in_context->r[REG_R9], in_context->r[REG_R10], in_context->r[REG_R11]\
		, in_context->r[REG_R12], in_context->r[REG_R13], in_context->r[REG_R14], in_context->r[REG_R15]);
	debug_log(debug_buf);
#endif

	memset(&xx_inst, 0, sizeof(struct XX_INST));


	memcpy(&xx_inst, inst, sizeof(struct XX_INST));



	memset(tmp, 0, sizeof(tmp));
	nstr_hex(xx_inst.xx_inst_code.opcode_type, sop_len, tmp);

	byte_opposite(tmp, sop_len / 2, (char*)&op_type);

#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "disasm:[%s] op_type:%s-%08x  nitem:%08x", \
		xx_inst.xx_inst_mic.disasm, xx_inst.xx_inst_mic.opcode_mic, op_type, xx_inst.xx_inst_items.nitem);
	debug_log(debug_buf);
#endif

	memset(item_var, 0, sizeof(item_var));
	for (n = 0; n < xx_inst.xx_inst_items.nitem; n++)
	{

		iret = conv_item(&xx_inst, op_type, n, in_context, &item_var[n]);
		if (iret == 0)
		{
			//debug_log("conv_item");
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "conv_item error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}
		item_var[n].flag = iret;

#ifdef DEBUG_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "item[%d]: addr=%llx data=%llx %llx ", \
			n, item_var[n].addr, item_var[n].var, item_var[n].size);
		debug_log(debug_buf);
#endif

	}

	iret = 0;
	switch (op_type)
	{
	case op_cmove:
		//op_cmove            0x010101 
		iret = func_op_cmove(&xx_inst, item_var, in_context, out_context);
		break;
	case op_comvne:
		//op_comvne           0x010102
		iret = func_op_cmovne(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmova:
		//op_cmova            0x010103 
		iret = func_op_cmova(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovae:
		//op_cmovae           0x010104
		iret = func_op_cmovae(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovb:
		//op_cmovb            0x010105
		iret = func_op_cmovb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovbe:
		//op_cmovbe           0x010106
		iret = func_op_cmovbe(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovg:
		//op_cmovg            0x010107
		iret = func_op_cmovg(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovge:
		//op_cmovge           0x010108
		iret = func_op_cmovge(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovl:
		//op_cmovl            0x010109
		iret = func_op_cmovl(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovle:
		//op_cmovle           0x01010a
		iret = func_op_cmovle(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovc:
		//op_cmovc            0x01010b  没有
		break;
	case op_cmovnc:
		//op_cmovnc           0x01010c  没有
		break;
	case op_cmovo:
		//op_cmovo            0x01010d
		iret = func_op_cmovo(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovno:
		//op_cmovno           0x01010e
		iret = func_op_cmovno(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovs:
		//op_cmovs            0x01010f
		iret = func_op_cmovs(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovns:
		//op_cmovns           0x010110
		iret = func_op_cmovns(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovp:
		//op_cmovp            0x010111
		iret = func_op_cmovp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmovnp:
		//op_cmovnp           0x010112
		iret = func_op_cmovnp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_xchg:
		//op_xchg             0x010113
		iret = func_op_xchg(&xx_inst, item_var, in_context, out_context);
		break;
	case op_bswap:
		iret = func_op_bswap(&xx_inst, item_var, in_context, out_context);
		//op_bswap            0x010114
		break;
	case op_xadd:
		//op_xadd             0x010115
		iret = func_op_xadd(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmpxchg:
		//op_cmpxchg          0x010116
		break;
	case op_cmpxchg8b:
		//op_cmpxchg8b        0x010117
		break;
	case op_push:
		//op_push             0x010118
		iret = func_op_push(&xx_inst, item_var, in_context, out_context);
		break;
	case op_pop:
		//op_pop              0x010119
		iret = func_op_pop(&xx_inst, item_var, in_context, out_context);
		break;
	case op_pushad:
		//op_pushad           0x01011a 
		//iret = func_op_pushad(&xx_inst, item_var, in_context, out_context);
		break;
	case op_popad:
		//op_popad            0x01011b
		//iret = func_op_popad(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cdq:
		//op_cwd  cdq   66 cwd         0x01011c
		iret = func_op_cdq(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cwde:
		//op_cbw   cwde  66 cbw         0x01011d
		iret = func_op_cwde(&xx_inst, item_var, in_context, out_context);
		break;
	case op_movsx:
		//op_movsx            0x01011e
		iret = func_op_movsx(&xx_inst, item_var, in_context, out_context);
		break;
	case op_movzx:
		//op_movzx            0x01011f
		iret = func_op_movzx(&xx_inst, item_var, in_context, out_context);
		break;
	case op_mov:
		//op_mov              0x010120
		iret = func_op_mov(&xx_inst, item_var, in_context, out_context);
		break;
	case op_adcx:
		//op_adcx          0x010201
		break;
	case op_adox:
		//op_adox          0x010202 
		break;
	case op_add:
		//op_add           0x010203
		iret = func_op_add(&xx_inst, item_var, in_context, out_context);
		break;
	case op_adc:
		//op_adc           0x010204
		iret = func_op_adc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sub:
		//op_sub           0x010205
		iret = func_op_sub(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sbb:
		//op_sbb           0x010206 
		iret = func_op_sbb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_imul:
		//op_imul          0x010207
		iret = func_op_imul(&xx_inst, item_var, in_context, out_context);
		break;
	case op_mul:
		//op_mul           0x010208
		iret = func_op_mul(&xx_inst, item_var, in_context, out_context);
		break;
	case op_idiv:
		//op_idiv          0x010209
		iret = func_op_idiv(&xx_inst, item_var, in_context, out_context);
		break;
	case op_div:
		//op_div           0x01020a
		iret = func_op_div(&xx_inst, item_var, in_context, out_context);
		break;
	case op_inc:
		//op_inc           0x01020b
		iret = func_op_inc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_dec:
		//op_dec           0x01020c
		iret = func_op_dec(&xx_inst, item_var, in_context, out_context);
		break;
	case op_neg:
		//op_neg           0x01020d
		iret = func_op_neg(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmp:
		//op_cmp           0x01020e
		iret = func_op_cmp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_daa:
		//op_daa           0x010301
		//iret = func_op_daa(&xx_inst, item_var, in_context, out_context);
		break;
	case op_das:
		//op_das           0x010302
		//iret = func_op_das(&xx_inst, item_var, in_context, out_context);
		break;
	case op_aaa:
		//op_aaa           0x010303
		//iret = func_op_aaa(&xx_inst, item_var, in_context, out_context);
		break;
	case op_aas:
		//op_aas           0x010304 
		//iret = func_op_aas(&xx_inst, item_var, in_context, out_context);
		break;
	case op_aam:
		//op_aam           0x010305
		//iret = func_op_aam(&xx_inst, item_var, in_context, out_context);
		break;
	case op_aad:
		//op_aad           0x010306
		//iret = func_op_aad(&xx_inst, item_var, in_context, out_context);
		break;
	case op_and:
		//op_and           0x010401
		iret = func_op_and(&xx_inst, item_var, in_context, out_context);
		break;
	case op_or:
		//op_or            0x010402
		iret = func_op_or(&xx_inst, item_var, in_context, out_context);
		break;
	case op_xor:
		//op_xor           0x010403
		iret = func_op_xor(&xx_inst, item_var, in_context, out_context);
		break;
	case op_not:
		//op_not           0x010404
		iret = func_op_not(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sar:
		//op_sar           0x010501 
		iret = func_op_sar(&xx_inst, item_var, in_context, out_context);
		break;
	case op_shr:
		//op_shr           0x010502
		iret = func_op_shr(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sal:
		//op_sal           0x010503
		iret = func_op_sal(&xx_inst, item_var, in_context, out_context);
		break;
	case op_shl:
		//op_shl           0x010504
		iret = func_op_shl(&xx_inst, item_var, in_context, out_context);
		break;
	case op_shrd:
		//op_shrd          0x010505
		iret = func_op_shrd(&xx_inst, item_var, in_context, out_context);
		break;
	case op_shld:
		//op_shld          0x010506
		iret = func_op_shld(&xx_inst, item_var, in_context, out_context);
		break;
	case op_ror:
		//op_ror           0x010507
		iret = func_op_ror(&xx_inst, item_var, in_context, out_context);
		break;
	case op_rol:
		//op_rol           0x010508
		iret = func_op_rol(&xx_inst, item_var, in_context, out_context);
		break;
	case op_rcr:
		//op_rcr           0x010509
		iret = func_op_rcr(&xx_inst, item_var, in_context, out_context);
		break;
	case op_rcl:
		//op_rcl           0x01050a
		iret = func_op_rcl(&xx_inst, item_var, in_context, out_context);
		break;
	case op_bt:
		//op_bt                 0x010601
		iret = func_op_bt(&xx_inst, item_var, in_context, out_context);
		break;
	case op_bts:
		//op_bts                0x010602
		iret = func_op_bts(&xx_inst, item_var, in_context, out_context);
		break;
	case op_btr:
		//op_btr                0x010603
		iret = func_op_btr(&xx_inst, item_var, in_context, out_context);
		break;
	case op_btc:
		//op_btc                0x010604
		iret = func_op_btc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_bsf:
		//op_bsf                0x010605
		iret = func_op_bsf(&xx_inst, item_var, in_context, out_context);
		break;
	case op_bsr:
		//op_bsr                0x010606
		iret = func_op_bsr(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sete:
		//op_sete               0x010607
		iret = func_op_sete(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setne:
		//op_setne              0x010608
		iret = func_op_setne(&xx_inst, item_var, in_context, out_context);
		break;
	case op_stea:
		//op_stea               0x010609
		iret = func_op_seta(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setae:
		//op_setae              0x01060a
		iret = func_op_setae(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setb:
		//op_setb               0x01060b
		iret = func_op_setb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setbe:
		//op_setbe              0x01060c
		iret = func_op_setbe(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setg:
		//op_setg               0x01060d
		iret = func_op_setg(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setge:
		//op_setge              0x01060e
		iret = func_op_setge(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setl:
		//op_setl               0x01060f
		iret = func_op_setl(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setle:
		//op_setle              0x010610
		iret = func_op_setle(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sets:
		//op_sets               0x010611
		iret = func_op_sets(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setns:
		//op_setns              0x010612
		iret = func_op_setns(&xx_inst, item_var, in_context, out_context);
		break;
	case op_seto:
		//op_seto               0x010613
		iret = func_op_seto(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setno:
		//op_setno              0x010614
		iret = func_op_setno(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setp:
		//op_setpe              0x010615
		iret = func_op_setp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_setnp:
		//op_setpo              0x010616
		iret = func_op_setnp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_test:
		//op_test               0x010617
		iret = func_op_test(&xx_inst, item_var, in_context, out_context);
		break;
	case op_crc32:
		//op_crc32              0x010618
		break;
	case op_popcnt:
		//op_popcnt             0x010619
		break;
	case op_jmp:
		//op_jmp               0x010701
		iret = func_op_jmp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_je:
		//op_je                0x010702
		iret = func_op_je(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jne:
		//op_jne               0x010703
		iret = func_op_jne(&xx_inst, item_var, in_context, out_context);
		break;
	case op_ja:
		//op_ja                0x010704
		iret = func_op_ja(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jae:
		//op_jae               0x010705
		iret = func_op_jae(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jb:
		//op_jb                0x010706
		iret = func_op_jb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jbe:
		//op_jbe               0x010707
		iret = func_op_jbe(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jg:
		//op_jg                0x010708
		iret = func_op_jg(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jge:
		//op_jge               0x010709
		iret = func_op_jge(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jl:
		//op_jl                0x01070a
		iret = func_op_jl(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jle:
		//op_jle               0x01070b
		iret = func_op_jle(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jc:
		//op_jc                0x01070c
		iret = func_op_jc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jnc:
		//op_jnc               0x01070d
		iret = func_op_jnc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jo:
		//op_jo                0x01070e
		iret = func_op_jo(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jno:
		//op_jno               0x01070f
		iret = func_op_jno(&xx_inst, item_var, in_context, out_context);
		break;
	case op_js:
		//op_js                0x010710
		iret = func_op_js(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jns:
		//op_jns               0x010711
		iret = func_op_jns(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jnp:
		//op_jpo               0x010712
		iret = func_op_jnp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jp:
		//op_jpe               0x010713
		iret = func_op_jp(&xx_inst, item_var, in_context, out_context);
		break;
	case op_jecxz:
		//op_jecxz             0x010714 
		iret = func_op_jecxz(&xx_inst, item_var, in_context, out_context);
		break;
	case op_loop:
		//op_loop              0x010715
		//iret = func_op_loop(&xx_inst, item_var, in_context, out_context);
		break;
	case op_loope:
		//op_loope             0x010716
		//iret = func_op_loope(&xx_inst, item_var, in_context, out_context);
		break;
	case op_loopne:
		//op_loopne            0x010717
		//iret = func_op_loopne(&xx_inst, item_var, in_context, out_context);
		break;
	case op_call:
		iret = func_op_call(&xx_inst, item_var, in_context, out_context);
		//op_call              0x010718
		break;
	case op_retn:
		//op_retn               0x010719
		iret = func_op_retn(&xx_inst, item_var, in_context, out_context);
		break;
	case op_iret:
		//op_iret              0x01071a
		break;
	case op_int:
		//op_int               0x01071b
		break;
	case op_into:
		//op_into              0x01071c
		break;
	case op_bound:
		//op_bound             0x01071d 
		break;
	case op_enter:
		//op_enter         0x01071e
		break;
	case op_leave:
		//op_leave         0x01071f
		//iret = func_op_leave(&xx_inst, item_var, in_context, out_context);
		break;
	case op_movsb:
		//op_movsb       0x010801
		iret = func_op_movsb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_movs:
		//op_movs      0x010802
		//iret = func_op_movs(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmpsb:
		//op_cmps       0x010804
		break;
	case op_cmps:
		//op_cmpsw      0x010805
		break;
	case op_scasb:
		//op_scas       0x010807
		break;
	case op_scas:
		//op_scasw      0x010808
		break;
	case op_lodsb:
		//op_lodsb      0x01080a
		//iret = func_op_lodsb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_lods:
		//op_lods       0x01080b
		//iret = func_op_lods(&xx_inst, item_var, in_context, out_context);
		break;
	case op_stosb:
		//op_stosb       0x01080d
		//iret = func_op_stosb(&xx_inst, item_var, in_context, out_context);
		break;
	case op_stos:
		//op_stos       0x01080e
		//iret = func_op_stos(&xx_inst, item_var, in_context, out_context);
		break;
	case op_rep:
		//op_rep        0x010810
		break;
	case op_repe:
		//op_repe       0x010811
		break;
	case op_repne:
		//op_repne      0x010812
		break;
	case op_in:
		//op_in       0x010901
		break;
	case op_out:
		//op_out      0x010902
		break;
	case op_ins:
		//op_ins      0x010903
		break;
	case op_insw:
		//op_insw     0x010904
		break;
	case op_insd:
		//op_insd     0x010905
		break;
	case op_outs:
		//op_outs     0x010906
		break;
	case op_outsw:
		//op_outsw    0x010907
		break;
	case op_outsd:
		//op_outsd    0x010908
		break;
	case op_stc:
		//op_stc              0x010b01
		iret = func_op_stc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_clc:
		//op_clc              0x010b02
		iret = func_op_clc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cmc:
		//op_cmc              0x010b03
		iret = func_op_cmc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cld:
		//op_cld              0x010b04
		iret = func_op_cld(&xx_inst, item_var, in_context, out_context);
		break;
	case op_std:
		//op_std              0x010b05
		iret = func_op_std(&xx_inst, item_var, in_context, out_context);
		break;
	case op_lahf:
		//op_lahf             0x010b06
		iret = func_op_lahf(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sahf:
		//op_sahf             0x010b07
		iret = func_op_sahf(&xx_inst, item_var, in_context, out_context);
		break;
	case op_pushfd:
		//op_pushfd           0x010b08
		iret = func_op_pushfd(&xx_inst, item_var, in_context, out_context);
		break;
	case op_popfd:
		//op_popfd            0x010b09
		iret = func_op_popfd(&xx_inst, item_var, in_context, out_context);
		break;
	case op_sti:
		//op_sti              0x010b0a
		iret = func_op_sti(&xx_inst, item_var, in_context, out_context);
		break;
	case op_cli:
		//op_cli              0x010b0b
		iret = func_op_cli(&xx_inst, item_var, in_context, out_context);
		break;
	case op_lds:
		//op_lds           0x010c01
		break;
	case op_les:
		//op_les           0x010c02
		break;
	case op_lfs:
		//op_lfs           0x010c03
		break;
	case op_lgs:
		//op_lgs           0x010c04
		break;
	case op_lss:
		//op_lss           0x010c05
		break;
	case op_lea:
		//op_lea           0x010d01
		iret = func_op_lea(&xx_inst, item_var, in_context, out_context);
		break;
	case op_nop:
		//op_nop           0x010d02
		iret = func_op_nop(&xx_inst, item_var, in_context, out_context);
		break;
	case op_ud2:
		//op_ud2           0x010d03
		break;
	case op_xlat:
		//op_xlat          0x010d04
		break;
	case op_cpuid:
		//op_cpuid         0x010d05
		iret = func_op_cpuid(&xx_inst, item_var, in_context, out_context);
		break;
	case op_movbe:
		//op_movbe         0x010d06
		break;
	case op_prefetchw:
		//op_prefetchw     0x010d07
		break;
	case op_prefetchwt1:
		//op_prefetchwt1   0x010d08
		break;
	case op_clflush:
		//op_clflush       0x010d09
		break;
	case op_clflushopt:
		//op_clflushopt    0x010d0a
		break;
	case op_rdtsc:
		//op_rdtsc         0x010d0b
		iret = func_op_rdtsc(&xx_inst, item_var, in_context, out_context);
		break;
	case op_xsave:
		//op_xsave         0x010e01
		break;
	case op_xsavec:
		//op_xsavec        0x010e02
		break;
	case op_xsaveopt:
		//op_xsaveopt      0x010e03
		break;
	case op_xrstor:
		//op_xrstor        0x010e04
		break;
	case op_xgetbv:
		//op_xgetbv        0x010e05
		break;
	default:
		return 0;
	}

	/*修复rflag*/
	out_context->eflag = out_context->eflag & 0x000000000000fffd;
	if ((out_context->eflag & 0x0000000000000100)==0x0000000000000100)
	{
		/*有断点暂停*/
		return 0;
	}
#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "rax:%llx rcx:%llx rdx:%llx rbx:%llx rsp:%llx rbp:%llx rsi:%llx rdi:%llx rip:%llx rflag:%llx", \
		out_context->r[REG_RAX], out_context->r[REG_RCX], out_context->r[REG_RDX], out_context->r[REG_RBX]\
		, out_context->r[REG_RSP], out_context->r[REG_RBP], out_context->r[REG_RSI], out_context->r[REG_RDI]\
		, out_context->ip, out_context->eflag);
	debug_log(debug_buf);

	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "r8:%llx r9:%llx r10:%llx r11:%llx r12:%llx r13:%llx r14:%llx r15:%llx ", \
		out_context->r[REG_R8], out_context->r[REG_R9], out_context->r[REG_R10], out_context->r[REG_R11]\
		, out_context->r[REG_R12], out_context->r[REG_R13], out_context->r[REG_R14], out_context->r[REG_R15]);
	debug_log(debug_buf);
#endif
	

	return iret;
}


int calc_item(struct XX_INST *inst, int nitem, struct ITEM_VAR64 *out_var, \
	struct XX_CONTEXT64 *out_context, unsigned long long result,int sign_flag)
{
	int iret = 0;
	int index = 0;
	char base_def[100];
	unsigned long *ptmp = 0;

	if (inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_IADDR || \
		inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_DIS || \
		inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_IMM || \
		inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_CONST)
	{
		return 1;
	}
	else if (inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_REG)
	{
		memset(base_def, 0, sizeof(base_def));
		nstr_hex(inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def, \
			inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len, base_def);

		index = get_reg_index(inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def);
		if (base_def[5] == 0)
		{
			//al
			memcpy((char*)&out_context->r[index], (char*)&result, 1);
		}
		else if (base_def[5] == 1)
		{
			//ah
			memcpy((char*)&out_context->r[index] + 1, (char*)&result, 1);
		}
		else if (base_def[5] == 2)
		{
			//ax
			memcpy((char*)&out_context->r[index], (char*)&result, 2);
		}
		else if (base_def[5] == 4)
		{
			//eax
			/*64位目的寄存器符号扩展*/
			if (inst->xx_inst_exist.rexw_flag != INST_FLAG_EXIST && sign_flag==1)
			{

				ptmp = (unsigned long*)&out_context->r[index];
				if (result >= 0x80000000)
				{
					ptmp[1] = 0xffffffff;
				}
				else
				{
					ptmp[1] = 0x00000000;
				}
			}
			else if (inst->xx_inst_exist.rexw_flag != INST_FLAG_EXIST && sign_flag == 2)
			{

				ptmp = (unsigned long*)&out_context->r[index];
				ptmp[1] = 0x00000000;
			}
			memcpy((char*)&out_context->r[index], (char*)&result, 4);
		}
		else if (base_def[5] == 6)
		{
			//rax
			memcpy((char*)&out_context->r[index], (char*)&result, 8);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		iret = xx_write_fmem(out_var[nitem].addr, (unsigned char*)&result, (unsigned long)out_var[nitem].size);
		if (iret == 0)
		{
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf,"xx_write_fmem addr:[%llx] error\r\n", out_var[nitem].addr);
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}
#ifdef DEBUG_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "xx_write_fmem  ip:[%llx] ", out_context->ip);
		debug_log(debug_buf);
#endif
	}

	return 1;
}


/*计算操作项的值
返回
	0:失败
	1:不是内存地址
	2:内存地址
*/
int conv_item(struct XX_INST *inst, unsigned int op_type, int nitem, \
	struct XX_CONTEXT64 *in_context, struct ITEM_VAR64 *out_var)
{
	int iret = 0;
	int n = 0;
	unsigned long long base_reg = 0;
	unsigned long long index_reg = 0;
	unsigned long long index_scale = 0;
	unsigned long long imm = 0;
	unsigned long long var = 0;
	int index = 0;  //reg index
	char var_prefix = 0;
	int var_const = 0;
	char base_def[100];


	var = 0;
#if 0
	if (op_type == op_push || op_type == op_pop)
	{
		out_var->size = inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;
	}
	else
	{
		out_var->size = inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size;
	}
#else
	out_var->size = inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size;
#endif

	/*操作项转化为一个具体的值*/
	if (inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_IADDR || \
		inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_DIS || \
		inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_IMM || \
		inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_CONST)
	{
		if (op_type == op_push)
		{
			out_var->size = inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;
		}
		var = *(unsigned  long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const;
		out_var->var = var;

	}
	else if (inst->xx_inst_items.xx_inst_items_flag[nitem].item_type == INST_ITEM_TYPE_REG)
	{
		/*区分寄存器，重写*/
		memset(base_def, 0, sizeof(base_def));
		nstr_hex(inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def, \
			inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len, base_def);

		index = get_reg_index(inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def);
		if (base_def[5] == 0)
		{
			//al
			memcpy((char*)&var, (char*)&in_context->r[index], 1);
		}
		else if (base_def[5] == 1)
		{
			//ah
			memcpy((char*)&var, (char*)&in_context->r[index] + 1, 1);
		}
		else if (base_def[5] == 2)
		{
			//ax
			memcpy((char*)&var, (char*)&in_context->r[index], 2);
		}
		else if (base_def[5] == 4)
		{
			//eax
			memcpy((char*)&var, (char*)&in_context->r[index], 4);
		}
		else if (base_def[5] == 6)
		{
			//rax
			memcpy((char*)&var, (char*)&in_context->r[index], 8);
		}
		else
		{
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item_base_def[5] =%x",\
				(unsigned char)inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def[5]);
			debug_log(debug_buf);
#endif
			return 0;
		}
		//var=var+in_context->r[index];
		out_var->var = var;

	}
	else
	{
		if (inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len)
		{
			index = get_reg_index(inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def);
			var = var + in_context->r[index];
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item[%d]: +base=%llx", nitem, var);
			debug_log(debug_buf);
#endif
			if (index == REG_RSP)
			{
				if (op_type == op_pop)
				{
					var = var + out_var->size;
				}
			}
		}

		if (inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len)
		{
			if (inst->xx_inst_items.xx_inst_items_var[nitem].index_scale)
			{
				index = get_reg_index(inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def);
				var = var + ((in_context->r[index])*(inst->xx_inst_items.xx_inst_items_var[nitem].index_scale));
			}
			else
			{
				index = get_reg_index(inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def);
				var = var + in_context->r[index];
			}
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item[%d]: +index=%llx", nitem, var);
			debug_log(debug_buf);
#endif
			if (index == REG_RSP)
			{
				if (op_type == op_pop)
				{
					var = var + out_var->size;
				}
			}
		}

		if (inst->xx_inst_items.xx_inst_items_var[nitem].const_size == 1)
		{
			if (inst->xx_inst_items.xx_inst_items_var[nitem].item_const[0] >= 0x80)
			{
				var = var - (~(*(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const) & 0x000000ff) - 1;
			}
			else
			{
				var = var + *(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const;
			}
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item[%d]: +bconst=%llx", nitem, var);
			debug_log(debug_buf);
#endif
		}
		else if (inst->xx_inst_items.xx_inst_items_var[nitem].const_size == 2)
		{
			if (inst->xx_inst_items.xx_inst_items_var[nitem].item_const[1] >= 0x80)
			{
				var = var - (~(*(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const) & 0x0000ffff) - 1;
			}
			else
			{
				var = var + *(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const;
			}
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item[%d]: +wconst=%llx", nitem, var);
			debug_log(debug_buf);
#endif
		}
		else if (inst->xx_inst_items.xx_inst_items_var[nitem].const_size == 4)
		{
			if (inst->xx_inst_items.xx_inst_items_var[nitem].item_const[3] >= 0x80)
			{
				var = var - (~(*(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const) & 0xffffffff) - 1;
			}
			else
			{
				var = var + *(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const;
			}
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item[%d]: +dconst=%llx", nitem, var);
			debug_log(debug_buf);
#endif
		}
		else if (inst->xx_inst_items.xx_inst_items_var[nitem].const_size == 8)
		{
			if (inst->xx_inst_items.xx_inst_items_var[nitem].item_const[7] >= 0x80)
			{
				var = var - (~(*(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const) & 0xffffffffffffffff) - 1;
			}
			else
			{
				var = var + *(unsigned long long*)inst->xx_inst_items.xx_inst_items_var[nitem].item_const;
			}
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "item[%d]: +dconst=%llx", nitem, var);
			debug_log(debug_buf);
#endif
		}
		//64  gs 
#if 1
		for (n = 0; n < (inst->xx_inst_code.prefix_len); n++)
		{
			var_prefix = inst->xx_inst_code.prefix[n];
			if (var_prefix == (unsigned char)0x65)
			{
				var = seg_gs + var;
				break;
			}
		}
		
#endif
		out_var->addr = var;

		if (op_type == op_lea)
		{
			return 1;
		}

		iret = xx_read_fmem(out_var->addr, (char*)&out_var->var, (unsigned long)out_var->size);
		if (iret == 0)
		{
			//debug_log("xx_read_fmem");
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "xx_read_fmem  error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}
	}

	return 1;
}


/*函数，取内存数据*/
int xx_read_fmem(unsigned long long addr, char* pdata, unsigned int datasize)
{
	int iret = 0;
	char filename[500];
	unsigned long long foffset = 0;
	/*获取内存数据文件*/
#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "xx_read_fmem(%llx, %08x )", addr, datasize);
	debug_log(debug_buf);
#endif

	memset(filename, 0, sizeof(filename));
	iret = xx_get_fmem(addr, filename, &foffset);
	if (iret == 0)
	{
		//debug_log("xx_get_fmem");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_get_fmem  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "xx_read_file(%s,%llx,%08x )", filename, foffset, datasize);
	debug_log(debug_buf);
#endif
	/*从文件中读取数据*/
	iret = xx_read_file(filename, pdata, (unsigned long)foffset, datasize);
	if (iret == 0)
	{
		//debug_log("xx_read_file");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_read_file  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
	return 1;
}

/*函数，写内存数据*/
int xx_write_fmem(unsigned long long addr, char* pdata, unsigned int datasize)
{
	int iret = 0;
	char filename[500];
	unsigned long long foffset = 0;
	/*获取内存数据文件*/
#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "xx_write_fmem(%llx, %08x )", addr, datasize);
	debug_log(debug_buf);
#endif

	memset(filename, 0, sizeof(filename));
	iret = xx_get_fmem(addr, filename, &foffset);
	if (iret == 0)
	{
		//debug_log("xx_get_fmem");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_get_fmem  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "addr:[%llx] xx_scover_file(%s,%llx,%08x )", addr, filename, foffset, datasize);
	debug_log(debug_buf);
#endif

	/*写入文件数据*/
	iret = xx_scover_file(filename, pdata, (unsigned long)foffset, datasize);
	if (iret == 0)
	{
		//debug_log("xx_scover_file");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_scover_file  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	return 1;
}



/*获取内存数据文件*/
int xx_get_fmem(unsigned long long addr, char *outname, unsigned long long *out_offset)
{
	int iret = 0;
	struct XX_LINK_NODE *var_link;
	struct XX_FMEM64 *var_fmem;
	struct XX_FMEM64 buf_fmem;
	/*从链表中获取节点xx_fmem*/


	var_link = xx_list_get(xx_link, 1);

	if (var_link == 0)
	{
		//debug_log("xx_list_get error");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "var_link  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
	while (1)
	{
		var_fmem = 0;
		var_link = xx_list_get(var_link, 0);
		if (var_link == 0)
		{
			break;
		}

		var_fmem = var_link->pdata;
		if ((addr >= var_fmem->start) && (addr < var_fmem->end))
		{
			break;
		}
	}

	if (var_fmem != 0)
	{
		/*有则，赋值给outname，out_offset，成功返回1*/
		memcpy(outname, var_fmem->filename, strlen(var_fmem->filename));
		*out_offset = addr - var_fmem->start;

#ifdef DEBUG_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "var_fmem->start:%llx var_fmem->end:%llx var_fmem->filename:%s offset:%llx", \
			var_fmem->start, var_fmem->end, var_fmem->filename, *out_offset);
		debug_log(debug_buf);
#endif
		return 1;
	}

	/*没有，则生成数据文件*/
	memset(&buf_fmem, 0, sizeof(struct XX_FMEM64));
	iret = xx_gen_fmem(addr, &buf_fmem, 1);
	if (iret != 1)
	{
		//debug_log("xx_gen_fmem");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_gen_fmem  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	/*将xx_fmem插入链表*/
	xx_list_insert(xx_link, &buf_fmem, sizeof(struct XX_FMEM64));

	/*赋值给outname，out_offset,成功返回1*/
	memcpy(outname, buf_fmem.filename, strlen(buf_fmem.filename));
	*out_offset = addr - buf_fmem.start;

#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "buf_fmem.start:%llx buf_fmem.end:%llx buf_fmem.filename:%s offset:%llx", \
		buf_fmem.start, buf_fmem.end, buf_fmem.filename, *out_offset);
	debug_log(debug_buf);
#endif
	return 1;
}



////////////////////////////////////////////////////////////////////////////
int xx_gen_fmem(unsigned long long addr, struct XX_FMEM64 *xx_fmem, int flag)
{
	unsigned int iret = 0;
	MEMORY_BASIC_INFORMATION meminfo;
	char *buf = 0;
	char tmp[0x400];
	HANDLE hp = 0;

	/*check*/
	if (xx_fmem == 0)
	{
		//debug_log("xx_fmem==0");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_fmem  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	/*获取addr相应内存信息*/
	hp = (HANDLE)DbgGetProcessHandle();
	//DbgMemMap(MEMMAP* memmap);

	memset(&meminfo, 0, sizeof(MEMORY_BASIC_INFORMATION));
	iret = VirtualQueryEx((HANDLE)hp, (LPCVOID)addr, &meminfo, sizeof(meminfo));
	if (iret == 0)
	{
#ifdef DEBUG_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "addr:[%llx]",addr);
		debug_log(debug_buf);
#endif
		//memset(debug_buf,0,sizeof(debug_buf));
		//sprintf(debug_buf,"%s","VirtualQueryEx  error\r\n");
		//xx_append_file(log_file,debug_buf,strlen(debug_buf));
		return  -1;
	}

	if (meminfo.State != MEM_COMMIT)
	{
#ifdef DEBUG_LOG
		xx_fmem->start = (unsigned long long)meminfo.BaseAddress;
		xx_fmem->end = ((unsigned long long)meminfo.BaseAddress + meminfo.RegionSize);
		debug_log("VirtualQueryEx");
#endif
		//memset(debug_buf,0,sizeof(debug_buf));
		//sprintf(debug_buf,"%s","VirtualQueryEx  error\r\n");
		//xx_append_file(log_file,debug_buf,strlen(debug_buf));
		return 0;
	}

	if (flag ==0)
	{
		if (meminfo.AllocationBase != meminfo.BaseAddress)
		{
			xx_fmem->start = (unsigned long long)meminfo.BaseAddress;
			xx_fmem->end = ((unsigned long long)meminfo.BaseAddress + meminfo.RegionSize);
			//debug_log("VirtualQueryEx");
			return 0;
		}
	}

#ifdef DEBUG_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "addr:%llx meminfo.AllocationBase:%llx meminfo.BaseAddress:%llx meminfo.RegionSize:%llx ", \
		addr, (unsigned long long)meminfo.AllocationBase, (unsigned long long)meminfo.BaseAddress, (unsigned long long) meminfo.RegionSize);
	debug_log(debug_buf);
#endif

	/*申请空间，拷贝数据*/
	buf = malloc(meminfo.RegionSize);
	if (buf == 0)
	{
		//debug_log("malloc");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "malloc  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}
	memset(buf, 0, meminfo.RegionSize);
	
	iret = DbgMemRead((unsigned long long)meminfo.BaseAddress, buf, meminfo.RegionSize);
	if (iret == 0)
	{
		free(buf);
		//debug_log("_Readmemory");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "_Readmemory  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	/*生成文件名，赋值xx_fmem*/
	memset(tmp, 0, sizeof(tmp));
	sprintf(tmp, "%s%llx-%llx.x", fmempath, (unsigned long long)meminfo.BaseAddress, \
		((unsigned long long)meminfo.BaseAddress + meminfo.RegionSize));

	xx_fmem->start = (unsigned long long)meminfo.BaseAddress;
	xx_fmem->end = ((unsigned long long)meminfo.BaseAddress + meminfo.RegionSize);
	memcpy(xx_fmem->filename, tmp, strlen(tmp));


	/*写入文件，释放空间*/
	iret = xx_cover_file(xx_fmem->filename, buf, meminfo.RegionSize);
	if (iret == 0)
	{
		free(buf);
		//debug_log("xx_cover_file");
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "%s", "xx_cover_file  error\r\n");
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
		return 0;
	}

	free(buf);

	return 1;
}


int xdbg_gen_fmem()
{
	int iret = 0;
	MEMMAP mmap;
	int n = 0;
	MEMORY_BASIC_INFORMATION *meminfo;
	char *buf = 0;
	char tmp[0x400];
	struct XX_FMEM64 xx_fmem;
	char tset_buf[100];

	memset(&mmap, 0, sizeof(mmap));
	iret = DbgMemMap(&mmap);
	if (iret == 0)
	{
		MessageBox(0, "DbgMemMap", "error", MB_OK);
		return 0;
	}

	//memset(debug_buf, 0, sizeof(debug_buf));
	//sprintf(debug_buf, "%d", mmap.count);
	//MessageBox(0, debug_buf, "error", MB_OK);

	for (n = 0; n < mmap.count; n++)
	{
		meminfo = &(mmap.page[n].mbi);
		buf = 0;

		if (meminfo->State != MEM_COMMIT)
		{
			/*需要测试精确判断*/
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%d %llx-%llx-%llx  %d",n, \
				(ulong64)meminfo->BaseAddress, (ulong64)meminfo->AllocationBase, (ulong64)meminfo->RegionSize,meminfo->State);
			debug_log(debug_buf);
#endif
			continue;
		}
		/*尝试读取，读取失败则下一个*/
		memset(tset_buf, 0, sizeof(tset_buf));
		iret = DbgMemRead((unsigned long long)meminfo->BaseAddress, tset_buf, 10);
		if (iret == 0)
		{
			/*尝试读取失败，继续下一个*/
			continue;
		}


		/*申请空间，拷贝数据*/
		buf = malloc(meminfo->RegionSize);
		if (buf == 0)
		{
			//debug_log("malloc");
			//memset(debug_buf, 0, sizeof(debug_buf));
			//sprintf(debug_buf, "%s", "malloc  error\r\n");
			//xx_append_file(log_file, debug_buf, strlen(debug_buf));
			//return 0;

			/*申请失败，继续下一个*/
			continue;
		}
		memset(buf, 0, meminfo->RegionSize);

		iret = DbgMemRead((unsigned long long)meminfo->BaseAddress, buf, meminfo->RegionSize);
		if (iret == 0)
		{
			free(buf);
			//debug_log("_Readmemory");
			//memset(debug_buf, 0, sizeof(debug_buf));
			//sprintf(debug_buf, "%s", "DbgMemRead  error\r\n");
			//xx_append_file(log_file, debug_buf, strlen(debug_buf));
			//return 0;

			/*读取失败，继续下一个*/
			continue;
		}

		/*生成文件名，赋值xx_fmem*/
		memset(tmp, 0, sizeof(tmp));
		sprintf(tmp, "%s%llx-%llx.x", fmempath, (unsigned long long)meminfo->BaseAddress, \
			((unsigned long long)meminfo->BaseAddress + meminfo->RegionSize));

		memset(&xx_fmem, 0, sizeof(xx_fmem));
		xx_fmem.start = (unsigned long long)meminfo->BaseAddress;
		xx_fmem.end = ((unsigned long long)meminfo->BaseAddress + meminfo->RegionSize);
		memcpy(xx_fmem.filename, tmp, strlen(tmp));


		/*写入文件，释放空间*/
		iret = xx_cover_file(xx_fmem.filename, buf, meminfo->RegionSize);
		if (iret == 0)
		{
			free(buf);
			//debug_log("xx_cover_file");
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", "xx_cover_file  error\r\n");
			xx_append_file(log_file, debug_buf, strlen(debug_buf));
			return 0;
		}

		free(buf);

		xx_list_insert(xx_link, &xx_fmem, sizeof(struct XX_FMEM64));
	}

	return 1;
}


/*模拟执行初始化
包括日志文件检查
*/
int init_mem_file(char *dbg_path)
{
	int iret = 0;
	unsigned long long addr = 0;
	struct XX_FMEM64 buf_fmem;
	HANDLE hprocess = 0;
	time_t ttime;
	struct tm *pt;

	if (strlen(log_file) == 0)
	{
		MessageBox(0, "log error. init log ", "error", MB_OK);
		return 0;
	}


	/*初始化链表*/
	if (xx_link == 0)
	{
		xx_link = xx_list_init();
	}

	iret = free_mem_file();
	if (iret == 0)
	{
		MessageBox(0, "data clean error", "error", MB_OK);
		return 0;
	}

	/*创建文件目录*/
	//iret = CreateDirectory(fmempath, NULL);
	if (strlen(dbg_path) == 0)
	{
		MessageBox(0, "dbg_path error", "error", MB_OK);
		return 0;
	}

	//fmempath_size = strlen(dbg_path) + sizeof("xx_fmem\\") + 1;
	fmempath_size = strlen(dbg_path) + 10 + 15 + 1;
	fmempath = malloc(fmempath_size);
	if (fmempath == 0)
	{
		MessageBox(0, "malloc error", "error", MB_OK);
		return 0;
	}
	memset(fmempath, 0, fmempath_size);

	memset(&ttime, 0, sizeof(ttime));
	time(&ttime);
	pt = localtime(&ttime);

	sprintf(fmempath, "%s%s_%04d%02d%02d%02d%02d%02d\\", dbg_path, "xx_fmem", \
		(1900 + pt->tm_year), (1 + pt->tm_mon), pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);

	CreateDirectory(fmempath, NULL);

	memset(debug_buf, 0, sizeof(debug_buf));

	if (xx_link == 0)
	{
		MessageBox(0, "link error", "error", MB_OK);
		return 0;
	}

#if 0
	/*生成数据文件*/
	addr = 0x1000;
	while (1)
	{
		/*生成数据文件*/
		memset(&buf_fmem, 0, sizeof(struct XX_FMEM64));
		iret = xx_gen_fmem(addr, &buf_fmem, 0);
		if (iret == -1)
		{
			//debug_log("init xx_gen_fmem finish");
			break;
		}
		else if (iret == 1)
		{
			/*将xx_fmem插入链表*/
			xx_list_insert(xx_link, &buf_fmem, sizeof(struct XX_FMEM64));

			//continue;
}
		//addr = addr + 0x1000;
		addr = buf_fmem.end;
	}
#else
	iret = xdbg_gen_fmem();
	if (iret == 0)
	{
		MessageBox(0, "xdbg_gen_fmem", "error", MB_OK);
		return 0;
	}
#endif

	/*环境配置*/
	hprocess = (HANDLE)DbgGetProcessHandle();

	//_plugin_logputs("hprocess:[%llx]",hprocess);
	if (hprocess == 0)
	{
		//_plugin_logputs("_Plugingetvalue VAL_HPROCESS error");
		MessageBox(0, "no process error", "error", MB_OK);
		return 0;
	}

#if 0
	/*测试关闭，重写*/
	iret = init_vmsreg(hinst);
	if (iret == -1)
	{
		//_plugin_logputs("init_vmreg error");
		MessageBox(0, "not activated error", "error", MB_OK);
		return 0;
	}
#endif

	return 1;
}


int free_mem_file()
{
	int iret = 0;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char DirSpec[MAX_PATH];  // directory specification
	int dwError;


	/*清空链表*/
	xx_list_clear(xx_link);

	/*删除文件*/
	if (fmempath == 0)
	{
		return 1;
	}

	memset(DirSpec, 0, sizeof(DirSpec));
	sprintf(DirSpec, "%s%s", fmempath, "*.x");

	hFind = FindFirstFile(DirSpec, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		memset(DirSpec, 0, sizeof(DirSpec));
		sprintf(DirSpec, "%s%s", fmempath, FindFileData.cFileName);
		iret = DeleteFile(DirSpec);
		if (iret == 0)
		{
			MessageBox(0, "DeleteFile error", "error", MB_OK);
			return 0;
	}
		while (FindNextFile(hFind, &FindFileData) != 0)
		{
			//printf ("Next file name is %s\n", FindFileData.cFileName);
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "%s", FindFileData.cFileName);
			debug_log(debug_buf);
#endif

			memset(DirSpec, 0, sizeof(DirSpec));
			sprintf(DirSpec, "%s%s", fmempath, FindFileData.cFileName);
			iret = DeleteFile(DirSpec);
			if (iret == 0)
			{
				MessageBox(0, "DeleteFile error", "error", MB_OK);
				return 0;
			}
		}

		dwError = GetLastError();
		FindClose(hFind);
		if (dwError != ERROR_NO_MORE_FILES)
		{
			//debug_log("FindNextFile error. Error is");
#ifdef DEBUG_LOG
			memset(debug_buf, 0, sizeof(debug_buf));
			sprintf(debug_buf, "FindNextFile error. %x", dwError);
			debug_log(debug_buf);
#endif
			MessageBox(0, "FindNextFile error", "error", MB_OK);
			return 0;
		}
			}


	if (fmempath)
	{
		RemoveDirectory(fmempath);
		free(fmempath);
		fmempath = 0;
		fmempath_size = 0;
	}

	return 1;
}



int xx_init64(char* mainname)
{
	char *pname=0;
	int iret = 0;

	char sret[200];
	char tmp[200];
	char dsret[] = "xxxx";


	if (strlen(xdbg_path) == 0)
	{
		//MessageBox(0, "x64dbg Path", "error", MB_OK);
		return 0;
	}

	memset(log_file, 0, sizeof(log_file));

	pname = malloc(strlen(mainname) + 1);
	if (pname == 0)
	{
		return 0;
	}
	memset(pname, 0, strlen(mainname) + 1);
	memcpy(pname, mainname, strlen(mainname));


	iret = init_file_path(xdbg_path, pname, log_file);
	if (iret)
	{
		memset(log_file, 0, sizeof(log_file));
		//MessageBox(0, "Init Log error,open a process first,please", "error", MB_OK);
		free(pname);
		return 0;
	}

	memset(opmz_file, 0, sizeof(opmz_file));
	memcpy(opmz_file, log_file, strlen(log_file));

	memset(opmz_dfile, 0, sizeof(opmz_dfile));
	memcpy(opmz_dfile, log_file, strlen(log_file)); 

	memset(opmz_log, 0, sizeof(opmz_log));
	memcpy(opmz_log, log_file, strlen(log_file));

	memset(opmz_cmd, 0, sizeof(opmz_cmd));
	sprintf(opmz_cmd, "xx_opmz.exe %s", log_file);

	memset(exp_file, 0, sizeof(exp_file));
	memcpy(exp_file, xdbg_path, strlen(xdbg_path));
	/*创建目录*/
	iret = CreateDirectory(log_file, NULL);
	
	/*初始化日志文件名*/
	strcat(log_file, pname);
	strcat(log_file, ".txt");
	//MessageBox(0, log_file, "success", MB_OK);

	/*初始化优化文件名opmz_file*/
	strcat(opmz_file, inst_file_name);
	//MessageBox(0, opmz_file, "success", MB_OK);
	strcat(opmz_dfile, inst_dfile_name);

	strcat(opmz_log, opmz_log_name);

	strcat(exp_file, exp_file_name);

	free(pname);
	return 1;
};


int init_file_path(char *ppath, char *pname, char *file)
{
	//char filename[500];
	char filepath[0x400];

	//int *p = (int *)&filename;
	time_t ttime;
	struct tm *pt;
	char tmp[200];


	if (pname == 0 || file == 0)
	{
		return -1;
	}

	if (strlen(pname) == 0)
	{
		return -1;
	}

	memset(filepath, 0, sizeof(filepath));
	memset(tmp, 0, sizeof(tmp));
	memset(&ttime, 0, sizeof(ttime));
	//memcpy(filepath,XX_VM_LOG_PATH,sizeof(XX_VM_LOG_PATH));
	strcat(filepath, ppath);
	strcat(filepath, pname);
	strcat(filepath, "_");
	time(&ttime);
	pt = localtime(&ttime);
	sprintf(filepath + strlen(filepath), "%04d%02d%02d%02d%02d%02d", (1900 + pt->tm_year), (1 + pt->tm_mon), pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);
	strcat(filepath, "\\");

	memcpy(file, filepath, strlen(filepath));

	return 0;
}


int debug_log(char *tmp)
{

	xx_append_file("debug_log.txt", tmp, strlen(tmp));
	xx_append_file("debug_log.txt", "\r\n", strlen("\r\n"));

	return 1;
}

