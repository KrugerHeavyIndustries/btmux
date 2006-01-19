/*
 * signal.c
 *
 * $Id $
 */

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "debug.h"
#include "mudconf.h"
#include "externs.h"
#include "flags.h"

void signal_TERM(int, siginfo_t *, void *);
void signal_PIPE(int, siginfo_t *, void *);
void signal_USR1(int, siginfo_t *, void *);
void signal_SEGV(int, siginfo_t *, void *);
void signal_BUS(int, siginfo_t *, void *);

struct sigaction saTERM = {.sa_handler = NULL,.sa_sigaction = signal_TERM,
	.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_RESTART
};

struct sigaction saPIPE = {.sa_handler = NULL,.sa_sigaction = signal_PIPE,
	.sa_flags = SA_SIGINFO
};

struct sigaction saUSR1 = {.sa_handler = NULL,.sa_sigaction = signal_USR1,
	.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_RESTART 
};

struct sigaction saSEGV = {.sa_handler = NULL,.sa_sigaction = signal_SEGV,
	.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_RESTART | SA_ONSTACK
};

struct sigaction saBUS = {.sa_handler = NULL,.sa_sigaction = signal_BUS,
	.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_RESTART | SA_ONSTACK
};

stack_t sighandler_stack;  
stack_t regular_stack;

void bind_signals()
{
	sighandler_stack.ss_sp = malloc(262144);
    sighandler_stack.ss_size = 262144;
    sighandler_stack.ss_flags = 0;
    sigaltstack(&sighandler_stack, &regular_stack);
    dprintk("Current stack at %p with length 0x%08x and flags 0x%08x\n", 
            regular_stack.ss_sp, regular_stack.ss_size, regular_stack.ss_flags);
    dprintk("Signal stack at %p with length 0x%08x and flags 0x%08x\n", 
            sighandler_stack.ss_sp, sighandler_stack.ss_size, sighandler_stack.ss_flags);
    sigaction(SIGTERM, &saTERM, NULL);
	sigaction(SIGPIPE, &saPIPE, NULL);
	sigaction(SIGUSR1, &saUSR1, NULL);
	sigaction(SIGSEGV, &saSEGV, NULL);
	sigaction(SIGBUS, &saBUS, NULL);
    signal(SIGCHLD, SIG_IGN);
}

void unbind_signals()
{
	signal(SIGTERM, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGUSR1, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

void signal_TERM(int signo, siginfo_t * siginfo, void *ucontext)
{
	dprintk(".");
	do_shutdown(NOTHING, 0, SHUTDN_EXIT, "received SIGTERM from kernel.");
}

void signal_PIPE(int signo, siginfo_t * siginfo, void *ucontext)
{
	dprintk(".");
#ifdef HAVE_SIGINFO_T_SI_FD
	eradicate_broken_fd(siginfo->si_fd);
#else
	eradicate_broken_fd(-1);
#endif
}

void signal_USR1(int signo, siginfo_t * siginfo, void *ucontext)
{
	do_restart(1, 1, 0);
}

void signal_SEGV(int signo, siginfo_t * siginfo, void *ucontext)
{
	int child;
	if(!(child = fork())) {
		dump_restart_db();
		execl(mudstate.executable_path, mudstate.executable_path,
			  mudconf.config_file, NULL);
	} else {
		switch (siginfo->si_code) {
		case SEGV_MAPERR:
			raw_broadcast(0,
						  "Game: Invalid access of unamapped memory at %p. Restarting from Checkpoint.",
						  siginfo->si_addr);
			break;
		case SEGV_ACCERR:
			raw_broadcast(0,
						  "Game: Invalid access of protected memory at %p. Restarting from Checkpoint.",
						  siginfo->si_addr);
			break;
		default:
			raw_broadcast(0,
						  "Game: Unhandled SEGV at %p. Restarting from checkpoint.",
						  siginfo->si_addr);
			break;
		}
		dump_database_internal(DUMP_CRASHED);
		report();
	}
}

void signal_BUS(int signo, siginfo_t * siginfo, void *ucontext)
{
	int child;
	if(mudconf.sig_action != SA_EXIT && !(child = fork())) {

		dump_restart_db();
		execl(mudstate.executable_path, mudstate.executable_path,
			  mudconf.config_file, NULL);
	} else {
		switch (siginfo->si_code) {
		case BUS_ADRALN:
			raw_broadcast(0,
						  "Game: Invalid address alignment accessing %p. Restarting from Checkpoint.",
						  siginfo->si_addr);
			break;
		case BUS_ADRERR:
			raw_broadcast(0,
						  "Game: Invalid access of non-existent physical memory at %p. Restarting from Checkpoint.",
						  siginfo->si_addr);
			break;
		case BUS_OBJERR:
			raw_broadcast(0,
						  "Game: Invalid object specific hardware error access at %p. Restarting from Checkpoint.",
						  siginfo->si_addr);
			break;
		default:
			raw_broadcast(0,
						  "Game: Unhandled SEGV at %p. Restarting from checkpoint.",
						  siginfo->si_addr);
			break;
		}
		report();
		dump_database_internal(DUMP_CRASHED);
	}
}
