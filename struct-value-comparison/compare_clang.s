	.text
	.intel_syntax noprefix
	.file	"compare.c"
	.globl	point_eq_fields                 # -- Begin function point_eq_fields
	.p2align	4, 0x90
	.type	point_eq_fields,@function
point_eq_fields:                        # @point_eq_fields
	.cfi_startproc
# %bb.0:
	cmp	rdi, rsi
	sete	al
	ret
.Lfunc_end0:
	.size	point_eq_fields, .Lfunc_end0-point_eq_fields
	.cfi_endproc
                                        # -- End function
	.globl	large_eq_fields                 # -- Begin function large_eq_fields
	.p2align	4, 0x90
	.type	large_eq_fields,@function
large_eq_fields:                        # @large_eq_fields
	.cfi_startproc
# %bb.0:
	push	rax
	.cfi_def_cfa_offset 16
	lea	rdi, [rsp + 16]
	lea	rsi, [rsp + 64]
	mov	edx, 48
	call	memcmp@PLT
	test	eax, eax
	sete	al
	pop	rcx
	.cfi_def_cfa_offset 8
	ret
.Lfunc_end1:
	.size	large_eq_fields, .Lfunc_end1-large_eq_fields
	.cfi_endproc
                                        # -- End function
	.globl	padding_eq_fields               # -- Begin function padding_eq_fields
	.p2align	4, 0x90
	.type	padding_eq_fields,@function
padding_eq_fields:                      # @padding_eq_fields
	.cfi_startproc
# %bb.0:
	xor	rdi, rsi
	movabs	rax, -4294967041
	test	rdi, rax
	sete	al
	ret
.Lfunc_end2:
	.size	padding_eq_fields, .Lfunc_end2-padding_eq_fields
	.cfi_endproc
                                        # -- End function
	.globl	point_eq_memcmp                 # -- Begin function point_eq_memcmp
	.p2align	4, 0x90
	.type	point_eq_memcmp,@function
point_eq_memcmp:                        # @point_eq_memcmp
	.cfi_startproc
# %bb.0:
	cmp	rdi, rsi
	sete	al
	ret
.Lfunc_end3:
	.size	point_eq_memcmp, .Lfunc_end3-point_eq_memcmp
	.cfi_endproc
                                        # -- End function
	.globl	large_eq_memcmp                 # -- Begin function large_eq_memcmp
	.p2align	4, 0x90
	.type	large_eq_memcmp,@function
large_eq_memcmp:                        # @large_eq_memcmp
	.cfi_startproc
# %bb.0:
	push	rax
	.cfi_def_cfa_offset 16
	lea	rdi, [rsp + 16]
	lea	rsi, [rsp + 64]
	mov	edx, 48
	call	bcmp@PLT
	test	eax, eax
	sete	al
	pop	rcx
	.cfi_def_cfa_offset 8
	ret
.Lfunc_end4:
	.size	large_eq_memcmp, .Lfunc_end4-large_eq_memcmp
	.cfi_endproc
                                        # -- End function
	.globl	padding_eq_memcmp               # -- Begin function padding_eq_memcmp
	.p2align	4, 0x90
	.type	padding_eq_memcmp,@function
padding_eq_memcmp:                      # @padding_eq_memcmp
	.cfi_startproc
# %bb.0:
	cmp	rdi, rsi
	sete	al
	ret
.Lfunc_end5:
	.size	padding_eq_memcmp, .Lfunc_end5-padding_eq_memcmp
	.cfi_endproc
                                        # -- End function
	.globl	point_eq_memcmp_ptr             # -- Begin function point_eq_memcmp_ptr
	.p2align	4, 0x90
	.type	point_eq_memcmp_ptr,@function
point_eq_memcmp_ptr:                    # @point_eq_memcmp_ptr
	.cfi_startproc
# %bb.0:
	mov	rax, qword ptr [rdi]
	cmp	rax, qword ptr [rsi]
	sete	al
	ret
.Lfunc_end6:
	.size	point_eq_memcmp_ptr, .Lfunc_end6-point_eq_memcmp_ptr
	.cfi_endproc
                                        # -- End function
	.globl	large_eq_memcmp_ptr             # -- Begin function large_eq_memcmp_ptr
	.p2align	4, 0x90
	.type	large_eq_memcmp_ptr,@function
