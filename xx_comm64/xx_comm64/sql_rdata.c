#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_def.h"
#include <windows.h>

char *sql_list_file="sql_list";
//////////////////////////////////////


extern void* get_sql_struct(int tbl);
extern int get_sql_rows_num(int tbl);
extern void* get_sql_lstruct(int tbl);
extern int get_sql_lrows_num(int tbl);
extern int get_sql_item_size(int tbl);
extern int get_sql_litem_size(int tbl);
extern int get_sql_list_num();


extern struct XX_INIT_TBL_LIST* get_sql_list_st();

extern int get_res_data(char* rctype,int rcid,char* pdata,int size);
extern int get_res_size(char* rctype,int rcid,int *psize);


int init_flag=0;
extern HINSTANCE hinst;


int get_res_size(char* rctype, int rcid, int *psize)
{
	HRSRC hrc = 0;
	int size = 0;

	hrc = FindResource(hinst, rcid, rctype);
	if (hrc == 0)
	{
		return 0;
	}
	size = SizeofResource(hinst, hrc);
	if (size == 0)
	{
		return 0;
	}
	*psize = size;
	return 1;
}


int get_res_data(char* rctype, int rcid, char* pdata, int size)
{
	HRSRC hrc = 0;
	HGLOBAL  hg = 0;
	char *pres = 0;

	hrc = FindResource(hinst, rcid, rctype);
	if (hrc == 0)
	{
		return 0;
	}
	hg = LoadResource(hinst, hrc);
	if (hg == 0)
	{
		return 0;
	}
	pres = LockResource(hg);
	if (pres == 0)
	{
		return 0;
	}
	memcpy(pdata, pres, size);
	return 1;
}




/////////////////////////////////////
_export int init_sqlfile()
{
	int n=0;
	int iret=0;
	int file_size=0;
	struct XX_INIT_TBL_LIST  *sql_list;
	int tbl_num=0;
	char *ptmp=0;

	if(init_flag==1)
	{
		return 0;
	}

	sql_list=get_sql_list_st();
	if(sql_list==0)
	{
		return -1;
	}

	tbl_num=get_sql_list_num();
	if(tbl_num<=0)
	{
		return -1;
	}
	

	for(n=0;n<tbl_num;n++)
	{
		file_size=0;
		iret=get_res_size(sql_list[n].data_name,100,&file_size);
		if(iret==0 || file_size==0)
		{
			return -1;
		}
		ptmp=malloc(file_size+1);
		if(ptmp==0)
		{
			return -1;
		}
		memset(ptmp,0,sizeof(ptmp));
		iret=get_res_data(sql_list[n].data_name,100,ptmp,file_size);
		if(iret==0)
		{
			return -1;
		}
		sql_list[n].ptbl=ptmp;
		sql_list[n].rows_num=file_size/(sql_list[n].item_size);
		
		//ldata
		file_size=0;
		iret=get_res_size(sql_list[n].ldata_name,100,&file_size);
		if(iret==0 || file_size==0)
		{
			return -1;
		}
		ptmp=malloc(file_size+1);
		if(ptmp==0)
		{
			return -1;
		}
		memset(ptmp,0,sizeof(ptmp));
		iret=get_res_data(sql_list[n].ldata_name,100,ptmp,file_size);
		if(iret==0)
		{
			return -1;
		}
		sql_list[n].pltbl=ptmp;
		sql_list[n].lrows_num=file_size/(sql_list[n].litem_size);
		///////////////////////////////////////////////

	}

	init_flag=1;
	return 0;
}

_export int free_sqlfile()
{
	int n=0;
	int num=0;
	struct XX_INIT_TBL_LIST  *sql_list;


	if(init_flag==0)
	{
		return 0;
	}


	sql_list=get_sql_list_st();
	if(sql_list==0)
	{
		return -1;
	}

	num=get_sql_list_num();
	if(num<=0)
	{
		return -1;
	}
	
	for(n=0;n<num;n++)
	{
		free(sql_list[n].ptbl);
		free(sql_list[n].pltbl);
		sql_list[n].ptbl=0;
		sql_list[n].pltbl=0;
		sql_list[n].rows_num=0;
		sql_list[n].lrows_num=0;
	}

	init_flag=0;
	return 0;
}











