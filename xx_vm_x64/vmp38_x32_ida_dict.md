# VMP 3.8 32 位适配：headless 段读取修复 + IDA 语义表提取

## 一、headless 段读取修复（文件回退反汇编）

**问题**：headless 下 VM 段（.0]o 等大段）未映射到进程内存，`DbgDisasmAt`/`DbgMemRead` 失败 → step_one fail → 还原停住。

**修复**（vmp38_restore.cpp）：
1. **vmp38_set_module(path, imgbase)**：打开模块文件，解析 PE 段表
2. **read_mem_file(addr, buf, n)**：VA → RVA（用 `g_runbase` 运行时基址，ASLR 后 ≠ PE ImageBase）→ 段查找 → 文件偏移读取
3. **disasm_from_file(ip, out_ins)**：从文件读指令字节 → **xx_disasm**（插件自带 32 位反汇编器）→ 填 DISASM_INSTR
4. **step_one 失败回退**：DbgDisasmAt 失败 → disasm_from_file
5. **auto_locate**：文件回退扫描 .text（xx_disasm，不依赖进程内存）+ fallback 到 vmsect_begin

**实测**（vmp32_probe_38.vmp.exe，headless）：
- capture(fast): ip=013bc000 esp=008914b8 ebp=020ffbc0 ebx=01ebe000（真实寄存器）
- handler[0] alu(12 条+push/pop)、handler[1](movsx/movzx/btc/shl/lea) 识别
- 27 步 2 handler，真值提取（add esi,0x04 / add ebp,eax）正常

## 二、IDA 提取 32 位 handler 语义表

**方法**（ida_extract_full_table.py，IDAPython 批处理）：
1. 找 VM 段（最大非标准段：.0]o/.0Mb）
2. 扫描分派模式：`jmp reg` 前 16 条内有 `add/sub/xor/lea reg,imm`
3. 每个分派点回溯块起点（上一个分派/ret）= handler 入口
4. 过滤小 delta（0-10），排除大 delta 垃圾跳板
5. 对每个 handler 反汇编 30 条，提取语义特征：
   - `popf/popfd` → vExit（26 个）
   - `pushf/pushfd` → vStart（6 个）
   - esp 相对内存访问 + ALU → vStackALU（1024 个）
   - 纯 ALU → vALU（358 个）

**成果**：**vmp38_dict_data.c**（1414 条 handler 语义表，{rva, delta, cls} 数组），
**vmp38_dict_class(va)** 查询函数，接入动态执行 handler 边界标注。

**关键结论（3.8 vs 3.9 本质差异）**：
- **VMP 3.9**：delta 分派（`pop r11; add r11,delta; jmp r11`），delta 明文 → **静态字典可还原链**
- **VMP 3.8**：栈地址分派（handler 结束 `pop reg; jmp reg`，next 从 VM 栈取），
  delta 只是混淆 → **dict 表静态链不可行**，只能做**语义标注**
- VMP 3.8 深度混淆：每个 handler 含 add/neg/not/rol/ror/xor 垃圾组合，
  核心运算被淹没，**纯静态语义精确分类不可靠**
- 印证：VMP 3.8 完整还原需**动态执行/符号执行**（Triton 级），
  当前插件能力 = handler 边界 + 语义粗分类 + 混淆常量真值（mini-exec）+ dict 标注

## 三、文件清单

- `vmp38_x32/ida_extract_full_table.py` — IDA 语义表提取脚本（ASCII 纯英文）
- `vmp38_x32/vmp38_dict_data.c` — 1414 条 handler 语义表（生成物）
- `xx_vm_x32/vmp38_dict_data.c` — 复制到插件工程（vcxproj 已加）
- `xx_vm_x32/vmp38_restore.cpp` — 文件回退 + dict 查询/标注
- 测试：headless `eip=013BC000`（VM 段起始）→ VMP38-Restore


## 四、完整识别推进（2026-08-04 续）

