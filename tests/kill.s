	.file	"kill.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 16
.globl main
	.type	 main,@function
main:
	pushl %ebp
	movl %esp,%ebp
	pushl $19
	pushl $0
	call kill
	addl $8,%esp
.L4:
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe1:
	.size	 main,.Lfe1-main
	.ident	"GCC: (GNU) 2.7.2.2"
