#include <hck/syscall.h>

void init_libtrace(void)
{
	_sys_write(1,"Hola\n",5);
}
