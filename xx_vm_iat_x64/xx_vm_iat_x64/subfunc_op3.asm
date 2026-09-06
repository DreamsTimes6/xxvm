include xx_vm.inc





;ml64 /c %(filename).asm
;%(filename).obj;%(Outputs)

.code



asm_xor proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	xor al,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	xor ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	xor eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	xor rax,rcx
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

asm_xor endp
;=================================================================================




asm_and proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	and al,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	and ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	and eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	and rax,rcx
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

asm_and endp
;=================================================================================


asm_or proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	or al,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	or ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	or eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	or rax,rcx
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

asm_or endp
;=================================================================================



asm_test proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	test al,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	test ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	test eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	test rax,rcx
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

asm_test endp
;=================================================================================



asm_not proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	not al
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	not ax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	not eax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	not rax
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

asm_not endp
;=================================================================================


asm_neg proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	neg al
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	neg ax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	neg eax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	neg rax
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

asm_neg endp
;=================================================================================


asm_cmp proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	cmp al,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	cmp ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	cmp eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	cmp rax,rcx
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

asm_cmp endp
;=================================================================================


asm_bsr proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	jmp @l_else
	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bsr ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bsr eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bsr rax,rcx
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

asm_bsr endp
;=================================================================================


asm_bsf proc 
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

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	jmp @l_else
	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bsf ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bsf eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	mov rcx,@var2
	push @flag
	popfq
	bsf rax,rcx
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

asm_bsf endp
;=================================================================================


end