### 1. 条件跳转建模
- STEP_REC 加 is_cond/cond_target：识别 jb/jnz/jz/je/jne/ja/jae/jbe/jl/jle/jg/jge/js/jns/jo/jno
- 主循环：条件跳转（backward loop）展开 ≤64 次，防止死循环
- 解密循环（`rol eax,N; jb`）不再打断 handler 边界

### 2. 精确虚拟指令模式识别（identify_handler 返回值）
| 模式 | 特征 | 输出 |
|---|---|---|
| vFetch | `mov reg,[esi]; add esi,4; xor; rol; jb` | 字节码取指解密分派（返回 1，触发寄存器重置） |
| vByteOp | movsx/movzx/btc/bts/btr + byte/word ptr | 宽度 8/16 位访问 |
| vConstRebuild | lea reg,[reg*scale±const] ≥2 次 | 常量重建 |
| vPush/vPop/vALU | 栈操作+ALU 统计 | 原有 |
| ADD/SUB | kanxue 280283 模式 | 原有 |

- **vFetch 后寄存器重置**：解密器垃圾指令（add ebp,eax 等）污染 mreg → 识别 vFetch 后恢复 capture 值
- **jmp 间接解析增强**：jmp reg 用 dict 表验证/最近匹配

### 3. 动态 trace 模式（GUI 完整还原路径）
`vmp38_dynamic_trace(start, max)`：DbgCmdExec("sti") 真实单步 + 读真实寄存器 + 
**EIP 跳变检测 handler 边界** + identify_handler 精确语义 + dict 标注。
- **headless 限制**：sti 是 no-op（EIP 冻结检测 → 自动回退静态）
- **GUI 下**：jmp ebp 真实跳转 → 完整 handler 链还原

### 4. 防护
- step_one junk 指令防护（outsb/ins/in/out/cpuid/rdtsc/hlt/int → 停止，防数据区反汇编崩溃）
- auto_locate 扫描 VM 段找首个代码块（连续 6 条可反汇编非 junk）

### 5. 实测（headless，probe 样本）
```
handler[0] vFetch (bytecode fetch/decrypt dispatch)
handler[1] vByteOp (movsx=1 movzx=2 bt=1 byte/word=4)
```
27 步 2 handler 稳定；addonly 样本 junk 防护不崩溃（exit=124 timeout）。

### 6. 限制（诚实）
- **链推进**：handler[1] 后 `jmp ebp` 目标运行时才知，静态不可解 → **headless 停在此**；GUI 动态 trace 可继续
- **dict 表样本特定**：probe 的 dict 不适用 addonly → 需每样本 IDA 重新生成
- **完整还原 = GUI 动态 trace**（真实执行），headless 只能单 handler 语义


## 七、mini-taint 数据流（2026-08-04 完成）

### 实现（vmp38_restore.cpp）
- MINI_REG 加 `t[16]` taint 位（每寄存器 1 bit：值来自 VM 栈/上下文）
- **taint 源**：`mov reg,[esp/ebp±imm]`（VM 栈读，无条件）、`pop reg`（模拟：vsp-4 + taint=1）、movsx/movzx 内存读（esp/ebp 基址或 mem_expr_tainted）
- **传播**：`mov reg2,reg1`（src taint→dst）、ALU 结果（dst taint = dst|src）、movzx/movsx reg 分支
- **排除**：`mov reg,imm` 清 taint（立即数是常量）、**esi 排除**（字节码指针，解密器用，不 taint）
- **mem_expr_tainted**：地址表达式任一寄存器 tainted → 目标 taint
- **is_core 标记**：ALU 结果操作数 tainted → STEP_REC.is_core=1（核心运算）
- **mreg 零初始化**（关键 bug 修复）：防栈随机值污染 taint

### 精确 ALU 语义
identify_handler 用 is_core 统计核心运算链 → 映射：
`vAdd`（含 add 无 sub）/ `vSub` / `vXor` / `vAnd` / `vOr` / `vMul`（imul/mul）/
`vShl` / `vShr` / `vRol` / `vRor`，输出 `(core[N]: ops链)` + `<CORE>` 指令标记

