	.file	"libtrace.c"
	.version	"01.01"
gcc2_compiled.:
.data
	.align 4
	.type	 fd,@object
	.size	 fd,4
fd:
	.long 2
	.type	 ax_jmp,@object
ax_jmp:
.byte -72
.byte 1
.byte 2
.byte 3
.byte 4
.byte -93
.byte 5
.byte 6
.byte 7
.byte 8
.byte -1
.byte 37
.byte 1
.byte 2
.byte 3
.byte 4
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
.byte 0
	.size	 ax_jmp,28
	.align 4
	.type	 new_func_ptr,@object
	.size	 new_func_ptr,4
new_func_ptr:
	.long 0
.section	.rodata
.LC0:
	.string	"LTRACE_FILENAME"
.LC1:
	.string	"Can't open LTRACE_FILENAME\n"
.LC2:
	.string	"Not enough fd's?\n"
.LC3:
	.string	"LTRACE_SYMTAB"
.LC4:
	.string	"No LTRACE_SYMTAB...\n"
.LC5:
	.string	"LTRACE_SYMTAB_LEN"
.LC6:
	.string	"No LTRACE_SYMTAB_LEN...\n"
.LC7:
	.string	"LTRACE_STRTAB"
.LC8:
	.string	"No LTRACE_STRTAB...\n"
.LC9:
	.string	"Cannot mmap?\n"
.LC10:
	.string	"ltrace starting...\n"
