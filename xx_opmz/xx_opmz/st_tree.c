
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


#include "st_tree.h"
#include "xx_comm32.h"





////////////////////////////////////////////////////////////////////////////////////
/*接口用到的子函数*/

void xx_tree_free_func(struct ST_NODE *pnode);
struct ST_NODE *xx_tree_data_lowsub_func(struct ST_NODE *proot, void *pdata, def_tree_cb_cmp cb);
struct ST_NODE *xx_tree_lowsub_func(struct ST_NODE *proot, struct ST_NODE *pnode, def_tree_cb_cmp cb);
struct ST_NODE *xx_tree_sub_func(struct ST_NODE *proot, struct ST_NODE *pnode, def_tree_cb_cmp cb);
int xx_tree_check_func(struct ST_NODE *pnode, def_tree_cb_data cb);
int xx_tree_low_check_func(struct ST_NODE *pnode, def_tree_cb_data cb);
void xx_tree_data_num_func(struct ST_NODE *pnode, int *pnum);
int xx_tree_print_func(struct ST_NODE *pnode, def_tree_cb_data cb);
void *xx_tree_find_lowdata_func(struct ST_NODE *pnode, void *pdata, def_tree_cb_cmp cb);
void xx_tree_set_idata_func(struct ST_NODE *pnode, def_tree_cb_set_idata cb, int idata);
void xx_tree_set_low_idata_func(struct ST_NODE *pnode, def_tree_cb_set_idata cb, int idata);


int xx_tree_node_exp_func(struct ST_NODE *pnode, def_tree_cb_node_exp cb);
void xx_tree_exp_free_func(struct ST_NODE *pnode);
//void xx_tree_opmz_exp_func(struct ST_NODE *pnode, def_tree_cb_opmz_exp cb);
int xx_tree_opmz_clr_exp_func(struct ST_NODE *pnode, def_tree_cb_node_exp cb);
void xx_tree_opmz_idef_func(struct ST_NODE *pnode, def_tree_cb_opmz_idef cb);
void xx_tree_opmz_exp_func(struct ST_NODE *pnode, def_tree_cb_opmz_exp cb);