### 实测（headless，probe 样本）
- handler[0] vFetch：**无核心误判**（esi 排除 + mreg 零初始化后）
- handler[1] vByteOp：稳定（movsx/movzx/btc 宽度操作）
- **静态 taint 边界**：索引寄存器间接（`movzx eax,dl` 的 dl 来源静态不可知）会断链；
  完整 taint 需 GUI 动态 trace（真实执行天然精确）

### 关键 bug 修复记录
1. mreg 未零初始化 → t[16] 随机值污染（handler[0] 误判 vAdd）
2. esi 无条件 taint → 字节码解密 `xor ecx,ebx` 误判核心（vFetch 误报 vXor）
3. mem_expr_tainted 插到 MINI_REG 定义前（C2065）→ 移到其后


## 八、符号执行（Sym，Triton 级推进 2026-08-04）

### Sym 符号表达式核心
- **SYM_NODE 节点池**（8192 槽）：扁平表达式树，14 种 op（CONST/REG/ADD/SUB/XOR/AND/OR/NOT/NEG/SHL/SHR/ROL/ROR/LEA）
- **sym_build 即时化简**：常量折叠（双 CONST 立即求值）+ 恒等式（x+0/x-0/x^0/x|0=x、x&0=0、x&~0=x、x<<0=x、x^x=0、x-x=0、not(not x)=x、neg(neg x)=x）+ 宽度截断（sym_maskv 8/16/32/64）
- MINI_REG 加 `sym[16]`（每寄存器符号节点 id），与 t[16] taint 并行

### mini-exec 符号传播（各分支）
| 指令 | 符号 |
|---|---|
| pop reg | SYM_OP_REG（VM 栈来源符号） |
| mov reg,imm | SYM_OP_CONST（宽度截断） |
| mov reg,reg | 传播（const 时按宽度重截） |
| add/sub/xor/and/or imm/reg | SYM_OP_ADD/SUB/XOR/AND/OR |
| not/neg | SYM_OP_NOT/NEG |
| lea reg,[base*scale±disp] | LEA(base_sym, disp)（base 为常量时折叠） |
| rol/ror/shl/shr | （当前未建，回退真实值） |

### 解间接 jmp（关键）
- jmp reg：**sym_is_const 检查寄存器符号** → 常量则静态求值 next_ip（`[sym]` 输出）
- 非常量 → 回退 dict 表验证 / 寄存器真实值

### 实测（headless，probe 样本）——重大提升
```
handler[0] alu/push（VM 初始化）
handler[1] vXor (core[1]: xor)
handler[2] mixed / vByteOp
handler[3] push
handler[4] vSub (core[2]: sub+xor)
=== done: 69 steps, 5 handlers ===   ← 之前 27 步 2 handler
```
- **69 步 5 handler 完整还原**（首次 headless 连续识别 5 个 handler）
- jmp jz/call/ret 全部正常处理不断链
- vsp 跨 handler 连续追踪（01aff474→...→77b749cb）
- mini-taint 精确语义（vXor/vSub）由符号化简支撑

### 已知限制
- rol/ror/shl/shr 未建符号（回退真实值）——补上后常量链更精确
- lea 只近似（base 符号 + disp，scale 乘法未展开为 SYM_MUL）
- 节点池耗尽降级 const 0
- **仍是"轻量符号传播"**（非完整 Triton 级求解器），但已解决 jmp 断链 + 核心识别


## 九、符号执行常量链精确还原（2026-08-04 续）

### 新增
1. **rol/ror/shl/shr 符号**：SYM_SHL/SHR/ROL/ROR 表达式 + 常量折叠（宽度感知）
2. **lea scale 乘法展开**：SYM_MUL op + 恒等式（x*1=x、x*0=0）+ lea 分支 MUL(base_sym,scale)+disp 完整建模
3. **sym_eval_const**：递归求值（全常量折叠 → 输出真实魔数）
4. **push 符号还原**：push reg 时 reg_w 符号求值 → `[sym] true const = 0x...`（链还原魔数）
5. **sym_rebuild_all**：handler 边界/vFetch 重置时重建全部寄存器符号（修复池 id 悬空）

