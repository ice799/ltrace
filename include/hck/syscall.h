#ifndef _HCK_SYSCALL_H
#define _HCK_SYSCALL_H

#include <errno.h>

#include <utime.h>	/* utime */
#include <linux/types.h>	/* chmod */
#include <asm/signal.h>
#include <linux/resource.h>
#include <linux/dirent.h>
#include <asm/statfs.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <linux/time.h>
#include <linux/socket.h>

#define __NR_exit	1
#define __NR_fork	2
#define __NR_read	3
#define __NR_write	4
#define __NR_open	5
#define __NR_close	6
#define __NR_waitpid	7
#define __NR_creat	8
#define __NR_link	9
#define __NR_unlink	10
#define __NR_execve	11
#define __NR_chdir	12
#define __NR_time	13
#define __NR_mknod	14
#define __NR_chmod	15
#define __NR_chown	16
#define __NR_lseek	19
#define __NR_getpid	20
#define __NR_mount	21
#define __NR_umount	22
#define __NR_setuid	23
#define __NR_getuid	24
#define __NR_alarm	27
#define __NR_pause	29
#define __NR_utime	30
#define __NR_access	33
#define __NR_sync	36
#define __NR_kill	37
#define __NR_rename	38
#define __NR_mkdir	39
#define __NR_rmdir	40
#define __NR_dup	41
#define __NR_pipe	42
#define __NR_brk	45
#define __NR_getgid	47
#define __NR_signal	48
#define __NR_geteuid	49
#define __NR_getegid	50
#define __NR_ioctl	54
#define __NR_fcntl	55
#define __NR_umask	60
#define __NR_dup2	63
#define __NR_setsid	66
#define __NR_sigaction	67
#define __NR_setrlimit	75
#define __NR_getrusage	77
#define __NR_gettimeofday	78
#define __NR_symlink	83
#define __NR_readlink	85
#define __NR_swapon	87
#define __NR_reboot	88
#define __NR_readdir	89
#define __NR_mmap	90
#define __NR_fchmod	94
#define __NR_fchown	95
#define __NR_statfs	99
#define __NR_socketcall	102
#define __NR_stat	106
#define __NR_lstat	107
#define __NR_fstat	108
#define __NR_wait4	114
#define __NR_swapoff	115
#define __NR_uname	122
#define __NR_llseek	140
#define __NR_select	142

#define __NR__exit __NR_exit

#ifndef _INLINE
#define _INLINE static inline
#endif

#define __syscall_return(type, res) \
do { \
	if ((unsigned long)(res) >= (unsigned long)(-125)) { \
		errno = -(res); \
		res = -1; \
	} \
	return (type) (res); \
} while (0)

#define _syscall0(type,name) \
_INLINE type _sys_##name(void) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name)); \
__syscall_return(type,__res); \
}

#define _syscall1(type,name,type1,arg1) \
_INLINE type _sys_##name(type1 arg1) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name),"b" ((long)(arg1))); \
__syscall_return(type,__res); \
}

#define _syscall2(type,name,type1,arg1,type2,arg2) \
_INLINE type _sys_##name(type1 arg1,type2 arg2) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2))); \
__syscall_return(type,__res); \
}

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
_INLINE type _sys_##name(type1 arg1,type2 arg2,type3 arg3) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
                  "d" ((long)(arg3))); \
__syscall_return(type,__res); \
}

#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
_INLINE type _sys_##name (type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
          "d" ((long)(arg3)),"S" ((long)(arg4))); \
__syscall_return(type,__res); \
}

#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
          type5,arg5) \
