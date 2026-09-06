include xx_vm.inc



;ml64 /c %(filename).asm
;%(filename).obj;%(Outputs)

.code


asm_dec proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	push @flag
	popfq
	dec al
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	push @flag
	popfq
	dec ax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	push @flag
	popfq
	dec eax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	push @flag
	popfq
	dec rax
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

asm_dec endp
;=============================================================================


asm_inc proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	mov rax,@var1
	push @flag
	popfq
	inc al
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size2:
	cmp rbx,2h
	jne @l_size4
	;size==2

	mov rax,@var1
	push @flag
	popfq
	inc ax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rax,@var1
	push @flag
	popfq
	inc eax
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rax,@var1
	push @flag
	popfq
	inc rax
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

asm_inc endp
;==================================================================================


asm_sbb proc 
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
	sbb al,cl
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
	sbb ax,cx
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
	sbb eax,ecx
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
	sbb rax,rcx
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

asm_sbb endp
;=================================================================================



asm_add proc 
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
	add al,cl
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
	add ax,cx
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
	add eax,ecx
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
	add rax,rcx
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

asm_add endp
;=================================================================================



asm_sub proc 
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
	sub al,cl
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
	sub ax,cx
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
	sub eax,ecx
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
	sub rax,rcx
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

asm_sub endp
;=================================================================================





end

