#include <unistd.h>
#include <stdlib.h>

int main (int argc, char ** argv)
{
  execl (argv[1], argv[1], NULL);
  abort ();
}
