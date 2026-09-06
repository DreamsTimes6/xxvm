#include <stdio.h>


#define    sop_len   6

#define    sop_cmove            "010101"
#define    sop_cmove            "010101"
#define    sop_comvne           "010102"
#define    sop_comvnz           "010102"
#define    sop_cmova            "010103"
#define    sop_cmovnbe          "010103"
#define    sop_cmovae           "010104"
#define    sop_cmovnb           "010104"
#define    sop_cmovb            "010105"
#define    sop_cmovnae          "010105"
#define    sop_cmovbe           "010106"
#define    sop_cmovna           "010106"
#define    sop_cmovg            "010107"
#define    sop_cmovnle          "010107"
#define    sop_cmovge           "010108"
#define    sop_cmovnl           "010108"
#define    sop_cmovl            "010109"
#define    sop_cmovnge          "010109"
#define    sop_cmovle           "01010a"
#define    sop_cmovng           "01010a"
#define    sop_cmovc            "01010b"
#define    sop_cmovnc           "01010c"
#define    sop_cmovo            "01010d"
#define    sop_cmovno           "01010e"
#define    sop_cmovs            "01010f"
#define    sop_cmovns           "010110"
#define    sop_cmovp            "010111"
#define    sop_cmovpe           "010111"
#define    sop_cmovnp           "010112"
#define    sop_cmovpo           "010112"
#define    sop_xchg             "010113"
#define    sop_bswap            "010114"
#define    sop_xadd             "010115"
#define    sop_cmpxchg          "010116"
#define    sop_cmpxchg8b        "010117"
#define    sop_push             "010118"
#define    sop_pop              "010119"
#define    sop_pusha            "01011a"
#define    sop_pushad           "01011a"
#define    sop_popa             "01011b"
#define    sop_popad            "01011b"
#define    sop_cwd              "01011c"
#define    sop_cdq              "01011c"
#define    sop_cbw              "01011d"
#define    sop_cwde             "01011d"
#define    sop_movsx            "01011e"
#define    sop_movzx            "01011f"
#define    sop_mov              "010120"


#define    sop_adcx          "010201"
#define    sop_adox          "010202"
#define    sop_add           "010203"
#define    sop_adc           "010204"
#define    sop_sub           "010205"
#define    sop_sbb           "010206"
#define    sop_imul          "010207"
#define    sop_mul           "010208"
#define    sop_idiv          "010209"
#define    sop_div           "01020a"
#define    sop_inc           "01020b"
#define    sop_dec           "01020c"
#define    sop_neg           "01020d"
#define    sop_cmp           "01020e"


#define    sop_daa           "010301"
#define    sop_das           "010302"
#define    sop_aaa           "010303"
#define    sop_aas           "010304"
#define    sop_aam           "010305"
#define    sop_aad           "010306"



#define    sop_and           "010401"
#define    sop_or            "010402"
#define    sop_xor           "010403"
#define    sop_not           "010404"


#define    sop_sar           "010501"
#define    sop_shr           "010502"
#define    sop_sal           "010503"
#define    sop_shl           "010504"
#define    sop_shrd          "010505"
#define    sop_shld          "010506"
#define    sop_ror           "010507"
#define    sop_rol           "010508"
#define    sop_rcr           "010509"
#define    sop_rcl           "01050a"



#define    sop_bt                 "010601"
#define    sop_bts                "010602"
#define    sop_btr                "010603"
#define    sop_btc                "010604"
#define    sop_bsf                "010605"
#define    sop_bsr                "010606"
#define    sop_sete               "010607"
#define    sop_setz               "010607"
#define    sop_setne              "010608"
#define    sop_setnz              "010608"
#define    sop_stea               "010609"
#define    sop_setnbe             "010609"
#define    sop_setae              "01060a"
#define    sop_setnb              "01060a"
#define    sop_setnc              "01060a"
#define    sop_setb               "01060b"
#define    sop_setnae             "01060b"
#define    sop_setc               "01060b"
#define    sop_setbe              "01060c"
#define    sop_setna              "01060c"
#define    sop_setg               "01060d"
#define    sop_setnle             "01060d"
#define    sop_setge              "01060e"
#define    sop_setnl              "01060e"
#define    sop_setl               "01060f"
#define    sop_setnge             "01060f"
#define    sop_setle              "010610"
#define    sop_setng              "010610"
#define    sop_sets               "010611"
#define    sop_setns              "010612"
#define    sop_seto               "010613"
#define    sop_setno              "010614"
#define    sop_setpe              "010615"
#define    sop_setp               "010615"
#define    sop_setpo              "010616"
#define    sop_setnp              "010616"
#define    sop_test               "010617"
#define    sop_crc32              "010618"
#define    sop_popcnt             "010619"



