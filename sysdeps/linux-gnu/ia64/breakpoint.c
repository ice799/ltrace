/* IA64 breakpoint support.  Much of this clagged from gdb
 *  -Ian Wienand <ianw@gelato.unsw.edu.au> 10/3/2005
 */

#include "config.h"

#include <sys/ptrace.h>
#include <string.h>
#include "arch.h"
#include "options.h"
#include "output.h"
#include "debug.h"

static long long extract_bit_field(char *bundle, int from, int len)
{
	long long result = 0LL;
	int to = from + len;
	int from_byte = from / 8;
	int to_byte = to / 8;
	unsigned char *b = (unsigned char *)bundle;
	unsigned char c;
	int lshift;
	int i;

	c = b[from_byte];
	if (from_byte == to_byte)
		c = ((unsigned char)(c << (8 - to % 8))) >> (8 - to % 8);
	result = c >> (from % 8);
	lshift = 8 - (from % 8);

	for (i = from_byte + 1; i < to_byte; i++) {
		result |= ((long long)b[i]) << lshift;
		lshift += 8;
	}

	if (from_byte < to_byte && (to % 8 != 0)) {
		c = b[to_byte];
		c = ((unsigned char)(c << (8 - to % 8))) >> (8 - to % 8);
		result |= ((long long)c) << lshift;
	}

	return result;
}

/* Replace the specified bits in an instruction bundle */
static void replace_bit_field(char *bundle, long long val, int from, int len)
{
	int to = from + len;
	int from_byte = from / 8;
	int to_byte = to / 8;
	unsigned char *b = (unsigned char *)bundle;
	unsigned char c;

	if (from_byte == to_byte) {
		unsigned char left, right;
		c = b[from_byte];
		left = (c >> (to % 8)) << (to % 8);
		right =
		    ((unsigned char)(c << (8 - from % 8))) >> (8 - from % 8);
		c = (unsigned char)(val & 0xff);
		c = (unsigned char)(c << (from % 8 + 8 - to % 8)) >> (8 -
								      to % 8);
		c |= right | left;
		b[from_byte] = c;
	} else {
		int i;
		c = b[from_byte];
		c = ((unsigned char)(c << (8 - from % 8))) >> (8 - from % 8);
		c = c | (val << (from % 8));
		b[from_byte] = c;
		val >>= 8 - from % 8;

		for (i = from_byte + 1; i < to_byte; i++) {
			c = val & 0xff;
			val >>= 8;
			b[i] = c;
		}

		if (to % 8 != 0) {
			unsigned char cv = (unsigned char)val;
			c = b[to_byte];
			c = c >> (to % 8) << (to % 8);
			c |= ((unsigned char)(cv << (8 - to % 8))) >> (8 -
								       to % 8);
			b[to_byte] = c;
		}
	}
}

/* Return the contents of slot N (for N = 0, 1, or 2) in
   and instruction bundle */
static long long slotN_contents(char *bundle, int slotnum)
{
	return extract_bit_field(bundle, 5 + 41 * slotnum, 41);
}

/* Store an instruction in an instruction bundle */

static void replace_slotN_contents(char *bundle, long long instr, int slotnum)
{
	replace_bit_field(bundle, instr, 5 + 41 * slotnum, 41);
}

typedef enum instruction_type {
	A,			/* Integer ALU ;    I-unit or M-unit */
	I,			/* Non-ALU integer; I-unit */
	M,			/* Memory ;         M-unit */
	F,			/* Floating-point ; F-unit */
	B,			/* Branch ;         B-unit */
	L,			/* Extended (L+X) ; I-unit */
	X,			/* Extended (L+X) ; I-unit */
	undefined		/* undefined or reserved */
} instruction_type;

static enum instruction_type template_encoding_table[32][3] = {
	{M, I, I},		/* 00 */
	{M, I, I},		/* 01 */
	{M, I, I},		/* 02 */
	{M, I, I},		/* 03 */
	{M, L, X},		/* 04 */
	{M, L, X},		/* 05 */
	{undefined, undefined, undefined},	/* 06 */
	{undefined, undefined, undefined},	/* 07 */
	{M, M, I},		/* 08 */
	{M, M, I},		/* 09 */
	{M, M, I},		/* 0A */
	{M, M, I},		/* 0B */
	{M, F, I},		/* 0C */
	{M, F, I},		/* 0D */
	{M, M, F},		/* 0E */
	{M, M, F},		/* 0F */
	{M, I, B},		/* 10 */
	{M, I, B},		/* 11 */
	{M, B, B},		/* 12 */
	{M, B, B},		/* 13 */
	{undefined, undefined, undefined},	/* 14 */
	{undefined, undefined, undefined},	/* 15 */
	{B, B, B},		/* 16 */
	{B, B, B},		/* 17 */
	{M, M, B},		/* 18 */
	{M, M, B},		/* 19 */
	{undefined, undefined, undefined},	/* 1A */
	{undefined, undefined, undefined},	/* 1B */
	{M, F, B},		/* 1C */
	{M, F, B},		/* 1D */
	{undefined, undefined, undefined},	/* 1E */
	{undefined, undefined, undefined},	/* 1F */
};

union bundle_t {
	char cbundle[16];
	unsigned long ubundle[2];
};

void arch_enable_breakpoint(pid_t pid, struct breakpoint *sbp)
{

	unsigned long addr = (unsigned long)sbp->addr;
	union bundle_t bundle;
	int slotnum = (int)(addr & 0x0f) & 0x3;
	long long instr;
	int template;

	debug(1, "Enable Breakpoint at %p)", sbp->addr);

	if (slotnum > 2)
		printf
		    ("Can't insert breakpoint for slot numbers greater than 2.");

	addr &= ~0x0f;
	bundle.ubundle[0] = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	bundle.ubundle[1] = ptrace(PTRACE_PEEKTEXT, pid, addr + 8, 0);

	/* Check for L type instruction in 2nd slot, if present then
	   bump up the slot number to the 3rd slot  */
	template = extract_bit_field(bundle.cbundle, 0, 5);
	if (slotnum == 1 && template_encoding_table[template][1] == L) {
		slotnum = 2;
	}

	instr = slotN_contents(bundle.cbundle, slotnum);

	memcpy(sbp->orig_value, &instr, sizeof(instr));

	replace_slotN_contents(bundle.cbundle, 0x00002000040LL, slotnum);

	ptrace(PTRACE_POKETEXT, pid, addr, bundle.ubundle[0]);
	ptrace(PTRACE_POKETEXT, pid, addr + 8, bundle.ubundle[1]);

}

void arch_disable_breakpoint(pid_t pid, const struct breakpoint *sbp)
{

	unsigned long addr = (unsigned long)sbp->addr;
	int slotnum = (int)(addr & 0x0f) & 0x3;
	union bundle_t bundle;
	unsigned long instr;

	debug(1, "Disable Breakpoint at %p", sbp->addr);

	addr &= ~0x0f;

	bundle.ubundle[0] = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	bundle.ubundle[1] = ptrace(PTRACE_PEEKTEXT, pid, addr + 8, 0);

	memcpy(&instr, sbp->orig_value, sizeof(instr));

	replace_slotN_contents(bundle.cbundle, instr, slotnum);
	ptrace(PTRACE_POKETEXT, pid, addr, bundle.ubundle[0]);
	ptrace(PTRACE_POKETEXT, pid, addr + 8, bundle.ubundle[1]);
}