large_eq_memcmp_ptr:                    # @large_eq_memcmp_ptr
	.cfi_startproc
# %bb.0:
	push	rax
	.cfi_def_cfa_offset 16
	mov	edx, 48
	call	bcmp@PLT
	test	eax, eax
	sete	al
	pop	rcx
	.cfi_def_cfa_offset 8
	ret
.Lfunc_end7:
	.size	large_eq_memcmp_ptr, .Lfunc_end7-large_eq_memcmp_ptr
	.cfi_endproc
                                        # -- End function
	.globl	padding_eq_memcmp_ptr           # -- Begin function padding_eq_memcmp_ptr
	.p2align	4, 0x90
	.type	padding_eq_memcmp_ptr,@function
padding_eq_memcmp_ptr:                  # @padding_eq_memcmp_ptr
	.cfi_startproc
# %bb.0:
	mov	rax, qword ptr [rdi]
	cmp	rax, qword ptr [rsi]
	sete	al
	ret
.Lfunc_end8:
	.size	padding_eq_memcmp_ptr, .Lfunc_end8-padding_eq_memcmp_ptr
	.cfi_endproc
                                        # -- End function
	.globl	point_eq_intcast                # -- Begin function point_eq_intcast
	.p2align	4, 0x90
	.type	point_eq_intcast,@function
point_eq_intcast:                       # @point_eq_intcast
	.cfi_startproc
# %bb.0:
	cmp	rdi, rsi
	sete	al
	ret
.Lfunc_end9:
	.size	point_eq_intcast, .Lfunc_end9-point_eq_intcast
	.cfi_endproc
                                        # -- End function
	.globl	large_eq_xor                    # -- Begin function large_eq_xor
	.p2align	4, 0x90
	.type	large_eq_xor,@function
large_eq_xor:                           # @large_eq_xor
	.cfi_startproc
# %bb.0:
	mov	rax, qword ptr [rdi]
	mov	rcx, qword ptr [rdi + 8]
	mov	rdx, qword ptr [rdi + 16]
	mov	r8, qword ptr [rdi + 24]
	mov	r9, qword ptr [rdi + 32]
	xor	rax, qword ptr [rsi]
	xor	rcx, qword ptr [rsi + 8]
	mov	rdi, qword ptr [rdi + 40]
	or	rcx, rax
	xor	rdx, qword ptr [rsi + 16]
	xor	r8, qword ptr [rsi + 24]
	or	r8, rdx
	or	r8, rcx
	xor	r9, qword ptr [rsi + 32]
	xor	rdi, qword ptr [rsi + 40]
	or	r9, r8
	or	rdi, r9
	sete	al
	ret
.Lfunc_end10:
	.size	large_eq_xor, .Lfunc_end10-large_eq_xor
	.cfi_endproc
                                        # -- End function
	.globl	point_eq_builtin                # -- Begin function point_eq_builtin
	.p2align	4, 0x90
	.type	point_eq_builtin,@function
point_eq_builtin:                       # @point_eq_builtin
	.cfi_startproc
# %bb.0:
	cmp	rdi, rsi
	sete	al
	ret
.Lfunc_end11:
	.size	point_eq_builtin, .Lfunc_end11-point_eq_builtin
	.cfi_endproc
                                        # -- End function
	.globl	large_eq_builtin                # -- Begin function large_eq_builtin
	.p2align	4, 0x90
	.type	large_eq_builtin,@function
large_eq_builtin:                       # @large_eq_builtin
	.cfi_startproc
# %bb.0:
	push	rax
	.cfi_def_cfa_offset 16
	lea	rdi, [rsp + 16]
	lea	rsi, [rsp + 64]
	mov	edx, 48
	call	bcmp@PLT
	test	eax, eax
	sete	al
	pop	rcx
	.cfi_def_cfa_offset 8
	ret
.Lfunc_end12:
	.size	large_eq_builtin, .Lfunc_end12-large_eq_builtin
	.cfi_endproc
                                        # -- End function
	.globl	padding_eq_builtin              # -- Begin function padding_eq_builtin
	.p2align	4, 0x90
	.type	padding_eq_builtin,@function
