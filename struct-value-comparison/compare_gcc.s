	.file	"compare.c"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	point_eq_fields
	.type	point_eq_fields, @function
point_eq_fields:
.LFB14:
	.cfi_startproc
	endbr64
	xor	eax, eax
	cmp	edi, esi
	je	.L5
	ret
	.p2align 4,,10
	.p2align 3
.L5:
	sar	rdi, 32
	sar	rsi, 32
	cmp	edi, esi
	sete	al
	ret
	.cfi_endproc
.LFE14:
	.size	point_eq_fields, .-point_eq_fields
	.p2align 4
	.globl	large_eq_fields
	.type	large_eq_fields, @function
large_eq_fields:
.LFB15:
	.cfi_startproc
	endbr64
	mov	rdx, QWORD PTR 56[rsp]
	xor	eax, eax
	cmp	QWORD PTR 8[rsp], rdx
	je	.L13
.L6:
	ret
	.p2align 4,,10
	.p2align 3
.L13:
	mov	rcx, QWORD PTR 64[rsp]
	cmp	QWORD PTR 16[rsp], rcx
	jne	.L6
	mov	rsi, QWORD PTR 72[rsp]
	cmp	QWORD PTR 24[rsp], rsi
	jne	.L6
	mov	rdi, QWORD PTR 80[rsp]
	cmp	QWORD PTR 32[rsp], rdi
	jne	.L6
	mov	rdi, QWORD PTR 88[rsp]
	cmp	QWORD PTR 40[rsp], rdi
	jne	.L6
	mov	rax, QWORD PTR 96[rsp]
	cmp	QWORD PTR 48[rsp], rax
	sete	al
	ret
	.cfi_endproc
.LFE15:
	.size	large_eq_fields, .-large_eq_fields
	.p2align 4
	.globl	padding_eq_fields
	.type	padding_eq_fields, @function
padding_eq_fields:
.LFB16:
	.cfi_startproc
	endbr64
	xor	eax, eax
	cmp	dil, sil
	je	.L17
	ret
	.p2align 4,,10
	.p2align 3
.L17:
	sar	rdi, 32
	sar	rsi, 32
	cmp	edi, esi
	sete	al
	ret
	.cfi_endproc
.LFE16:
	.size	padding_eq_fields, .-padding_eq_fields
	.p2align 4
	.globl	point_eq_memcmp
	.type	point_eq_memcmp, @function
point_eq_memcmp:
.LFB17:
	.cfi_startproc
	endbr64
	cmp	rdi, rsi
	sete	al
	ret
	.cfi_endproc
.LFE17:
	.size	point_eq_memcmp, .-point_eq_memcmp
	.p2align 4
	.globl	large_eq_memcmp
	.type	large_eq_memcmp, @function
large_eq_memcmp:
.LFB18:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR 8[rsp]
	mov	rdx, QWORD PTR 16[rsp]
	xor	rax, QWORD PTR 56[rsp]
	xor	rdx, QWORD PTR 64[rsp]
	or	rax, rdx
	je	.L22
.L20:
	mov	eax, 1
.L21:
	test	eax, eax
	sete	al
	ret
	.p2align 4,,10
	.p2align 3
.L22:
	mov	rax, QWORD PTR 24[rsp]
	mov	rdx, QWORD PTR 32[rsp]
	xor	rax, QWORD PTR 72[rsp]
	xor	rdx, QWORD PTR 80[rsp]
	or	rax, rdx
	jne	.L20
	mov	rax, QWORD PTR 40[rsp]
	mov	rdx, QWORD PTR 48[rsp]
	xor	rax, QWORD PTR 88[rsp]
	xor	rdx, QWORD PTR 96[rsp]
	or	rax, rdx
	jne	.L20
	xor	eax, eax
	jmp	.L21
	.cfi_endproc
.LFE18:
	.size	large_eq_memcmp, .-large_eq_memcmp
	.p2align 4
	.globl	padding_eq_memcmp
	.type	padding_eq_memcmp, @function
padding_eq_memcmp:
.LFB19:
	.cfi_startproc
	endbr64
	cmp	rdi, rsi
	sete	al
	ret
	.cfi_endproc
.LFE19:
	.size	padding_eq_memcmp, .-padding_eq_memcmp
	.p2align 4
	.globl	point_eq_memcmp_ptr
	.type	point_eq_memcmp_ptr, @function
point_eq_memcmp_ptr:
.LFB20:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR [rsi]
	cmp	QWORD PTR [rdi], rax
	sete	al
	ret
	.cfi_endproc
.LFE20:
	.size	point_eq_memcmp_ptr, .-point_eq_memcmp_ptr
	.p2align 4
	.globl	large_eq_memcmp_ptr
	.type	large_eq_memcmp_ptr, @function
