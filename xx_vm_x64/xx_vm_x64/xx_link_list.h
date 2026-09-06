#include <stdio.h>

#pragma pack(push)
#pragma pack(1)

/*节点结构体*/
struct XX_LINK_NODE
{
	void* pdata;          //指向节点数据
	int  flag;            //是否有值，1有值，0无值
	struct XX_LINK_NODE* previous;       //指向上一个节点
	struct XX_LINK_NODE* next;           //指向下一个节点
};


struct XX_LINK_NODE* xx_list_init();
void xx_list_insert(struct XX_LINK_NODE* xx_node,void* pdata,int size);
struct XX_LINK_NODE* xx_list_clear(struct XX_LINK_NODE* xx_node);
struct XX_LINK_NODE* xx_list_get(struct XX_LINK_NODE* xx_node,int begin);


#pragma pack(pop)