padding_eq_builtin:                     # @padding_eq_builtin
	.cfi_startproc
# %bb.0:
	cmp	rdi, rsi
	sete	al
	ret
.Lfunc_end13:
	.size	padding_eq_builtin, .Lfunc_end13-padding_eq_builtin
	.cfi_endproc
                                        # -- End function
	.globl	point_eq_fields_ptr             # -- Begin function point_eq_fields_ptr
	.p2align	4, 0x90
	.type	point_eq_fields_ptr,@function
point_eq_fields_ptr:                    # @point_eq_fields_ptr
	.cfi_startproc
# %bb.0:
	mov	eax, dword ptr [rdi]
	cmp	eax, dword ptr [rsi]
	jne	.LBB14_1
# %bb.2:
	mov	eax, dword ptr [rdi + 4]
	cmp	eax, dword ptr [rsi + 4]
	sete	al
                                        # kill: def $al killed $al killed $eax
	ret
.LBB14_1:
	xor	eax, eax
                                        # kill: def $al killed $al killed $eax
	ret
.Lfunc_end14:
	.size	point_eq_fields_ptr, .Lfunc_end14-point_eq_fields_ptr
	.cfi_endproc
                                        # -- End function
	.globl	large_eq_fields_ptr             # -- Begin function large_eq_fields_ptr
	.p2align	4, 0x90
	.type	large_eq_fields_ptr,@function
large_eq_fields_ptr:                    # @large_eq_fields_ptr
	.cfi_startproc
# %bb.0:
	mov	rax, qword ptr [rdi]
	cmp	rax, qword ptr [rsi]
	jne	.LBB15_6
# %bb.1:
	mov	rax, qword ptr [rdi + 8]
	cmp	rax, qword ptr [rsi + 8]
	jne	.LBB15_6
# %bb.2:
	mov	rax, qword ptr [rdi + 16]
	cmp	rax, qword ptr [rsi + 16]
	jne	.LBB15_6
# %bb.3:
	mov	rax, qword ptr [rdi + 24]
	cmp	rax, qword ptr [rsi + 24]
	jne	.LBB15_6
# %bb.4:
	mov	rax, qword ptr [rdi + 32]
	cmp	rax, qword ptr [rsi + 32]
	jne	.LBB15_6
# %bb.5:
	mov	rax, qword ptr [rdi + 40]
	cmp	rax, qword ptr [rsi + 40]
	sete	al
                                        # kill: def $al killed $al killed $eax
	ret
.LBB15_6:
	xor	eax, eax
                                        # kill: def $al killed $al killed $eax
	ret
.Lfunc_end15:
	.size	large_eq_fields_ptr, .Lfunc_end15-large_eq_fields_ptr
	.cfi_endproc
                                        # -- End function
	.globl	padding_eq_fields_ptr           # -- Begin function padding_eq_fields_ptr
	.p2align	4, 0x90
	.type	padding_eq_fields_ptr,@function
padding_eq_fields_ptr:                  # @padding_eq_fields_ptr
	.cfi_startproc
# %bb.0:
	movzx	eax, byte ptr [rdi]
	cmp	al, byte ptr [rsi]
	jne	.LBB16_1
# %bb.2:
	mov	eax, dword ptr [rdi + 4]
	cmp	eax, dword ptr [rsi + 4]
	sete	al
                                        # kill: def $al killed $al killed $eax
	ret
.LBB16_1:
	xor	eax, eax
                                        # kill: def $al killed $al killed $eax
	ret
.Lfunc_end16:
	.size	padding_eq_fields_ptr, .Lfunc_end16-padding_eq_fields_ptr
	.cfi_endproc
                                        # -- End function
	.globl	twolong_eq_fields               # -- Begin function twolong_eq_fields
	.p2align	4, 0x90
	.type	twolong_eq_fields,@function
twolong_eq_fields:                      # @twolong_eq_fields
	.cfi_startproc
# %bb.0:
	xor	rdi, rdx
	xor	rsi, rcx
	or	rsi, rdi
	sete	al
	ret
