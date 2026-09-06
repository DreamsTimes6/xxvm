#include <stdio.h>



#ifdef __cplusplus
extern "C" {
#endif


struct ST_SLIST_NODE
{
	int seq;  //序号
	void *pnext; //下一个节点
	void *pdata; //节点数据
};


struct ST_SLIST
{
	int total;  //节点数目
	struct ST_SLIST_NODE *pstart;  //初始节点
};


/*
链中节点数据比较的回调函数
*/
typedef void*(*def_slist_cb_cmp)(void *pdata1, void *pdata2);

/*
链中节点数据打印的回调函数
*/
typedef int(*def_slist_cb_print)(void *pdata);


/*
获取链中节点数据的回调函数
*/
typedef int(*def_slist_cb_data)(void *pdata);


/*
链中节点数据比较的回调函数
*/
typedef int (*def_slist_cb_exist)(void *pdata1, void *pdata2);


/*
新建单项链表
返回链表结构，
外部只传递结构
返回：链表结构
*/
void *xx_slist_new();




/*
释放单项链表
参数1：链表结构
返回：无
*/
void xx_slist_free(void *pst_slist);



/*
释放单项链表
参数1：链表结构
返回：无
只释放节点，不释放节点数据
*/
void xx_slist_free_node(void *pst_slist);

/*
插入数据
参数1：链表结构
参数2：待插入的数据
参数3：数据大小
返回：成功-1.失败-0
内部复制一份数据，用于数据为局部变量的情况
*/
int xx_slist_insert_data(void *pst_slist, void *pdata, int data_size);



/*
查找，从头部开始
参数1：链表结构
参数2：待查找的关联数据
参数3：数据比较回调函数
返回：成功-需要的数据，失败-0
*/
void *xx_slist_find1(void *pst_slist, void *pdata, def_slist_cb_cmp pcb);




/*
查找，从尾部开始
参数1：链表结构
参数2：待查找的关联数据
参数3：数据比较回调函数
返回：成功-需要的数据，失败-0
*/
void *xx_slist_find2(void *pst_slist, void *pdata, def_slist_cb_cmp pcb);





/*
打印链中的节点数据
参数1：链结构
参数2：打印回调函数
返回：无
*/
void xx_slist_print(void *pst_slist, def_slist_cb_print cb);




/////////////////////////////////////////// 新增 ////////////////////////////////////////////////////////////////

/*
插入数据
参数1：链表结构
参数2：待插入的数据
参数3：数据大小
返回：成功-1.失败-0
内部直接使用数据指针，用于数据为动态申请的情况
动态申请的的情况需要自定义释放函数
*/
int xx_slist_insert_data2(void *pst_slist, void *pdata);



/*
遍历所有节点数据
*/
void xx_slist_data(void *pst_slist, def_slist_cb_data cb);




/*
插入数据
参数1：链表结构
参数2：待插入的数据
参数3：数据大小
参数4：数据比较回调函数
返回：成功-1.失败-0
内部复制一份数据，
并且在回调函数中判断数据是否重复
用于排序的回调函数
用于数据为局部变量的情况
*/
int xx_slist_insert_data3(void *pst_slist, void *pdata, int data_size, def_slist_cb_exist pcb, def_slist_cb_exist pcb_sort);









#ifdef __cplusplus
}
#endif