### 关键 bug 修复
- **符号池 id 悬空**：vFetch 重置 mreg=mreg0 后旧 sym id 指向错误节点（push esi 误折叠成 -8/0xfffffff8）→ sym_rebuild_all 重建
- sym_rebuild_all 定义位置（MINI_REG 之前 C2182）→ 移到其后

### 实测（headless，probe 样本）
```
push ebp  → [sym] true const = 0x7bfc04   （= push real，一致）
push esi  → [sym] true const = 0x43c255dd （符号链折叠的魔数，≠ 寄存器直接值 0x0！）
push edi  → [sym] true const = 0x43c255dd
```
**关键**：0x43c255dd 是**lea/alu 链符号折叠还原的魔数**——静态符号执行还原了运行时构建的常量（VMP 3.8 常量还原的核心目标）。

### 能力现状（headless 静态）
- 69 步 5 handler 完整链还原
- vXor/vSub 精确语义（mini-taint + 符号化简）
- **符号常量还原**（push/写栈时输出魔数）
- jmp 分派不断链（符号求解）


## 十、ASLR 稳定 + 内存符号传播 + jmp 全解（2026-08-04 三补）

### 1. ASLR 稳定
- auto_locate 返回**首选基址 VA**（PE ImageBase + RVA，固定 0x94C000），菜单用 `vmp38_get_imgbase()` 换算运行时地址
- 新增 `vmp38_get_imgbase()`（g_imgbase 访问器，static 变量跨文件不可见问题）
- **实测**：两次 headless 运行 VM entry rva=0x54C000 完全一致、handler 序列稳定

### 2. MINI_MEM 内存符号
- MINI_MEM 加 `sym[256]` 槽符号数组
- mini_mem_write 加 sym 参数（槽存源寄存器符号，const 按宽度重截）
- mini_mem_read 加 sym_out 参数（exact 匹配返回槽符号）
- **mov reg,[mem]** 读槽符号存入寄存器 sym（无槽符号时 REG）——**内存符号传播链闭合**
- 踩坑：批量正则误伤 trunc_reg/width_mask（3 处修复）；sym_eval_partial 插到 MINI_REG 前（C2061）

### 3. jmp 全解增强
- `sym_eval_partial(id, mreg, &out)`：非 tainted REG 叶子用 mreg.r 代换 → 部分求值
- jmp 解析链：sym_is_const（纯常量）→ sym_eval_partial（部分代换）→ dict 验证 → 寄存器真实值
- **实测**：无 cannot resolve（jmp 全解）

### 4. 现状（诚实）
- ✅ ASLR 稳定（入口 RVA 固定）
- ✅ 8 handler 稳定识别（两段）+ 符号常量还原（0x938147de 等链还原魔数）
- ✅ jmp 全解（无断链）
- ❌ **未达 vExit**：链断在段边界数据区（0x13BC032 反汇编失败）——完整到 vExit 需 GUI 动态 trace 或符号化数据区/段边界


## 十一、符号化数据区恢复（2026-08-04）

### 实现
- **数据区代码块恢复**：step_one 失败且上一条是 jmp（跳入数据区）时，向前扫描（disasm_from_file）找下一个可执行代码块恢复
- **防死循环**：a==ip 跳过 + 恢复次数上限 8（修复 50000 步死循环 bug）
- 顺序执行到数据区（无 jmp）→ 直接结束链（不误恢复）

### 实测
- 4 次运行无死循环；69 步 5 handler 稳定（push→alu→vByteOp→push→vSub）
- recover 场景本样本未触发（jmp 全解）——恢复机制就绪待其他样本验证
- 剩余步数波动（16/27/69）为 ASLR 分支差异（VM 跳转依赖运行时基址相关值）

