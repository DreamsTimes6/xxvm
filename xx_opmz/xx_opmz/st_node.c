#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "st_node.h"



void xx_node_sub_del_func(struct ST_NODE *pnode);

////////////////////////////////////////////////////////////////////////////////////////////////////
/*
api
新建节点
*/
__declspec(noinline) struct ST_NODE * xx_node_new()
{
	struct ST_NODE *pst = 0;
	char *ptmp = 0;

	pst = malloc(sizeof(struct ST_NODE));
	if (pst == 0)
	{
		return 0;
	}

	memset(pst, 0, sizeof(struct ST_NODE));

	//pst->sub_num = 0;


	return pst;
}

/*
api
释放节点
	释放以后，节点无效，节点指针不能再使用
参数1：待释放的节点
返回：1-成功，0-失败
*/
__declspec(noinline) void xx_node_free(struct ST_NODE *pnode)
{
	if (pnode == 0)
	{
		return ;
	}

	//释放pdata
	if (pnode->pdata != 0)
	{
		free(pnode->pdata);
	}
	

	//释放pexp
	if (pnode->pexp != 0)
	{
		free(pnode->pexp);
	}


	//释放子节点组
	if (pnode->psub != 0)
	{
		free(pnode->psub);
	}


	/*释放节点*/
	free(pnode);

	return ;
}


/*
向节点插入数据
	附加设置操作，新数据覆盖旧数据
	待插入的数据必须是数据区指针或者是完整的结构体指针，结构体内不能有指针成员
	如果参数为0，则不操作
参数1：待操作的节点
参数2：数据指针
参数3：数据大小
参数4：扩展数据指针
参数5：扩展数据大小
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_node_set_data(struct ST_NODE *pnode,void *pdata,int data_size)
{
	int iret = 0;
	char *p = 0;

	if (pnode == 0)
	{
		return 0;
	}

	//为空则不操作数据域
	if (pdata != 0 && data_size != 0)
	{
		if (pnode->pdata != 0)
		{
			free(pnode->pdata);
			pnode->data_size = 0;
		}


		p = malloc(data_size);
		if (p == 0)
		{
			return 0;
		}
		memset(p, 0, data_size);

		pnode->pdata = p;
		pnode->data_size = data_size;
		memcpy(pnode->pdata, pdata, data_size);

	}


	return 1;
}




/*
	设置节点时间戳
	参数1：节点结构
	返回：无
	*/
__declspec(noinline) void xx_node_set_time(struct ST_NODE *pnode, long sec, long nsec)
{
	pnode->sec = sec;
	pnode->nsec = nsec;
	return;
}


/*
	设置随机数
	参数1：节点结构
	返回：无
	*/
__declspec(noinline) void xx_node_set_rand(struct ST_NODE *pnode, long rand)
{
	pnode->rand = rand;
	return;
}


/*
添加节点的子节点
参数1：节点结构
参数2：子节点结构
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_node_add_sub(struct ST_NODE *pst, struct ST_NODE *psub)
{
	int n = 0;
	int iret = 0;
	int isub = 0;
	struct ST_SUBNODE *pgsub;

	//参数检查
	if (pst == 0 || psub == 0)
	{
		return 0;
	}

	isub = pst->sub_num + 1;

	//重新扩展节点的子节点组
	pgsub = malloc(sizeof(struct ST_SUBNODE)*isub);
	if (pgsub == 0)
	{
		return 0;
	}
	memset(pgsub, 0, sizeof(struct ST_SUBNODE)*isub);

	if (pst->sub_num != 0)
	{
		memcpy(pgsub, pst->psub, sizeof(struct ST_SUBNODE)*pst->sub_num);

		free(pst->psub);
	}
	pst->psub = pgsub;
	pst->sub_num = isub;

	/*设置子节点*/
	pst->psub[isub-1].pst_node = psub;
	pst->psub[isub-1].seq = isub-1;

	/*设置父节点*/
	psub->phead = pst;


	return 1;
}






/*
设置节点的子节点数目
参数1：节点结构
参数2：子节点数目
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_node_set_sub_num(struct ST_NODE *pst, int sub_num)
{
	char *p = 0;
	int n = 0;

	if (pst == 0 || sub_num == 0)
	{
		return 0;
	}

	pst->sub_num = sub_num;

	if (pst->psub != 0)
	{
		/*如果有子节点，则失败*/
		return 0;
	}

	p = malloc(sizeof(struct ST_SUBNODE)*pst->sub_num);
	if (p == 0)
	{
		pst->sub_num = 0;
		return 0;
	}

	memset(p, 0, sizeof(struct ST_SUBNODE)*pst->sub_num);

	pst->psub = (struct ST_SUBNODE*)p;

	for (n = 0; n < pst->sub_num; n++)
	{
		pst->psub[n].seq = n;
	}

	return 1;
}

/*
设置节点的子节点
参数1：节点结构
参数2：子节点序号
	序号由0开始
参数3：子节点结构
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_node_set_sub(struct ST_NODE *pst, int seq,struct ST_NODE *psub)
{
	int iret = 0;

	if (pst == 0 || psub == 0 || seq>=pst->sub_num)
	{
		return 0;
	}

	if (pst->psub==0)
	{
		return 0;
	}

	/*子节点设置*/
	pst->psub[seq].pst_node=psub;
	pst->psub[seq].seq = seq;

	/*设置父节点*/
	psub->phead = pst;


	return 1;
}





