	.file	"kk.c"
	.version	"01.01"
gcc2_compiled.:
.section	.rodata
.LC0:
	.string	"Hola\n"
.text
	.align 16
	.type	 initialize,@function
initialize:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	call .L2
.L2:
	popl %ebx
	addl $_GLOBAL_OFFSET_TABLE_+[.-.L2],%ebx
	leal .LC0@GOTOFF(%ebx),%edx
	movl %edx,%eax
	pushl %eax
	call printf@PLT
	addl $4,%esp
.L1:
	movl -4(%ebp),%ebx
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe1:
	.size	 initialize,.Lfe1-initialize
#APP
	.section .init
#NO_APP
	.align 16
.globl _init
	.type	 _init,@function
_init:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	call .L4
.L4:
	popl %ebx
	addl $_GLOBAL_OFFSET_TABLE_+[.-.L4],%ebx
	call initialize@PLT
.L3:
	movl -4(%ebp),%ebx
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe2:
	.size	 _init,.Lfe2-_init
	.ident	"GCC: (GNU) 2.7.2"