large_eq_memcmp_ptr:
.LFB21:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR [rdi]
	mov	rdx, QWORD PTR 8[rdi]
	xor	rax, QWORD PTR [rsi]
	xor	rdx, QWORD PTR 8[rsi]
	or	rax, rdx
	je	.L28
.L26:
	mov	eax, 1
.L27:
	test	eax, eax
	sete	al
	ret
	.p2align 4,,10
	.p2align 3
.L28:
	mov	rax, QWORD PTR 16[rdi]
	mov	rdx, QWORD PTR 24[rdi]
	xor	rax, QWORD PTR 16[rsi]
	xor	rdx, QWORD PTR 24[rsi]
	or	rax, rdx
	jne	.L26
	mov	rax, QWORD PTR 32[rdi]
	mov	rdx, QWORD PTR 40[rdi]
	xor	rax, QWORD PTR 32[rsi]
	xor	rdx, QWORD PTR 40[rsi]
	or	rax, rdx
	jne	.L26
	xor	eax, eax
	jmp	.L27
	.cfi_endproc
.LFE21:
	.size	large_eq_memcmp_ptr, .-large_eq_memcmp_ptr
	.p2align 4
	.globl	padding_eq_memcmp_ptr
	.type	padding_eq_memcmp_ptr, @function
padding_eq_memcmp_ptr:
.LFB43:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR [rdi]
	cmp	QWORD PTR [rsi], rax
	sete	al
	ret
	.cfi_endproc
.LFE43:
	.size	padding_eq_memcmp_ptr, .-padding_eq_memcmp_ptr
	.p2align 4
	.globl	point_eq_intcast
	.type	point_eq_intcast, @function
point_eq_intcast:
.LFB23:
	.cfi_startproc
	endbr64
	cmp	rdi, rsi
	sete	al
	ret
	.cfi_endproc
.LFE23:
	.size	point_eq_intcast, .-point_eq_intcast
	.p2align 4
	.globl	large_eq_xor
	.type	large_eq_xor, @function
large_eq_xor:
.LFB24:
	.cfi_startproc
	endbr64
	mov	rax, rsi
	mov	rcx, QWORD PTR 8[rdi]
	mov	rsi, QWORD PTR [rdi]
	xor	rcx, QWORD PTR 8[rax]
	xor	rsi, QWORD PTR [rax]
	or	rsi, rcx
	mov	rcx, QWORD PTR 16[rdi]
	xor	rcx, QWORD PTR 16[rax]
	or	rcx, rsi
	mov	rsi, QWORD PTR 24[rdi]
	xor	rsi, QWORD PTR 24[rax]
	or	rsi, rcx
	mov	rdx, QWORD PTR 40[rdi]
	mov	rcx, QWORD PTR 32[rdi]
	xor	rdx, QWORD PTR 40[rax]
	xor	rcx, QWORD PTR 32[rax]
	or	rcx, rsi
	mov	rax, rdx
	or	rax, rcx
	sete	al
	ret
	.cfi_endproc
.LFE24:
	.size	large_eq_xor, .-large_eq_xor
	.p2align 4
	.globl	point_eq_builtin
	.type	point_eq_builtin, @function
point_eq_builtin:
.LFB25:
	.cfi_startproc
	endbr64
	cmp	rdi, rsi
	sete	al
	ret
	.cfi_endproc
.LFE25:
	.size	point_eq_builtin, .-point_eq_builtin
	.p2align 4
	.globl	large_eq_builtin
	.type	large_eq_builtin, @function
large_eq_builtin:
.LFB26:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR 8[rsp]
	mov	rdx, QWORD PTR 16[rsp]
	xor	rax, QWORD PTR 56[rsp]
	xor	rdx, QWORD PTR 64[rsp]
	or	rax, rdx
	je	.L36
.L34:
	mov	eax, 1
.L35:
	test	eax, eax
	sete	al
	ret
	.p2align 4,,10
	.p2align 3
.L36:
	mov	rax, QWORD PTR 24[rsp]
	mov	rdx, QWORD PTR 32[rsp]
	xor	rax, QWORD PTR 72[rsp]
	xor	rdx, QWORD PTR 80[rsp]
	or	rax, rdx
	jne	.L34
	mov	rax, QWORD PTR 40[rsp]
	mov	rdx, QWORD PTR 48[rsp]
	xor	rax, QWORD PTR 88[rsp]
	xor	rdx, QWORD PTR 96[rsp]
	or	rax, rdx
	jne	.L34
	xor	eax, eax
	jmp	.L35
	.cfi_endproc
.LFE26:
	.size	large_eq_builtin, .-large_eq_builtin
	.p2align 4
	.globl	padding_eq_builtin
	.type	padding_eq_builtin, @function
