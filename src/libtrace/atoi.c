static unsigned long atoi(const char *cp)
{
	unsigned long result;

	result = 0;

	while(*cp >= '0' && *cp <= '9') {
		result = 10*result + *cp-'0';
		cp++;
	}
	return(result);
}