void xx_tree_time_flush_func(struct ST_NODE *pnode, long sec, long nsec);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
根据根数据和子数据建立树结构
参数1：根数据
参数2：根数据大小
参数3：子数据数目
参数4：子数据指针数组
参数5：子数据大小的数组
返回：树结构-成功，0-失败
*/
  __declspec(noinline) struct ST_TREE* xx_tree_new(void *proot_data, int root_size, \
	int sub_num, void **psub_data, int *sub_size)
{
	int iret = 0;
	int n = 0;
	struct ST_TREE *ptree = 0;
	struct ST_NODE *pnode = 0;
	struct _timespec32  _time;


	/*检查参数*/
	if (proot_data == 0 || root_size==0 || sub_num==0 || \
		psub_data==0 || sub_size==0)
	{
		return 0;
	}

	/*检查子项参数*/
	for (n = 0; n < sub_num; n++)
	{
		if (psub_data[n] == 0 || sub_size[n] == 0)
		{
			return 0;
		}
	}

	/*新建树结构*/
	ptree = malloc(sizeof(struct ST_TREE));
	if (ptree == 0)
	{
		goto l_err;
	}

	memset(ptree, 0, sizeof(struct ST_TREE));

	/*设置树时间戳*/
	_timespec32_get(&_time, TIME_UTC);
	ptree->sec = (long)_time.tv_sec;
	ptree->nsec = (long)_time.tv_nsec;

	/*设置随机数*/
	srand(RAND_MAX);
	ptree->rand = rand();

	/*新建父节点*/
	pnode = xx_node_new();
	if (pnode == 0)
	{
		goto l_err;
	}

	/*设置为根节点*/
	ptree->proot = pnode;

	/*设置根节点数据*/
	iret = xx_node_set_data(ptree->proot, proot_data, root_size);
	if (iret == 0)
	{
		goto l_err;
	}

	/*设置根节点时间戳*/
	xx_node_set_time(ptree->proot, ptree->sec, ptree->nsec);

	/*设置根节点随机数*/
	xx_node_set_rand(ptree->proot, ptree->rand);


	/*设置子节点数目*/
	iret = xx_node_set_sub_num(ptree->proot, sub_num);
	if (iret == 0)
	{
		goto l_err;
	}

	for (n = 0; n < sub_num; n++)
	{
		pnode = 0;
		/*新建节点*/
		pnode = xx_node_new();
		if (pnode == 0)
		{
			goto l_err;
		}

		/*设置为子节点*/
		iret = xx_node_set_sub(ptree->proot, n, pnode);
		if (iret == 0)
		{
			goto l_err;
		}

		/*设置子节点数据*/
		iret = xx_node_set_data(pnode, psub_data[n], sub_size[n]);
		if (iret == 0)
		{
			goto l_err;
		}

		/*设置时间戳*/
		xx_node_set_time(pnode, ptree->sec, ptree->nsec);

		/*设置随机数*/
		xx_node_set_rand(pnode, ptree->rand);
	}

	/*总节点数*/
	ptree->inode = sub_num + 1;

	return ptree;

#if 0
l_err2:
	for (n = 0; n < ptree->proot->sub_num; n++)
	{
		if (ptree->proot->psub[n].pst_node)
		{
			xx_node_free(ptree->proot);
		}
	}

l_err1:
	if (ptree)
	{
		if (ptree->proot)
		{
			xx_node_free(ptree->proot);
		}
		free(ptree);
	}
#else

l_err:
	xx_tree_free(ptree);
#endif

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////


/*由一个根数据生成树
参数1：父节点数据
参数2：父节点数据大小
返回：树结构
*/
__declspec(noinline) struct ST_TREE *xx_tree_new_root(void *pdata,int data_size)
{
	int iret = 0;
	int n = 0;
	struct ST_TREE *ptree = 0;
	struct ST_NODE *pnode = 0;
	struct _timespec32  _time;


	/*检查参数*/
	if (pdata == 0 )
	{
		return 0;
	}

	/*新建树结构*/
	ptree = malloc(sizeof(struct ST_TREE));
	if (ptree == 0)
	{
		goto l_err;
	}

	memset(ptree, 0, sizeof(struct ST_TREE));

	/*设置树时间戳*/
	_timespec32_get(&_time, TIME_UTC);
	ptree->sec = (long)_time.tv_sec;
	ptree->nsec = (long)_time.tv_nsec;

	/*设置随机数*/
	srand(RAND_MAX);
	ptree->rand = rand();

	/*总节点数*/
	ptree->inode = 1;

	/*新建父节点*/
	pnode = xx_node_new();
	if (pnode == 0)
	{
		goto l_err;
	}

	/*设置为根节点*/
	ptree->proot = pnode;

	/*设置根节点数据*/
	iret = xx_node_set_data(pnode, pdata, data_size);
	if (iret == 0)
	{
		goto l_err;
	}

	/*设置根节点时间戳*/
	xx_node_set_time(ptree->proot, ptree->sec, ptree->nsec);

	/*设置根节点随机数*/
	xx_node_set_rand(ptree->proot, ptree->rand);

	return ptree;

l_err:
	xx_tree_free(ptree);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

/*
给树的根节点添加子数据节点
参数1：树结构
参数2：子数据
参数3：子数据大小
返回：1-成功，0-失败
*/
__declspec(noinline) int  xx_tree_add_data(struct ST_TREE *ptree,void *pdata,int data_size)
{
	int iret = 0;
	struct ST_NODE *pnode = 0;


	/*检查参数*/
	if (ptree == 0 || pdata == 0 || data_size == 0)
	{
		return 0;
	}

	pnode = 0;
	/*新建节点*/
	pnode = xx_node_new();
	if (pnode == 0)
	{
		goto l_err;
	}

	/*设置子节点数据*/
	iret = xx_node_set_data(pnode, pdata, data_size);
	if (iret == 0)
	{
		goto l_err;
	}

	/*给根节点添加子节点*/
	iret = xx_node_add_sub(ptree->proot, pnode);
	if (iret == 0)
	{
		goto l_err;
	}

	/*设置时间戳*/
	xx_node_set_time(pnode, ptree->sec, ptree->nsec);

	/*设置随机数*/
	xx_node_set_rand(pnode, ptree->rand);

	/*树成员赋值，总节点数*/
	ptree->inode = ptree->inode + 1;

	return 1;

l_err:

	free(pnode);

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////



/*释放树结构
生成树失败，也进行释放操作
不能重复调用，重复调用会出错
*/
__declspec(noinline) void  xx_tree_free(struct ST_TREE *ptree)
{
	int iret = 0;
	int n = 0;
	int isub = 0;
	struct ST_NODE *pnode = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return;
	}

	/*开始释放*/
	xx_tree_free_func(ptree->proot);

	if (ptree->pexp != 0)
	{
		free(ptree->pexp);
		ptree->pexp = 0;
	}

	/*释放树*/
	free(ptree);

	return ;
};


__declspec(noinline) void  xx_tree_free_func(struct ST_NODE *pnode)
{
	int n = 0;

	if (pnode == 0)
	{
		return ;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_free_func(pnode->psub[n].pst_node);
	}

	xx_node_free(pnode);

	return ;
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
	判断该数据是否已经存在
		包括根节点
	参数1：树结构
	参数2：待查找的数据，ST_TREE_DATA
	参数3：数据比较的回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
	返回：1-存在，0-不存在
*/
__declspec(noinline) int xx_tree_data_lowsub(struct ST_TREE *ptree, void *pdata, def_tree_cb_cmp cb)
{
	struct ST_NODE *pret = 0;

	/*检查参数*/
	if (ptree == 0 || pdata == 0)
	{
		return 0;
	}


	pret = xx_tree_data_lowsub_func(ptree->proot, pdata, cb);
	if (pret == 0)
	{
		return 0;
	}

	return 1;
}

__declspec(noinline) struct ST_NODE *xx_tree_data_lowsub_func(struct ST_NODE *pnode, void *pdata, def_tree_cb_cmp cb)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *pret = 0;


	if (pnode == 0)
	{
		return 0;
	}

	/*最低级子节点必有头节点，且没有子节点*/
	if (pnode->phead != 0 && pnode->sub_num == 0)
	{
		if (cb != 0)
		{
			/*调用自定义回调函数*/
			iret = cb(pnode->pdata,pdata);
			if (iret != 0)
			{
				//printf("sec:%x-%x  nsec:%x-%x\n", proot->sec, proot->nsec, pnode->sec, pnode->nsec);
				return pnode;
			}
		}
		else
		{
			/*比较数据，一样则返回成功*/
			iret = memcmp(pnode->pdata, pdata, pnode->data_size);
			if (iret == 0)
			{
				return pnode;
			}
		}
		return 0;

	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		pret = 0;
		pret = xx_tree_data_lowsub_func(pnode->psub[n].pst_node, pdata, cb);
		if (pret != 0)
		{
			return pret;
		}
	}

	return 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
判断树2的根节点是否是树1的最低级子节点
参数1：树结构1
参数2：树结构2
参数3：数据比较回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
返回：1-树1的某个最低级子节点；0-不是树1的最低级子节点
*/
__declspec(noinline) int xx_tree_is_lowsub(struct ST_TREE *ptree1, struct ST_TREE *ptree2, def_tree_cb_cmp cb)
{
	struct ST_NODE *pnode = 0;

	/*检查参数*/
	if (ptree1 == 0 || ptree2 == 0)
	{
		return 0;
	}

	/*遍历树1的节点*/
	pnode = xx_tree_lowsub_func(ptree1->proot, ptree2->proot, cb);
	if (pnode == 0)
	{
		return 0;
	}
	return 1;
}

__declspec(noinline) struct ST_NODE *xx_tree_lowsub_func(struct ST_NODE *proot, struct ST_NODE *pnode, def_tree_cb_cmp cb)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *pret = 0;


	if (proot == 0)
	{
		return 0;
	}

	/*最低级子节点必有头节点，且没有子节点*/
	if (proot->phead != 0 && proot->sub_num == 0)
	{
		if (proot->sec != pnode->sec || proot->nsec != pnode->nsec)
		{
			if (cb != 0)
			{
				/*调用自定义回调函数*/
				iret = cb(proot->pdata, pnode->pdata);
				if (iret != 0)
				{
					//printf("sec:%x-%x  nsec:%x-%x\n", proot->sec, proot->nsec, pnode->sec, pnode->nsec);
					return proot;
				}
			}
			else
			{
				/*比较数据，一样则返回成功*/
				iret = memcmp(proot->pdata, pnode->pdata, proot->data_size);
				if (iret == 0)
				{
					return proot;
				}
			}
			return 0;
		}
	}

	for (n = 0; n < proot->sub_num; n++)
	{
		pret = 0;
		pret = xx_tree_lowsub_func(proot->psub[n].pst_node, pnode, cb);
		if (pret != 0)
		{
			return pret;
		}
	}

	return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
判断树2的根节点是否是树1的子节点
参数1：树结构1
参数2：树结构2
参数3：数据比较回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
返回：1-树1的某个子节点；0-不是树1的子节点
*/
__declspec(noinline) int xx_tree_is_sub(struct ST_TREE *ptree1, struct ST_TREE *ptree2, def_tree_cb_cmp cb)
{
	struct ST_NODE *pnode = 0;

	/*检查参数*/
	if (ptree1 == 0 || ptree2 == 0)
	{
		return 0;
	}

	/*遍历树1的节点*/
	pnode = xx_tree_sub_func(ptree1->proot, ptree2->proot, cb);
	if (pnode == 0)
	{
		return 0;
	}
	return 1;
}

__declspec(noinline) struct ST_NODE *xx_tree_sub_func(struct ST_NODE *proot, struct ST_NODE *pnode, def_tree_cb_cmp cb)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *pret = 0;
	struct ST_NODE *ptmp = 0;

	if (proot == 0)
	{
		return 0;
	}

	/*子节点必有头节点*/
	if (proot->phead != 0)
	{
		if (proot->sec != pnode->sec || proot->nsec != pnode->nsec)
		{
			if (cb != 0)
			{
				/*调用自定义回调函数*/
				iret = cb(proot->pdata, pnode->pdata);
				if (iret != 0)
				{
					return proot;
				}
			}
			else
			{
				/*比较数据，一样则返回成功*/
				iret = memcmp(proot->pdata, pnode->pdata, proot->data_size);
				if (iret == 0)
				{
					return proot;
				}
			}
			return 0;
		}
	}


	for (n = 0; n < proot->sub_num; n++)
	{
		pret = 0;
		pret = xx_tree_sub_func(proot->psub[n].pst_node, pnode, cb);
		if (pret != 0)
		{
			return pret;
		}
	}

	return 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
/*
合并两个树，树2的根节点接入树1的最低级子节点
	树2完全复制并将副本接入树1，使合并后的树1和树2完全独立，
	也就是节点及节点的存储数据独立，树2可以完全释放
	释放树2由外部调用，内部不释放
参数1：树结构1
参数2：树结构2
参数3：数据比较回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
返回：已合并的最低级子节点数目-成功，0-失败
*/
__declspec(noinline) int xx_tree_merge(struct ST_TREE *ptree1, struct ST_TREE *ptree2, def_tree_cb_cmp cb)
{
	int iret = 0;
	int n = 0;
	struct ST_NODE *pnode = 0;


	/*检查参数*/
	if (ptree1 == 0 || ptree2 == 0)
	{
		return 0;
	}


	/*可能是树1的多个最低级子节点*/
	n = 0;
	while (1)
	{
		/*判断树2的根节点是否是树1的最低级子节点*/
		pnode = 0;
		pnode = xx_tree_lowsub_func(ptree1->proot, ptree2->proot, cb);
		if (pnode == 0)
		{
			break;
		}

		iret = xx_node_copy(pnode, ptree2->proot);
		if (iret == 0)
		{
			break;
		}

		n++;
	}

	/*计算树1的总节点数*/
	ptree1->inode = ptree1->inode + (ptree2->inode*n) - n;

	/*合并完成后，刷新树时间戳*/
	xx_tree_time_flush_func(ptree1->proot, ptree1->sec, ptree1->nsec);

	return n;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/*
复制一个树
*/
__declspec(noinline) struct ST_TREE *xx_tree_copy(struct ST_TREE *ptree)
{
	int iret = 0;
	int n = 0;
	struct ST_NODE *pnode = 0;
	struct ST_TREE *ret_tree = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return 0;
	}

	/*申请树结构*/
	ret_tree = malloc(sizeof(struct ST_TREE));
	if (ret_tree == 0)
	{
		return 0;
	}
	memset(ret_tree, 0, sizeof(struct ST_TREE));

	/*复制树结构*/
	memcpy(ret_tree, ptree, sizeof(struct ST_TREE));

	/*新建根节点*/
	pnode = xx_node_new();
	if (pnode == 0)
	{
		goto l_err;
	}

	/*设置为根节点*/
	ret_tree->proot = pnode;

	/*复制节点*/
	iret = xx_node_copy(pnode, ptree->proot);
	if (iret == 0)
	{
		goto l_err;
	}

	return ret_tree;

l_err:
	if (ret_tree)
	{
		xx_tree_free(ret_tree);
	}
	return 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////

/*
检查分析树的完整性
参数1：树结构
参数2：数据检查回调函数，为0则只检查是否有空节点
返回：1-完整，0-不完整
*/
__declspec(noinline) int xx_tree_check(struct ST_TREE *ptree, def_tree_cb_data cb)
{
	int iret = 0;
	struct ST_NODE *phead = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return 0;
	}

	iret = xx_tree_check_func(ptree->proot, cb);
	if (iret == 0)
	{
		return 0;
	}
	return 1;
}

__declspec(noinline) int xx_tree_check_func(struct ST_NODE *pnode, def_tree_cb_data cb)
{
	int n = 0;
	int iret = 0;

	if (pnode == 0)
	{
		return 0;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		/*只要有1个检查不通过则返回失败*/
		iret = xx_tree_check_func(pnode->psub[n].pst_node, cb);
		if (iret == 0)
		{
			return 0;
		}
	}

	if (cb != 0)
	{
		iret = cb(pnode->pdata);
		if (iret != 0)
		{
			/*回调检查通过返回1*/
			return 1;
		}
	}
	else
	{
		if (pnode->pdata != 0 && pnode->data_size != 0)
		{
			/*无回调检查通过返回1*/
			return 1;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
检查分析树的完整性，只检查最低级子项
参数1：树结构
参数2：数据检查回调函数，为0则只检查是否有空节点
返回：1-完整，0-不完整
*/
__declspec(noinline) int xx_tree_low_check(struct ST_TREE *ptree, def_tree_cb_data cb)
{
	int iret = 0;
	struct ST_NODE *phead = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return 0;
	}

	iret = xx_tree_low_check_func(ptree->proot, cb);
	if (iret == 0)
	{
		return 0;
	}
	return 1;
}

__declspec(noinline) int xx_tree_low_check_func(struct ST_NODE *pnode, def_tree_cb_data cb)
{
	int n = 0;
	int iret = 0;

	if (pnode == 0)
	{
		return 0;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		/*只要有1个检查不通过则返回失败*/
		iret = xx_tree_low_check_func(pnode->psub[n].pst_node, cb);
		if (iret == 0)
		{
			return 0;
		}
	}

	if (pnode->sub_num == 0)
	{
		/*无子项时，检查不通过返回0*/
		if (cb != 0)
		{
			iret = cb(pnode->pdata);
			if (iret != 0)
			{
				/*回调检查通过返回1*/
				return 1;
			}
		}
		else
		{
			if (pnode->pdata != 0 && pnode->data_size != 0)
			{
				/*无回调检查通过返回1*/
				return 1;
			}
		}

		return 0;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
重新计算并获取树的数据或者节点个数
	每个节点只有一个数据数据
参数1：树结构
返回：数据个数
*/
__declspec(noinline) int xx_tree_data_num(struct ST_TREE *ptree)
{
	int iret = 0;
	struct ST_NODE *phead = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return 0;
	}

	ptree->inode = 0;

	xx_tree_data_num_func(ptree->proot, &ptree->inode);

	return ptree->inode;
}

__declspec(noinline) void xx_tree_data_num_func(struct ST_NODE *pnode,int *pnum)
{
	int n = 0;
	int iret = 0;

	if (pnode == 0)
	{
		return ;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		/*只要有1个检查不通过则返回失败*/
		xx_tree_data_num_func(pnode->psub[n].pst_node, pnum);
	}

	*pnum = *pnum + 1;

	return ;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
打印树结构
参数1：树结构
参数2：打印回调函数
	打印回调函数，为0则控制台输出，有则自定义输出
返回：1-正常输出，0-树结构错误
*/
__declspec(noinline) int xx_tree_print(struct ST_TREE *ptree, def_tree_cb_data cb)
{
	int iret = 0;


	/*检查参数*/
	if (ptree == 0)
	{
		return 0;
	}

	iret = xx_tree_print_func(ptree->proot, cb);
	if (iret == 0)
	{
		return 0;
	}

	printf("======ptree:[%x]  proot:[%x]  inode:[%x]  time:[%x %x]======\n"
		"======exp_size:[%x]  exp:[%s]======\n", \
		(XLADDR)ptree, (XLADDR)ptree->proot, ptree->inode, ptree->sec, ptree->nsec, \
		ptree->exp_size, ptree->pexp);

	return 1;
}


__declspec(noinline) int xx_tree_print_func(struct ST_NODE *pnode, def_tree_cb_data cb)
{
	int n = 0;
	int iret = 0;

	if (pnode == 0)
	{
		return 0;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		iret = xx_tree_print_func(pnode->psub[n].pst_node, cb);
		if (iret == 0)
		{
			return 0;
		}
	}

	printf("pnode:[%x]-->  pdata:[%x]  data_size:[%x]  phead:[%x]  sub_num:[%x]  time:[%x %x]\n"
		"exp_size:[%x] exp:[%s]\n", \
		(XLADDR)pnode, (XLADDR)pnode->pdata, (XLADDR)pnode->data_size, \
		(XLADDR)pnode->phead, (XLADDR)pnode->sub_num, (XLADDR)pnode->sec, (XLADDR)pnode->nsec, \
		(XLADDR)pnode->exp_size, (char*)pnode->pexp);

	if (cb != 0)
	{
		iret = cb(pnode->pdata);
		if (iret == 0)
		{
			return 0;
		}
	}

	return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
新增
获取根节点的数据
*/
void* xx_tree_root_data(struct ST_TREE *ptree)
{
	if (ptree == 0 )
	{
		return 0;
	}

	return ptree->proot->pdata;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
	新增
	查找数据，根据数据2查找树1中的最低级子节点的数据
	参数1：树结构
	参数2：待查找的数据
	参数3：数据查找回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
	返回：成功-树1中的节点数据，失败-0
	*/
__declspec(noinline) void *xx_tree_find_lowdata(struct ST_TREE *ptree, void *pdata, def_tree_cb_cmp cb)
{
	void *pret = 0;

	/*检查参数*/
	if (ptree == 0 || pdata == 0)
	{
		return 0;
	}


	pret = xx_tree_find_lowdata_func(ptree->proot, pdata, cb);
	if (pret == 0)
	{
		return 0;
	}

	return pret;
}

__declspec(noinline) void *xx_tree_find_lowdata_func(struct ST_NODE *pnode, void *pdata, def_tree_cb_cmp cb)
{
	int n = 0;
	int iret = 0;
	void *pret = 0;


	if (pnode == 0)
	{
		return 0;
	}

	/*最低级子节点必有头节点，且没有子节点*/
	if (pnode->phead != 0 && pnode->sub_num == 0)
	{
		if (cb != 0)
		{
			/*调用自定义回调函数*/
			iret = cb(pnode->pdata, pdata);
			if (iret != 0)
			{
				//printf("sec:%x-%x  nsec:%x-%x\n", proot->sec, proot->nsec, pnode->sec, pnode->nsec);
				return pnode->pdata;
			}
		}
		else
		{
			/*比较数据，一样则返回成功*/
			iret = memcmp(pnode->pdata, pdata, pnode->data_size);
			if (iret == 0)
			{
				return pnode->pdata;
			}
		}
		return 0;

	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		pret = 0;
		pret = xx_tree_find_lowdata_func(pnode->psub[n].pst_node, pdata, cb);
		if (pret != 0)
		{
			return pret;
		}
	}

	return 0;
}








//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
树生成所有节点的表达式
参数1：树
参数2：回调函数
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_tree_node_exp(struct ST_TREE *ptree, def_tree_cb_node_exp cb)
{
	int iret = 0;

	/*参数检查，必须有回调函数*/
	if (ptree == 0 || cb == 0)
	{
		return 0;
	}

	iret = xx_tree_node_exp_func(ptree->proot, cb);
	if (iret == 0)
	{
		goto l_err;
	}
	
	return 1;

l_err:

	xx_tree_exp_free(ptree);

	return 0;
}


/*
生成树的表达式
前提是已经生成了节点表达式
*/
__declspec(noinline) int xx_tree_exp(struct ST_TREE *ptree, def_tree_cb_exp cb)
{
	int iret = 0;

	/*参数检查，必须有回调函数*/
	if (ptree == 0 || cb == 0)
	{
		return 0;
	}

	if (ptree->pexp != 0)
	{
		free(ptree->pexp);

		ptree->pexp = 0;
		ptree->exp_size = 0;
	}

	if (ptree->proot->pexp == 0 || ptree->proot->exp_size == 0)
	{
		return 0;
	}

	ptree->exp_size = cb(ptree->proot->pdata, ptree->proot->pexp, &ptree->pexp);
	if (ptree->exp_size == 0 || ptree->pexp == 0)
	{
		return 0;
	}

	return 1;
}


/*
由某个节点和它的子节点生成一个表达式
自定义回调函数，由外部生成表达式，返回给节点
回调函数的参数，
父节点数据指针，父节点表达式指针，
子节点数，
子节点数据指针数组，子节点表达式指针数组，
用来存放返回的节点表达式的指针的地址

节点数据为自定义数据，所以数据大小是已知的
由最低级开始遍历

参数1：节点结构
参数2：表达式回调函数
返回：1-表达式的存储空间大小，0-失败
*/
__declspec(noinline) int xx_tree_node_exp_func(struct ST_NODE *pnode, def_tree_cb_node_exp cb)
{
	int iret = 0;
	int n = 0;
	struct ST_NODE *ptmp_node = 0;
	char **psub_data = 0;
	char **psub_exp = 0;

	if (pnode == 0)
	{
		return 0;
	}


	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_node_exp_func(pnode->psub[n].pst_node, cb);
	}


	/*判断是否符合表达式生成条件*/
	iret = xx_node_exp_cnd(pnode);
	if (iret != 0)
	{
		/*申请子节点数据指针数组空间*/
		psub_data = malloc(sizeof(psub_data)*pnode->sub_num);
		if (psub_data == 0)
		{
			return 0;
		}
		memset(psub_data, 0, sizeof(psub_data)*pnode->sub_num);

		psub_exp = malloc(sizeof(psub_exp)*pnode->sub_num);
		if (psub_exp == 0)
		{
			return 0;
		}
		memset(psub_exp, 0, sizeof(psub_exp)*pnode->sub_num);

		for (n = 0; n < pnode->sub_num; n++)
		{
			ptmp_node = pnode->psub[n].pst_node;

			psub_data[n] = ptmp_node->pdata;

			psub_exp[n] = ptmp_node->pexp;
		}

		/*如果已经有表达式，则清理*/
		if (pnode->pexp)
		{
			free(pnode->pexp);
			pnode->pexp = 0;
		}

		/*普通节点*/
		iret = cb(pnode->pdata, pnode->sub_num, psub_data, psub_exp, (char**)&pnode->pexp);
		
		pnode->exp_size = iret;

		/*先释放回调函数的参数空间*/
		if (psub_data)
		{
			free(psub_data);
		}
		if (psub_exp)
		{
			free(psub_exp);
		}

		if (iret == 0)
		{
			return 0;
		}


	}
	

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
释放树表达式空间
参数1：树结构
*/
__declspec(noinline) void xx_tree_exp_free(struct ST_TREE *ptree)
{
	if (ptree == 0)
	{
		return;
	}

	xx_tree_exp_free_func(ptree->proot);

	if (ptree->pexp)
	{
		free(ptree->pexp);
		ptree->pexp = 0;
		ptree->exp_size = 0;
	}

	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
释放树子节点的表达式空间
参数1：树结构
*/
__declspec(noinline) void xx_tree_exp_sub_free(struct ST_TREE *ptree)
{
	if (ptree == 0)
	{
		return;
	}

	xx_tree_exp_free_func(ptree->proot);

	return;
}


__declspec(noinline) void xx_tree_exp_free_func(struct ST_NODE *pnode)
{
	int n = 0;

	if (pnode == 0)
	{
		return ;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_exp_free_func(pnode->psub[n].pst_node);
	}

	/*最低级开始释放*/
	if (pnode->pexp != 0)
	{
		free(pnode->pexp);
		pnode->pexp = 0;
		pnode->exp_size = 0;
	}

	pnode->opmz_flag = 0;

	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
/*
树结构优化
	2.优化节点表达式
参数1：树结构
参数2：优化回调函数
参数3：表达式生成回调函数
返回：1-成功，0-失败
	失败树结构不会损坏，表达式需要重新生成
	只有在内存不足时才会失败
	修改，优化表达式后，不删除子节点，不影响原有的树结构，重新生成优化后的表达式
*/
__declspec(noinline) int xx_tree_opmz_exp(struct ST_TREE *ptree, def_tree_cb_opmz_exp cb_opmz, def_tree_cb_node_exp cb_exp)
{
	int iret = 0;

	if (ptree == 0 || cb_opmz == 0 || cb_exp == 0)
	{
		return 0;
	}

	/*优化表达式*/
	xx_tree_opmz_exp_func(ptree->proot, cb_opmz);


	/*清理优化标志，并生成新的表达式*/
	iret = xx_tree_opmz_clr_exp_func(ptree->proot, cb_exp);
	if (iret == 0)
	{
		goto l_err;
	}


	if (ptree->pexp != 0)
	{
		free(ptree->pexp);

		ptree->pexp = 0;
		ptree->exp_size = 0;
	}

	/*表达式复制到树结构*/
	ptree->exp_size = strlen(ptree->proot->pexp) + 1;

	if (ptree->exp_size <= 1)
	{
		goto l_err;
	}

	ptree->pexp = malloc(ptree->exp_size);
	if (ptree->pexp == 0)
	{
		goto l_err;
	}
	memset(ptree->pexp, 0, ptree->exp_size);

	memcpy(ptree->pexp, ptree->proot->pexp, ptree->exp_size);

	return 1;


l_err:

	xx_tree_exp_free(ptree);

	return 0;
}


/*
回调函数
如果有优化后的表达式，则返回该表达式
没有则返回0，回调函数不出错
只操作该节点的表达式，不操作该节点的自定义数据
*/
__declspec(noinline) void xx_tree_opmz_exp_func(struct ST_NODE *pnode, def_tree_cb_opmz_exp cb)
{
	int n = 0;
	char *pret_exp = 0;

	if (pnode == 0)
	{
		return ;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_opmz_exp_func(pnode->psub[n].pst_node, cb);
	}

	if (pnode->pexp != 0)
	{
		pret_exp = cb(pnode->pexp);
		if (pret_exp != 0)
		{
			/*释放该节点的子节点*/
			//xx_node_sub_del(pnode);

			free(pnode->pexp);
			pnode->pexp = pret_exp;

			pnode->opmz_flag = 1;
		}
	}

	return ;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
优化后的表达式生成
先释放未优化的exp

*/
__declspec(noinline) int xx_tree_opmz_clr_exp_func(struct ST_NODE *pnode, def_tree_cb_node_exp cb)
{
	int iret = 0;
	int n = 0;
	struct ST_NODE *ptmp_node = 0;
	void **psub_data = 0;
	char **psub_exp = 0;

	if (pnode == 0)
	{
		return 0;
	}

	/*没有优化标志的exp释放*/
	if (pnode->opmz_flag == 0)
	{
		free(pnode->pexp);

		pnode->pexp = 0;
		pnode->exp_size = 0;

	}
	else
	{
		/*有优化标志，则成功返回，不损坏原树结构，不再生成表达式，优化标志恢复*/
		pnode->opmz_flag = 0;
		return 1;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_opmz_clr_exp_func(pnode->psub[n].pst_node, cb);
	}


	/*判断是否符合表达式生成条件*/
	iret = xx_node_exp_cnd(pnode);
	if (iret != 0)
	{
		/*申请子节点数据指针数组空间*/
		psub_data = malloc(sizeof(psub_data)*pnode->sub_num);
		if (psub_data == 0)
		{
			return 0;
		}
		memset(psub_data, 0, sizeof(psub_data)*pnode->sub_num);

		psub_exp = malloc(sizeof(psub_exp)*pnode->sub_num);
		if (psub_exp == 0)
		{
			return 0;
		}
		memset(psub_exp, 0, sizeof(psub_exp)*pnode->sub_num);

		for (n = 0; n < pnode->sub_num; n++)
		{
			ptmp_node = pnode->psub[n].pst_node;

			psub_data[n] = ptmp_node->pdata;

			psub_exp[n] = ptmp_node->pexp;
		}

		iret = cb(pnode->pdata, pnode->sub_num, psub_data, psub_exp, (char**)&pnode->pexp);

		pnode->exp_size = iret;

		/*先释放回调函数的参数空间*/
		if (psub_data)
		{
			free(psub_data);
		}
		if (psub_exp)
		{
			free(psub_exp);
		}

		if (iret == 0)
		{
			return 0;
		}


	}


	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
刷新树的所有节点的时间戳
	用于合并后，统一时间戳
参数1：根节点
参数2：秒
参数3：微妙
*/
void xx_tree_time_flush_func(struct ST_NODE *pnode, long sec, long nsec)
{
	int n = 0;


	if (pnode == 0)
	{
		return ;
	}

	/*设置时间戳，和目标树保持一致*/
	xx_node_set_time(pnode, sec, nsec);

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_time_flush_func(pnode->psub[n].pst_node, sec, nsec);
	}

	return ;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
为所有的节点设置一个数据
*/
__declspec(noinline) void xx_tree_set_idata(struct ST_TREE *ptree, def_tree_cb_set_idata cb, int idata)
{
	int iret = 0;
	struct ST_NODE *phead = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return ;
	}

	xx_tree_set_idata_func(ptree->proot, cb, idata);

	return ;
}

__declspec(noinline) void xx_tree_set_idata_func(struct ST_NODE *pnode, def_tree_cb_set_idata cb, int idata)
{
	int n = 0;
	int iret = 0;

	if (pnode == 0)
	{
		return ;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_set_idata_func(pnode->psub[n].pst_node, cb, idata);
	}

	if (cb != 0)
	{
		cb(pnode->pdata, idata);

	}

	return;
}




/*
为最低级的子节点设置一个数据
*/
__declspec(noinline) void xx_tree_set_low_idata(struct ST_TREE *ptree, def_tree_cb_set_idata cb, int idata)
{
	int iret = 0;
	struct ST_NODE *phead = 0;

	/*检查参数*/
	if (ptree == 0)
	{
		return;
	}

	xx_tree_set_low_idata_func(ptree->proot, cb, idata);

	return;
}

__declspec(noinline) void xx_tree_set_low_idata_func(struct ST_NODE *pnode, def_tree_cb_set_idata cb, int idata)
{
	int n = 0;
	int iret = 0;

	if (pnode == 0)
	{
		return;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_set_low_idata_func(pnode->psub[n].pst_node, cb, idata);
	}

	if (cb != 0 && pnode->sub_num == 0)
	{
		cb(pnode->pdata, idata);

	}

	return;
}





/////////////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
/*
树结构优化
	1.节点数据优化
	一些变量的转换，具体实现待定
参数1：树结构
参数2：优化回调函数
参数3：表达式生成回调函数
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_tree_opmz_data(struct ST_TREE *ptree, def_tree_cb_opmz_data cb_data)
{
	int iret = 0;

	if (ptree == 0 || cb_data == 0 )
	{
		return 0;
	}

	/*优化子节点数据*/
	xx_tree_opmz_data_func(ptree->proot, cb_data);


	return 1;

}


/*
回调函数
如果有优化后的数据，则返回数据，覆盖原数据
没有则返回0，回调函数不出错
*/
__declspec(noinline) void xx_tree_opmz_data_func(struct ST_NODE *pnode, def_tree_cb_opmz_data cb)
{
	int n = 0;
	struct ST_NODE *ptmp_node = 0;
	void **psub_data = 0;
	void *pret_data = 0;

	if (pnode == 0)
	{
		return;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_opmz_data_func(pnode->psub[n].pst_node, cb);
	}


	/*检查条件*/

	/*申请子节点数据指针数组空间*/
	psub_data = malloc(sizeof(psub_data)*pnode->sub_num);
	if (psub_data == 0)
	{
		return 0;
	}
	memset(psub_data, 0, sizeof(psub_data)*pnode->sub_num);


	for (n = 0; n < pnode->sub_num; n++)
	{
		ptmp_node = pnode->psub[n].pst_node;

		psub_data[n] = ptmp_node->pdata;

	}

	pret_data = cb(pnode->pdata, pnode->sub_num, psub_data);

	if (pret_data != 0)
	{
		free(pnode->pdata);

		pnode->pdata = pret_data;
	}

	free(psub_data);

	return;
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
分析树优化
回调函数进行对应的分析，
回调函数的返回值没有成功或者失败，只有指定操作类型
返回值决定对应的操作，是否对节点进行操作，一般的操作为删除子节点

回调函数返回值
1：删除某个子节点
2：删除所有子节点
未定义的不操作
*/
__declspec(noinline) void xx_tree_opmz_idef(struct ST_TREE *ptree, def_tree_cb_opmz_idef cb)
{
	int iret = 0;

	/*参数检查，必须有回调函数*/
	if (ptree == 0 || cb == 0)
	{
		return ;
	}

	xx_tree_opmz_idef_func(ptree->proot, cb);

	return ;
}



__declspec(noinline) void xx_tree_opmz_idef_func(struct ST_NODE *pnode, def_tree_cb_opmz_idef cb)
{
	int iret = 0;
	int n = 0;
	struct ST_NODE *ptmp_node = 0;
	char *psub_data[10];
	//char **psub_exp = 0;
	int sub_index = 0;

	if (pnode == 0)
	{
		return ;
	}


	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_opmz_idef_func(pnode->psub[n].pst_node, cb);
	}


	/*判断是否符合条件*/
	iret = xx_node_opmz_idef_cnd(pnode);
	if (iret != 0)
	{
		memset(psub_data, 0, sizeof(psub_data));

		for (n = 0; n < pnode->sub_num; n++)
		{
			ptmp_node = pnode->psub[n].pst_node;

			psub_data[n] = ptmp_node->pdata;
		}


		iret = cb(pnode->pdata, pnode->sub_num, psub_data, &sub_index);

		if (iret == 1)
		{
			/*删除某个字节点*/
			xx_node_sub_del(pnode, sub_index);
		}
		else if (iret == 2)
		{
			xx_node_sub_all_del(pnode);
		}
	}

	return ;
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
分析树优化
回调函数进行对应的分析，
回调函数的返回值没有成功或者失败，只有指定操作类型
返回值决定对应的操作，是否对节点进行操作，一般的操作为删除子节点
*/
__declspec(noinline) void xx_tree_opmz_exp(struct ST_TREE *ptree, def_tree_cb_opmz_exp cb)
{
	int iret = 0;

	/*参数检查，必须有回调函数*/
	if (ptree == 0 || cb == 0)
	{
		return ;
	}

	xx_tree_opmz_exp_func(ptree->proot, cb);

	return ;
}



__declspec(noinline) void xx_tree_opmz_exp_func(struct ST_NODE *pnode, def_tree_cb_opmz_exp cb)
{
	int iret = 0;
	int n = 0;
	struct ST_NODE *ptmp_node = 0;
	//char *psub_data[10] = 0;
	char *psub_exp[10];
	int sub_index = 0;

	if (pnode == 0)
	{
		return;
	}


	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_tree_opmz_exp_func(pnode->psub[n].pst_node, cb);
	}


	/*判断是否符合条件*/
	iret = xx_node_opmz_exp_cnd(pnode);
	if (iret != 0)
	{
		memset(psub_exp, 0, sizeof(psub_exp));

		for (n = 0; n < pnode->sub_num; n++)
		{
			ptmp_node = pnode->psub[n].pst_node;

			psub_exp[n] = ptmp_node->pexp;
		}

		iret = cb(pnode->pdata, pnode->pexp, pnode->sub_num, psub_exp, &sub_index);
		if (iret == 1)
		{
			/*删除某个字节点*/
			xx_node_sub_del(pnode, sub_index);
		}
		else if (iret == 2)
		{
			xx_node_sub_all_del(pnode);
		}
	}

	return;
}













