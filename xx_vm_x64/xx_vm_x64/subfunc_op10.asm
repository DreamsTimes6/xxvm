include xx_vm.inc





;ml64 /c %(filename).asm
;%(filename).obj;%(Outputs)

.code




asm_movsb proc 
	local @var1:DQ
	local @var2:DQ
	local @flag:DQ

	push rbx
	push rsi
	push rdi

	;local
	mov rcx,rcx
	mov rsi,rdx
	mov rdi,r8
	mov rax,r9
	call rax
	

	pop rdi
	pop rsi
	pop rbx
	mov rax,1
	ret

asm_movsb endp
;=================================================================================



end












