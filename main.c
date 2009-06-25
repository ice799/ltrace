#include <stdio.h>
#include <unistd.h>

#include "ltrace.h"

/*
static void
callback(Event * ev) {
	printf("\n\tcallback(ev->type=%d)\n", ev->type);
}
*/

int
main(int argc, char *argv[]) {
	ltrace_init(argc, argv);
/*
	ltrace_add_callback(callback);
*/
	ltrace_main();
	return 0;
}