padding_eq_builtin:
.LFB27:
	.cfi_startproc
	endbr64
	cmp	rdi, rsi
	sete	al
	ret
	.cfi_endproc
.LFE27:
	.size	padding_eq_builtin, .-padding_eq_builtin
	.p2align 4
	.globl	point_eq_fields_ptr
	.type	point_eq_fields_ptr, @function
point_eq_fields_ptr:
.LFB28:
	.cfi_startproc
	endbr64
	mov	edx, DWORD PTR [rsi]
	xor	eax, eax
	cmp	DWORD PTR [rdi], edx
	je	.L41
	ret
	.p2align 4,,10
	.p2align 3
.L41:
	mov	eax, DWORD PTR 4[rsi]
	cmp	DWORD PTR 4[rdi], eax
	sete	al
	ret
	.cfi_endproc
.LFE28:
	.size	point_eq_fields_ptr, .-point_eq_fields_ptr
	.p2align 4
	.globl	large_eq_fields_ptr
	.type	large_eq_fields_ptr, @function
large_eq_fields_ptr:
.LFB29:
	.cfi_startproc
	endbr64
	mov	rdx, QWORD PTR [rsi]
	xor	eax, eax
	cmp	QWORD PTR [rdi], rdx
	je	.L49
.L42:
	ret
	.p2align 4,,10
	.p2align 3
.L49:
	mov	rcx, QWORD PTR 8[rsi]
	cmp	QWORD PTR 8[rdi], rcx
	jne	.L42
	mov	rcx, QWORD PTR 16[rsi]
	cmp	QWORD PTR 16[rdi], rcx
	jne	.L42
	mov	rcx, QWORD PTR 24[rsi]
	cmp	QWORD PTR 24[rdi], rcx
	jne	.L42
	mov	rcx, QWORD PTR 32[rsi]
	cmp	QWORD PTR 32[rdi], rcx
	jne	.L42
	mov	rax, QWORD PTR 40[rsi]
	cmp	QWORD PTR 40[rdi], rax
	sete	al
	ret
	.cfi_endproc
.LFE29:
	.size	large_eq_fields_ptr, .-large_eq_fields_ptr
	.p2align 4
	.globl	padding_eq_fields_ptr
	.type	padding_eq_fields_ptr, @function
padding_eq_fields_ptr:
.LFB30:
	.cfi_startproc
	endbr64
	movzx	edx, BYTE PTR [rsi]
	xor	eax, eax
	cmp	BYTE PTR [rdi], dl
	je	.L53
	ret
	.p2align 4,,10
	.p2align 3
.L53:
	mov	eax, DWORD PTR 4[rsi]
	cmp	DWORD PTR 4[rdi], eax
	sete	al
	ret
	.cfi_endproc
.LFE30:
	.size	padding_eq_fields_ptr, .-padding_eq_fields_ptr
	.p2align 4
	.globl	twolong_eq_fields
	.type	twolong_eq_fields, @function
twolong_eq_fields:
.LFB31:
	.cfi_startproc
	endbr64
	xor	eax, eax
	cmp	rdi, rdx
	je	.L57
	ret
	.p2align 4,,10
	.p2align 3
.L57:
	cmp	rsi, rcx
	sete	al
	ret
	.cfi_endproc
.LFE31:
	.size	twolong_eq_fields, .-twolong_eq_fields
	.p2align 4
	.globl	twolong_eq_memcmp
	.type	twolong_eq_memcmp, @function
twolong_eq_memcmp:
.LFB32:
	.cfi_startproc
	endbr64
	xor	rdx, rdi
	xor	rcx, rsi
	mov	QWORD PTR -24[rsp], rdi
	or	rdx, rcx
	mov	QWORD PTR -16[rsp], rsi
	sete	al
	ret
	.cfi_endproc
.LFE32:
	.size	twolong_eq_memcmp, .-twolong_eq_memcmp
	.p2align 4
	.globl	twolong_eq_memcmp_ptr
	.type	twolong_eq_memcmp_ptr, @function
twolong_eq_memcmp_ptr:
.LFB33:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR [rdi]
	mov	rdx, QWORD PTR 8[rdi]
	xor	rax, QWORD PTR [rsi]
	xor	rdx, QWORD PTR 8[rsi]
	or	rax, rdx
	sete	al
	ret
	.cfi_endproc
.LFE33:
	.size	twolong_eq_memcmp_ptr, .-twolong_eq_memcmp_ptr
	.p2align 4
	.globl	twolong_eq_builtin
	.type	twolong_eq_builtin, @function
