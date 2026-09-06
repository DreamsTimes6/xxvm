#include <stdio.h>


//#define    op_len   6

#define    op_cmove            0x010101 
#define    op_cmovz            0x010101 
#define    op_comvne           0x010102 
#define    op_comvnz           0x010102 
#define    op_cmova            0x010103 
#define    op_cmovnbe          0x010103 
#define    op_cmovae           0x010104 
#define    op_cmovnb           0x010104 
#define    op_cmovb            0x010105 
#define    op_cmovnae          0x010105 
#define    op_cmovbe           0x010106 
#define    op_cmovna           0x010106 
#define    op_cmovg            0x010107 
#define    op_cmovnle          0x010107 
#define    op_cmovge           0x010108 
#define    op_cmovnl           0x010108 
#define    op_cmovl            0x010109 
#define    op_cmovnge          0x010109 
#define    op_cmovle           0x01010a 
#define    op_cmovng           0x01010a 
#define    op_cmovc            0x01010b 
#define    op_cmovnc           0x01010c 
#define    op_cmovo            0x01010d 
#define    op_cmovno           0x01010e 
#define    op_cmovs            0x01010f 
#define    op_cmovns           0x010110 
#define    op_cmovp            0x010111 
#define    op_cmovpe           0x010111 
#define    op_cmovnp           0x010112 
#define    op_cmovpo           0x010112 
#define    op_xchg             0x010113 
#define    op_bswap            0x010114 
#define    op_xadd             0x010115 
#define    op_cmpxchg          0x010116 
#define    op_cmpxchg8b        0x010117 
#define    op_push             0x010118 
#define    op_pop              0x010119 
#define    op_pusha            0x01011a 
#define    op_pushad           0x01011a 
#define    op_popa             0x01011b 
#define    op_popad            0x01011b 
#define    op_cwd              0x01011c 
#define    op_cdq              0x01011c 
#define    op_cbw              0x01011d 
#define    op_cwde             0x01011d 
#define    op_movsx            0x01011e 
#define    op_movzx            0x01011f 
#define    op_mov              0x010120 


#define    op_adcx          0x010201 
#define    op_adox          0x010202 
#define    op_add           0x010203 
#define    op_adc           0x010204 
#define    op_sub           0x010205 
#define    op_sbb           0x010206 
#define    op_imul          0x010207 
#define    op_mul           0x010208 
#define    op_idiv          0x010209 
#define    op_div           0x01020a 
#define    op_inc           0x01020b 
#define    op_dec           0x01020c 
#define    op_neg           0x01020d 
#define    op_cmp           0x01020e 


#define    op_daa           0x010301 
#define    op_das           0x010302 
#define    op_aaa           0x010303 
#define    op_aas           0x010304 
#define    op_aam           0x010305 
#define    op_aad           0x010306 



#define    op_and           0x010401 
#define    op_or            0x010402 
#define    op_xor           0x010403 
#define    op_not           0x010404 


#define    op_sar           0x010501 
#define    op_shr           0x010502 
#define    op_sal           0x010503 
#define    op_shl           0x010504 
#define    op_shrd          0x010505 
#define    op_shld          0x010506 
#define    op_ror           0x010507 
#define    op_rol           0x010508 
#define    op_rcr           0x010509 
#define    op_rcl           0x01050a 



#define    op_bt                 0x010601 
#define    op_bts                0x010602 
#define    op_btr                0x010603 
#define    op_btc                0x010604 
#define    op_bsf                0x010605 
#define    op_bsr                0x010606 
#define    op_sete               0x010607 
#define    op_setz               0x010607 
#define    op_setne              0x010608 
#define    op_setnz              0x010608 
#define    op_stea               0x010609 
#define    op_setnbe             0x010609 
#define    op_setae              0x01060a 
#define    op_setnb              0x01060a 
#define    op_setnc              0x01060a 
#define    op_setb               0x01060b 
#define    op_setnae             0x01060b 
#define    op_setc               0x01060b 
#define    op_setbe              0x01060c 
#define    op_setna              0x01060c 
#define    op_setg               0x01060d 
#define    op_setnle             0x01060d 
#define    op_setge              0x01060e 
#define    op_setnl              0x01060e 
#define    op_setl               0x01060f 
#define    op_setnge             0x01060f 
#define    op_setle              0x010610 
#define    op_setng              0x010610 
#define    op_sets               0x010611 
#define    op_setns              0x010612 
#define    op_seto               0x010613 
#define    op_setno              0x010614 
#define    op_setpe              0x010615 
#define    op_setp               0x010615 
#define    op_setpo              0x010616 
#define    op_setnp              0x010616 
#define    op_test               0x010617 
#define    op_crc32              0x010618 
#define    op_popcnt             0x010619 



