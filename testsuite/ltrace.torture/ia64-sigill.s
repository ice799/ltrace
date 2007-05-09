	.file	"pokus.c"
	.pred.safe_across_calls p1-p5,p16-p63
	.section	.rodata
	.align 8
.LC0:
	stringz	""
	.text
	.align 16
	.global main#
	.proc main#
main:
	.prologue 14, 32
	.save ar.pfs, r33
	alloc r33 = ar.pfs, 0, 4, 1, 0
	.vframe r34
	mov r34 = r12
	mov r35 = r1
	.save rp, r32
	mov r32 = b0
	.body
	addl r36 = @ltoffx(.LC0), r1
	;;
	ld8.mov r36 = [r36], .LC0
	br.call.sptk.many b0 = printf#
	nop.b 0x0
	nop.b 0x1
	nop.b 0x2
	nop.b 0x0
	nop.b 0x1
	nop.b 0x2
	mov r1 = r35
	addl r14 = 234, r0
	;;
	mov r8 = r14
	mov ar.pfs = r33
	mov b0 = r32
	.restore sp
	mov r12 = r34
	br.ret.sptk.many b0
	;;
	.endp main#
	.section	.note.GNU-stack,"",@progbits
	.ident	"GCC: (GNU) 3.4.6 20060404 (Red Hat 3.4.6-3)"
