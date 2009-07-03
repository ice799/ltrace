#include <stdio.h>
#include <unistd.h>

#include "ltrace.h"

/*
static int count_call =0;
static int count_ret  =0;

static void
callback_call(Event * ev) {
	count_call ++;
}
static void
callback_ret(Event * ev) {
	count_ret ++;
}

static void
endcallback(Event *ev) {
	printf("%d calls\n%d rets\n",count_call, count_ret);
}
*/

int
main(int argc, char *argv[]) {
	ltrace_init(argc, argv);

/*
	ltrace_add_callback(callback_call, EVENT_SYSCALL);
	ltrace_add_callback(callback_ret, EVENT_SYSRET);
	ltrace_add_callback(endcallback, EVENT_EXIT);
*/

	ltrace_main();
	return 0;
}