#define    op_jmp               0x010701 
#define    op_je                0x010702 
#define    op_jz                0x010702 
#define    op_jne               0x010703 
#define    op_jnz               0x010703 
#define    op_ja                0x010704 
#define    op_jnbe              0x010704 
#define    op_jae               0x010705 
#define    op_jnb               0x010705 
#define    op_jb                0x010706 
#define    op_jnae              0x010706 
#define    op_jbe               0x010707 
#define    op_jna               0x010707 
#define    op_jg                0x010708 
#define    op_jnle              0x010708 
#define    op_jge               0x010709 
#define    op_jnl               0x010709 
#define    op_jl                0x01070a 
#define    op_jnge              0x01070a 
#define    op_jle               0x01070b 
#define    op_jng               0x01070b 
#define    op_jc                0x01070c 
#define    op_jnc               0x01070d 
#define    op_jo                0x01070e 
#define    op_jno               0x01070f 
#define    op_js                0x010710 
#define    op_jns               0x010711 
#define    op_jpo               0x010712 
#define    op_jnp               0x010712 
#define    op_jpe               0x010713 
#define    op_jp                0x010713 
#define    op_jcxz              0x010714 
#define    op_jecxz             0x010714 
#define    op_loop              0x010715 
#define    op_loopz             0x010716 
#define    op_loope             0x010716 
#define    op_loopnz            0x010717 
#define    op_loopne            0x010717 
#define    op_call              0x010718 
#define    op_retn              0x010719 
#define    op_ret               0x010719 
#define    op_iret              0x01071a 
#define    op_int               0x01071b 
#define    op_into              0x01071c 
#define    op_bound             0x01071d 
#define    op_enter             0x01071e 
#define    op_leave             0x01071f 


#define    op_movsb      0x010801 
#define    op_movs       0x010802 

#define    op_cmpsb       0x010804 
#define    op_cmps      0x010805 
//#define    op_cmps       0x010806 
//#define    op_cmpsd      0x010806 
#define    op_scasb       0x010807 
#define    op_scas      0x010808 
//#define    op_scas       0x010808 
//#define    op_scasw      0x010808 
//#define    op_scas       0x010809 
//#define    op_scasd      0x010809 
#define    op_lodsb      0x01080a
#define    op_lods       0x01080b  
//#define    op_lodsw      0x01080b 
//#define    op_lodsd      0x01080c 

#define    op_stosb      0x01080d 
#define    op_stos       0x01080e 

#define    op_rep        0x010810 
#define    op_repe       0x010811 
#define    op_repz       0x010811 
#define    op_repne      0x010812 
#define    op_repnz      0x010812 



#define    op_in       0x010901 
#define    op_out      0x010902 
#define    op_ins      0x010903 
#define    op_insb     0x010903 
//#define    op_ins      0x010904 
#define    op_insw     0x010904 
//#define    op_ins      0x010905 
#define    op_insd     0x010905 
#define    op_outs     0x010906 
#define    op_outsb    0x010906 
//#define    op_outs     0x010907 
#define    op_outsw    0x010907 
//#define    op_outs     0x010908 
#define    op_outsd    0x010908 


//#define    op_enter         0x010a01 
//#define    op_leave         0x010a02 


#define    op_stc              0x010b01 
#define    op_clc              0x010b02 
#define    op_cmc              0x010b03 
#define    op_cld              0x010b04 
#define    op_std              0x010b05 
#define    op_lahf             0x010b06 
#define    op_sahf             0x010b07 
#define    op_pushf            0x010b08 
#define    op_pushfd           0x010b08 
#define    op_popf             0x010b09 
#define    op_popfd            0x010b09 
#define    op_sti              0x010b0a 
#define    op_cli              0x010b0b 


#define    op_lds           0x010c01 
#define    op_les           0x010c02 
#define    op_lfs           0x010c03 
#define    op_lgs           0x010c04 
#define    op_lss           0x010c05 


#define    op_lea           0x010d01 
#define    op_nop           0x010d02 
#define    op_ud2           0x010d03 
#define    op_xlat          0x010d04 
#define    op_xlatb         0x010d04 
#define    op_cpuid         0x010d05 
#define    op_movbe         0x010d06 
#define    op_prefetchw     0x010d07 
#define    op_prefetchwt1   0x010d08 
#define    op_clflush       0x010d09 
#define    op_clflushopt    0x010d0a 
#define    op_rdtsc         0x010d0b

#define    op_xsave         0x010e01 
#define    op_xsavec        0x010e02 
#define    op_xsaveopt      0x010e03 
#define    op_xrstor        0x010e04 
#define    op_xgetbv        0x010e05 


#define    op_rdrand        0x010f01 
#define    op_rdseed        0x010f02 



/*¼Ä´æÆ÷ºê¶¨Òå*/


