twolong_eq_builtin:
.LFB34:
	.cfi_startproc
	endbr64
	xor	rdx, rdi
	xor	rcx, rsi
	mov	QWORD PTR -24[rsp], rdi
	or	rdx, rcx
	mov	QWORD PTR -16[rsp], rsi
	sete	al
	ret
	.cfi_endproc
.LFE34:
	.size	twolong_eq_builtin, .-twolong_eq_builtin
	.p2align 4
	.globl	twolong_eq_fields_ptr
	.type	twolong_eq_fields_ptr, @function
twolong_eq_fields_ptr:
.LFB35:
	.cfi_startproc
	endbr64
	mov	rdx, QWORD PTR [rsi]
	xor	eax, eax
	cmp	QWORD PTR [rdi], rdx
	je	.L70
	ret
	.p2align 4,,10
	.p2align 3
.L70:
	mov	rax, QWORD PTR 8[rsi]
	cmp	QWORD PTR 8[rdi], rax
	sete	al
	ret
	.cfi_endproc
.LFE35:
	.size	twolong_eq_fields_ptr, .-twolong_eq_fields_ptr
	.p2align 4
	.globl	twolong_eq_intcast
	.type	twolong_eq_intcast, @function
twolong_eq_intcast:
.LFB36:
	.cfi_startproc
	endbr64
	cmp	rdi, rdx
	sete	al
	cmp	rsi, rcx
	sete	dl
	and	eax, edx
	ret
	.cfi_endproc
.LFE36:
	.size	twolong_eq_intcast, .-twolong_eq_intcast
	.p2align 4
	.globl	padded16_eq_fields
	.type	padded16_eq_fields, @function
padded16_eq_fields:
.LFB37:
	.cfi_startproc
	endbr64
	xor	eax, eax
	cmp	dil, dl
	je	.L75
	ret
	.p2align 4,,10
	.p2align 3
.L75:
	cmp	rsi, rcx
	sete	al
	ret
	.cfi_endproc
.LFE37:
	.size	padded16_eq_fields, .-padded16_eq_fields
	.p2align 4
	.globl	padded16_eq_memcmp
	.type	padded16_eq_memcmp, @function
padded16_eq_memcmp:
.LFB38:
	.cfi_startproc
	endbr64
	mov	rax, rdi
	mov	QWORD PTR -40[rsp], rdx
	xor	rax, rdx
	mov	rdx, rsi
	mov	QWORD PTR -24[rsp], rdi
	xor	rdx, rcx
	mov	QWORD PTR -16[rsp], rsi
	or	rax, rdx
	mov	QWORD PTR -32[rsp], rcx
	sete	al
	ret
	.cfi_endproc
.LFE38:
	.size	padded16_eq_memcmp, .-padded16_eq_memcmp
	.p2align 4
	.globl	padded16_eq_memcmp_ptr
	.type	padded16_eq_memcmp_ptr, @function
padded16_eq_memcmp_ptr:
.LFB45:
	.cfi_startproc
	endbr64
	mov	rax, QWORD PTR [rdi]
	mov	rdx, QWORD PTR 8[rdi]
	xor	rax, QWORD PTR [rsi]
	xor	rdx, QWORD PTR 8[rsi]
	or	rax, rdx
	sete	al
	ret
	.cfi_endproc
.LFE45:
	.size	padded16_eq_memcmp_ptr, .-padded16_eq_memcmp_ptr
	.p2align 4
	.globl	padded16_eq_builtin
	.type	padded16_eq_builtin, @function
padded16_eq_builtin:
.LFB40:
	.cfi_startproc
	endbr64
	mov	rax, rdi
	mov	QWORD PTR -40[rsp], rdx
	xor	rax, rdx
	mov	rdx, rsi
	mov	QWORD PTR -24[rsp], rdi
	xor	rdx, rcx
	mov	QWORD PTR -16[rsp], rsi
	or	rax, rdx
	mov	QWORD PTR -32[rsp], rcx
	sete	al
	ret
	.cfi_endproc
.LFE40:
	.size	padded16_eq_builtin, .-padded16_eq_builtin
	.p2align 4
	.globl	padded16_eq_fields_ptr
	.type	padded16_eq_fields_ptr, @function
padded16_eq_fields_ptr:
.LFB41:
	.cfi_startproc
	endbr64
	movzx	edx, BYTE PTR [rsi]
	xor	eax, eax
	cmp	BYTE PTR [rdi], dl
	je	.L88
	ret
	.p2align 4,,10
	.p2align 3
.L88:
	mov	rax, QWORD PTR 8[rsi]
	cmp	QWORD PTR 8[rdi], rax
	sete	al
	ret
	.cfi_endproc
.LFE41:
	.size	padded16_eq_fields_ptr, .-padded16_eq_fields_ptr
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