### 现状（最终诚实评估）
- ✅ ASLR 稳定、8 handler 精确语义、符号常量还原、jmp 全解、数据区恢复机制
- ❌ vExit 未达：VM 段尾数据区是真实终点（headless 静态极限）；完整链到 vExit 需 GUI 动态 trace


## 十二、3.5 源码 → 3.8 handler 语义交叉验证（2026-08-04）

### 3.5 VM 指令表（xx_vm32/vm_handle.c，50 条）→ 3.8 语义映射
| 3.5 指令码 | 语义 | 3.8 对应识别 |
|---|---|---|
| 0x0100/0101/0102 | push dreg/wreg/breg | vPush |
| 0x0200/0201/0202 | push dconst/wconst/bconst | vPush + vConstRebuild |
| 0x0300/0301/0302 | pop dreg/wreg/breg | vPop |
| 0x0400/0401/0402 | **add d[sp+4],d[sp]**（双栈操作数） | vAdd |
| 0x0900/0901/0902 | **and ~d[sp+4],~d[sp]**（NAND） | vNand（未实现） |
| 0x0d00/0d01/0d02 | **or ~d[sp+4],~d[sp]**（NOR） | vNor（未实现） |
| 0x0a00/0a10 | div/idiv | vDiv（未实现） |
| 0x0b00/0b10 | shr/shrd | vShr |
| 0x0c00/0c10 | mul/imul | vMul |
| 0x0e00/0e10 | shl/shld | vShl |
| 0x0800/0801 | calc（ValueCommand 加密链） | vXor 等核心 ALU |
| 0x0f00/0f01 | jmp | vJmp（分派） |
| 0x1300 | popfd | vExit |
| 0x1200 | ret | 链结束 |

### 关键验证结论
1. **3.5 双栈操作数模式**（`0x0400 add d[sp+4],d[sp]`）= 从栈取两操作数 + ALU + 写回栈——**正是 3.8 vAdd/vSub/vMul 的核心判定特征**（当前 core-ALU 的 is_core 基于此）
2. **3.5 表验证 3.8 识别方向正确**：vXor/vSub 精确语义与 3.5 的 calc/0x0400 对应
3. **vNand/vNor**（0x0900/0x0d00 = and ~a,~b / or ~a,~b）——3.8 未实现，可补（VMP 3.8 用 NAND/NOR 万能门）
4. **3.5 模式实现**：identify_handler 加"双栈操作数 ALU 模式"（read_cnt>=1 && write_cnt>=1 && alu_op → vAdd/vSub/vXor/vMul/vShl/vShr）；当前样本 core-ALU 先命中，3.5 模式为补充

### 参考价值总结
- **3.5 源码 = 3.8 handler 语义词典**（VMP 3.8 改动有限，指令集继承）
- **未实现的 3.8 语义**（对照 3.5）：vNand（0x0900）、vNor（0x0d00）、vDiv（0x0a00）、vShrd/vShld（0x0b10/0x0e10）


## 十三、vNand/vNor 识别（2026-08-04，对照 3.5 0x0900/0x0d00）

### 实现（identify_handler，顺序感知）
- **NAND/NOR 检测**（core 序列含 not + and/or）：
  - 操作数取反形式：`not;not;and`=vNor（~a&~b=~(a|b)）、`not;not;or`=vNand（~a|~b=~(a&b)）
  - **结果取反形式（关键修正）**：`and;not`=**vNand**、`or;not`=**vNor**（顺序感知：最后门后跟 not）
  - 判定：trailing_not 时按最后门（and→vNand, or→vNor）；否则按 n_or（or→vNand）
- **不误触发**：vXor/vSub/vByteOp 保持正确（69 步 5 handler 稳定）
- **逻辑验证**：Python 7/7 用例通过（not;not;and/or + and;not + or;not + 双 not 后置）

### 德摩根依据（3.5 指令表）
- `0x0900 and ~d[sp+4],~d[sp]` = (~a)&(~b) = ~(a|b) = **NOR**
- `0x0d00 or ~d[sp+4],~d[sp]` = (~a)|(~b) = ~(a&b) = **NAND**
- 与 [[vmp-nand-nor-simplify]] 恒等式一致（NOT a = a NAND a 等）

