#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "st_slist.h"



/*
新建单项链表
返回链表结构，
外部只传递结构
返回：链表结构
*/
__declspec(noinline) void *xx_slist_new()
{
	void *p = 0;

	p = malloc(sizeof(struct ST_SLIST));
	if (p)
	{
		memset(p, 0, sizeof(struct ST_SLIST));
	}
	return p;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void xx_slist_free_func(void *pnode);
/*
释放单项链表
参数1：链表结构
返回：无
*/
__declspec(noinline) void xx_slist_free(void *pst_slist)
{
	struct ST_SLIST *pslist = 0;

	if (pst_slist == 0)
	{
		return;
	}

	pslist = pst_slist;
	
	/*先释放节点空间*/
	xx_slist_free_func(pslist->pstart);

	/*释放数据链空间*/
	pslist->pstart = 0;
	pslist->total = 0;
	free(pst_slist);

	return;
}


__declspec(noinline) void xx_slist_free_func(void *pnode)
{
	struct ST_SLIST_NODE *pvar = 0;

	if (pnode == 0)
	{
		return;
	}

	pvar = pnode;

	if (pvar->pnext)
	{
		xx_slist_free_func(pvar->pnext);
	}

	/*先释放数据*/
	if (pvar->pdata)
	{
		free(pvar->pdata);
		pvar->pdata = 0;
		pvar->seq = 0;
	}

	/*再释放节点空间*/
	free(pvar);

	return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void xx_slist_free_node_func(void *pnode);
/*
释放单项链表
参数1：链表结构
返回：无
*/
__declspec(noinline) void xx_slist_free_node(void *pst_slist)
{
	struct ST_SLIST *pslist = 0;

	if (pst_slist == 0)
	{
		return;
	}

	pslist = pst_slist;

	/*先释放节点空间*/
	xx_slist_free_node_func(pslist->pstart);

	/*释放数据链空间*/
	pslist->pstart = 0;
	pslist->total = 0;
	free(pst_slist);

	return;
}


__declspec(noinline) void xx_slist_free_node_func(void *pnode)
{
	struct ST_SLIST_NODE *pvar = 0;

	if (pnode == 0)
	{
		return;
	}

	pvar = pnode;

	if (pvar->pnext)
	{
		xx_slist_free_node_func(pvar->pnext);
	}

	/*先释放数据*/
	if (pvar->pdata)
	{
		//free(pvar->pdata);
		pvar->pdata = 0;
		pvar->seq = 0;
	}

	/*再释放节点空间*/
	free(pvar);

	return;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int xx_slist_insert_data_func(void *pnext, void *pdata);
/*
插入数据
参数1：链表结构
参数2：待插入的数据
参数3：数据大小
返回：成功-1.失败-0
*/
__declspec(noinline) int xx_slist_insert_data(void *pst_slist, void *pdata, int data_size)
{
	int iret = 0;
	struct ST_SLIST *pst = 0;
	void *ptmp = 0;

	/*参数检查*/
	if (pst_slist == 0 || pdata == 0 || data_size == 0)
	{
		return 0;
	}

	pst = pst_slist;

	/*申请数据空间*/
	ptmp = malloc(data_size);
	if (ptmp == 0)
	{
		return 0;
	}
	memset(ptmp, 0, data_size);

	memcpy(ptmp, pdata, data_size);

	/*检查是否有起始节点，没有则赋值*/
	if (pst->pstart == 0)
	{
		/*新建链表节点*/
		pst->pstart = malloc(sizeof(struct ST_SLIST_NODE));
		if (pst->pstart == 0)
		{
			free(ptmp);
			return 0;
		}
		memset(pst->pstart, 0, sizeof(struct ST_SLIST_NODE));

		/*新节点数据赋值*/
		pst->pstart->pdata = ptmp;
		pst->pstart->pnext = 0;
		pst->pstart->seq = 0;

	}
	else
	{
		/*插入数据*/
		iret = xx_slist_insert_data_func(pst->pstart, ptmp);
		if (iret == 0)
		{
			free(ptmp);
			return 0;
		}
	}
	/*节点总数加1*/
	pst->total = pst->total + 1;

	return 1;
}


/*
插入数据
*/
__declspec(noinline) int xx_slist_insert_data_func(void *pnext,void *pdata)
{
	int iret = 0;
	struct ST_SLIST_NODE *pst_next = 0;
	struct ST_SLIST_NODE *pnode = 0;

	if (pdata == 0)
	{
		return 0;
	}

	pst_next = pnext;

	if (pst_next->pnext)
	{
		iret = xx_slist_insert_data_func(pst_next->pnext, pdata);
		if (iret == 0)
		{
			return 0;
		}
	}
	else
	{
		/*新建链表节点*/
		pnode = malloc(sizeof(struct ST_SLIST_NODE));
		if (pnode == 0)
		{
			return 0;
		}
		memset(pnode, 0, sizeof(struct ST_SLIST_NODE));

		/*新节点数据赋值*/
		pnode->pdata = pdata;
		pnode->pnext = 0;
		pnode->seq = pst_next->seq + 1;

		/*节点指针赋值*/
		pst_next->pnext = pnode;
	}
	return 1;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *xx_slist_find_func1(void *pnext, void *pdata, def_slist_cb_cmp pcb);
/*
查找，从头部开始
参数1：链表结构
参数2：待查找的关联数据
参数3：数据比较回调函数
返回：成功-需要的数据，失败-0
*/
__declspec(noinline) void *xx_slist_find1(void *pst_slist, void *pdata, def_slist_cb_cmp pcb)
{
	struct ST_SLIST *pst = 0;
	void *pret = 0;
	
	/*参数检查*/
	if (pst_slist == 0 || pdata == 0 || pcb == 0)
	{
		return 0;
	}

	pst = pst_slist;
	
	pret = xx_slist_find_func1(pst->pstart, pdata, pcb);
	
	return pret;
}

__declspec(noinline) void *xx_slist_find_func1(void *pnext, void *pdata, def_slist_cb_cmp pcb)
{
	struct ST_SLIST_NODE *pnode = 0;
	void *pret = 0;

	if (pnext == 0 || pdata == 0 || pcb == 0)
	{
		return 0;
	}

	pnode = pnext;

	pret = pcb(pdata, pnode->pdata);
	if (pret)
	{
		return pret;
	}

	pret = xx_slist_find_func1(pnode->pnext, pdata, pcb);
	

	return pret;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *xx_slist_find_func2(void *pnext, void *pdata, def_slist_cb_cmp pcb);
/*
查找，从尾部开始
参数1：链表结构
参数2：待查找的关联数据
参数3：数据比较回调函数
返回：成功-需要的数据，失败-0
*/
__declspec(noinline) void *xx_slist_find2(void *pst_slist, void *pdata, def_slist_cb_cmp pcb)
{
	struct ST_SLIST *pst = 0;
	void *pret = 0;

	/*参数检查*/
	if (pst_slist == 0 || pdata == 0 || pcb == 0)
	{
		return 0;
	}

	pst = pst_slist;

	if (pst->pstart == 0)
	{
		return 0;
	}

	pret = xx_slist_find_func2(pst->pstart, pdata, pcb);

	return pret;
}

__declspec(noinline) void *xx_slist_find_func2(void *pnext, void *pdata, def_slist_cb_cmp pcb)
{
	struct ST_SLIST_NODE *pnode = 0;
	void *pret = 0;

	if (pdata == 0 || pcb == 0)
	{
		return 0;
	}

	pnode = pnext;

	if (pnode->pnext != 0)
	{
		pret = xx_slist_find_func2(pnode->pnext, pdata, pcb);
		if (pret != 0)
		{
			return pret;
		}
	}

	pret = pcb(pdata, pnode->pdata);
	
	return pret;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
打印链中的节点数据
参数1：链结构
参数2：打印回调函数
返回：无
*/
__declspec(noinline) void xx_slist_print(void *pst_slist, def_slist_cb_print cb)
{
	struct ST_SLIST *pst = 0;
	struct ST_SLIST_NODE *pnode = 0;

	/*检查参数*/
	if (pst_slist == 0 || cb == 0)
	{
		return;
	}

	pst = pst_slist;
	pnode = pst->pstart;

	if (pnode == 0)
	{
		return;
	}

	while (1)
	{
		cb(pnode->pdata);

		if (pnode->pnext == 0)
		{
			break;
		}

		pnode = pnode->pnext;
	}

	return ;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
插入数据
参数1：链表结构
参数2：待插入的数据
参数3：数据大小
返回：成功-1.失败-0
内部直接使用数据指针，用于数据为动态申请的情况
*/
__declspec(noinline) int xx_slist_insert_data2(void *pst_slist, void *pdata)
{
	int iret = 0;
	struct ST_SLIST *pst = 0;

	/*参数检查*/
	if (pst_slist == 0 || pdata == 0)
	{
		return 0;
	}

	pst = pst_slist;


	/*检查是否有起始节点，没有则赋值*/
	if (pst->pstart == 0)
	{
		/*新建链表节点*/
		pst->pstart = malloc(sizeof(struct ST_SLIST_NODE));
		if (pst->pstart == 0)
		{
			return 0;
		}
		memset(pst->pstart, 0, sizeof(struct ST_SLIST_NODE));

		/*新节点数据赋值*/
		pst->pstart->pdata = pdata;
		pst->pstart->pnext = 0;
		pst->pstart->seq = 0;

	}
	else
	{
		/*插入数据*/
		iret = xx_slist_insert_data_func(pst->pstart, pdata);
		if (iret == 0)
		{
			return 0;
		}
	}
	/*节点总数加1*/
	pst->total = pst->total + 1;

	return 1;
}






__declspec(noinline) void xx_slist_data(void *pst_slist, def_slist_cb_data cb)
{
	struct ST_SLIST *pst = 0;
	struct ST_SLIST_NODE *pnode = 0;

	/*检查参数*/
	if (pst_slist == 0 || cb == 0)
	{
		return;
	}

	pst = pst_slist;
	pnode = pst->pstart;

	if (pnode == 0)
	{
		return;
	}

	while (1)
	{
		cb(pnode->pdata);

		if (pnode->pnext == 0)
		{
			break;
		}

		pnode = pnode->pnext;
	}

	return;
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int xx_slist_insert_data_func3(void *pnext, void *pdata, def_slist_cb_exist pcb, def_slist_cb_exist pcb_sort);
/*
插入数据
参数1：链表结构
参数2：待插入的数据
参数3：数据大小
返回：成功-1.失败-0
*/
__declspec(noinline) int xx_slist_insert_data3(void *pst_slist, void *pdata, int data_size, def_slist_cb_exist pcb, def_slist_cb_exist pcb_sort)
{
	int iret = 0;
	struct ST_SLIST *pst = 0;
	void *ptmp = 0;

	/*参数检查*/
	if (pst_slist == 0 || pdata == 0 || data_size == 0)
	{
		return 0;
	}

	pst = pst_slist;

	/*申请数据空间*/
	ptmp = malloc(data_size);
	if (ptmp == 0)
	{
		return 0;
	}
	memset(ptmp, 0, data_size);

	memcpy(ptmp, pdata, data_size);

	/*检查是否有起始节点，没有则赋值*/
	if (pst->pstart == 0)
	{
		/*新建链表节点*/
		pst->pstart = malloc(sizeof(struct ST_SLIST_NODE));
		if (pst->pstart == 0)
		{
			free(ptmp);
			return 0;
		}
		memset(pst->pstart, 0, sizeof(struct ST_SLIST_NODE));

		/*新节点数据赋值*/
		pst->pstart->pdata = ptmp;
		pst->pstart->pnext = 0;
		pst->pstart->seq = 0;

	}
	else
	{
		/*插入数据*/
		iret = xx_slist_insert_data_func3(pst->pstart, ptmp, pcb, pcb_sort);
		if (iret == 0)
		{
			free(ptmp);
			return 0;
		}
		else if (iret == 2)
		{
			/*当节点数据重复时，需要释放为节点数据申请的空间*/
			free(ptmp);
			return 1;
		}
	}

	/*节点总数加1*/

	pst->total = pst->total + 1;

	return 1;
}


/*
插入数据
*/
__declspec(noinline) int xx_slist_insert_data_func3(void *pnext, void *pdata, def_slist_cb_exist pcb, def_slist_cb_exist pcb_sort)
{
	int iret = 0;
	struct ST_SLIST_NODE *pst_next = 0;
	struct ST_SLIST_NODE *pnode = 0;

	if (pdata == 0)
	{
		return 0;
	}

	pst_next = pnext;

	/*判断数据是否重复，重复则直接返回*/
	iret = pcb(pdata, pst_next->pdata);
	if (iret == 1)
	{
		return 2;
	}

	/*判断数据排序，返回1则插入*/
	iret = pcb_sort(pdata, pst_next->pdata);
	if (iret == 1)
	{
		/*此处插入新的节点*/
		pnode = malloc(sizeof(struct ST_SLIST_NODE));
		if (pnode == 0)
		{
			return 0;
		}
		memset(pnode, 0, sizeof(struct ST_SLIST_NODE));

		/*新节点数据赋值*/
		pnode->pdata = pst_next->pdata;
		pnode->pnext = pst_next->pnext;


		/*当前节点数据赋值*/
		pst_next->pdata = pdata;
		pst_next->pnext = pnode;

		return 1;
	}

	if (pst_next->pnext)
	{
		iret = xx_slist_insert_data_func3(pst_next->pnext, pdata, pcb, pcb_sort);
		return iret;

	}
	else
	{
		/*新建链表节点*/
		pnode = malloc(sizeof(struct ST_SLIST_NODE));
		if (pnode == 0)
		{
			return 0;
		}
		memset(pnode, 0, sizeof(struct ST_SLIST_NODE));

		/*新节点数据赋值*/
		pnode->pdata = pdata;
		pnode->pnext = 0;
		pnode->seq = pst_next->seq + 1;

		/*节点指针赋值*/
		pst_next->pnext = pnode;

		return 1;
	}

	return 0;
}



