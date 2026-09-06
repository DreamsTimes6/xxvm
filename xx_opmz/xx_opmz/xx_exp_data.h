#include <stdio.h>




/*
表达式操作
输入表达式，输出一条汇编指令

生成的表达式和还原数据的表达式采用同一结构

变量扫描，可能存在的变量，寄存器(reg[])，常数(0x1234)，sp或者sp+x
变量表示：变量数，变量名
表达式变量替换：替换变量名，生成通用表达式
表达式生成汇编代码，带变量名
替换汇编代码的变量名



定义表达式的表示


客户端
还原数据从服务器获取，方便更新添加，
获取条件，优化数据是否更新的条件
读取已有规则
匹配规则


客户端功能
1.检查还原数据更新
2.开始化简还原


服务端，
输入规则
生成规则


*/

#define DEF_EXP_SIZE  0x100




struct XX_EXP_VAR
{
	int idef;            //变量类型
	int flag;            //变量实际名称定值标志，定值参与匹配，因为有常数的情况
	int calc_var;        //运算变量序号，从0开始，-1表示无
	int calc_type;       //运算类型,同指令操作定义
	char sname[0x10];     //变量命名
	char svar[0x30];      //变量实际名称
	char ivar[0x10];       //变量值，常量或者寄存器索引
};


/*
表达式定义
需要存储，使用固定空间
*/
struct XX_EXP
{
	//char pin_exp[0x100];    //输入的表达式，减小体积
	//int  exp_size;    //输入的表达式空间大小，通用也使用此大小，减小体积
	char pcomm_exp[0x100];  //通用表达式
	char pasm_inst[0x100];  //汇编指令
	int  var_num;     //变量数
	int  idef;       //指令定义，中间指令
	unsigned long lv_flag;  //等级标志
	int   sys_flag;    //位数标志,32位第一位为1，64位第二位为1
	int mark_flag;             //标记标志，只对非中间指令生效，1-只标记输出，2-标记所有，3-标记输出，临时标记其他
								//4-标记所有，不输出
	int mark_place;			   //标记位置，并在此序号处生成优化结果，1-开始，2-结束，只对非中间指令生效
	//int restart_flag;          //重新开始标志，部分指令识别后，重新分析此指令段
	struct XX_EXP_VAR exp_var[8];  //变量 
};


/*
初始化exp_tbl
*/
 int xx_exp_data_init(char *);


 /*
释放exp_tbl
*/
 void xx_exp_data_free();

/*
获取exp_tbl
*/
 struct XX_EXP  *xx_exp_data_get();


/*
获取exp_num
*/
 int xx_exp_data_num();




 /*
 输入一个原始表达式，生成一条汇编指令
 参数1：原始表达式
 参数2：返回汇编指令
 返回：1-成功；0-失败
 */
 int xx_exp_to_asm(char *pexp, struct XX_EXP *out_asm, int lv);



















