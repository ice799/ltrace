#include <signal.h>

main()
{
kill(0,SIGSTOP);
}