#define    sop_jmp               "010701"
#define    sop_je                "010702"
#define    sop_jz                "010702"
#define    sop_jne               "010703"
#define    sop_jnz               "010703"
#define    sop_ja                "010704"
#define    sop_jnbe              "010704"
#define    sop_jae               "010705"
#define    sop_jnb               "010705"
#define    sop_jb                "010706"
#define    sop_jnae              "010706"
#define    sop_jbe               "010707"
#define    sop_jna               "010707"
#define    sop_jg                "010708"
#define    sop_jnle              "010708"
#define    sop_jge               "010709"
#define    sop_jnl               "010709"
#define    sop_jl                "01070a"
#define    sop_jnge              "01070a"
#define    sop_jle               "01070b"
#define    sop_jng               "01070b"
#define    sop_jc                "01070c"
#define    sop_jnc               "01070d"
#define    sop_jo                "01070e"
#define    sop_jno               "01070f"
#define    sop_js                "010710"
#define    sop_jns               "010711"
#define    sop_jpo               "010712"
#define    sop_jnp               "010712"
#define    sop_jpe               "010713"
#define    sop_jp                "010713"
#define    sop_jcxz              "010714"
#define    sop_jecxz             "010714"
#define    sop_loop              "010715"
#define    sop_loopz             "010716"
#define    sop_loope             "010716"
#define    sop_loopnz            "010717"
#define    sop_loopne            "010717"
#define    sop_call              "010718"
#define    sop_ret               "010719"
#define    sop_retn              "010719"
#define    sop_iret              "01071a"
#define    sop_int               "01071b"
#define    sop_into              "01071c"
#define    sop_bound             "01071d"
#define    sop_enter             "01071e"
#define    sop_leave             "01071f"


#define    sop_movsb       "010801"
#define    sop_movs        "010802"

#define    sop_cmps       "010804"
#define    sop_cmpsw      "010805"
//#define    sop_cmps       "010806"
#define    sop_cmpsd      "010806"
#define    sop_scas       "010807"
#define    sop_scasb      "010807"
//#define    sop_scas       "010808"
#define    sop_scasw      "010808"
//#define    sop_scas       "010809"
#define    sop_scasd      "010809"
#define    sop_lodsb      "01080a"
#define    sop_lods       "01080b"
//#define    sop_lodsw      "01080b"
//#define    sop_lodsd      "01080c"

#define    sop_stosb      "01080d"
#define    sop_stos       "01080e"

#define    sop_rep        "010810"
#define    sop_repe       "010811"
#define    sop_repz       "010811"
#define    sop_repne      "010812"
#define    sop_repnz      "010812"



#define    sop_in       "010901"
#define    sop_out      "010902"
#define    sop_ins      "010903"
#define    sop_insb     "010903"
//#define    sop_ins      "010904"
#define    sop_insw     "010904"
//#define    sop_ins      "010905"
#define    sop_insd     "010905"
#define    sop_outs     "010906"
#define    sop_outsb    "010906"
//#define    sop_outs     "010907"
#define    sop_outsw    "010907"
//#define    sop_outs     "010908"
#define    sop_outsd    "010908"


//#define    sop_enter         "010a01"
//#define    sop_leave         "010a02"


#define    sop_stc              "010b01"
#define    sop_clc              "010b02"
#define    sop_cmc              "010b03"
#define    sop_cld              "010b04"
#define    sop_std              "010b05"
#define    sop_lahf             "010b06"
#define    sop_sahf             "010b07"
#define    sop_pushf            "010b08"
#define    sop_pushfd           "010b08"
#define    sop_popf             "010b09"
#define    sop_popfd            "010b09"
#define    sop_sti              "010b0a"
#define    sop_cli              "010b0b"


#define    sop_lds           "010c01"
#define    sop_les           "010c02"
#define    sop_lfs           "010c03"
#define    sop_lgs           "010c04"
#define    sop_lss           "010c05"


#define    sop_lea           "010d01"
#define    sop_nop           "010d02"
#define    sop_ud2           "010d03"
#define    sop_xlat          "010d04"
#define    sop_xlatb         "010d04"
#define    sop_cpuid         "010d05"
#define    sop_movbe         "010d06"
#define    sop_prefetchw     "010d07"
#define    sop_prefetchwt1   "010d08"
#define    sop_clflush       "010d09"
#define    sop_clflushopt    "010d0a"
#define    sop_rdtsc         "010d0b"

#define    sop_xsave         "010e01"
#define    sop_xsavec        "010e02"
#define    sop_xsaveopt      "010e03"
#define    sop_xrstor        "010e04"
#define    sop_xgetbv        "010e05"


#define    sop_rdrand        "010f01"
#define    sop_rdseed        "010f02"




























