/* Handle signals in whatever os specific way is necessary.
   Basically, we really want to use polling.  In order to do this
   signal_poll_flag is set to TRUE when a signal comes in, and is
   checked and reset by the bytecode emulator at frequent intervals
   when it is safe to field an interrupt.

   BUG: This can delay interrupt handling when waiting for input.
 */

#include <stdio.h>
#include "config.h"

#ifdef SIGNALS

#ifdef __STDC__
volatile int signal_poll_flag = 0;
#else
int signal_poll_flag = 0;
#endif



#include <signal.h>

#ifdef BSD_OR_MACH
#define INTR_HANDLER_TYPE int
#endif
#ifdef USG
#define INTR_HANDLER_TYPE void
#endif

#ifdef BSD_OR_MACH

#if !defined(__with_signal_action)
static struct sigvec signal_trap_vec = {0,0,0};
#else
struct sigaction action;
#endif

/*ARGSUSED*/
#if !defined(__with_signal_action)
static INTR_HANDLER_TYPE intr_proc(sig,code,scp)
     int sig, code;
     struct sigcontext *scp;
{
  signal_poll_flag += 1;
}
#else
static void intr_proc(int sig, siginfo_t *scp, void *code)
{
  signal_poll_flag += 1;
}
#endif

void enable_signal_polling()
{
  signal_poll_flag = 0;
#if !defined(__with_signal_action)
  signal_trap_vec.sv_handler = intr_proc;
  if (sigvec(SIGINT, &signal_trap_vec, (struct sigvec *)NULL))
    fprintf(stderr, "Unable to enable signal polling.\n");
#else
  action._funcptr._sigaction = intr_proc;
  action.sa_flags = 0;
  if ( sigaction (SIGINT, &action, NULL)) {
    fprintf(stderr, "Unable to enable signal polling.\n");
  }
#endif
}

void disable_signal_polling()
{
  signal_poll_flag = 0;
#if !defined(__with_signal_action)
  signal_trap_vec.sv_handler=SIG_DFL;
  if (sigvec(SIGINT, &signal_trap_vec, (struct sigvec *)NULL))
    fprintf(stderr, "Unable to disable signal polling.\n");
#else
  action._funcptr._sigaction = SIG_DFL;
  action.sa_flags = 0;
  if ( sigaction (SIGINT, &action, NULL)) {
    fprintf(stderr, "Unable to disable signal polling.\n");
  }
#endif
}
#endif BSD_OR_MACH

#ifdef USG
static INTR_HANDLER_TYPE intr_proc()
{
  signal_poll_flag += 1;
}

void enable_signal_polling()
{
  signal_poll_flag = 0;
  if (signal(SIGINT, intr_proc) == SIG_ERR)
    fprintf(stderr, "Unable to enable signal polling.\n");
}

void disable_signal_polling()
{
  signal_poll_flag = 0;
  if (signal(SIGINT, SIG_DFL) == SIG_ERR)
    fprintf(stderr, "Unable to disable signal polling.\n");
}
#endif USG

void clear_signal()
{
  signal_poll_flag = 0;
}

#endif
