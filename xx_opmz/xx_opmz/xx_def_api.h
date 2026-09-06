#pragma once
#include <stdio.h>





/*
参数1：优化等级
*/
int def_api_init(int lv);


////////////////////  自定义指令操作  ///////////////////////////////

/*
获取指令的序号，其它操作全部使用索引序号
参数1：指令数字定义
参数2：返回指令序号
返回：成功-1；失败-0
*/
int def_api_get_iseq(int idef, int *out_seq);

/*
获取指令示例文本
参数1：指令序号
参数2：接收缓冲区
返回：成功-1；失败-0
*/
int def_api_get_sdef(int seq, char *pdata, int size);


/*
获取指令是否必须标记
返回：1-标记，2-否
*/
int def_api_get_mark(int idef);

/*
获取指令操作数字定义，
参数1：指令序号
参数2：返回数字定义
返回：成功-1；失败-0
*/
int def_api_get_op_idef(int seq, int *out_idef);

/*
获取指令操作文本
参数1：指令序号
参数2：接收缓冲区
返回：成功-1；失败-0
*/
int def_api_get_op_sdef(int seq, char *pdata, int size);

/*
获取指令是否参与分析树生成，1-是，0-否
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
int def_api_get_tree_op(int seq, int *out_flag);

/*
获取指令是否开始分析标志，1 - 开始，0 - 否
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
int def_api_get_tree_analyze(int seq, int *out_flag);

/*
获取指令是否继续分析标志，（分析树存在，是否继续分析）1 - 继续;2-只操作sp，不生成树，返回成功;3-退出
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
int def_api_get_tree_continue(int seq, int *out_flag);

/*
获取指令合并失败后下一步操作标志，1 - 继续，0-退出，2-合并失败，外部break
参数1：指令序号
参数2：返回标志
返回：成功-1；失败-0
*/
int def_api_get_merge_flag(int seq, int *out_flag);

/*
获取指令执行后的sp变化，
参数1：指令序号
参数2：返回sp变化量
返回：成功-1；失败-0
*/
int def_api_get_sp_offset1(int seq, int *out_offset);

/*
获取指令执行前的sp变化，
参数1：指令序号
参数2：返回sp变化量
返回：成功-1；失败-0
*/
int def_api_get_sp_offset2(int seq, int *out_offset);



////////////////////////////  结果项  /////////////////////////////////
/*
获取结果项的指令定义，如果结果项为指令变量
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_rlst_get_inst(int seq, int *out_inst);

/*
获取结果项的数字定义
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_rlst_get_idef(int seq, int *out_idef);



/*获取结果项的可变化标志*/
int def_api_rlst_get_chg(int seq);

/*获取结果项的可变化指令定义*/
int def_api_rlst_get_sep_idef(int seq);

/*获取结果项的可变化指令定义*/
int def_api_rlst_get_expd_idef1(int seq);
int def_api_rlst_get_expd_idef2(int seq);

/*
获取结果项的文本
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_rlst_get_sdef(int seq, char *out_sdef, int size);

/*
获取结果项的大小
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_rlst_get_size(int seq, int *out_size);

/*
获取执行后结果项的sp偏移值，如果有，没有则失败，类型不对则失败
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
int def_api_rlst_get_offset1(int seq, int *out_offset);

/*
获取执行前结果项的sp偏移值，如果有，没有则失败，类型不对则失败
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
int def_api_rlst_get_offset2(int seq, int *out_offset);

///////////////////////  操作项  ///////////////////////////////
/*
获取指令操作项个数
参数1：指令序号
参数2：返回操作项个数
返回：成功-1；失败-0
*/
int def_api_get_item_num(int seq, int *out_num);


/*
判断操作项是否是指令变量，不使用
参数1：指令序号
参数2：操作项序号
返回：是-1；否-0
*/
int def_api_item_judge_inst(int seq, int item_seq);

/*
获取操作项的指令定义，如果操作项为指令变量
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_item_get_inst(int seq, int item_seq, int *out_inst);

/*
获取操作项的数字定义
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_item_get_idef(int seq, int item_seq, int *out_idef);

/*
获取操作项的文本
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_item_get_sdef(int seq, int item_seq, char *out_sdef, int size);

/*
获取操作项的大小
参数1：指令序号
参数2：操作项序号
参数3：返回定义
返回：是-1；否-0
*/
int def_api_item_get_size(int seq, int item_seq, int *out_size);

/*
获取执行后操作项的sp偏移值，如果有
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
int def_api_item_get_offset1(int seq, int item_seq, int *out_offset);

/*
获取执行前操作项的sp偏移值，如果有
参数1：指令序号
参数2：操作项序号
参数3：返回偏移值
返回：是-1；否-0
*/
int def_api_item_get_offset2(int seq, int item_seq, int *out_offset);


/*
查询tree_analyze标志操作
参数1：指令定义
返回：1-是；0-否
*/
int def_api_tree_analyze(int idef);

/*
查询rflag标志
参数1：指令定义
返回：1-有标志；0-没有
*/
int def_api_rflag(int idef);


/*
获取指令操作项的可变化标志
参数1：指令序号
参数2：操作项序号
返回：成功-拆分指令定义；失败-0
*/
int def_api_item_get_chg(int seq, int item_seq);


/*
获取指令的操作项的变化指令定义
参数1：指令定义
参数2：操作项序号
返回：成功-拆分指令定义；失败-0
*/
int def_api_item_get_sep_idef(int seq, int item_seq);

/*
获取指令的操作项的变化指令定义
参数1：指令定义
参数2：操作项序号
返回：成功-拆分指令定义；失败-0
*/
int def_api_item_get_expd_idef1(int seq, int item_seq);
int def_api_item_get_expd_idef2(int seq, int item_seq);



/*
设置常量寄存器的索引
*/
void def_api_creg_index_set(int index);



/*
设置常量寄存器的值
*/
int def_api_creg_var_set(int curt_sys, unsigned char *pvar);



/*
获取常量寄存器的索引
*/
int def_api_creg_index_get();




/*
获取常量寄存器的值
*/
unsigned char* def_api_creg_var_get();










