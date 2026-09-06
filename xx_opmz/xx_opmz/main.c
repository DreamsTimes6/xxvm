#include <stdio.h>
#include <Windows.h>
#include "zlog.h"
#include "xx_def.h"
#include "xx_comm32.h"
#include "xx_exp_data.h"
#include "xx_opmz.h"
#include "analyze_cfg.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HINSTANCE hinst;
int pause_flag;
char xdbg_path[0x400];
char gopmz_log[0x500];
char ginst_dfile[0x500];
char gexp_file[0x500];
char ginst_file[0x500];

zlog_category_t *zc;

struct ST_VM_INST *g_vm_inst;    //伪指令指针
int g_inst_num = 0;        //伪指令数目

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void log_init();
int xx_opmz_init_file(char *plog_path);
int  xx_opmz_file_check(char *pfile1, char *pfile2);
int  xx_opmz_read_inst(char *pinst_file);
int xx_opmz_piece_seq(struct ST_VM_INST *pvm_inst, int inst_num, int start_seq);
int xx_opmz_bak(char *pinst_file, char *pinst_dfile);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern int xx_opmz(struct ST_VM_INST *pvm_inst, int num);



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPCSTR lpCmdLine, int nCmdShow)
{
	int iret = 0;
	int piece_seq = 0;
	int piece_seq0 = 0;
	hinst = hInstance;
	struct ST_VM_INST *pvm_inst = 0;
	int buf_size = 0;
	char tmp[100];

	/*初始化日志*/
	log_init();

	//zlog_info(zc, "=========WinMain start [%s]=========\n", lpCmdLine);


	/*初始化文件*/
	iret = xx_opmz_init_file((char*)lpCmdLine);
	if (iret == 0)
	{
		MessageBox(0, "init file", "error", MB_OK);
		return 0;
	}

	//zlog_info(zc, "xx_opmz_init_file finish\n");


	/*初始化配置文件*/
	xx_cfg_init(xdbg_path);

	/*获取激活码*/
	memset(tmp, 0, sizeof(tmp));
	iret = xx_cfg_get_str("XX_ACT_CODE", tmp, sizeof(tmp));
	if (iret == 0)
	{
		MessageBox(0, "cfg file", "error", MB_OK);
		return 0;
	}



	/*从文件中读取指令*/
	iret = xx_opmz_read_inst(ginst_file);
	if (iret == 0)
	{
		MessageBox(0, "read inst", "error", MB_OK);
		return 0;
	}

	piece_seq0 = 0;
	piece_seq = 0;
	while (1)
	{
		if ((piece_seq0 + 1) >= g_inst_num)
		{
			break;
		}

		/*从piece_seq0开始查找片段指令，从下一个指令开始*/
		piece_seq = xx_opmz_piece_seq(g_vm_inst, g_inst_num, piece_seq0 + 1);
		if (piece_seq == -1)
		{
			/*如果没找到，则剩余的指令化为一个片段*/
			if ((piece_seq0 + 1) >= g_inst_num)
			{
				break;
			}
			/*最后需要包含retn*/
			//piece_seq = g_inst_num - 1;
			piece_seq = g_inst_num;
		}


		//zlog_info(zc, "xx_opmz piece_seq0:[%d] piece_seq:[%d]\n", piece_seq0, piece_seq);

		/*申请片段指令的缓冲区，方便中间指令修改*/
		buf_size = (piece_seq - piece_seq0) * sizeof(struct ST_VM_INST);
		pvm_inst = malloc(buf_size);
		if (iret == 0)
		{
			zlog_error(zc, "pvm_inst buf piece_seq0:[%d] piece_seq:[%d]\n", piece_seq0, piece_seq);
			break;
		}
		memset(pvm_inst, 0, buf_size);

		memcpy(pvm_inst, &g_vm_inst[piece_seq0], buf_size);
		
		/*分析指令片段，指令数目不包括片段指令*/
		//iret = xx_opmz(&g_vm_inst[piece_seq0], piece_seq - piece_seq0);
		iret = xx_opmz(pvm_inst, piece_seq - piece_seq0);
		if (iret == 0)
		{
			free(pvm_inst);
			pvm_inst = 0;
			MessageBox(0, "xx_opmz", "error", MB_OK);
			zlog_error(zc, "piece_seq0:[%d] piece_seq:[%d]\n", piece_seq0, piece_seq);
			break;
		}

		/*释放缓冲区*/
		free(pvm_inst);
		pvm_inst = 0;

		piece_seq0 = piece_seq;

		/*从下一个指令开始*/
		//piece_seq0 = piece_seq0 + 1;
	}

	/*结束释放空间*/
	if (g_vm_inst)
	{
		free(g_vm_inst);
		g_vm_inst = 0;
	}

	/*备份数据文件*/
	xx_opmz_bak(ginst_file, ginst_dfile);

	//zlog_info(zc, "=========WinMain finish =========\n");
	MessageBox(0, "Finish", "success", MB_OK);

	return 1;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////

__declspec(noinline) int xx_opmz_init_file(char *plog_path)
{
	int iret = 0;


	char tmp[0x100];

	//zlog_info(zc, "=========xx_opmz_init_file start [%s]=========\n", plog_path);

	/*检查参数*/
	if (plog_path == 0)
	{
		MessageBox(0, "log error. init log ", "error", MB_OK);
		return 0;
	}

	memset(xdbg_path, 0, sizeof(xdbg_path));
	iret = GetCurrentDirectory(sizeof(xdbg_path), xdbg_path);
	if (iret == 0)
	{
		MessageBox(0, "GetCurrentDirectory error", "error", MB_OK);
		return 0;
	}

	strcat(xdbg_path, "\\");

	/*初始化文件名*/
	memset(gexp_file, 0, sizeof(gexp_file));
	sprintf(gexp_file, "%s%s", xdbg_path, "xx_exp.x");

	memset(gopmz_log, 0, sizeof(gopmz_log));
	sprintf(gopmz_log, "%s\\%s", plog_path, "xx_opmz.txt");

	memset(ginst_dfile, 0, sizeof(ginst_dfile));
	sprintf(ginst_dfile, "%s\\%s", plog_path, "vm_opmz_data.x");

	memset(ginst_file, 0, sizeof(ginst_file));
	sprintf(ginst_file, "%s\\%s", plog_path, "vm_opmz.x");


	//zlog_info(zc, "gexp_file:[%s]\n", gexp_file);
	//zlog_info(zc, "gopmz_log:[%s]\n", gopmz_log);
	//zlog_info(zc, "ginst_dfile:[%s]\n", ginst_dfile);
	//zlog_info(zc, "ginst_file:[%s]\n", ginst_file);



	/*检查一下指令文件是否存在，不存在说明已经优化*/
	iret = xx_get_file_size(ginst_file);
	if (iret == 0)
	{
		MessageBox(0, "It's optimized already,or the log was not existed", "Info", MB_OK);
		return 0;
	}
	

	/*还原数据初始化，失败则退出*/
	iret = xx_exp_data_init(gexp_file);
	if (iret == 0)
	{
		MessageBox(0, "The file is not exist('xx_exp.x'),please check the opmz_version", "Info", MB_OK);
		return 0;
	}

	/*检查指令文件数据大小*/
	iret = xx_opmz_file_check(ginst_file, ginst_dfile);
	if (iret == 0)
	{
		MessageBox(0, "The size of file is error", "Info", MB_OK);
		return 0;
	}

	//zlog_info(zc, "=========xx_opmz_init_file finish=========\n");

	return 1;
}




/*
检查文件
vm_opmz.x   vm_opmz_data.x
检查文件大小，指令数是否相等
参数1：vm_opmz.x
参数2：vm_opmz_data.x
*/

__declspec(noinline) int  xx_opmz_file_check(char *pfile1, char *pfile2)
{
	int file_size = 0;
	int num1 = 0;
	int num2 = 0;


	//zlog_info(zc, "=========xx_opmz_file_check start=========\n");

	/*参数检查*/
	if (pfile1 == 0 || pfile2 == 0)
	{
		zlog_error(zc, "pfile1:[%s] pfile2:[%s]\n", pfile1, pfile2);
		return 0;
	}

	/*获取文件大小*/
	file_size = xx_get_file_size(pfile1);
	if (file_size == 0)
	{
		zlog_error(zc, "file_size:[%d]\n", file_size);
		return 0;
	}

	/*文件大小判断*/
	if ((file_size % sizeof(struct ST_VM_INST)) != 0)
	{
		return 0;
	}
	if (file_size < sizeof(struct ST_VM_INST))
	{
		return 0;
	}

	num1 = file_size / sizeof(struct ST_VM_INST);

	/*获取文件大小*/
	file_size = xx_get_file_size(pfile2);
	if (file_size == 0)
	{
		zlog_error(zc, "file_size:[%d]\n", file_size);
		return 0;
	}

	/*文件大小判断*/
	if ((file_size % sizeof(struct ST_VM_ITEM_DATA)) != 0)
	{
		return 0;
	}
	if (file_size < sizeof(struct ST_VM_ITEM_DATA))
	{
		return 0;
	}

	num2 = file_size / sizeof(struct ST_VM_ITEM_DATA);

	if (num1 != num2)
	{
		zlog_error(zc, "num1:[%d] num2:[%d]\n", num1, num2);
		return 0;
	}

	//zlog_info(zc, "=========xx_opmz_file_check finish=========\n");

	return 1;
};






/*
从文件中读取指令
*/
__declspec(noinline) int  xx_opmz_read_inst(char *pinst_file)
{
	int n = 0;
	int iret = 0;
	int file_size = 0;    //文件大小
	

	//zlog_info(zc, "=========xx_opmz_read_inst start=========\n");


	/*获取文件大小*/
	file_size = xx_get_file_size(pinst_file);
	if (file_size == 0)
	{
		zlog_error(zc, "xx_get_file_size pinst_file:[]\n", g_inst_num);
		goto l_err;
	}

	if (g_vm_inst)
	{
		free(g_vm_inst);
		g_vm_inst = 0;
	}

	/*申请空间*/
	g_vm_inst = malloc(file_size + 1);
	if (g_vm_inst == 0)
	{
		goto l_err;
	}
	memset(g_vm_inst, 0, file_size + 1);

	/*读入文件*/
	iret = xx_read_file(pinst_file, (char*)g_vm_inst, 0, file_size);
	if (iret == 0)
	{
		goto l_err;
	}

	g_inst_num = file_size / sizeof(struct ST_VM_INST);

	/*全局指令赋值序号*/
	for (n = 0; n < g_inst_num; n++)
	{
		g_vm_inst[n].seq = n;
	}

	//zlog_info(zc, "g_inst_num:[%d]\n", g_inst_num);

	//zlog_info(zc, "=========xx_opmz_read_inst finish=========\n");

	return 1;

l_err:
	if (g_vm_inst)
	{
		free(g_vm_inst);
		g_vm_inst = 0;
	}

	//zlog_info(zc, "=========xx_opmz_read_inst error finish=========\n");

	return 0;
}






/*
获取分段指令索引
*/
__declspec(noinline) int xx_opmz_piece_seq(struct ST_VM_INST *pvm_inst, int inst_num, int start_seq)
{
	int n = 0;
	int iret = 0;


	/*扫描分段指令*/
	n = 0;
	for (n = start_seq; n < inst_num; n++)
	{
		if ((pvm_inst[n].idef & 0xff00) == 0x1200)
		{
			pvm_inst[n].opmz_flag = -1;
		}

		/*考虑结束边界*/
		if ((pvm_inst[n].idef & 0xff00) == 0x0000 || \
			(pvm_inst[n].idef & 0xff00) == 0x0f00 || \
			(pvm_inst[n].idef & 0xff00) == 0x0f01)
		{
			pvm_inst[n].opmz_flag = -1;

			return n;
		}
	}

	return -1;
}







__declspec(noinline) void log_init()
{
	int iret = 0;
	char log_file[] = "xx_opmz.conf";

	iret = zlog_init(log_file);
	if (iret)
	{
		MessageBox(0, log_file, "log err", MB_OK);
		goto l_err;
	}

	zc = zlog_get_category("main");
	if (!zc)
	{
		MessageBox(0, "zlog_get_category xx_analyzer", "log err", MB_OK);
		goto l_err;
	}

	return;

l_err:
	zlog_fini();

	return;
}






/*
重复操作【分析-优化】时，指令文件清空，产生新文件，日志文件则追加
重复操作【优化】时，优化日志会重复，不允许重复操作【优化】，优化完成后，备份指令文件，删除指令文件
*/
__declspec(noinline) int xx_opmz_bak(char *pinst_file, char *pinst_dfile)
{
	int bak_size = 0;
	char *vm_opmz_bak = 0;
	char *vm_opmz_data_bak = 0;

	bak_size = strlen(pinst_file);
	bak_size = bak_size + 50;
	vm_opmz_bak = malloc(bak_size);
	if (vm_opmz_bak == 0)
	{
		return 0;
	}
	memset(vm_opmz_bak, 0, bak_size);
	sprintf(vm_opmz_bak, "%s.bak", pinst_file);
	DeleteFile(vm_opmz_bak);
	CopyFile(pinst_file, vm_opmz_bak, 0);
	DeleteFile(pinst_file);
	free(vm_opmz_bak);



	bak_size = strlen(pinst_dfile);
	bak_size = bak_size + 50;
	vm_opmz_data_bak = malloc(bak_size);
	if (vm_opmz_data_bak == 0)
	{
		return 0;
	}
	memset(vm_opmz_data_bak, 0, bak_size);
	sprintf(vm_opmz_data_bak, "%s.bak", pinst_dfile);
	DeleteFile(vm_opmz_data_bak);
	CopyFile(pinst_dfile, vm_opmz_data_bak, 0);
	DeleteFile(pinst_dfile);
	free(vm_opmz_data_bak);

	return 1;
}




