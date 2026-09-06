#pragma once
#include <stdio.h>
#include "st_node.h"





#ifdef __cplusplus
extern "C" {
#endif

	/*
	回调函数定义，返回，1-回调函数正确执行，0-回调函数内有错误
	*/
	typedef int(*def_tree_cb_cmp)(void *pdata1,void *pdata2);

	typedef int(*def_tree_cb_data)(void *pdata);


	typedef void(*def_tree_cb_set_idata)(void *pdata, int idata);

	/*
	自定义节点表达式生成回调函数，由外部生成表达式
	参数1：父节点数据指针
	参数2：子节点数，
	参数3：子节点数据指针数组，
	参数4：子节点表达式指针数组，
	参数5：用来存放返回的父节点表达式的指针的地址
	返回：1-生成了表达式，0-失败
		失败会释放所有的节点表达式
		节点数据为自定义数据，所以数据大小是已知的
		用来返回的表达式存储空间由回调函数计算和申请，由树结构释放
		回调函数只能使用malloc申请空间
	*/
	typedef int(*def_tree_cb_node_exp)(void *,int ,void **,char **,char **);

	typedef int(*def_tree_cb_exp)(void *, char *, char **);

	/*
	树优化的回调函数，表达式优化，优化树的内部表达式
	参数1：该节点的表达式
	返回：如果有优化后的表达式，则返回该表达式，没有则返回0不操作
	回调函数不出错
		只操作该节点的表达式，不操作该节点的自定义数据
		用来返回的表达式存储空间由回调函数计算和申请，由树结构释放
		回调函数只能使用malloc申请空间
	*/
	//typedef char*(*def_tree_cb_opmz_exp)(char *pexp);


	typedef void*(*def_tree_cb_opmz_data)(void *pdata, int sub_num, void **psub_data);

	typedef int(*def_tree_cb_opmz_idef)(void *pdata, int sub_num, void **psub_data, int*sub_index);

	typedef int(*def_tree_cb_opmz_exp)(void *pdata, char*pexp, int sub_num, char **psub_exp, int*sub_index);

	////////////////////////////////////////////  struct  ////////////////////////////////////////////////
	/*树结构*/
	struct ST_TREE
	{
		struct ST_NODE *proot;  //根节点

		int  inode;             //节点数，包括根节点
		int  level;             //树高度

		long sec;               //时间戳
		long nsec;              //时间戳
		long rand;              //随机数

		char *pexp;             //最终的表达式存储在树结构，方便释放
		int  exp_size;          //表达式空间大小
	};


	/////////////////////////////////////////// API  /////////////////////////////////////////////////////
	/*
	根据根数据和子数据建立树结构
	参数1：根数据
	参数2：根数据大小
	参数3：子数据数目
	参数4：子数据数组指针
	参数5：子数据大小的数组指针
	返回：树结构-成功，0-失败
	*/
	struct ST_TREE *xx_tree_new(void *proot_data, int root_size, int sub_num, void **psub_data, int *sub_size);


	/*由一个根数据生成树
	参数1：父节点数据
	参数2：父节点数据大小
	返回：树结构
	*/
	struct ST_TREE *xx_tree_new_root(void *pdata, int data_size);


	/*
	给树的根节点添加子数据节点
	参数1：树结构
	参数2：子数据
	参数3：子数据大小
	返回：1-成功，0-失败
	*/
	int xx_tree_add_data(struct ST_TREE *ptree, void *pdata, int data_size);


	/*释放树结构
		生成树失败，也进行释放操作
		完全释放，节点中的数据也会释放
		不能重复释放，重复释放会崩溃
	*/
	void xx_tree_free(struct ST_TREE *ptree);


	/*
	判断该数据是否是树的最低级数据
	参数1：树结构
	参数2：待查找的数据
	参数3：数据查找回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
	返回：1-是，0-不是
	*/
	int xx_tree_data_lowsub(struct ST_TREE *ptree, void *pdata, def_tree_cb_cmp cb);


	/*
	判断树2的根节点是否是树1的最低级子节点
	参数1：树结构1
	参数2：树结构2
	参数3：数据比较回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
	返回：1-树1的某个最低级子节点；0-不是树1的最低级子节点
	*/
	int xx_tree_is_lowsub(struct ST_TREE *ptree1, struct ST_TREE *ptree2, def_tree_cb_cmp cb);


	/*
	判断树2的根节点是否是树1的子节点
	参数1：树结构1
	参数2：树结构2
	参数3：数据比较回调函数，
		如果为0：比较数据区是否完全相同
		不为0：自定义比较回调函数
	返回：1-树1的某个子节点；0-不是树1的子节点
	*/
	int xx_tree_is_sub(struct ST_TREE *ptree1, struct ST_TREE *ptree2, def_tree_cb_cmp cb);


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
	返回：成功-1；失败-0
	*/
	int xx_tree_merge(struct ST_TREE *ptree1, struct ST_TREE *ptree2, def_tree_cb_cmp cb);

	
	/*
	复制一个树
	*/
	struct ST_TREE *xx_tree_copy(struct ST_TREE *ptree);

	/*
	检查分析树的完整性
	参数1：树结构
	参数2：数据检查回调函数，为0则只检查是否有空节点
	返回：1-完整，0-不完整
	*/
	int xx_tree_check(struct ST_TREE *ptree, def_tree_cb_data cb);


	/*
	检查分析树的完整性，只检查最低级子项
	参数1：树结构
	参数2：数据检查回调函数，为0则只检查是否有空节点
	返回：1-完整，0-不完整
	*/
	int xx_tree_low_check(struct ST_TREE *ptree, def_tree_cb_data cb);


	/*
	获取树的数据个数
		每个节点只有一个数据数据
	参数1：树结构
	返回：数据个数
	*/
	int xx_tree_data_num(struct ST_TREE *ptree);


	/*
	打印树结构
	参数1：树结构
	参数2：打印回调函数
		打印回调函数，为0则控制台输出，有则自定义输出
	返回：1-正常输出，0-树结构错误
	*/
	int xx_tree_print(struct ST_TREE *ptree, def_tree_cb_data cb);



	/*
	新增
	获取根节点的数据
	*/
	void* xx_tree_root_data(struct ST_TREE *ptree);



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
	void *xx_tree_find_lowdata(struct ST_TREE *ptree1, void *pdata2, def_tree_cb_cmp cb);


	/*
	为所有的节点设置一个数据
	*/
	void xx_tree_set_idata(struct ST_TREE *ptree, def_tree_cb_set_idata cb, int idata);


	/*
	为最低级的子节点设置一个数据
	*/
	void xx_tree_set_low_idata(struct ST_TREE *ptree, def_tree_cb_set_idata cb, int idata);
	////////////////////////////////  表达式相关接口  /////////////////////////////////////
	/*
	树生成最终表达式
	参数1：节点结构
	参数2：表达式回调函数
	返回：1-表达式的存储空间大小，0-失败
		失败会释放所有的节点表达式
	*/
	int xx_tree_node_exp(struct ST_TREE *ptree, def_tree_cb_node_exp cb);

	int xx_tree_exp(struct ST_TREE *ptree, def_tree_cb_exp cb);


	/*
	释放树的所有表达式空间
	参数1：树结构
	*/
	void xx_tree_exp_free(struct ST_TREE *ptree);


	/*
	释放树子节点的表达式空间
	参数1：树结构
	*/
	void xx_tree_exp_sub_free(struct ST_TREE *ptree);

	/*
	获取树表达式
	(直接从结构获取)
	参数1：树结构
	返回：树表达式-成功，0-失败
	*/
	

	/*
	获取树表达式的大小
	(直接从结构获取)
	参数1：树结构
	返回：树表达式大小-成功，0-失败
	*/


	////////////////////////////////  优化相关接口  /////////////////////////////////////
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
		失败会释放所有的节点表达式
	*/
	//int xx_tree_opmz_exp(struct ST_TREE *ptree, def_tree_cb_opmz_exp cb_opmz, def_tree_cb_node_exp cb_exp);

	/*
	分析树优化
	回调函数进行对应的分析，
	回调函数的返回值没有成功或者失败，只有指定操作类型
	返回值决定对应的操作，是否对节点进行操作，一般的操作为删除子节点
	
	回调函数返回值
	1：删除某个子节点
	未定义的不操作
	*/
	void xx_tree_opmz_idef(struct ST_TREE *ptree, def_tree_cb_opmz_idef cb);

	void xx_tree_opmz_exp(struct ST_TREE *ptree, def_tree_cb_opmz_exp cb);


	////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
	
	*/

#ifdef __cplusplus
}
#endif





