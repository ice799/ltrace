struct process {
	int pid;
	struct process * next;
};

extern struct process * list_of_processes;

int attach_process(const char * file, char * const argv[]);
void init_sighandler(void);