.Lfunc_end17:
	.size	twolong_eq_fields, .Lfunc_end17-twolong_eq_fields
	.cfi_endproc
                                        # -- End function
	.globl	twolong_eq_memcmp               # -- Begin function twolong_eq_memcmp
	.p2align	4, 0x90
	.type	twolong_eq_memcmp,@function
twolong_eq_memcmp:                      # @twolong_eq_memcmp
	.cfi_startproc
# %bb.0:
	mov	qword ptr [rsp - 16], rdi
	mov	qword ptr [rsp - 8], rsi
	mov	qword ptr [rsp - 32], rdx
	mov	qword ptr [rsp - 24], rcx
	movdqu	xmm0, xmmword ptr [rsp - 16]
	movdqu	xmm1, xmmword ptr [rsp - 32]
	pcmpeqb	xmm1, xmm0
	pmovmskb	eax, xmm1
	cmp	eax, 65535
	sete	al
	ret
.Lfunc_end18:
	.size	twolong_eq_memcmp, .Lfunc_end18-twolong_eq_memcmp
	.cfi_endproc
                                        # -- End function
	.globl	twolong_eq_memcmp_ptr           # -- Begin function twolong_eq_memcmp_ptr
	.p2align	4, 0x90
	.type	twolong_eq_memcmp_ptr,@function
twolong_eq_memcmp_ptr:                  # @twolong_eq_memcmp_ptr
	.cfi_startproc
# %bb.0:
	movdqu	xmm0, xmmword ptr [rdi]
	movdqu	xmm1, xmmword ptr [rsi]
	pcmpeqb	xmm1, xmm0
	pmovmskb	eax, xmm1
	cmp	eax, 65535
	sete	al
	ret
.Lfunc_end19:
	.size	twolong_eq_memcmp_ptr, .Lfunc_end19-twolong_eq_memcmp_ptr
	.cfi_endproc
                                        # -- End function
	.globl	twolong_eq_builtin              # -- Begin function twolong_eq_builtin
	.p2align	4, 0x90
	.type	twolong_eq_builtin,@function
twolong_eq_builtin:                     # @twolong_eq_builtin
	.cfi_startproc
# %bb.0:
	mov	qword ptr [rsp - 16], rdi
	mov	qword ptr [rsp - 8], rsi
	mov	qword ptr [rsp - 32], rdx
	mov	qword ptr [rsp - 24], rcx
	movdqu	xmm0, xmmword ptr [rsp - 16]
	movdqu	xmm1, xmmword ptr [rsp - 32]
	pcmpeqb	xmm1, xmm0
	pmovmskb	eax, xmm1
	cmp	eax, 65535
	sete	al
	ret
.Lfunc_end20:
	.size	twolong_eq_builtin, .Lfunc_end20-twolong_eq_builtin
	.cfi_endproc
                                        # -- End function
	.globl	twolong_eq_fields_ptr           # -- Begin function twolong_eq_fields_ptr
	.p2align	4, 0x90
	.type	twolong_eq_fields_ptr,@function
twolong_eq_fields_ptr:                  # @twolong_eq_fields_ptr
	.cfi_startproc
# %bb.0:
	mov	rax, qword ptr [rdi]
	cmp	rax, qword ptr [rsi]
	jne	.LBB21_1
# %bb.2:
	mov	rax, qword ptr [rdi + 8]
	cmp	rax, qword ptr [rsi + 8]
	sete	al
                                        # kill: def $al killed $al killed $eax
	ret
.LBB21_1:
	xor	eax, eax
                                        # kill: def $al killed $al killed $eax
	ret
.Lfunc_end21:
	.size	twolong_eq_fields_ptr, .Lfunc_end21-twolong_eq_fields_ptr
	.cfi_endproc
                                        # -- End function
	.globl	twolong_eq_intcast              # -- Begin function twolong_eq_intcast
	.p2align	4, 0x90
	.type	twolong_eq_intcast,@function
twolong_eq_intcast:                     # @twolong_eq_intcast
	.cfi_startproc
# %bb.0:
	xor	rdi, rdx
	xor	rsi, rcx
	or	rsi, rdi
	sete	al
	ret
.Lfunc_end22:
	.size	twolong_eq_intcast, .Lfunc_end22-twolong_eq_intcast
	.cfi_endproc
                                        # -- End function
	.globl	padded16_eq_fields              # -- Begin function padded16_eq_fields
	.p2align	4, 0x90
	.type	padded16_eq_fields,@function
