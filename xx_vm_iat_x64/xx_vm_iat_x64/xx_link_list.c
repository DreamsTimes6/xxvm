#include <stdlib.h>
#include <string.h>
#include "xx_link_list.h"
#include <Windows.h>


extern int debug_log(char *tmp);

/*初始化时建立两个节点的循环链表，都为空节点，调用方获得一个节点指针，称为头节点*/
struct XX_LINK_NODE* xx_list_init()
{
	struct XX_LINK_NODE *xx_new_node1;
	struct XX_LINK_NODE *xx_new_node2;

	xx_new_node1=(struct XX_LINK_NODE*)malloc(sizeof(struct XX_LINK_NODE));
	xx_new_node2=(struct XX_LINK_NODE*)malloc(sizeof(struct XX_LINK_NODE));

	memset(xx_new_node1,0,sizeof(struct XX_LINK_NODE));
	memset(xx_new_node2,0,sizeof(struct XX_LINK_NODE));

	xx_new_node1->previous=xx_new_node2;
	xx_new_node1->next=xx_new_node2;

	xx_new_node2->previous=xx_new_node1;
	xx_new_node2->next=xx_new_node1;

	return xx_new_node1;
}

/*插入时新建一个节点，获得头节点的前驱，
设置新建节点的前驱为头节点前驱，后继为头节点
设置头节点前驱的后继为新建节点，设置头节点的前驱为新建节点
给新建节点复制，设置flag为有值*/
void xx_list_insert(struct XX_LINK_NODE* xx_node,void* pdata,int size)
{
	struct XX_LINK_NODE *xx_new_node;
	struct XX_LINK_NODE *xx_node_previous;
	struct XX_LINK_NODE *xx_node_next;

	/*建立新节点*/
	xx_new_node=(struct XX_LINK_NODE*)malloc(sizeof(struct XX_LINK_NODE));
	memset(xx_new_node,0,sizeof(struct XX_LINK_NODE));
	/*申请新节点的数据缓冲区*/
	xx_new_node->pdata=malloc(size+1);
	memset(xx_new_node->pdata,0,size+1);

	/*获得头节点的前驱，后继*/
	xx_node_previous=xx_node->previous;
	xx_node_next=xx_node->next;
	
	/*设置新节点前驱为头节点前驱*/
	xx_new_node->previous=xx_node_previous;
	/*设置新节点后继为头节点*/
	xx_new_node->next=xx_node;

	/*设置头节点前驱的后继为新节点*/
	xx_node_previous->next=xx_new_node;

	/*设置头节点的前驱为新节点*/
	xx_node->previous=xx_new_node;

	/*给新节点的数据域复制*/
	memcpy(xx_new_node->pdata,pdata,size);
	xx_new_node->flag=1;
}

/*清空链表，保留两个空数据节点*/
struct XX_LINK_NODE* xx_list_clear(struct XX_LINK_NODE* xx_node)
{
	struct XX_LINK_NODE* xx_node1;
	struct XX_LINK_NODE* xx_node2;

	xx_node1=xx_node;
	xx_node2=xx_node->next;

	while(1)
	{
		
		if(!(xx_node1->flag) && !(xx_node1->previous->flag) && !( xx_node2->flag) && !(xx_node2->next->flag)  )
		{
			/*当节点1和它的前驱flag，节点2和它的后继flag都为0时，结束循环*/
			break;
		}

		if(xx_node1->flag==1)
		{
			/*去掉xx_node1*/
			xx_node1->previous->next=xx_node2;
			xx_node2->previous=xx_node1->previous;
			free(xx_node1->pdata);
			free(xx_node1);
		}
		xx_node1=xx_node2;
		xx_node2=xx_node2->next;

	}
	return xx_node1;
}

/*获取一个有效数据节点，可以循环调用获取所有的值*/
/*begin==1 第一次调用，记录起始节点*/
/*begin==0 直到返回节点=起始节点，返回值为0*/
struct XX_LINK_NODE *xx_node_start=0;
struct XX_LINK_NODE* xx_list_get(struct XX_LINK_NODE* xx_node,int begin)
{
	struct XX_LINK_NODE* xx_node_tmp;

	xx_node_tmp=xx_node;

	if(begin==1)
	{
		xx_node_start=xx_node_tmp;
		return xx_node_start;
	}
	else
	{
		while(1)
		{
			xx_node_tmp=xx_node_tmp->next;
			if(xx_node_tmp->flag==1)
			{
				return xx_node_tmp;
			}
			
			if(xx_node_tmp==xx_node_start )
			{
				break;
			}
		}
	}

	return 0;
}



