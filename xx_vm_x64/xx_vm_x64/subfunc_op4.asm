include xx_vm.inc





;ml64 /c %(filename).asm
;%(filename).obj;%(Outputs)

.code


asm_bt proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rsi,rcx

	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var1,rax

	mov rbx,(ITEM_VAR64 ptr [rsi]).v_size

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	mov rdi,(ITEM_VAR64 ptr [rsi]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	

@l_size22:
	cmp rbx,2
	jne @l_size44
	cmp rdi,2
	jne @l_size44

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bt ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size44:
	cmp rbx,4
	jne @l_size88
	cmp rdi,4
	jne @l_size88
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bt eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size88:
	cmp rbx,8
	jne @l_size21
	cmp rdi,8
	jne @l_size21
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bt rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size21:
	cmp rbx,2
	jne @l_size41
	cmp rdi,1
	jne @l_size41
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bt ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size41:
	cmp rbx,4
	jne @l_size81
	cmp rdi,1
	jne @l_size81
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bt eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size81:
	cmp rbx,8
	jne @l_else
	cmp rdi,1
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bt rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_else:
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,0
	ret
@l_end:

	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_bt endp
;=================================================================================



asm_btc proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rsi,rcx

	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var1,rax

	mov rbx,(ITEM_VAR64 ptr [rsi]).v_size

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	mov rdi,(ITEM_VAR64 ptr [rsi]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	

@l_size22:
	cmp rbx,2
	jne @l_size44
	cmp rdi,2
	jne @l_size44

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btc ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size44:
	cmp rbx,4
	jne @l_size88
	cmp rdi,4
	jne @l_size88
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btc eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size88:
	cmp rbx,8
	jne @l_size21
	cmp rdi,8
	jne @l_size21
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btc rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size21:
	cmp rbx,2
	jne @l_size41
	cmp rdi,1
	jne @l_size41
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btc ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size41:
	cmp rbx,4
	jne @l_size81
	cmp rdi,1
	jne @l_size81
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btc eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size81:
	cmp rbx,8
	jne @l_else
	cmp rdi,1
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btc rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_else:
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,0
	ret
@l_end:

	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_btc endp
;=================================================================================


asm_btr proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rsi,rcx

	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var1,rax

	mov rbx,(ITEM_VAR64 ptr [rsi]).v_size

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	mov rdi,(ITEM_VAR64 ptr [rsi]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	

@l_size22:
	cmp rbx,2
	jne @l_size44
	cmp rdi,2
	jne @l_size44

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btr ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size44:
	cmp rbx,4
	jne @l_size88
	cmp rdi,4
	jne @l_size88
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btr eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size88:
	cmp rbx,8
	jne @l_size21
	cmp rdi,8
	jne @l_size21
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btr rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size21:
	cmp rbx,2
	jne @l_size41
	cmp rdi,1
	jne @l_size41
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btr ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size41:
	cmp rbx,4
	jne @l_size81
	cmp rdi,1
	jne @l_size81
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btr eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size81:
	cmp rbx,8
	jne @l_else
	cmp rdi,1
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	btr rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_else:
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,0
	ret
@l_end:

	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_btr endp
;=================================================================================


asm_bts proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rsi,rcx

	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var1,rax

	mov rbx,(ITEM_VAR64 ptr [rsi]).v_size

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	mov rdi,(ITEM_VAR64 ptr [rsi]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	

@l_size22:
	cmp rbx,2
	jne @l_size44
	cmp rdi,2
	jne @l_size44

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bts ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size44:
	cmp rbx,4
	jne @l_size88
	cmp rdi,4
	jne @l_size88
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bts eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size88:
	cmp rbx,8
	jne @l_size21
	cmp rdi,8
	jne @l_size21
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bts rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size21:
	cmp rbx,2
	jne @l_size41
	cmp rdi,1
	jne @l_size41
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bts ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size41:
	cmp rbx,4
	jne @l_size81
	cmp rdi,1
	jne @l_size81
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bts eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size81:
	cmp rbx,8
	jne @l_else
	cmp rdi,1
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bts rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_else:
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,0
	ret
@l_end:

	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_bts endp
;=================================================================================



asm_cli proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	push @flag
	popfq
	cli
	pushfq
	pop @flag


	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_cli endp
;=============================================================================


asm_cmc proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax


	push @flag
	popfq
	cmc
	pushfq
	pop @flag


	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_cmc endp
;=============================================================================



asm_sti proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax


	push @flag
	popfq
	sti
	pushfq
	pop @flag


	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_sti endp
;=============================================================================




end