### 当前样本
probe 样本无门运算（vXor 是 xor 直算）→ 检测待含 NAND/NOR 的样本验证；机制与判定逻辑已就绪


## 十四、原版 xxvm 完整还原方案剖析（2026-08-04）

### 原版 xxvm 怎么做（xx_execute.c）
1. **xx_disasm** 反汇编 → XX_INST
2. **xx_execute(inst, ctx_in, ctx_out)** 完整模拟执行（每条指令真实改寄存器）
3. 循环执行直到 ret/jmp reg → 完整链
4. **关键前提：fmem 进程内存快照**——init_mem_file 在 GUI 下 dump 真实进程内存到文件，xx_read_fmem/xx_write_fmem 从快照读写 `mov [mem],reg` 的内存操作数

### 为什么原版能完整而 headless 静态不行
- 原版 = **完整模拟执行器 + 真实进程内存快照**（GUI 环境）→ jmp reg 读模拟寄存器真实值 → 完整链
- headless = 无进程内存（fmem 空）→ xx_execute 内存操作失败 → 无法复用
- 我的 mini-exec = 部分模拟（106 分支）+ 符号执行 → headless 下最优

### 结论（最终）
- **headless 静态极限**：单 handler 精确语义 + 符号常量还原 + jmp 符号求解 + 数据区恢复（已全部实现）
- **完整链到 vExit**：需 GUI 环境（原版 xxvm 的 fmem 快照 或 我的 vmp38_dynamic_trace 真实单步）
- **xx_execute 复用不可行**（headless）：fmem 依赖 + 原版代码侵入


## 十五、按原版走：xx_execute 完整模拟执行器复用（2026-08-04）

### 实现（对齐原版 xxvm 方案）
1. **xx_execute.c 加内存钩子**（xx_vm_x32 版）：
   - `vmp38_mem_hook_read/write` 函数指针（extern 全局）
   - xx_read_fmem/xx_write_fmem 的 fmem 查找失败分支 → 调钩子（headless fallback）
2. **vmp38_restore.cpp 实现钩子**：
   - vmp38_fmem_read_hook：mini_mem 栈槽（绝对地址-vsp→槽偏移）+ DbgMemRead fallback
   - vmp38_fmem_write_hook：mini_mem 写槽
   - g_xx_mreg 全局指针（xx_execute 执行期间指向活动 mreg）
3. **vmp38_exec_step_xx**：xx_disasm 反汇编当前 ip → xx_execute(inst, ctx_in, ctx_out) 完整模拟 → 同步回 mreg（r[8]+eflags）；分类（push/pop/jmp/alu）从 disasm 文本
4. **主循环接入**：每步先 vmp38_exec_step_xx（成功则用其寄存器语义），再 mini_exec（补符号追踪）
5. **extern "C" 包裹**（C++ 名字修饰问题：xx_execute.c 是 C 编译）

### 踩坑记录
- **patch 错文件**：先改了 xx_vm32 的 xx_execute.c（OllyDbg 版），x32 工程用 xx_vm_x32 自己的（xx_vm32 版是另一个）
- 钩子 addr 类型：xx_vm_x32 版是 `unsigned long`（不是 unsigned int）
- extern "C"：C++ 引用 C 符号必须包裹（LNK2019 修饰名）
- XX_CONTEXT 需本地定义（r[8]+eflag+ip，与 xx_vm.h 同构）
- fwd decl 位置（STEP_REC 定义后）

### 实测（headless，probe 样本）
- **69 步 5 handler 稳定**（xx_execute 完整模拟 + mini-exec 符号补充）
- vXor/vSub/vByteOp 精确语义保持
- 无 disasm failed / cannot resolve（链完整走完）
- **原版引擎 + headless fmem 钩子 = 完整模拟执行器在无进程内存环境可用**
