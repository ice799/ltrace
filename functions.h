
/*
 *	Lista de types:
 */

#define	_T_INT		1
#define	_T_ADDR		6

#define	_T_UNKNOWN	-1
#define	_T_VOID		0
#define	_T_INT		1
#define	_T_UINT		2
#define	_T_OCTAL	3
#define	_T_CHAR		4
#define	_T_STRING	5
#define	_T_ADDR		6
#define	_T_FILE		7
#define	_T_HEX		8
#define	_T_FORMAT	9		/* printf-like format */

#define	_T_OUTPUT	0x80		/* OR'ed if arg is an OUTPUT value */

struct function {
	const char * function_name;
	int return_type;
	int num_params;
	int params_type[10];
	struct function * next;
};

extern struct function * list_of_functions;
