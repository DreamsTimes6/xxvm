include xx_vm.inc





;ml64 /c %(filename).asm
;%(filename).obj;%(Outputs)

.code




asm_xadd proc 
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
	xadd al,cl
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
	xadd ax,cx
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
	xadd eax,ecx
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
	xadd rax,rcx
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

	mov [rsi+8],rcx

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_xadd endp
;=================================================================================


asm_lahf proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	push [rsi]
	pop @var1

	mov rax,@var1
	push @flag
	popfq
	lahf
	pushfq
	pop @flag

	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_lahf endp
;=============================================================================




asm_sahf proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	push [rsi]
	pop @var1

	mov rax,@var1
	push @flag
	popfq
	sahf
	pushfq
	pop @flag

	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_sahf endp
;=============================================================================



asm_adc proc 
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
	adc al,cl
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
	adc ax,cx
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
	adc eax,ecx
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
	adc rax,rcx
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

	mov [rsi+8],rcx

	mov rsi,@flag
	mov (OUT_VAR64 ptr [r8]).o_flag,rsi
	
	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_adc endp
;=================================================================================




asm_rdtsc proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	

	rdtsc


	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rdx

	mov [rsi+8],rax

	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_rdtsc endp
;=============================================================================



asm_cpuid proc 
	local @var1:DQ
	local @var2:DQ
	local @var3:DQ
	local @var4:DQ
	local @flag:DQ

	push rcx
	push rdx
	push rbx
	push rsi
	push rdi

	;local
	
	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	;mov @var1,[rsi]      ;RAX
	push [rsi]
	pop @var1
	;mov @var2,[rsi+8]    ;RCX
	push [rsi+8]
	pop @var2
	;mov @var3,[rsi+10h]  ;RDX
	push [rsi+10h]
	pop @var3
	;mov @var4,[rsi+18h]  ;RBX
	push [rsi+18h]
	pop @var4

	mov rax,@var1
	mov rcx,@var2
	mov rdx,@var3
	mov rbx,@var4
	cpuid


	lea rsi, (OUT_VAR64 ptr [r8]).o_var
	mov [rsi],rax
	mov [rsi+8],rcx
	mov [rsi+10h],rdx
	mov [rsi+18h],rbx

	pop rdi
	pop rsi
	pop rbx
	pop rdx
	pop rcx
	mov rax,1
	ret

asm_cpuid endp
;=============================================================================




asm_mul proc 
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
	mov @var3,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	;mov @var1,[rsi]      ;RAX
	push [rsi]
	pop @var2

	;mov @var3,[rsi+10h]  ;RDX
	push [rsi+10h]
	pop @var1
	

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	
	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	mul cl
	pushfq
	pop @flag

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
	mul cx
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
	mul ecx
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
	mul rcx
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

asm_mul endp
;=================================================================================



asm_div proc 
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
	mov @var3,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	;mov @var1,[rsi]      ;RAX
	push [rsi]
	pop @var2

	;mov @var3,[rsi+10h]  ;RDX
	push [rsi+10h]
	pop @var1
	

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	
	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	div cl
	pushfq
	pop @flag

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
	div cx
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
	div ecx
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
	div rcx
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

asm_div endp
;=================================================================================



asm_imul1 proc 
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
	mov @var3,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	;mov @var1,[rsi]      ;RAX
	push [rsi]
	pop @var2

	;mov @var3,[rsi+10h]  ;RDX
	push [rsi+10h]
	pop @var1
	

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	
	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul cl
	pushfq
	pop @flag

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
	imul cx
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
	imul ecx
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
	imul rcx
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

asm_imul1 endp
;=================================================================================


asm_imul2 proc 
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
	imul ax,cx
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
	imul eax,ecx
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
	imul rax,rcx
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

asm_imul2 endp
;=================================================================================



asm_imul3 proc 
	local @var1:DQ
	local @var2:DQ
	local @var3:DQ
	local @size1:DQ
	local @size2:DQ
	local @size3:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rsi,rcx

	mov rax,(ITEM_VAR64 ptr [rsi]).v_size
	mov @size1,rax

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var2,rax

	mov rax,(ITEM_VAR64 ptr [rsi]).v_size
	mov @size1,rax

	add rsi,type ITEM_VAR64
	mov rax,(ITEM_VAR64 ptr [rsi]).v_var
	mov @var3,rax

	mov rax,(ITEM_VAR64 ptr [rsi]).v_size
	mov @size1,rax
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax


@l_size221:
	cmp @size1,2
	jne @l_size441
	cmp @size2,2
	jne @l_size441
	cmp @size3,1
	jne @l_size441

	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size441:
	cmp @size1,4
	jne @l_size881
	cmp @size2,4
	jne @l_size881
	cmp @size3,1
	jne @l_size881

	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size881:
	cmp @size1,8
	jne @l_size222
	cmp @size2,8
	jne @l_size222
	cmp @size3,1
	jne @l_size222

	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul rax,rcx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size222:
	cmp @size1,2
	jne @l_size444
	cmp @size2,2
	jne @l_size444
	cmp @size3,2
	jne @l_size444

	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul ax,cx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size444:
	cmp @size1,4
	jne @l_size888
	cmp @size2,4
	jne @l_size888
	cmp @size3,4
	jne @l_size888

	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul eax,ecx
	pushfq
	pop @flag

	jmp @l_end   ;ennd
@l_size888:
	cmp @size1,8
	jne @l_else
	cmp @size2,8
	jne @l_else
	cmp @size3,8
	jne @l_else

	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	imul rax,rcx
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

asm_imul3 endp
;=================================================================================



asm_idiv proc 
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
	mov @var3,rax

	mov rbx,(ITEM_VAR64 ptr [rcx]).v_size
	
	mov rax,(XX_CONTEXT64 ptr [rdx]).c_flag
	mov @flag,rax

	lea rsi,(XX_CONTEXT64 ptr [rdx]).c_r
	;mov @var1,[rsi]      ;RAX
	push [rsi]
	pop @var2

	;mov @var3,[rsi+10h]  ;RDX
	push [rsi+10h]
	pop @var1
	

@l_size1:
	cmp rbx,1h
	jne @l_size2

	;size==1
	
	mov rdx,@var1
	mov rax,@var2
	mov rcx,@var3
	push @flag
	popfq
	idiv cl
	pushfq
	pop @flag

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
	idiv cx
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
	idiv ecx
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
	idiv rcx
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

asm_idiv endp
;=================================================================================



end












