	.file	"libtest.c"
	.version	"01.01"
gcc2_compiled.:
.section	.rodata
.LC0:
	.string	"Hello, world\n"
.text
	.align 16
	.type	 initialize,@function
initialize:
	pushl %ebp
	movl %esp,%ebp
	pushl $.LC0
	call printf
	movl %ebp,%esp
	popl %ebp
	ret
.Lfe1:
	.size	 initialize,.Lfe1-initialize
	.ident	"GCC: (GNU) 2.7.2"
.section	.init
	movl	$36,%eax
	int	$0x80
	ret	$0x0
