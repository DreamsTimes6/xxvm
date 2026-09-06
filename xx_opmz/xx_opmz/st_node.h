#pragma once


#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif




#ifdef _WIN64

#ifndef XLADDR
#define XLADDR \
	unsigned long long
#endif

#else

#ifndef XLADDR
#define XLADDR \
	unsigned long 
#endif

#endif // _WIN64



#define _export   \
	 __declspec(dllexport)

#define DLL_PUBLIC   \
	 __declspec(dllexport)




	//////////////////////////////////////////////////////////////////////////////////////

	/*子节点结构*/
	struct ST_SUBNODE
	{
		int seq;           //子节点序号
		void *pst_node;    //子节点指针
	};


	/*节点结构*/
	struct ST_NODE
	{
		void *pdata;     //数据指针，如果是结构体，必须是完整的数据，不能有指针成员。
		int  data_size;   //数据大小

		void *pexp;      //扩展数据指针
		int  exp_size;   //扩展数据大小
		int  opmz_flag;  //优化标志，用于优化后重新生成表达式

		struct ST_NODE *phead;  //头节点，头节点为一个

		long sec;               //时间戳
		long nsec;              //时间戳
		long rand;              //随机数

		int  sub_num;    //子节点个数
		struct ST_SUBNODE  *psub;      //子节点数组
	};

	/////////////////////////////////////////////////////////////////////////////////////

	/*
	新建节点
	返回：节点结构-成功，0-失败
	*/
	struct ST_NODE * xx_node_new();


	/*
	释放节点
		释放以后，节点无效，节点指针不能再使用
	参数1：待释放的节点
	返回：1-成功，0-失败
	*/
	void xx_node_free(struct ST_NODE *pnode);


	/*
	设置节点时间戳
	参数1：节点结构
	返回：无
	*/
	void xx_node_set_time(struct ST_NODE *pnode,long sec,long nsec);


	/*
	设置随机数
	参数1：节点结构
	返回：无
	*/
	void xx_node_set_rand(struct ST_NODE *pnode, long rand);


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
	int xx_node_set_data(struct ST_NODE *pnode, void *pdata, int data_size);


	/*
	设置节点的子节点数目
	参数1：节点结构
	参数2：子节点数目
	返回：1-成功，0-失败
	*/
	int xx_node_set_sub_num(struct ST_NODE *pst, int sub_num);


	/*
	设置节点的子节点
	参数1：节点结构
	参数2：子节点序号
		序号由0开始
	参数3：子节点结构
	返回：1-成功，0-失败
	*/
	int xx_node_set_sub(struct ST_NODE *pst, int seq, struct ST_NODE *psub);


	/*
	添加节点的子节点
	参数1：节点结构
	参数2：子节点结构
	返回：1-成功，0-失败
	*/
	int xx_node_add_sub(struct ST_NODE *pst, struct ST_NODE *psub);


	/*
	api
	节点链完整拷贝
	参数1：目的节点结构
	参数2：源节点结构
	返回：1-成功，0-失败
	*/
	int xx_node_copy(struct ST_NODE *pdes, struct ST_NODE *psrc);



	/*
	删除该节点的子节点链，不包括它本身
	参数1：节点结构
	*/
	void xx_node_sub_all_del(struct ST_NODE *pnode);

	/*
	删除该节点的某个子节点链，不包括该节点本身
	参数1：节点结构
	*/
	__declspec(noinline) void xx_node_sub_del(struct ST_NODE *pnode, int index);

	/*
	判断该节点是否可以生成表达式
	所有子节点如果有表达式或者是最低级子节点，则符合条件
	如果子节点没有表达式并且不是最低级子节点，则不符合
	参数1：节点结构
	返回：1-符合条件，0-不符合
	*/
	int xx_node_exp_cnd(struct ST_NODE *pnode);



	/*
	判断该节点是否进行定义优化
	*/
	__declspec(noinline) int xx_node_opmz_idef_cnd(struct ST_NODE *pnode);



	/*
	判断该节点是否进行表达式优化
	*/
	__declspec(noinline) int xx_node_opmz_exp_cnd(struct ST_NODE *pnode);



#ifdef __cplusplus
}
#endif




