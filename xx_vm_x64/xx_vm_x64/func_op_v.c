#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm64.h"

void *pfunc_v = 0;


void func_op_vmem_free();

/*
…Í«Îø’º‰
*/
int func_op_vmem()
{
	if (pfunc_v != 0)
	{
		func_op_vmem_free();
	}

	pfunc_v = VirtualAlloc(0, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (pfunc_v == 0)
	{
		return 0;
	}

	memset(pfunc_v, 0, 0x1000);

	return 1;
}



void func_op_vmem_free()
{
	VirtualFree(pfunc_v, 0, MEM_DECOMMIT);

	pfunc_v = 0;

	return;
}



























