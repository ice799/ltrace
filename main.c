#include "ltrace.h"

int
main(int argc, char *argv[]) {
	ltrace_init(argc, argv);
	ltrace_main();
	return 0;
}