padded16_eq_fields:                     # @padded16_eq_fields
	.cfi_startproc
# %bb.0:
	cmp	dil, dl
	sete	dl
	cmp	rsi, rcx
	sete	al
	and	al, dl
	ret
.Lfunc_end23:
	.size	padded16_eq_fields, .Lfunc_end23-padded16_eq_fields
	.cfi_endproc
                                        # -- End function
	.globl	padded16_eq_memcmp              # -- Begin function padded16_eq_memcmp
	.p2align	4, 0x90
	.type	padded16_eq_memcmp,@function
padded16_eq_memcmp:                     # @padded16_eq_memcmp
	.cfi_startproc
# %bb.0:
	mov	byte ptr [rsp - 16], dil
	mov	qword ptr [rsp - 8], rsi
	mov	byte ptr [rsp - 32], dl
	mov	qword ptr [rsp - 24], rcx
	movdqu	xmm0, xmmword ptr [rsp - 16]
	movdqu	xmm1, xmmword ptr [rsp - 32]
	pcmpeqb	xmm1, xmm0
	pmovmskb	eax, xmm1
	cmp	eax, 65535
	sete	al
	ret
.Lfunc_end24:
	.size	padded16_eq_memcmp, .Lfunc_end24-padded16_eq_memcmp
	.cfi_endproc
                                        # -- End function
	.globl	padded16_eq_memcmp_ptr          # -- Begin function padded16_eq_memcmp_ptr
	.p2align	4, 0x90
	.type	padded16_eq_memcmp_ptr,@function
padded16_eq_memcmp_ptr:                 # @padded16_eq_memcmp_ptr
	.cfi_startproc
# %bb.0:
	movdqu	xmm0, xmmword ptr [rdi]
	movdqu	xmm1, xmmword ptr [rsi]
	pcmpeqb	xmm1, xmm0
	pmovmskb	eax, xmm1
	cmp	eax, 65535
	sete	al
	ret
.Lfunc_end25:
	.size	padded16_eq_memcmp_ptr, .Lfunc_end25-padded16_eq_memcmp_ptr
	.cfi_endproc
                                        # -- End function
	.globl	padded16_eq_builtin             # -- Begin function padded16_eq_builtin
	.p2align	4, 0x90
	.type	padded16_eq_builtin,@function
padded16_eq_builtin:                    # @padded16_eq_builtin
	.cfi_startproc
# %bb.0:
	mov	byte ptr [rsp - 16], dil
	mov	qword ptr [rsp - 8], rsi
	mov	byte ptr [rsp - 32], dl
	mov	qword ptr [rsp - 24], rcx
	movdqu	xmm0, xmmword ptr [rsp - 16]
	movdqu	xmm1, xmmword ptr [rsp - 32]
	pcmpeqb	xmm1, xmm0
	pmovmskb	eax, xmm1
	cmp	eax, 65535
	sete	al
	ret
.Lfunc_end26:
	.size	padded16_eq_builtin, .Lfunc_end26-padded16_eq_builtin
	.cfi_endproc
                                        # -- End function
	.globl	padded16_eq_fields_ptr          # -- Begin function padded16_eq_fields_ptr
	.p2align	4, 0x90
	.type	padded16_eq_fields_ptr,@function
padded16_eq_fields_ptr:                 # @padded16_eq_fields_ptr
	.cfi_startproc
# %bb.0:
	movzx	eax, byte ptr [rdi]
	cmp	al, byte ptr [rsi]
	jne	.LBB27_1
# %bb.2:
	mov	rax, qword ptr [rdi + 8]
	cmp	rax, qword ptr [rsi + 8]
	sete	al
                                        # kill: def $al killed $al killed $eax
	ret
.LBB27_1:
	xor	eax, eax
                                        # kill: def $al killed $al killed $eax
	ret
.Lfunc_end27:
	.size	padded16_eq_fields_ptr, .Lfunc_end27-padded16_eq_fields_ptr
	.cfi_endproc
                                        # -- End function
	.ident	"Ubuntu clang version 18.1.3 (1ubuntu1)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym memcmp
