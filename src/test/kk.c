static void initialize(void)
{
	printf("Hola\n");
}

__asm__(".section .init");

void _init(void)
{
	initialize();
}
