include xx_vm.inc



.code


asm_sal proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	sal al,cl
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
	sal ax,cl
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
	sal eax,cl
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
	sal rax,cl
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

asm_sal endp
;=================================================================================



asm_sar proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	sar al,cl
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
	sar ax,cl
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
	sar eax,cl
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
	sar rax,cl
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

asm_sar endp
;=================================================================================


asm_shl proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	shl al,cl
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
	shl ax,cl
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
	shl eax,cl
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
	shl rax,cl
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

asm_shl endp
;=================================================================================


asm_shr proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	shr al,cl
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
	shr ax,cl
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
	shr eax,cl
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
	shr rax,cl
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

asm_shr endp
;=================================================================================



asm_rcl proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	rcl al,cl
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
	rcl ax,cl
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
	rcl eax,cl
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
	rcl rax,cl
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

asm_rcl endp
;=================================================================================


asm_rcr proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	rcr al,cl
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
	rcr ax,cl
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
	rcr eax,cl
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
	rcr rax,cl
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

asm_rcr endp
;=================================================================================


asm_rol proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	rol al,cl
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
	rol ax,cl
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
	rol eax,cl
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
	rol rax,cl
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

asm_rol endp
;=================================================================================



asm_ror proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rax,(ITEM_VAR64 ptr [rcx]).v_var
	mov @var1,rax

	mov rsi,rcx
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
	ror al,cl
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
	ror ax,cl
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
	ror eax,cl
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
	ror rax,cl
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

asm_ror endp
;=================================================================================



asm_shld proc 
	local @var1:DQ
	local @var2:DQ
	local @var3:DQ
	local @flag:DQ

	push rbx
	push rdx
	push rsi
	push rdi

	;local
	mov rsi,rcx
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var1,rax

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var3,rax
	
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

	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	shld dx,ax,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	shld edx,eax,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	shld rdx,rax,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_else:
	
	pop rdi
	pop rsi
	pop rdx
	pop rbx
	mov rax,0
	ret
@l_end:


	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rdx

	mov [rsi+8],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rdx
	pop rbx
	mov rax,1
	ret

asm_shld endp
;=================================================================================



asm_shrd proc 
	local @var1:DQ
	local @var2:DQ
	local @var3:DQ
	local @flag:DQ

	push rbx
	push rdx
	push rsi
	push rdi

	;local
	mov rsi,rcx
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var1,rax

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var3,rax
	
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

	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	shrd dx,ax,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size4:
	cmp rbx,4h
	jne @l_size8
	;size==4

	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	shrd edx,eax,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size8:
	cmp rbx,8h
	jne @l_else
	;size==8

	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	shrd rdx,rax,cl
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_else:
	
	pop rdi
	pop rsi
	pop rdx
	pop rbx
	mov rax,0
	ret
@l_end:


	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rdx

	mov [rsi+8],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rdx
	pop rbx
	mov rax,1
	ret

asm_shrd endp
;=================================================================================



end