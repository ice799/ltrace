void function(i)
{
	int * sp;

	sp = &i;

	printf("%d\n", sp[0]);
}

main()
{
	function(23);
}