/*
api 
节点链完整拷贝
参数1：目的节点结构
参数2：源节点结构
返回：1-成功，0-失败
*/
__declspec(noinline) int xx_node_copy(struct ST_NODE *pdes,struct ST_NODE *psrc)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *psub = 0;


	/*设置目的节点的数据*/
	iret = xx_node_set_data(pdes, psrc->pdata, psrc->data_size);
	if (iret == 0)
	{
		return 0;
	}

	/*设置时间戳，和目标树保持一致*/
	xx_node_set_time(pdes, psrc->sec, psrc->nsec);

	/*设置随机数，和目标树保持一致*/
	xx_node_set_rand(pdes, psrc->rand);

	if (psrc->sub_num == 0)
	{
		return 1;
	}

	/*设置目的节点的子节点数目*/
	iret = xx_node_set_sub_num(pdes, psrc->sub_num);
	if (iret == 0)
	{
		return 0;
	}

	for (n = 0; n < psrc->sub_num; n++)
	{
		psub = 0;
		/*新建节点*/
		psub = xx_node_new();
		if (psub == 0)
		{
			return 0;
		}
		
		/*设置为树1子节点的子节点*/
		iret = xx_node_set_sub(pdes, n, psub);
		if (iret == 0)
		{
			return 0;
		}

		iret=xx_node_copy(psub, psrc->psub[n].pst_node);
		if (iret == 0)
		{
			return 0;
		}
	}
	return 1;
}




/*
删除该节点的子节点链，不包括它本身
参数1：节点结构
*/
__declspec(noinline) void xx_node_sub_all_del(struct ST_NODE *pnode)
{
	int n = 0;

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_node_sub_del_func(pnode->psub[n].pst_node);
	}

	//释放子节点组
	if (pnode->psub != 0)
	{
		free(pnode->psub);
		pnode->psub = 0;
	}

	pnode->sub_num = 0;

	return;
}


__declspec(noinline) void xx_node_sub_del_func(struct ST_NODE *pnode)
{
	int n = 0;

	for (n = 0; n < pnode->sub_num; n++)
	{
		xx_node_sub_del_func(pnode->psub[n].pst_node);
	}

	xx_node_free(pnode);

	return;
}


/*
删除该节点的某个子节点链，不包括该节点本身
参数1：节点结构
*/
__declspec(noinline) void xx_node_sub_del(struct ST_NODE *pnode, int index)
{
	int n = 0;
	int m = 0;
	struct ST_SUBNODE  *p = 0;

	if (index >= pnode->sub_num)
	{
		return;
	}

	/*释放子节点链*/
	xx_node_sub_del_func(pnode->psub[index].pst_node);

	pnode->psub[index].pst_node = 0;

	/*申请新的子节点组空间*/
	p = malloc(sizeof(struct ST_SUBNODE)*pnode->sub_num);
	if (p == 0)
	{
		return ;
	}

	memset(p, 0, sizeof(struct ST_SUBNODE)*pnode->sub_num);

	n = 0;
	m = 0;
	while (1)
	{
		if (m >= pnode->sub_num)
		{
			break;
		}
		if (pnode->psub[m].pst_node != 0)
		{
			p[n].pst_node = pnode->psub[m].pst_node;
			p[n].seq = n;

			n++;
		}
		m++;
	}
	

	//释放子节点组
	free(pnode->psub);
	pnode->psub = p;
	pnode->sub_num = pnode->sub_num - 1;

	return;
}



/*
判断该节点是否可以生成表达式
	所有子节点如果有表达式或者是最低级子节点，则符合条件
	如果子节点没有表达式并且不是最低级子节点，则不符合
参数1：节点结构
返回：1-符合条件，0-不符合
*/
__declspec(noinline) int xx_node_exp_cnd(struct ST_NODE *pnode)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *psub = 0;

	/*检查参数，如果该节点为最低级子节点，则不符合*/
	if (pnode == 0 || pnode->sub_num == 0)
	{
		return 0;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		/*如果子节点没有表达式并且不是最低级子节点，则不符合*/
		psub = pnode->psub[n].pst_node;
		if (psub->pexp == 0 && psub->sub_num != 0)
		{
			return 0;
		}
	}

	return 1;
}





/*
判断该节点是否进行定义优化
*/
__declspec(noinline) int xx_node_opmz_idef_cnd(struct ST_NODE *pnode)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *psub = 0;

	/*检查参数，如果该节点为最低级子节点，则不符合*/
	if (pnode == 0 || pnode->sub_num == 0)
	{
		return 0;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		/*如果子节点不是最低级子节点，则不符合*/
		psub = pnode->psub[n].pst_node;
		if (psub->sub_num != 0)
		{
			return 0;
		}
	}

	return 1;
}





/*
判断该节点是否进行表达式优化
*/
__declspec(noinline) int xx_node_opmz_exp_cnd(struct ST_NODE *pnode)
{
	int n = 0;
	int iret = 0;
	struct ST_NODE *psub = 0;

	/*检查参数，如果该节点为最低级子节点，则不符合*/
	if (pnode == 0 || pnode->sub_num == 0)
	{
		return 0;
	}

	for (n = 0; n < pnode->sub_num; n++)
	{
		/*如果子节点没有表达式，则不符合*/
		psub = pnode->psub[n].pst_node;
		if (psub->pexp == 0)
		{
			return 0;
		}
	}

	return 1;
}