_INLINE type _sys_##name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5) \
{ \
long __res; \
__asm__ volatile ("int $0x80" \
        : "=a" (__res) \
        : "0" (__NR_##name),"b" ((long)(arg1)),"c" ((long)(arg2)), \
          "d" ((long)(arg3)),"S" ((long)(arg4)),"D" ((long)(arg5))); \
__syscall_return(type,__res); \
}

_syscall1(void,_exit,int,exitcode)
_syscall0(int,fork)
_syscall3(int,read,int,fd,char *,buf,long,count)
_syscall3(int,write,int,fd,const char *,buf,long,count)
_syscall3(int,open,const char *,file,int,flag,int,mode)
_syscall1(int,close,int,fd)
_syscall3(pid_t,waitpid,pid_t,pid,int *,wait_stat,int,options)
_syscall2(int,creat,const char *,file,mode_t,mode)
_syscall2(int,link,const char *,oldpath,const char *,newpath)
_syscall1(int,unlink,const char *,file)
_syscall3(int,execve,const char *,file,const char **,argv,const char **,envp)
_syscall1(int,chdir,const char *,path)
_syscall1(time_t,time,time_t *,t)
_syscall3(int,mknod,const char *,pathname,mode_t,mode,dev_t,dev)
_syscall2(int,chmod,const char *,file,mode_t,mode)
_syscall3(int,chown,const char *,file,uid_t,owner,gid_t,group)
_syscall3(off_t,lseek,int,fd,off_t,offset,int,whence)
_syscall0(pid_t,getpid)
_syscall5(int,mount,const char *,file,const char *,dir,const char *,fstype,unsigned long,rwflag,const void *,data)
_syscall1(int,umount,const char *,file)
_syscall1(uid_t,setuid,uid_t,uid)
_syscall0(gid_t,getuid)
_syscall1(long,alarm,long,seconds)
_syscall0(int,pause)
_syscall2(int,utime,const char *,file,const struct utimbuf *,buf)
_syscall2(int,access,const char *,pathname,int,mode)
_syscall0(int,sync)
_syscall2(int,kill,int,pid,int,sig)
_syscall2(int,rename,const char *,oldpath,const char *,newpath)
_syscall2(int,mkdir,const char *,pathname,int,mode)
_syscall1(int,rmdir,const char *,pathname)
_syscall1(int,dup,int,oldfd)
_syscall1(int,pipe,int *,filedes)
_syscall1(void *,brk,void *,brk)
_syscall0(gid_t,getgid)
_syscall0(uid_t,geteuid)
_syscall0(gid_t,getegid)
_syscall2(__sighandler_t,signal,int,signum,__sighandler_t,handle)
_syscall3(int,ioctl,unsigned int,fd,unsigned int,cmd,unsigned long,arg)
_syscall3(int,fcntl,unsigned int,fd,unsigned int,cmd,unsigned long,arg)
_syscall1(int,umask,int,mask)
_syscall2(int,dup2,int,oldfd,int,newfd)
_syscall0(pid_t,setsid)
_syscall3(int,sigaction,int,signum,const struct sigaction *,act,const struct sigaction*,oldact)
_syscall2(int,setrlimit,int,resource,const struct rlimit *,rlim)
_syscall2(int,getrusage,int,who,struct rusage *,usage)
_syscall2(int,gettimeofday,struct timeval *,tv,struct timezone *,tz)
_syscall2(int,symlink,const char *,oldpath,const char *,newpath)
_syscall3(int,readlink,const char *,path,char *,buf,size_t,bufsiz)
_syscall2(int,swapon,const char *,path,int,swapflags)
_syscall3(int,reboot,int,magic,int,magic_too,int,flag)
_syscall3(int,readdir,unsigned int,fd,struct dirent *,dirp,unsigned int,count)
_syscall1(long,mmap,unsigned long *,buffer)
_syscall2(int,fchmod,int,fd,mode_t,mode)
_syscall3(int,fchown,int,fd,uid_t,owner,gid_t,group)
_syscall2(int,statfs,const char *,path,struct statfs *,buf)
_syscall2(int,socketcall,int,call,unsigned long *,args)
_syscall2(int,stat,const char *,file_name,struct stat *,buff)
_syscall2(int,lstat,const char *,file_name,struct stat *,buff)
_syscall2(int,fstat,int,filedes,struct stat *,buff)
_syscall4(pid_t,wait4,pid_t,pid,int *,status,int,options,struct rusage *,rusage)
_syscall1(int,swapoff,const char *,path)
_syscall1(int,uname,struct utsname *,buf)
_syscall5(int,llseek,int,fd,unsigned long,hi,unsigned long,lo,loff_t *,result,unsigned int,whence)
_syscall5(int,select,int,n,fd_set *,readfds,fd_set *,writefds,fd_set *,exceptfds,struct timeval *,timeout)

#endif /* _HCK_SYSCALL_H */
