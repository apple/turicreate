/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(Process.h)
#include KWSYS_HEADER(System.h)

/* Work-around CMake dependency scanning limitation.  This must
   duplicate the above list of headers.  */
#if 0
#  include "Process.h.in"
#  include "System.h.in"
#endif

/*

Implementation for UNIX

On UNIX, a child process is forked to exec the program.  Three output
pipes are read by the parent process using a select call to block
until data are ready.  Two of the pipes are stdout and stderr for the
child.  The third is a special pipe populated by a signal handler to
indicate that a child has terminated.  This is used in conjunction
with the timeout on the select call to implement a timeout for program
even when it closes stdout and stderr and at the same time avoiding
races.

*/

/*

TODO:

We cannot create the pipeline of processes in suspended states.  How
do we cleanup processes already started when one fails to load?  Right
now we are just killing them, which is probably not the right thing to
do.

*/

#if defined(__CYGWIN__)
/* Increase the file descriptor limit for select() before including
   related system headers. (Default: 64) */
#  define FD_SETSIZE 16384
#endif

#include <assert.h>    /* assert */
#include <ctype.h>     /* isspace */
#include <dirent.h>    /* DIR, dirent */
#include <errno.h>     /* errno */
#include <fcntl.h>     /* fcntl */
#include <signal.h>    /* sigaction */
#include <stddef.h>    /* ptrdiff_t */
#include <stdio.h>     /* snprintf */
#include <stdlib.h>    /* malloc, free */
#include <string.h>    /* strdup, strerror, memset */
#include <sys/stat.h>  /* open mode */
#include <sys/time.h>  /* struct timeval */
#include <sys/types.h> /* pid_t, fd_set */
#include <sys/wait.h>  /* waitpid */
#include <time.h>      /* gettimeofday */
#include <unistd.h>    /* pipe, close, fork, execvp, select, _exit */

#if defined(__VMS)
#  define KWSYSPE_VMS_NONBLOCK , O_NONBLOCK
#else
#  define KWSYSPE_VMS_NONBLOCK
#endif

#if defined(KWSYS_C_HAS_PTRDIFF_T) && KWSYS_C_HAS_PTRDIFF_T
typedef ptrdiff_t kwsysProcess_ptrdiff_t;
#else
typedef int kwsysProcess_ptrdiff_t;
#endif

#if defined(KWSYS_C_HAS_SSIZE_T) && KWSYS_C_HAS_SSIZE_T
typedef ssize_t kwsysProcess_ssize_t;
#else
typedef int kwsysProcess_ssize_t;
#endif

#if defined(__BEOS__) && !defined(__ZETA__)
/* BeOS 5 doesn't have usleep(), but it has snooze(), which is identical. */
#  include <be/kernel/OS.h>
static inline void kwsysProcess_usleep(unsigned int msec)
{
  snooze(msec);
}
#else
#  define kwsysProcess_usleep usleep
#endif

/*
 * BeOS's select() works like WinSock: it's for networking only, and
 * doesn't work with Unix file handles...socket and file handles are
 * different namespaces (the same descriptor means different things in
 * each context!)
 *
 * So on Unix-like systems where select() is flakey, we'll set the
 * pipes' file handles to be non-blocking and just poll them directly
 * without select().
 */
#if !defined(__BEOS__) && !defined(__VMS) && !defined(__MINT__) &&            \
  !defined(KWSYSPE_USE_SELECT)
#  define KWSYSPE_USE_SELECT 1
#endif

/* Some platforms do not have siginfo on their signal handlers.  */
#if defined(SA_SIGINFO) && !defined(__BEOS__)
#  define KWSYSPE_USE_SIGINFO 1
#endif

/* The number of pipes for the child's output.  The standard stdout
   and stderr pipes are the first two.  One more pipe is used to
   detect when the child process has terminated.  The third pipe is
   not given to the child process, so it cannot close it until it
   terminates.  */
#define KWSYSPE_PIPE_COUNT 3
#define KWSYSPE_PIPE_STDOUT 0
#define KWSYSPE_PIPE_STDERR 1
#define KWSYSPE_PIPE_SIGNAL 2

/* The maximum amount to read from a pipe at a time.  */
#define KWSYSPE_PIPE_BUFFER_SIZE 1024

/* Keep track of times using a signed representation.  Switch to the
   native (possibly unsigned) representation only when calling native
   functions.  */
typedef struct timeval kwsysProcessTimeNative;
typedef struct kwsysProcessTime_s kwsysProcessTime;
struct kwsysProcessTime_s
{
  long tv_sec;
  long tv_usec;
};

typedef struct kwsysProcessCreateInformation_s
{
  int StdIn;
  int StdOut;
  int StdErr;
  int ErrorPipe[2];
} kwsysProcessCreateInformation;

static void kwsysProcessVolatileFree(volatile void* p);
static int kwsysProcessInitialize(kwsysProcess* cp);
static void kwsysProcessCleanup(kwsysProcess* cp, int error);
static void kwsysProcessCleanupDescriptor(int* pfd);
static void kwsysProcessClosePipes(kwsysProcess* cp);
static int kwsysProcessSetNonBlocking(int fd);
static int kwsysProcessCreate(kwsysProcess* cp, int prIndex,
                              kwsysProcessCreateInformation* si);
static void kwsysProcessDestroy(kwsysProcess* cp);
static int kwsysProcessSetupOutputPipeFile(int* p, const char* name);
static int kwsysProcessSetupOutputPipeNative(int* p, int des[2]);
static int kwsysProcessGetTimeoutTime(kwsysProcess* cp, double* userTimeout,
                                      kwsysProcessTime* timeoutTime);
static int kwsysProcessGetTimeoutLeft(kwsysProcessTime* timeoutTime,
                                      double* userTimeout,
                                      kwsysProcessTimeNative* timeoutLength,
                                      int zeroIsExpired);
static kwsysProcessTime kwsysProcessTimeGetCurrent(void);
static double kwsysProcessTimeToDouble(kwsysProcessTime t);
static kwsysProcessTime kwsysProcessTimeFromDouble(double d);
static int kwsysProcessTimeLess(kwsysProcessTime in1, kwsysProcessTime in2);
static kwsysProcessTime kwsysProcessTimeAdd(kwsysProcessTime in1,
                                            kwsysProcessTime in2);
static kwsysProcessTime kwsysProcessTimeSubtract(kwsysProcessTime in1,
                                                 kwsysProcessTime in2);
static void kwsysProcessSetExitExceptionByIndex(kwsysProcess* cp, int sig,
                                                int idx);
static void kwsysProcessChildErrorExit(int errorPipe);
static void kwsysProcessRestoreDefaultSignalHandlers(void);
static pid_t kwsysProcessFork(kwsysProcess* cp,
                              kwsysProcessCreateInformation* si);
static void kwsysProcessKill(pid_t process_id);
#if defined(__VMS)
static int kwsysProcessSetVMSFeature(const char* name, int value);
#endif
static int kwsysProcessesAdd(kwsysProcess* cp);
static void kwsysProcessesRemove(kwsysProcess* cp);
#if KWSYSPE_USE_SIGINFO
static void kwsysProcessesSignalHandler(int signum, siginfo_t* info,
                                        void* ucontext);
#else
static void kwsysProcessesSignalHandler(int signum);
#endif

/* A structure containing results data for each process.  */
typedef struct kwsysProcessResults_s kwsysProcessResults;
struct kwsysProcessResults_s
{
  /* The status of the child process. */
  int State;

  /* The exceptional behavior that terminated the process, if any.  */
  int ExitException;

  /* The process exit code.  */
  int ExitCode;

  /* The process return code, if any.  */
  int ExitValue;

  /* Description for the ExitException.  */
  char ExitExceptionString[KWSYSPE_PIPE_BUFFER_SIZE + 1];
};

/* Structure containing data used to implement the child's execution.  */
struct kwsysProcess_s
{
  /* The command lines to execute.  */
  char*** Commands;
  volatile int NumberOfCommands;

  /* Descriptors for the read ends of the child's output pipes and
     the signal pipe. */
  int PipeReadEnds[KWSYSPE_PIPE_COUNT];

  /* Descriptors for the child's ends of the pipes.
     Used temporarily during process creation.  */
  int PipeChildStd[3];

  /* Write descriptor for child termination signal pipe.  */
  int SignalPipe;

  /* Buffer for pipe data.  */
  char PipeBuffer[KWSYSPE_PIPE_BUFFER_SIZE];

  /* Process IDs returned by the calls to fork.  Everything is volatile
     because the signal handler accesses them.  You must be very careful
     when reaping PIDs or modifying this array to avoid race conditions.  */
  volatile pid_t* volatile ForkPIDs;

  /* Flag for whether the children were terminated by a failed select.  */
  int SelectError;

  /* The timeout length.  */
  double Timeout;

  /* The working directory for the process. */
  char* WorkingDirectory;

  /* Whether to create the child as a detached process.  */
  int OptionDetach;

  /* Whether the child was created as a detached process.  */
  int Detached;

  /* Whether to treat command lines as verbatim.  */
  int Verbatim;

  /* Whether to merge stdout/stderr of the child.  */
  int MergeOutput;

  /* Whether to create the process in a new process group.  */
  volatile sig_atomic_t CreateProcessGroup;

  /* Time at which the child started.  Negative for no timeout.  */
  kwsysProcessTime StartTime;

  /* Time at which the child will timeout.  Negative for no timeout.  */
  kwsysProcessTime TimeoutTime;

  /* Flag for whether the timeout expired.  */
  int TimeoutExpired;

  /* The number of pipes left open during execution.  */
  int PipesLeft;

#if KWSYSPE_USE_SELECT
  /* File descriptor set for call to select.  */
  fd_set PipeSet;
#endif

  /* The number of children still executing.  */
  int CommandsLeft;

  /* The status of the process structure.  Must be atomic because
     the signal handler checks this to avoid a race.  */
  volatile sig_atomic_t State;

  /* Whether the process was killed.  */
  volatile sig_atomic_t Killed;

  /* Buffer for error message in case of failure.  */
  char ErrorMessage[KWSYSPE_PIPE_BUFFER_SIZE + 1];

  /* process results.  */
  kwsysProcessResults* ProcessResults;

  /* The exit codes of each child process in the pipeline.  */
  int* CommandExitCodes;

  /* Name of files to which stdin and stdout pipes are attached.  */
  char* PipeFileSTDIN;
  char* PipeFileSTDOUT;
  char* PipeFileSTDERR;

  /* Whether each pipe is shared with the parent process.  */
  int PipeSharedSTDIN;
  int PipeSharedSTDOUT;
  int PipeSharedSTDERR;

  /* Native pipes provided by the user.  */
  int PipeNativeSTDIN[2];
  int PipeNativeSTDOUT[2];
  int PipeNativeSTDERR[2];

  /* The real working directory of this process.  */
  int RealWorkingDirectoryLength;
  char* RealWorkingDirectory;
};

kwsysProcess* kwsysProcess_New(void)
{
  /* Allocate a process control structure.  */
  kwsysProcess* cp = (kwsysProcess*)malloc(sizeof(kwsysProcess));
  if (!cp) {
    return 0;
  }
  memset(cp, 0, sizeof(kwsysProcess));

  /* Share stdin with the parent process by default.  */
  cp->PipeSharedSTDIN = 1;

  /* No native pipes by default.  */
  cp->PipeNativeSTDIN[0] = -1;
  cp->PipeNativeSTDIN[1] = -1;
  cp->PipeNativeSTDOUT[0] = -1;
  cp->PipeNativeSTDOUT[1] = -1;
  cp->PipeNativeSTDERR[0] = -1;
  cp->PipeNativeSTDERR[1] = -1;

  /* Set initial status.  */
  cp->State = kwsysProcess_State_Starting;

  return cp;
}

void kwsysProcess_Delete(kwsysProcess* cp)
{
  /* Make sure we have an instance.  */
  if (!cp) {
    return;
  }

  /* If the process is executing, wait for it to finish.  */
  if (cp->State == kwsysProcess_State_Executing) {
    if (cp->Detached) {
      kwsysProcess_Disown(cp);
    } else {
      kwsysProcess_WaitForExit(cp, 0);
    }
  }

  /* Free memory.  */
  kwsysProcess_SetCommand(cp, 0);
  kwsysProcess_SetWorkingDirectory(cp, 0);
  kwsysProcess_SetPipeFile(cp, kwsysProcess_Pipe_STDIN, 0);
  kwsysProcess_SetPipeFile(cp, kwsysProcess_Pipe_STDOUT, 0);
  kwsysProcess_SetPipeFile(cp, kwsysProcess_Pipe_STDERR, 0);
  free(cp->CommandExitCodes);
  free(cp->ProcessResults);
  free(cp);
}

int kwsysProcess_SetCommand(kwsysProcess* cp, char const* const* command)
{
  int i;
  if (!cp) {
    return 0;
  }
  for (i = 0; i < cp->NumberOfCommands; ++i) {
    char** c = cp->Commands[i];
    while (*c) {
      free(*c++);
    }
    free(cp->Commands[i]);
  }
  cp->NumberOfCommands = 0;
  if (cp->Commands) {
    free(cp->Commands);
    cp->Commands = 0;
  }
  if (command) {
    return kwsysProcess_AddCommand(cp, command);
  }
  return 1;
}

int kwsysProcess_AddCommand(kwsysProcess* cp, char const* const* command)
{
  int newNumberOfCommands;
  char*** newCommands;

  /* Make sure we have a command to add.  */
  if (!cp || !command || !*command) {
    return 0;
  }

  /* Allocate a new array for command pointers.  */
  newNumberOfCommands = cp->NumberOfCommands + 1;
  if (!(newCommands =
          (char***)malloc(sizeof(char**) * (size_t)(newNumberOfCommands)))) {
    /* Out of memory.  */
    return 0;
  }

  /* Copy any existing commands into the new array.  */
  {
    int i;
    for (i = 0; i < cp->NumberOfCommands; ++i) {
      newCommands[i] = cp->Commands[i];
    }
  }

  /* Add the new command.  */
  if (cp->Verbatim) {
    /* In order to run the given command line verbatim we need to
       parse it.  */
    newCommands[cp->NumberOfCommands] =
      kwsysSystem_Parse_CommandForUnix(*command, 0);
    if (!newCommands[cp->NumberOfCommands] ||
        !newCommands[cp->NumberOfCommands][0]) {
      /* Out of memory or no command parsed.  */
      free(newCommands);
      return 0;
    }
  } else {
    /* Copy each argument string individually.  */
    char const* const* c = command;
    kwsysProcess_ptrdiff_t n = 0;
    kwsysProcess_ptrdiff_t i = 0;
    while (*c++)
      ;
    n = c - command - 1;
    newCommands[cp->NumberOfCommands] =
      (char**)malloc((size_t)(n + 1) * sizeof(char*));
    if (!newCommands[cp->NumberOfCommands]) {
      /* Out of memory.  */
      free(newCommands);
      return 0;
    }
    for (i = 0; i < n; ++i) {
      assert(command[i]); /* Quiet Clang scan-build. */
      newCommands[cp->NumberOfCommands][i] = strdup(command[i]);
      if (!newCommands[cp->NumberOfCommands][i]) {
        break;
      }
    }
    if (i < n) {
      /* Out of memory.  */
      for (; i > 0; --i) {
        free(newCommands[cp->NumberOfCommands][i - 1]);
      }
      free(newCommands);
      return 0;
    }
    newCommands[cp->NumberOfCommands][n] = 0;
  }

  /* Successfully allocated new command array.  Free the old array. */
  free(cp->Commands);
  cp->Commands = newCommands;
  cp->NumberOfCommands = newNumberOfCommands;

  return 1;
}

void kwsysProcess_SetTimeout(kwsysProcess* cp, double timeout)
{
  if (!cp) {
    return;
  }
  cp->Timeout = timeout;
  if (cp->Timeout < 0) {
    cp->Timeout = 0;
  }
  // Force recomputation of TimeoutTime.
  cp->TimeoutTime.tv_sec = -1;
}

int kwsysProcess_SetWorkingDirectory(kwsysProcess* cp, const char* dir)
{
  if (!cp) {
    return 0;
  }
  if (cp->WorkingDirectory == dir) {
    return 1;
  }
  if (cp->WorkingDirectory && dir && strcmp(cp->WorkingDirectory, dir) == 0) {
    return 1;
  }
  if (cp->WorkingDirectory) {
    free(cp->WorkingDirectory);
    cp->WorkingDirectory = 0;
  }
  if (dir) {
    cp->WorkingDirectory = strdup(dir);
    if (!cp->WorkingDirectory) {
      return 0;
    }
  }
  return 1;
}

int kwsysProcess_SetPipeFile(kwsysProcess* cp, int prPipe, const char* file)
{
  char** pfile;
  if (!cp) {
    return 0;
  }
  switch (prPipe) {
    case kwsysProcess_Pipe_STDIN:
      pfile = &cp->PipeFileSTDIN;
      break;
    case kwsysProcess_Pipe_STDOUT:
      pfile = &cp->PipeFileSTDOUT;
      break;
    case kwsysProcess_Pipe_STDERR:
      pfile = &cp->PipeFileSTDERR;
      break;
    default:
      return 0;
  }
  if (*pfile) {
    free(*pfile);
    *pfile = 0;
  }
  if (file) {
    *pfile = strdup(file);
    if (!*pfile) {
      return 0;
    }
  }

  /* If we are redirecting the pipe, do not share it or use a native
     pipe.  */
  if (*pfile) {
    kwsysProcess_SetPipeNative(cp, prPipe, 0);
    kwsysProcess_SetPipeShared(cp, prPipe, 0);
  }
  return 1;
}

void kwsysProcess_SetPipeShared(kwsysProcess* cp, int prPipe, int shared)
{
  if (!cp) {
    return;
  }

  switch (prPipe) {
    case kwsysProcess_Pipe_STDIN:
      cp->PipeSharedSTDIN = shared ? 1 : 0;
      break;
    case kwsysProcess_Pipe_STDOUT:
      cp->PipeSharedSTDOUT = shared ? 1 : 0;
      break;
    case kwsysProcess_Pipe_STDERR:
      cp->PipeSharedSTDERR = shared ? 1 : 0;
      break;
    default:
      return;
  }

  /* If we are sharing the pipe, do not redirect it to a file or use a
     native pipe.  */
  if (shared) {
    kwsysProcess_SetPipeFile(cp, prPipe, 0);
    kwsysProcess_SetPipeNative(cp, prPipe, 0);
  }
}

void kwsysProcess_SetPipeNative(kwsysProcess* cp, int prPipe, int p[2])
{
  int* pPipeNative = 0;

  if (!cp) {
    return;
  }

  switch (prPipe) {
    case kwsysProcess_Pipe_STDIN:
      pPipeNative = cp->PipeNativeSTDIN;
      break;
    case kwsysProcess_Pipe_STDOUT:
      pPipeNative = cp->PipeNativeSTDOUT;
      break;
    case kwsysProcess_Pipe_STDERR:
      pPipeNative = cp->PipeNativeSTDERR;
      break;
    default:
      return;
  }

  /* Copy the native pipe descriptors provided.  */
  if (p) {
    pPipeNative[0] = p[0];
    pPipeNative[1] = p[1];
  } else {
    pPipeNative[0] = -1;
    pPipeNative[1] = -1;
  }

  /* If we are using a native pipe, do not share it or redirect it to
     a file.  */
  if (p) {
    kwsysProcess_SetPipeFile(cp, prPipe, 0);
    kwsysProcess_SetPipeShared(cp, prPipe, 0);
  }
}

int kwsysProcess_GetOption(kwsysProcess* cp, int optionId)
{
  if (!cp) {
    return 0;
  }

  switch (optionId) {
    case kwsysProcess_Option_Detach:
      return cp->OptionDetach;
    case kwsysProcess_Option_MergeOutput:
      return cp->MergeOutput;
    case kwsysProcess_Option_Verbatim:
      return cp->Verbatim;
    case kwsysProcess_Option_CreateProcessGroup:
      return cp->CreateProcessGroup;
    default:
      return 0;
  }
}

void kwsysProcess_SetOption(kwsysProcess* cp, int optionId, int value)
{
  if (!cp) {
    return;
  }

  switch (optionId) {
    case kwsysProcess_Option_Detach:
      cp->OptionDetach = value;
      break;
    case kwsysProcess_Option_MergeOutput:
      cp->MergeOutput = value;
      break;
    case kwsysProcess_Option_Verbatim:
      cp->Verbatim = value;
      break;
    case kwsysProcess_Option_CreateProcessGroup:
      cp->CreateProcessGroup = value;
      break;
    default:
      break;
  }
}

int kwsysProcess_GetState(kwsysProcess* cp)
{
  return cp ? cp->State : kwsysProcess_State_Error;
}

int kwsysProcess_GetExitException(kwsysProcess* cp)
{
  return (cp && cp->ProcessResults && (cp->NumberOfCommands > 0))
    ? cp->ProcessResults[cp->NumberOfCommands - 1].ExitException
    : kwsysProcess_Exception_Other;
}

int kwsysProcess_GetExitCode(kwsysProcess* cp)
{
  return (cp && cp->ProcessResults && (cp->NumberOfCommands > 0))
    ? cp->ProcessResults[cp->NumberOfCommands - 1].ExitCode
    : 0;
}

int kwsysProcess_GetExitValue(kwsysProcess* cp)
{
  return (cp && cp->ProcessResults && (cp->NumberOfCommands > 0))
    ? cp->ProcessResults[cp->NumberOfCommands - 1].ExitValue
    : -1;
}

const char* kwsysProcess_GetErrorString(kwsysProcess* cp)
{
  if (!cp) {
    return "Process management structure could not be allocated";
  } else if (cp->State == kwsysProcess_State_Error) {
    return cp->ErrorMessage;
  }
  return "Success";
}

const char* kwsysProcess_GetExceptionString(kwsysProcess* cp)
{
  if (!(cp && cp->ProcessResults && (cp->NumberOfCommands > 0))) {
    return "GetExceptionString called with NULL process management structure";
  } else if (cp->State == kwsysProcess_State_Exception) {
    return cp->ProcessResults[cp->NumberOfCommands - 1].ExitExceptionString;
  }
  return "No exception";
}

/* the index should be in array bound. */
#define KWSYSPE_IDX_CHK(RET)                                                  \
  if (!cp || idx >= cp->NumberOfCommands || idx < 0) {                        \
    return RET;                                                               \
  }

int kwsysProcess_GetStateByIndex(kwsysProcess* cp, int idx)
{
  KWSYSPE_IDX_CHK(kwsysProcess_State_Error)
  return cp->ProcessResults[idx].State;
}

int kwsysProcess_GetExitExceptionByIndex(kwsysProcess* cp, int idx)
{
  KWSYSPE_IDX_CHK(kwsysProcess_Exception_Other)
  return cp->ProcessResults[idx].ExitException;
}

int kwsysProcess_GetExitValueByIndex(kwsysProcess* cp, int idx)
{
  KWSYSPE_IDX_CHK(-1)
  return cp->ProcessResults[idx].ExitValue;
}

int kwsysProcess_GetExitCodeByIndex(kwsysProcess* cp, int idx)
{
  KWSYSPE_IDX_CHK(-1)
  return cp->CommandExitCodes[idx];
}

const char* kwsysProcess_GetExceptionStringByIndex(kwsysProcess* cp, int idx)
{
  KWSYSPE_IDX_CHK("GetExceptionString called with NULL process management "
                  "structure or index out of bound")
  if (cp->ProcessResults[idx].State == kwsysProcess_StateByIndex_Exception) {
    return cp->ProcessResults[idx].ExitExceptionString;
  }
  return "No exception";
}

#undef KWSYSPE_IDX_CHK

void kwsysProcess_Execute(kwsysProcess* cp)
{
  int i;

  /* Do not execute a second copy simultaneously.  */
  if (!cp || cp->State == kwsysProcess_State_Executing) {
    return;
  }

  /* Make sure we have something to run.  */
  if (cp->NumberOfCommands < 1) {
    strcpy(cp->ErrorMessage, "No command");
    cp->State = kwsysProcess_State_Error;
    return;
  }

  /* Initialize the control structure for a new process.  */
  if (!kwsysProcessInitialize(cp)) {
    strcpy(cp->ErrorMessage, "Out of memory");
    cp->State = kwsysProcess_State_Error;
    return;
  }

#if defined(__VMS)
  /* Make sure pipes behave like streams on VMS.  */
  if (!kwsysProcessSetVMSFeature("DECC$STREAM_PIPE", 1)) {
    kwsysProcessCleanup(cp, 1);
    return;
  }
#endif

  /* Save the real working directory of this process and change to
     the working directory for the child processes.  This is needed
     to make pipe file paths evaluate correctly.  */
  if (cp->WorkingDirectory) {
    int r;
    if (!getcwd(cp->RealWorkingDirectory,
                (size_t)(cp->RealWorkingDirectoryLength))) {
      kwsysProcessCleanup(cp, 1);
      return;
    }

    /* Some platforms specify that the chdir call may be
       interrupted.  Repeat the call until it finishes.  */
    while (((r = chdir(cp->WorkingDirectory)) < 0) && (errno == EINTR))
      ;
    if (r < 0) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  }

  /* If not running a detached child, add this object to the global
     set of process objects that wish to be notified when a child
     exits.  */
  if (!cp->OptionDetach) {
    if (!kwsysProcessesAdd(cp)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  }

  /* Setup the stdin pipe for the first process.  */
  if (cp->PipeFileSTDIN) {
    /* Open a file for the child's stdin to read.  */
    cp->PipeChildStd[0] = open(cp->PipeFileSTDIN, O_RDONLY);
    if (cp->PipeChildStd[0] < 0) {
      kwsysProcessCleanup(cp, 1);
      return;
    }

    /* Set close-on-exec flag on the pipe's end.  */
    if (fcntl(cp->PipeChildStd[0], F_SETFD, FD_CLOEXEC) < 0) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  } else if (cp->PipeSharedSTDIN) {
    cp->PipeChildStd[0] = 0;
  } else if (cp->PipeNativeSTDIN[0] >= 0) {
    cp->PipeChildStd[0] = cp->PipeNativeSTDIN[0];

    /* Set close-on-exec flag on the pipe's ends.  The read end will
       be dup2-ed into the stdin descriptor after the fork but before
       the exec.  */
    if ((fcntl(cp->PipeNativeSTDIN[0], F_SETFD, FD_CLOEXEC) < 0) ||
        (fcntl(cp->PipeNativeSTDIN[1], F_SETFD, FD_CLOEXEC) < 0)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  } else {
    cp->PipeChildStd[0] = -1;
  }

  /* Create the output pipe for the last process.
     We always create this so the pipe can be passed to select even if
     it will report closed immediately.  */
  {
    /* Create the pipe.  */
    int p[2];
    if (pipe(p KWSYSPE_VMS_NONBLOCK) < 0) {
      kwsysProcessCleanup(cp, 1);
      return;
    }

    /* Store the pipe.  */
    cp->PipeReadEnds[KWSYSPE_PIPE_STDOUT] = p[0];
    cp->PipeChildStd[1] = p[1];

    /* Set close-on-exec flag on the pipe's ends.  */
    if ((fcntl(p[0], F_SETFD, FD_CLOEXEC) < 0) ||
        (fcntl(p[1], F_SETFD, FD_CLOEXEC) < 0)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }

    /* Set to non-blocking in case select lies, or for the polling
       implementation.  */
    if (!kwsysProcessSetNonBlocking(p[0])) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  }

  if (cp->PipeFileSTDOUT) {
    /* Use a file for stdout.  */
    if (!kwsysProcessSetupOutputPipeFile(&cp->PipeChildStd[1],
                                         cp->PipeFileSTDOUT)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  } else if (cp->PipeSharedSTDOUT) {
    /* Use the parent stdout.  */
    kwsysProcessCleanupDescriptor(&cp->PipeChildStd[1]);
    cp->PipeChildStd[1] = 1;
  } else if (cp->PipeNativeSTDOUT[1] >= 0) {
    /* Use the given descriptor for stdout.  */
    if (!kwsysProcessSetupOutputPipeNative(&cp->PipeChildStd[1],
                                           cp->PipeNativeSTDOUT)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  }

  /* Create stderr pipe to be shared by all processes in the pipeline.
     We always create this so the pipe can be passed to select even if
     it will report closed immediately.  */
  {
    /* Create the pipe.  */
    int p[2];
    if (pipe(p KWSYSPE_VMS_NONBLOCK) < 0) {
      kwsysProcessCleanup(cp, 1);
      return;
    }

    /* Store the pipe.  */
    cp->PipeReadEnds[KWSYSPE_PIPE_STDERR] = p[0];
    cp->PipeChildStd[2] = p[1];

    /* Set close-on-exec flag on the pipe's ends.  */
    if ((fcntl(p[0], F_SETFD, FD_CLOEXEC) < 0) ||
        (fcntl(p[1], F_SETFD, FD_CLOEXEC) < 0)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }

    /* Set to non-blocking in case select lies, or for the polling
       implementation.  */
    if (!kwsysProcessSetNonBlocking(p[0])) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  }

  if (cp->PipeFileSTDERR) {
    /* Use a file for stderr.  */
    if (!kwsysProcessSetupOutputPipeFile(&cp->PipeChildStd[2],
                                         cp->PipeFileSTDERR)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  } else if (cp->PipeSharedSTDERR) {
    /* Use the parent stderr.  */
    kwsysProcessCleanupDescriptor(&cp->PipeChildStd[2]);
    cp->PipeChildStd[2] = 2;
  } else if (cp->PipeNativeSTDERR[1] >= 0) {
    /* Use the given handle for stderr.  */
    if (!kwsysProcessSetupOutputPipeNative(&cp->PipeChildStd[2],
                                           cp->PipeNativeSTDERR)) {
      kwsysProcessCleanup(cp, 1);
      return;
    }
  }

  /* The timeout period starts now.  */
  cp->StartTime = kwsysProcessTimeGetCurrent();
  cp->TimeoutTime.tv_sec = -1;
  cp->TimeoutTime.tv_usec = -1;

  /* Create the pipeline of processes.  */
  {
    kwsysProcessCreateInformation si = { -1, -1, -1, { -1, -1 } };
    int nextStdIn = cp->PipeChildStd[0];
    for (i = 0; i < cp->NumberOfCommands; ++i) {
      /* Setup the process's pipes.  */
      si.StdIn = nextStdIn;
      if (i == cp->NumberOfCommands - 1) {
        nextStdIn = -1;
        si.StdOut = cp->PipeChildStd[1];
      } else {
        /* Create a pipe to sit between the children.  */
        int p[2] = { -1, -1 };
        if (pipe(p KWSYSPE_VMS_NONBLOCK) < 0) {
          if (nextStdIn != cp->PipeChildStd[0]) {
            kwsysProcessCleanupDescriptor(&nextStdIn);
          }
          kwsysProcessCleanup(cp, 1);
          return;
        }

        /* Set close-on-exec flag on the pipe's ends.  */
        if ((fcntl(p[0], F_SETFD, FD_CLOEXEC) < 0) ||
            (fcntl(p[1], F_SETFD, FD_CLOEXEC) < 0)) {
          close(p[0]);
          close(p[1]);
          if (nextStdIn != cp->PipeChildStd[0]) {
            kwsysProcessCleanupDescriptor(&nextStdIn);
          }
          kwsysProcessCleanup(cp, 1);
          return;
        }
        nextStdIn = p[0];
        si.StdOut = p[1];
      }
      si.StdErr = cp->MergeOutput ? cp->PipeChildStd[1] : cp->PipeChildStd[2];

      {
        int res = kwsysProcessCreate(cp, i, &si);

        /* Close our copies of pipes used between children.  */
        if (si.StdIn != cp->PipeChildStd[0]) {
          kwsysProcessCleanupDescriptor(&si.StdIn);
        }
        if (si.StdOut != cp->PipeChildStd[1]) {
          kwsysProcessCleanupDescriptor(&si.StdOut);
        }
        if (si.StdErr != cp->PipeChildStd[2] && !cp->MergeOutput) {
          kwsysProcessCleanupDescriptor(&si.StdErr);
        }

        if (!res) {
          kwsysProcessCleanupDescriptor(&si.ErrorPipe[0]);
          kwsysProcessCleanupDescriptor(&si.ErrorPipe[1]);
          if (nextStdIn != cp->PipeChildStd[0]) {
            kwsysProcessCleanupDescriptor(&nextStdIn);
          }
          kwsysProcessCleanup(cp, 1);
          return;
        }
      }
    }
  }

  /* The parent process does not need the child's pipe ends.  */
  for (i = 0; i < 3; ++i) {
    kwsysProcessCleanupDescriptor(&cp->PipeChildStd[i]);
  }

  /* Restore the working directory. */
  if (cp->RealWorkingDirectory) {
    /* Some platforms specify that the chdir call may be
       interrupted.  Repeat the call until it finishes.  */
    while ((chdir(cp->RealWorkingDirectory) < 0) && (errno == EINTR))
      ;
    free(cp->RealWorkingDirectory);
    cp->RealWorkingDirectory = 0;
  }

  /* All the pipes are now open.  */
  cp->PipesLeft = KWSYSPE_PIPE_COUNT;

  /* The process has now started.  */
  cp->State = kwsysProcess_State_Executing;
  cp->Detached = cp->OptionDetach;
}

kwsysEXPORT void kwsysProcess_Disown(kwsysProcess* cp)
{
  /* Make sure a detached child process is running.  */
  if (!cp || !cp->Detached || cp->State != kwsysProcess_State_Executing ||
      cp->TimeoutExpired || cp->Killed) {
    return;
  }

  /* Close all the pipes safely.  */
  kwsysProcessClosePipes(cp);

  /* We will not wait for exit, so cleanup now.  */
  kwsysProcessCleanup(cp, 0);

  /* The process has been disowned.  */
  cp->State = kwsysProcess_State_Disowned;
}

typedef struct kwsysProcessWaitData_s
{
  int Expired;
  int PipeId;
  int User;
  double* UserTimeout;
  kwsysProcessTime TimeoutTime;
} kwsysProcessWaitData;
static int kwsysProcessWaitForPipe(kwsysProcess* cp, char** data, int* length,
                                   kwsysProcessWaitData* wd);

int kwsysProcess_WaitForData(kwsysProcess* cp, char** data, int* length,
                             double* userTimeout)
{
  kwsysProcessTime userStartTime = { 0, 0 };
  kwsysProcessWaitData wd = { 0, kwsysProcess_Pipe_None, 0, 0, { 0, 0 } };
  wd.UserTimeout = userTimeout;
  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing || cp->Killed ||
      cp->TimeoutExpired) {
    return kwsysProcess_Pipe_None;
  }

  /* Record the time at which user timeout period starts.  */
  if (userTimeout) {
    userStartTime = kwsysProcessTimeGetCurrent();
  }

  /* Calculate the time at which a timeout will expire, and whether it
     is the user or process timeout.  */
  wd.User = kwsysProcessGetTimeoutTime(cp, userTimeout, &wd.TimeoutTime);

  /* Data can only be available when pipes are open.  If the process
     is not running, cp->PipesLeft will be 0.  */
  while (cp->PipesLeft > 0 &&
         !kwsysProcessWaitForPipe(cp, data, length, &wd)) {
  }

  /* Update the user timeout.  */
  if (userTimeout) {
    kwsysProcessTime userEndTime = kwsysProcessTimeGetCurrent();
    kwsysProcessTime difference =
      kwsysProcessTimeSubtract(userEndTime, userStartTime);
    double d = kwsysProcessTimeToDouble(difference);
    *userTimeout -= d;
    if (*userTimeout < 0) {
      *userTimeout = 0;
    }
  }

  /* Check what happened.  */
  if (wd.PipeId) {
    /* Data are ready on a pipe.  */
    return wd.PipeId;
  } else if (wd.Expired) {
    /* A timeout has expired.  */
    if (wd.User) {
      /* The user timeout has expired.  It has no time left.  */
      return kwsysProcess_Pipe_Timeout;
    } else {
      /* The process timeout has expired.  Kill the children now.  */
      kwsysProcess_Kill(cp);
      cp->Killed = 0;
      cp->TimeoutExpired = 1;
      return kwsysProcess_Pipe_None;
    }
  } else {
    /* No pipes are left open.  */
    return kwsysProcess_Pipe_None;
  }
}

static int kwsysProcessWaitForPipe(kwsysProcess* cp, char** data, int* length,
                                   kwsysProcessWaitData* wd)
{
  int i;
  kwsysProcessTimeNative timeoutLength;

#if KWSYSPE_USE_SELECT
  int numReady = 0;
  int max = -1;
  kwsysProcessTimeNative* timeout = 0;

  /* Check for any open pipes with data reported ready by the last
     call to select.  According to "man select_tut" we must deal
     with all descriptors reported by a call to select before
     passing them to another select call.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    if (cp->PipeReadEnds[i] >= 0 &&
        FD_ISSET(cp->PipeReadEnds[i], &cp->PipeSet)) {
      kwsysProcess_ssize_t n;

      /* We are handling this pipe now.  Remove it from the set.  */
      FD_CLR(cp->PipeReadEnds[i], &cp->PipeSet);

      /* The pipe is ready to read without blocking.  Keep trying to
         read until the operation is not interrupted.  */
      while (((n = read(cp->PipeReadEnds[i], cp->PipeBuffer,
                        KWSYSPE_PIPE_BUFFER_SIZE)) < 0) &&
             (errno == EINTR))
        ;
      if (n > 0) {
        /* We have data on this pipe.  */
        if (i == KWSYSPE_PIPE_SIGNAL) {
          /* A child process has terminated.  */
          kwsysProcessDestroy(cp);
        } else if (data && length) {
          /* Report this data.  */
          *data = cp->PipeBuffer;
          *length = (int)(n);
          switch (i) {
            case KWSYSPE_PIPE_STDOUT:
              wd->PipeId = kwsysProcess_Pipe_STDOUT;
              break;
            case KWSYSPE_PIPE_STDERR:
              wd->PipeId = kwsysProcess_Pipe_STDERR;
              break;
          };
          return 1;
        }
      } else if (n < 0 && errno == EAGAIN) {
        /* No data are really ready.  The select call lied.  See the
           "man select" page on Linux for cases when this occurs.  */
      } else {
        /* We are done reading from this pipe.  */
        kwsysProcessCleanupDescriptor(&cp->PipeReadEnds[i]);
        --cp->PipesLeft;
      }
    }
  }

  /* If we have data, break early.  */
  if (wd->PipeId) {
    return 1;
  }

  /* Make sure the set is empty (it should always be empty here
     anyway).  */
  FD_ZERO(&cp->PipeSet);

  /* Setup a timeout if required.  */
  if (wd->TimeoutTime.tv_sec < 0) {
    timeout = 0;
  } else {
    timeout = &timeoutLength;
  }
  if (kwsysProcessGetTimeoutLeft(
        &wd->TimeoutTime, wd->User ? wd->UserTimeout : 0, &timeoutLength, 0)) {
    /* Timeout has already expired.  */
    wd->Expired = 1;
    return 1;
  }

  /* Add the pipe reading ends that are still open.  */
  max = -1;
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    if (cp->PipeReadEnds[i] >= 0) {
      FD_SET(cp->PipeReadEnds[i], &cp->PipeSet);
      if (cp->PipeReadEnds[i] > max) {
        max = cp->PipeReadEnds[i];
      }
    }
  }

  /* Make sure we have a non-empty set.  */
  if (max < 0) {
    /* All pipes have closed.  Child has terminated.  */
    return 1;
  }

  /* Run select to block until data are available.  Repeat call
     until it is not interrupted.  */
  while (((numReady = select(max + 1, &cp->PipeSet, 0, 0, timeout)) < 0) &&
         (errno == EINTR))
    ;

  /* Check result of select.  */
  if (numReady == 0) {
    /* Select's timeout expired.  */
    wd->Expired = 1;
    return 1;
  } else if (numReady < 0) {
    /* Select returned an error.  Leave the error description in the
       pipe buffer.  */
    strncpy(cp->ErrorMessage, strerror(errno), KWSYSPE_PIPE_BUFFER_SIZE);

    /* Kill the children now.  */
    kwsysProcess_Kill(cp);
    cp->Killed = 0;
    cp->SelectError = 1;
  }

  return 0;
#else
  /* Poll pipes for data since we do not have select.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    if (cp->PipeReadEnds[i] >= 0) {
      const int fd = cp->PipeReadEnds[i];
      int n = read(fd, cp->PipeBuffer, KWSYSPE_PIPE_BUFFER_SIZE);
      if (n > 0) {
        /* We have data on this pipe.  */
        if (i == KWSYSPE_PIPE_SIGNAL) {
          /* A child process has terminated.  */
          kwsysProcessDestroy(cp);
        } else if (data && length) {
          /* Report this data.  */
          *data = cp->PipeBuffer;
          *length = n;
          switch (i) {
            case KWSYSPE_PIPE_STDOUT:
              wd->PipeId = kwsysProcess_Pipe_STDOUT;
              break;
            case KWSYSPE_PIPE_STDERR:
              wd->PipeId = kwsysProcess_Pipe_STDERR;
              break;
          };
        }
        return 1;
      } else if (n == 0) /* EOF */
      {
/* We are done reading from this pipe.  */
#  if defined(__VMS)
        if (!cp->CommandsLeft)
#  endif
        {
          kwsysProcessCleanupDescriptor(&cp->PipeReadEnds[i]);
          --cp->PipesLeft;
        }
      } else if (n < 0) /* error */
      {
#  if defined(__VMS)
        if (!cp->CommandsLeft) {
          kwsysProcessCleanupDescriptor(&cp->PipeReadEnds[i]);
          --cp->PipesLeft;
        } else
#  endif
          if ((errno != EINTR) && (errno != EAGAIN)) {
          strncpy(cp->ErrorMessage, strerror(errno), KWSYSPE_PIPE_BUFFER_SIZE);
          /* Kill the children now.  */
          kwsysProcess_Kill(cp);
          cp->Killed = 0;
          cp->SelectError = 1;
          return 1;
        }
      }
    }
  }

  /* If we have data, break early.  */
  if (wd->PipeId) {
    return 1;
  }

  if (kwsysProcessGetTimeoutLeft(
        &wd->TimeoutTime, wd->User ? wd->UserTimeout : 0, &timeoutLength, 1)) {
    /* Timeout has already expired.  */
    wd->Expired = 1;
    return 1;
  }

  /* Sleep a little, try again. */
  {
    unsigned int msec =
      ((timeoutLength.tv_sec * 1000) + (timeoutLength.tv_usec / 1000));
    if (msec > 100000) {
      msec = 100000; /* do not sleep more than 100 milliseconds at a time */
    }
    kwsysProcess_usleep(msec);
  }
  return 0;
#endif
}

int kwsysProcess_WaitForExit(kwsysProcess* cp, double* userTimeout)
{
  int prPipe = 0;

  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing) {
    return 1;
  }

  /* Wait for all the pipes to close.  Ignore all data.  */
  while ((prPipe = kwsysProcess_WaitForData(cp, 0, 0, userTimeout)) > 0) {
    if (prPipe == kwsysProcess_Pipe_Timeout) {
      return 0;
    }
  }

  /* Check if there was an error in one of the waitpid calls.  */
  if (cp->State == kwsysProcess_State_Error) {
    /* The error message is already in its buffer.  Tell
       kwsysProcessCleanup to not create it.  */
    kwsysProcessCleanup(cp, 0);
    return 1;
  }

  /* Check whether the child reported an error invoking the process.  */
  if (cp->SelectError) {
    /* The error message is already in its buffer.  Tell
       kwsysProcessCleanup to not create it.  */
    kwsysProcessCleanup(cp, 0);
    cp->State = kwsysProcess_State_Error;
    return 1;
  }
  /* Determine the outcome.  */
  if (cp->Killed) {
    /* We killed the child.  */
    cp->State = kwsysProcess_State_Killed;
  } else if (cp->TimeoutExpired) {
    /* The timeout expired.  */
    cp->State = kwsysProcess_State_Expired;
  } else {
    /* The children exited.  Report the outcome of the child processes.  */
    for (prPipe = 0; prPipe < cp->NumberOfCommands; ++prPipe) {
      cp->ProcessResults[prPipe].ExitCode = cp->CommandExitCodes[prPipe];
      if (WIFEXITED(cp->ProcessResults[prPipe].ExitCode)) {
        /* The child exited normally.  */
        cp->ProcessResults[prPipe].State = kwsysProcess_StateByIndex_Exited;
        cp->ProcessResults[prPipe].ExitException = kwsysProcess_Exception_None;
        cp->ProcessResults[prPipe].ExitValue =
          (int)WEXITSTATUS(cp->ProcessResults[prPipe].ExitCode);
      } else if (WIFSIGNALED(cp->ProcessResults[prPipe].ExitCode)) {
        /* The child received an unhandled signal.  */
        cp->ProcessResults[prPipe].State = kwsysProcess_State_Exception;
        kwsysProcessSetExitExceptionByIndex(
          cp, (int)WTERMSIG(cp->ProcessResults[prPipe].ExitCode), prPipe);
      } else {
        /* Error getting the child return code.  */
        strcpy(cp->ProcessResults[prPipe].ExitExceptionString,
               "Error getting child return code.");
        cp->ProcessResults[prPipe].State = kwsysProcess_StateByIndex_Error;
      }
    }
    /* support legacy state status value */
    cp->State = cp->ProcessResults[cp->NumberOfCommands - 1].State;
  }
  /* Normal cleanup.  */
  kwsysProcessCleanup(cp, 0);
  return 1;
}

void kwsysProcess_Interrupt(kwsysProcess* cp)
{
  int i;
  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing || cp->TimeoutExpired ||
      cp->Killed) {
    return;
  }

  /* Interrupt the children.  */
  if (cp->CreateProcessGroup) {
    if (cp->ForkPIDs) {
      for (i = 0; i < cp->NumberOfCommands; ++i) {
        /* Make sure the PID is still valid. */
        if (cp->ForkPIDs[i]) {
          /* The user created a process group for this process.  The group ID
             is the process ID for the original process in the group.  */
          kill(-cp->ForkPIDs[i], SIGINT);
        }
      }
    }
  } else {
    /* No process group was created.  Kill our own process group.
       NOTE:  While one could argue that we could call kill(cp->ForkPIDs[i],
       SIGINT) as a way to still interrupt the process even though it's not in
       a special group, this is not an option on Windows.  Therefore, we kill
       the current process group for consistency with Windows.  */
    kill(0, SIGINT);
  }
}

void kwsysProcess_Kill(kwsysProcess* cp)
{
  int i;

  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing) {
    return;
  }

  /* First close the child exit report pipe write end to avoid causing a
     SIGPIPE when the child terminates and our signal handler tries to
     report it after we have already closed the read end.  */
  kwsysProcessCleanupDescriptor(&cp->SignalPipe);

#if !defined(__APPLE__)
  /* Close all the pipe read ends.  Do this before killing the
     children because Cygwin has problems killing processes that are
     blocking to wait for writing to their output pipes.  */
  kwsysProcessClosePipes(cp);
#endif

  /* Kill the children.  */
  cp->Killed = 1;
  for (i = 0; i < cp->NumberOfCommands; ++i) {
    int status;
    if (cp->ForkPIDs[i]) {
      /* Kill the child.  */
      kwsysProcessKill(cp->ForkPIDs[i]);

      /* Reap the child.  Keep trying until the call is not
         interrupted.  */
      while ((waitpid(cp->ForkPIDs[i], &status, 0) < 0) && (errno == EINTR))
        ;
    }
  }

#if defined(__APPLE__)
  /* Close all the pipe read ends.  Do this after killing the
     children because OS X has problems closing pipe read ends whose
     pipes are full and still have an open write end.  */
  kwsysProcessClosePipes(cp);
#endif

  cp->CommandsLeft = 0;
}

/* Call the free() function with a pointer to volatile without causing
   compiler warnings.  */
static void kwsysProcessVolatileFree(volatile void* p)
{
/* clang has made it impossible to free memory that points to volatile
   without first using special pragmas to disable a warning...  */
#if defined(__clang__) && !defined(__INTEL_COMPILER)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-qual"
#endif
  free((void*)p); /* The cast will silence most compilers, but not clang.  */
#if defined(__clang__) && !defined(__INTEL_COMPILER)
#  pragma clang diagnostic pop
#endif
}

/* Initialize a process control structure for kwsysProcess_Execute.  */
static int kwsysProcessInitialize(kwsysProcess* cp)
{
  int i;
  volatile pid_t* oldForkPIDs;
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    cp->PipeReadEnds[i] = -1;
  }
  for (i = 0; i < 3; ++i) {
    cp->PipeChildStd[i] = -1;
  }
  cp->SignalPipe = -1;
  cp->SelectError = 0;
  cp->StartTime.tv_sec = -1;
  cp->StartTime.tv_usec = -1;
  cp->TimeoutTime.tv_sec = -1;
  cp->TimeoutTime.tv_usec = -1;
  cp->TimeoutExpired = 0;
  cp->PipesLeft = 0;
  cp->CommandsLeft = 0;
#if KWSYSPE_USE_SELECT
  FD_ZERO(&cp->PipeSet);
#endif
  cp->State = kwsysProcess_State_Starting;
  cp->Killed = 0;
  cp->ErrorMessage[0] = 0;

  oldForkPIDs = cp->ForkPIDs;
  cp->ForkPIDs = (volatile pid_t*)malloc(sizeof(volatile pid_t) *
                                         (size_t)(cp->NumberOfCommands));
  kwsysProcessVolatileFree(oldForkPIDs);
  if (!cp->ForkPIDs) {
    return 0;
  }
  for (i = 0; i < cp->NumberOfCommands; ++i) {
    cp->ForkPIDs[i] = 0; /* can't use memset due to volatile */
  }

  free(cp->CommandExitCodes);
  cp->CommandExitCodes =
    (int*)malloc(sizeof(int) * (size_t)(cp->NumberOfCommands));
  if (!cp->CommandExitCodes) {
    return 0;
  }
  memset(cp->CommandExitCodes, 0,
         sizeof(int) * (size_t)(cp->NumberOfCommands));

  /* Allocate process result information for each process.  */
  free(cp->ProcessResults);
  cp->ProcessResults = (kwsysProcessResults*)malloc(
    sizeof(kwsysProcessResults) * (size_t)(cp->NumberOfCommands));
  if (!cp->ProcessResults) {
    return 0;
  }
  memset(cp->ProcessResults, 0,
         sizeof(kwsysProcessResults) * (size_t)(cp->NumberOfCommands));
  for (i = 0; i < cp->NumberOfCommands; i++) {
    cp->ProcessResults[i].ExitException = kwsysProcess_Exception_None;
    cp->ProcessResults[i].State = kwsysProcess_StateByIndex_Starting;
    cp->ProcessResults[i].ExitCode = 1;
    cp->ProcessResults[i].ExitValue = 1;
    strcpy(cp->ProcessResults[i].ExitExceptionString, "No exception");
  }

  /* Allocate memory to save the real working directory.  */
  if (cp->WorkingDirectory) {
#if defined(MAXPATHLEN)
    cp->RealWorkingDirectoryLength = MAXPATHLEN;
#elif defined(PATH_MAX)
    cp->RealWorkingDirectoryLength = PATH_MAX;
#else
    cp->RealWorkingDirectoryLength = 4096;
#endif
    cp->RealWorkingDirectory =
      (char*)malloc((size_t)(cp->RealWorkingDirectoryLength));
    if (!cp->RealWorkingDirectory) {
      return 0;
    }
  }

  return 1;
}

/* Free all resources used by the given kwsysProcess instance that were
   allocated by kwsysProcess_Execute.  */
static void kwsysProcessCleanup(kwsysProcess* cp, int error)
{
  int i;

  if (error) {
    /* We are cleaning up due to an error.  Report the error message
       if one has not been provided already.  */
    if (cp->ErrorMessage[0] == 0) {
      strncpy(cp->ErrorMessage, strerror(errno), KWSYSPE_PIPE_BUFFER_SIZE);
    }

    /* Set the error state.  */
    cp->State = kwsysProcess_State_Error;

    /* Kill any children already started.  */
    if (cp->ForkPIDs) {
      int status;
      for (i = 0; i < cp->NumberOfCommands; ++i) {
        if (cp->ForkPIDs[i]) {
          /* Kill the child.  */
          kwsysProcessKill(cp->ForkPIDs[i]);

          /* Reap the child.  Keep trying until the call is not
             interrupted.  */
          while ((waitpid(cp->ForkPIDs[i], &status, 0) < 0) &&
                 (errno == EINTR))
            ;
        }
      }
    }

    /* Restore the working directory.  */
    if (cp->RealWorkingDirectory) {
      while ((chdir(cp->RealWorkingDirectory) < 0) && (errno == EINTR))
        ;
    }
  }

  /* If not creating a detached child, remove this object from the
     global set of process objects that wish to be notified when a
     child exits.  */
  if (!cp->OptionDetach) {
    kwsysProcessesRemove(cp);
  }

  /* Free memory.  */
  if (cp->ForkPIDs) {
    kwsysProcessVolatileFree(cp->ForkPIDs);
    cp->ForkPIDs = 0;
  }
  if (cp->RealWorkingDirectory) {
    free(cp->RealWorkingDirectory);
    cp->RealWorkingDirectory = 0;
  }

  /* Close pipe handles.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    kwsysProcessCleanupDescriptor(&cp->PipeReadEnds[i]);
  }
  for (i = 0; i < 3; ++i) {
    kwsysProcessCleanupDescriptor(&cp->PipeChildStd[i]);
  }
}

/* Close the given file descriptor if it is open.  Reset its value to -1.  */
static void kwsysProcessCleanupDescriptor(int* pfd)
{
  if (pfd && *pfd > 2) {
    /* Keep trying to close until it is not interrupted by a
     * signal.  */
    while ((close(*pfd) < 0) && (errno == EINTR))
      ;
    *pfd = -1;
  }
}

static void kwsysProcessClosePipes(kwsysProcess* cp)
{
  int i;

  /* Close any pipes that are still open.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    if (cp->PipeReadEnds[i] >= 0) {
#if KWSYSPE_USE_SELECT
      /* If the pipe was reported by the last call to select, we must
         read from it.  This is needed to satisfy the suggestions from
         "man select_tut" and is not needed for the polling
         implementation.  Ignore the data.  */
      if (FD_ISSET(cp->PipeReadEnds[i], &cp->PipeSet)) {
        /* We are handling this pipe now.  Remove it from the set.  */
        FD_CLR(cp->PipeReadEnds[i], &cp->PipeSet);

        /* The pipe is ready to read without blocking.  Keep trying to
           read until the operation is not interrupted.  */
        while ((read(cp->PipeReadEnds[i], cp->PipeBuffer,
                     KWSYSPE_PIPE_BUFFER_SIZE) < 0) &&
               (errno == EINTR))
          ;
      }
#endif

      /* We are done reading from this pipe.  */
      kwsysProcessCleanupDescriptor(&cp->PipeReadEnds[i]);
      --cp->PipesLeft;
    }
  }
}

static int kwsysProcessSetNonBlocking(int fd)
{
  int flags = fcntl(fd, F_GETFL);
  if (flags >= 0) {
    flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
  return flags >= 0;
}

#if defined(__VMS)
int decc$set_child_standard_streams(int fd1, int fd2, int fd3);
#endif

static int kwsysProcessCreate(kwsysProcess* cp, int prIndex,
                              kwsysProcessCreateInformation* si)
{
  sigset_t mask, old_mask;
  int pgidPipe[2];
  char tmp;
  ssize_t readRes;

  /* Create the error reporting pipe.  */
  if (pipe(si->ErrorPipe) < 0) {
    return 0;
  }

  /* Create a pipe for detecting that the child process has created a process
     group and session.  */
  if (pipe(pgidPipe) < 0) {
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[1]);
    return 0;
  }

  /* Set close-on-exec flag on the pipe's write end.  */
  if (fcntl(si->ErrorPipe[1], F_SETFD, FD_CLOEXEC) < 0 ||
      fcntl(pgidPipe[1], F_SETFD, FD_CLOEXEC) < 0) {
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[1]);
    kwsysProcessCleanupDescriptor(&pgidPipe[0]);
    kwsysProcessCleanupDescriptor(&pgidPipe[1]);
    return 0;
  }

  /* Block SIGINT / SIGTERM while we start.  The purpose is so that our signal
     handler doesn't get called from the child process after the fork and
     before the exec, and subsequently start kill()'ing PIDs from ForkPIDs. */
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  if (sigprocmask(SIG_BLOCK, &mask, &old_mask) < 0) {
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[1]);
    kwsysProcessCleanupDescriptor(&pgidPipe[0]);
    kwsysProcessCleanupDescriptor(&pgidPipe[1]);
    return 0;
  }

/* Fork off a child process.  */
#if defined(__VMS)
  /* VMS needs vfork and execvp to be in the same function because
     they use setjmp/longjmp to run the child startup code in the
     parent!  TODO: OptionDetach.  Also
     TODO:  CreateProcessGroup.  */
  cp->ForkPIDs[prIndex] = vfork();
#else
  cp->ForkPIDs[prIndex] = kwsysProcessFork(cp, si);
#endif
  if (cp->ForkPIDs[prIndex] < 0) {
    sigprocmask(SIG_SETMASK, &old_mask, 0);
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[1]);
    kwsysProcessCleanupDescriptor(&pgidPipe[0]);
    kwsysProcessCleanupDescriptor(&pgidPipe[1]);
    return 0;
  }

  if (cp->ForkPIDs[prIndex] == 0) {
#if defined(__VMS)
    /* Specify standard pipes for child process.  */
    decc$set_child_standard_streams(si->StdIn, si->StdOut, si->StdErr);
#else
    /* Close the read end of the error reporting / process group
       setup pipe.  */
    close(si->ErrorPipe[0]);
    close(pgidPipe[0]);

    /* Setup the stdin, stdout, and stderr pipes.  */
    if (si->StdIn > 0) {
      dup2(si->StdIn, 0);
    } else if (si->StdIn < 0) {
      close(0);
    }
    if (si->StdOut != 1) {
      dup2(si->StdOut, 1);
    }
    if (si->StdErr != 2) {
      dup2(si->StdErr, 2);
    }

    /* Clear the close-on-exec flag for stdin, stdout, and stderr.
       All other pipe handles will be closed when exec succeeds.  */
    fcntl(0, F_SETFD, 0);
    fcntl(1, F_SETFD, 0);
    fcntl(2, F_SETFD, 0);

    /* Restore all default signal handlers. */
    kwsysProcessRestoreDefaultSignalHandlers();

    /* Now that we have restored default signal handling and created the
       process group, restore mask.  */
    sigprocmask(SIG_SETMASK, &old_mask, 0);

    /* Create new process group.  We use setsid instead of setpgid to avoid
       the child getting hung up on signals like SIGTTOU.  (In the real world,
       this has been observed where "git svn" ends up calling the "resize"
       program which opens /dev/tty.  */
    if (cp->CreateProcessGroup && setsid() < 0) {
      kwsysProcessChildErrorExit(si->ErrorPipe[1]);
    }
#endif

    /* Execute the real process.  If successful, this does not return.  */
    execvp(cp->Commands[prIndex][0], cp->Commands[prIndex]);
    /* TODO: What does VMS do if the child fails to start?  */
    /* TODO: On VMS, how do we put the process in a new group?  */

    /* Failure.  Report error to parent and terminate.  */
    kwsysProcessChildErrorExit(si->ErrorPipe[1]);
  }

#if defined(__VMS)
  /* Restore the standard pipes of this process.  */
  decc$set_child_standard_streams(0, 1, 2);
#endif

  /* We are done with the error reporting pipe and process group setup pipe
     write end.  */
  kwsysProcessCleanupDescriptor(&si->ErrorPipe[1]);
  kwsysProcessCleanupDescriptor(&pgidPipe[1]);

  /* Make sure the child is in the process group before we proceed.  This
     avoids race conditions with calls to the kill function that we make for
     signalling process groups.  */
  while ((readRes = read(pgidPipe[0], &tmp, 1)) > 0)
    ;
  if (readRes < 0) {
    sigprocmask(SIG_SETMASK, &old_mask, 0);
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);
    kwsysProcessCleanupDescriptor(&pgidPipe[0]);
    return 0;
  }
  kwsysProcessCleanupDescriptor(&pgidPipe[0]);

  /* Unmask signals.  */
  if (sigprocmask(SIG_SETMASK, &old_mask, 0) < 0) {
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);
    return 0;
  }

  /* A child has been created.  */
  ++cp->CommandsLeft;

  /* Block until the child's exec call succeeds and closes the error
     pipe or writes data to the pipe to report an error.  */
  {
    kwsysProcess_ssize_t total = 0;
    kwsysProcess_ssize_t n = 1;
    /* Read the entire error message up to the length of our buffer.  */
    while (total < KWSYSPE_PIPE_BUFFER_SIZE && n > 0) {
      /* Keep trying to read until the operation is not interrupted.  */
      while (((n = read(si->ErrorPipe[0], cp->ErrorMessage + total,
                        (size_t)(KWSYSPE_PIPE_BUFFER_SIZE - total))) < 0) &&
             (errno == EINTR))
        ;
      if (n > 0) {
        total += n;
      }
    }

    /* We are done with the error reporting pipe read end.  */
    kwsysProcessCleanupDescriptor(&si->ErrorPipe[0]);

    if (total > 0) {
      /* The child failed to execute the process.  */
      return 0;
    }
  }

  return 1;
}

static void kwsysProcessDestroy(kwsysProcess* cp)
{
  /* A child process has terminated.  Reap it if it is one handled by
     this object.  */
  int i;
  /* Temporarily disable signals that access ForkPIDs.  We don't want them to
     read a reaped PID, and writes to ForkPIDs are not atomic.  */
  sigset_t mask, old_mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  if (sigprocmask(SIG_BLOCK, &mask, &old_mask) < 0) {
    return;
  }

  for (i = 0; i < cp->NumberOfCommands; ++i) {
    if (cp->ForkPIDs[i]) {
      int result;
      while (((result = waitpid(cp->ForkPIDs[i], &cp->CommandExitCodes[i],
                                WNOHANG)) < 0) &&
             (errno == EINTR))
        ;
      if (result > 0) {
        /* This child has termianted.  */
        cp->ForkPIDs[i] = 0;
        if (--cp->CommandsLeft == 0) {
          /* All children have terminated.  Close the signal pipe
             write end so that no more notifications are sent to this
             object.  */
          kwsysProcessCleanupDescriptor(&cp->SignalPipe);

          /* TODO: Once the children have terminated, switch
             WaitForData to use a non-blocking read to get the
             rest of the data from the pipe.  This is needed when
             grandchildren keep the output pipes open.  */
        }
      } else if (result < 0 && cp->State != kwsysProcess_State_Error) {
        /* Unexpected error.  Report the first time this happens.  */
        strncpy(cp->ErrorMessage, strerror(errno), KWSYSPE_PIPE_BUFFER_SIZE);
        cp->State = kwsysProcess_State_Error;
      }
    }
  }

  /* Re-enable signals.  */
  sigprocmask(SIG_SETMASK, &old_mask, 0);
}

static int kwsysProcessSetupOutputPipeFile(int* p, const char* name)
{
  int fout;
  if (!name) {
    return 1;
  }

  /* Close the existing descriptor.  */
  kwsysProcessCleanupDescriptor(p);

  /* Open a file for the pipe to write.  */
  if ((fout = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
    return 0;
  }

  /* Set close-on-exec flag on the pipe's end.  */
  if (fcntl(fout, F_SETFD, FD_CLOEXEC) < 0) {
    close(fout);
    return 0;
  }

  /* Assign the replacement descriptor.  */
  *p = fout;
  return 1;
}

static int kwsysProcessSetupOutputPipeNative(int* p, int des[2])
{
  /* Close the existing descriptor.  */
  kwsysProcessCleanupDescriptor(p);

  /* Set close-on-exec flag on the pipe's ends.  The proper end will
     be dup2-ed into the standard descriptor number after fork but
     before exec.  */
  if ((fcntl(des[0], F_SETFD, FD_CLOEXEC) < 0) ||
      (fcntl(des[1], F_SETFD, FD_CLOEXEC) < 0)) {
    return 0;
  }

  /* Assign the replacement descriptor.  */
  *p = des[1];
  return 1;
}

/* Get the time at which either the process or user timeout will
   expire.  Returns 1 if the user timeout is first, and 0 otherwise.  */
static int kwsysProcessGetTimeoutTime(kwsysProcess* cp, double* userTimeout,
                                      kwsysProcessTime* timeoutTime)
{
  /* The first time this is called, we need to calculate the time at
     which the child will timeout.  */
  if (cp->Timeout > 0 && cp->TimeoutTime.tv_sec < 0) {
    kwsysProcessTime length = kwsysProcessTimeFromDouble(cp->Timeout);
    cp->TimeoutTime = kwsysProcessTimeAdd(cp->StartTime, length);
  }

  /* Start with process timeout.  */
  *timeoutTime = cp->TimeoutTime;

  /* Check if the user timeout is earlier.  */
  if (userTimeout) {
    kwsysProcessTime currentTime = kwsysProcessTimeGetCurrent();
    kwsysProcessTime userTimeoutLength =
      kwsysProcessTimeFromDouble(*userTimeout);
    kwsysProcessTime userTimeoutTime =
      kwsysProcessTimeAdd(currentTime, userTimeoutLength);
    if (timeoutTime->tv_sec < 0 ||
        kwsysProcessTimeLess(userTimeoutTime, *timeoutTime)) {
      *timeoutTime = userTimeoutTime;
      return 1;
    }
  }
  return 0;
}

/* Get the length of time before the given timeout time arrives.
   Returns 1 if the time has already arrived, and 0 otherwise.  */
static int kwsysProcessGetTimeoutLeft(kwsysProcessTime* timeoutTime,
                                      double* userTimeout,
                                      kwsysProcessTimeNative* timeoutLength,
                                      int zeroIsExpired)
{
  if (timeoutTime->tv_sec < 0) {
    /* No timeout time has been requested.  */
    return 0;
  } else {
    /* Calculate the remaining time.  */
    kwsysProcessTime currentTime = kwsysProcessTimeGetCurrent();
    kwsysProcessTime timeLeft =
      kwsysProcessTimeSubtract(*timeoutTime, currentTime);
    if (timeLeft.tv_sec < 0 && userTimeout && *userTimeout <= 0) {
      /* Caller has explicitly requested a zero timeout.  */
      timeLeft.tv_sec = 0;
      timeLeft.tv_usec = 0;
    }

    if (timeLeft.tv_sec < 0 ||
        (timeLeft.tv_sec == 0 && timeLeft.tv_usec == 0 && zeroIsExpired)) {
      /* Timeout has already expired.  */
      return 1;
    } else {
      /* There is some time left.  */
      timeoutLength->tv_sec = timeLeft.tv_sec;
      timeoutLength->tv_usec = timeLeft.tv_usec;
      return 0;
    }
  }
}

static kwsysProcessTime kwsysProcessTimeGetCurrent(void)
{
  kwsysProcessTime current;
  kwsysProcessTimeNative current_native;
#if KWSYS_C_HAS_CLOCK_GETTIME_MONOTONIC
  struct timespec current_timespec;
  clock_gettime(CLOCK_MONOTONIC, &current_timespec);

  current_native.tv_sec = current_timespec.tv_sec;
  current_native.tv_usec = current_timespec.tv_nsec / 1000;
#else
  gettimeofday(&current_native, 0);
#endif
  current.tv_sec = (long)current_native.tv_sec;
  current.tv_usec = (long)current_native.tv_usec;
  return current;
}

static double kwsysProcessTimeToDouble(kwsysProcessTime t)
{
  return (double)t.tv_sec + (double)(t.tv_usec) * 0.000001;
}

static kwsysProcessTime kwsysProcessTimeFromDouble(double d)
{
  kwsysProcessTime t;
  t.tv_sec = (long)d;
  t.tv_usec = (long)((d - (double)(t.tv_sec)) * 1000000);
  return t;
}

static int kwsysProcessTimeLess(kwsysProcessTime in1, kwsysProcessTime in2)
{
  return ((in1.tv_sec < in2.tv_sec) ||
          ((in1.tv_sec == in2.tv_sec) && (in1.tv_usec < in2.tv_usec)));
}

static kwsysProcessTime kwsysProcessTimeAdd(kwsysProcessTime in1,
                                            kwsysProcessTime in2)
{
  kwsysProcessTime out;
  out.tv_sec = in1.tv_sec + in2.tv_sec;
  out.tv_usec = in1.tv_usec + in2.tv_usec;
  if (out.tv_usec >= 1000000) {
    out.tv_usec -= 1000000;
    out.tv_sec += 1;
  }
  return out;
}

static kwsysProcessTime kwsysProcessTimeSubtract(kwsysProcessTime in1,
                                                 kwsysProcessTime in2)
{
  kwsysProcessTime out;
  out.tv_sec = in1.tv_sec - in2.tv_sec;
  out.tv_usec = in1.tv_usec - in2.tv_usec;
  if (out.tv_usec < 0) {
    out.tv_usec += 1000000;
    out.tv_sec -= 1;
  }
  return out;
}

#define KWSYSPE_CASE(type, str)                                               \
  cp->ProcessResults[idx].ExitException = kwsysProcess_Exception_##type;      \
  strcpy(cp->ProcessResults[idx].ExitExceptionString, str)
static void kwsysProcessSetExitExceptionByIndex(kwsysProcess* cp, int sig,
                                                int idx)
{
  switch (sig) {
#ifdef SIGSEGV
    case SIGSEGV:
      KWSYSPE_CASE(Fault, "Segmentation fault");
      break;
#endif
#ifdef SIGBUS
#  if !defined(SIGSEGV) || SIGBUS != SIGSEGV
    case SIGBUS:
      KWSYSPE_CASE(Fault, "Bus error");
      break;
#  endif
#endif
#ifdef SIGFPE
    case SIGFPE:
      KWSYSPE_CASE(Numerical, "Floating-point exception");
      break;
#endif
#ifdef SIGILL
    case SIGILL:
      KWSYSPE_CASE(Illegal, "Illegal instruction");
      break;
#endif
#ifdef SIGINT
    case SIGINT:
      KWSYSPE_CASE(Interrupt, "User interrupt");
      break;
#endif
#ifdef SIGABRT
    case SIGABRT:
      KWSYSPE_CASE(Other, "Child aborted");
      break;
#endif
#ifdef SIGKILL
    case SIGKILL:
      KWSYSPE_CASE(Other, "Child killed");
      break;
#endif
#ifdef SIGTERM
    case SIGTERM:
      KWSYSPE_CASE(Other, "Child terminated");
      break;
#endif
#ifdef SIGHUP
    case SIGHUP:
      KWSYSPE_CASE(Other, "SIGHUP");
      break;
#endif
#ifdef SIGQUIT
    case SIGQUIT:
      KWSYSPE_CASE(Other, "SIGQUIT");
      break;
#endif
#ifdef SIGTRAP
    case SIGTRAP:
      KWSYSPE_CASE(Other, "SIGTRAP");
      break;
#endif
#ifdef SIGIOT
#  if !defined(SIGABRT) || SIGIOT != SIGABRT
    case SIGIOT:
      KWSYSPE_CASE(Other, "SIGIOT");
      break;
#  endif
#endif
#ifdef SIGUSR1
    case SIGUSR1:
      KWSYSPE_CASE(Other, "SIGUSR1");
      break;
#endif
#ifdef SIGUSR2
    case SIGUSR2:
      KWSYSPE_CASE(Other, "SIGUSR2");
      break;
#endif
#ifdef SIGPIPE
    case SIGPIPE:
      KWSYSPE_CASE(Other, "SIGPIPE");
      break;
#endif
#ifdef SIGALRM
    case SIGALRM:
      KWSYSPE_CASE(Other, "SIGALRM");
      break;
#endif
#ifdef SIGSTKFLT
    case SIGSTKFLT:
      KWSYSPE_CASE(Other, "SIGSTKFLT");
      break;
#endif
#ifdef SIGCHLD
    case SIGCHLD:
      KWSYSPE_CASE(Other, "SIGCHLD");
      break;
#elif defined(SIGCLD)
    case SIGCLD:
      KWSYSPE_CASE(Other, "SIGCLD");
      break;
#endif
#ifdef SIGCONT
    case SIGCONT:
      KWSYSPE_CASE(Other, "SIGCONT");
      break;
#endif
#ifdef SIGSTOP
    case SIGSTOP:
      KWSYSPE_CASE(Other, "SIGSTOP");
      break;
#endif
#ifdef SIGTSTP
    case SIGTSTP:
      KWSYSPE_CASE(Other, "SIGTSTP");
      break;
#endif
#ifdef SIGTTIN
    case SIGTTIN:
      KWSYSPE_CASE(Other, "SIGTTIN");
      break;
#endif
#ifdef SIGTTOU
    case SIGTTOU:
      KWSYSPE_CASE(Other, "SIGTTOU");
      break;
#endif
#ifdef SIGURG
    case SIGURG:
      KWSYSPE_CASE(Other, "SIGURG");
      break;
#endif
#ifdef SIGXCPU
    case SIGXCPU:
      KWSYSPE_CASE(Other, "SIGXCPU");
      break;
#endif
#ifdef SIGXFSZ
    case SIGXFSZ:
      KWSYSPE_CASE(Other, "SIGXFSZ");
      break;
#endif
#ifdef SIGVTALRM
    case SIGVTALRM:
      KWSYSPE_CASE(Other, "SIGVTALRM");
      break;
#endif
#ifdef SIGPROF
    case SIGPROF:
      KWSYSPE_CASE(Other, "SIGPROF");
      break;
#endif
#ifdef SIGWINCH
    case SIGWINCH:
      KWSYSPE_CASE(Other, "SIGWINCH");
      break;
#endif
#ifdef SIGPOLL
    case SIGPOLL:
      KWSYSPE_CASE(Other, "SIGPOLL");
      break;
#endif
#ifdef SIGIO
#  if !defined(SIGPOLL) || SIGIO != SIGPOLL
    case SIGIO:
      KWSYSPE_CASE(Other, "SIGIO");
      break;
#  endif
#endif
#ifdef SIGPWR
    case SIGPWR:
      KWSYSPE_CASE(Other, "SIGPWR");
      break;
#endif
#ifdef SIGSYS
    case SIGSYS:
      KWSYSPE_CASE(Other, "SIGSYS");
      break;
#endif
#ifdef SIGUNUSED
#  if !defined(SIGSYS) || SIGUNUSED != SIGSYS
    case SIGUNUSED:
      KWSYSPE_CASE(Other, "SIGUNUSED");
      break;
#  endif
#endif
    default:
      cp->ProcessResults[idx].ExitException = kwsysProcess_Exception_Other;
      sprintf(cp->ProcessResults[idx].ExitExceptionString, "Signal %d", sig);
      break;
  }
}
#undef KWSYSPE_CASE

/* When the child process encounters an error before its program is
   invoked, this is called to report the error to the parent and
   exit.  */
static void kwsysProcessChildErrorExit(int errorPipe)
{
  /* Construct the error message.  */
  char buffer[KWSYSPE_PIPE_BUFFER_SIZE];
  kwsysProcess_ssize_t result;
  strncpy(buffer, strerror(errno), KWSYSPE_PIPE_BUFFER_SIZE);
  buffer[KWSYSPE_PIPE_BUFFER_SIZE - 1] = '\0';

  /* Report the error to the parent through the special pipe.  */
  result = write(errorPipe, buffer, strlen(buffer));
  (void)result;

  /* Terminate without cleanup.  */
  _exit(1);
}

/* Restores all signal handlers to their default values.  */
static void kwsysProcessRestoreDefaultSignalHandlers(void)
{
  struct sigaction act;
  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = SIG_DFL;
#ifdef SIGHUP
  sigaction(SIGHUP, &act, 0);
#endif
#ifdef SIGINT
  sigaction(SIGINT, &act, 0);
#endif
#ifdef SIGQUIT
  sigaction(SIGQUIT, &act, 0);
#endif
#ifdef SIGILL
  sigaction(SIGILL, &act, 0);
#endif
#ifdef SIGTRAP
  sigaction(SIGTRAP, &act, 0);
#endif
#ifdef SIGABRT
  sigaction(SIGABRT, &act, 0);
#endif
#ifdef SIGIOT
  sigaction(SIGIOT, &act, 0);
#endif
#ifdef SIGBUS
  sigaction(SIGBUS, &act, 0);
#endif
#ifdef SIGFPE
  sigaction(SIGFPE, &act, 0);
#endif
#ifdef SIGUSR1
  sigaction(SIGUSR1, &act, 0);
#endif
#ifdef SIGSEGV
  sigaction(SIGSEGV, &act, 0);
#endif
#ifdef SIGUSR2
  sigaction(SIGUSR2, &act, 0);
#endif
#ifdef SIGPIPE
  sigaction(SIGPIPE, &act, 0);
#endif
#ifdef SIGALRM
  sigaction(SIGALRM, &act, 0);
#endif
#ifdef SIGTERM
  sigaction(SIGTERM, &act, 0);
#endif
#ifdef SIGSTKFLT
  sigaction(SIGSTKFLT, &act, 0);
#endif
#ifdef SIGCLD
  sigaction(SIGCLD, &act, 0);
#endif
#ifdef SIGCHLD
  sigaction(SIGCHLD, &act, 0);
#endif
#ifdef SIGCONT
  sigaction(SIGCONT, &act, 0);
#endif
#ifdef SIGTSTP
  sigaction(SIGTSTP, &act, 0);
#endif
#ifdef SIGTTIN
  sigaction(SIGTTIN, &act, 0);
#endif
#ifdef SIGTTOU
  sigaction(SIGTTOU, &act, 0);
#endif
#ifdef SIGURG
  sigaction(SIGURG, &act, 0);
#endif
#ifdef SIGXCPU
  sigaction(SIGXCPU, &act, 0);
#endif
#ifdef SIGXFSZ
  sigaction(SIGXFSZ, &act, 0);
#endif
#ifdef SIGVTALRM
  sigaction(SIGVTALRM, &act, 0);
#endif
#ifdef SIGPROF
  sigaction(SIGPROF, &act, 0);
#endif
#ifdef SIGWINCH
  sigaction(SIGWINCH, &act, 0);
#endif
#ifdef SIGPOLL
  sigaction(SIGPOLL, &act, 0);
#endif
#ifdef SIGIO
  sigaction(SIGIO, &act, 0);
#endif
#ifdef SIGPWR
  sigaction(SIGPWR, &act, 0);
#endif
#ifdef SIGSYS
  sigaction(SIGSYS, &act, 0);
#endif
#ifdef SIGUNUSED
  sigaction(SIGUNUSED, &act, 0);
#endif
}

static void kwsysProcessExit(void)
{
  _exit(0);
}

#if !defined(__VMS)
static pid_t kwsysProcessFork(kwsysProcess* cp,
                              kwsysProcessCreateInformation* si)
{
  /* Create a detached process if requested.  */
  if (cp->OptionDetach) {
    /* Create an intermediate process.  */
    pid_t middle_pid = fork();
    if (middle_pid < 0) {
      /* Fork failed.  Return as if we were not detaching.  */
      return middle_pid;
    } else if (middle_pid == 0) {
      /* This is the intermediate process.  Create the real child.  */
      pid_t child_pid = fork();
      if (child_pid == 0) {
        /* This is the real child process.  There is nothing to do here.  */
        return 0;
      } else {
        /* Use the error pipe to report the pid to the real parent.  */
        while ((write(si->ErrorPipe[1], &child_pid, sizeof(child_pid)) < 0) &&
               (errno == EINTR))
          ;

        /* Exit without cleanup.  The parent holds all resources.  */
        kwsysProcessExit();
        return 0; /* Never reached, but avoids SunCC warning.  */
      }
    } else {
      /* This is the original parent process.  The intermediate
         process will use the error pipe to report the pid of the
         detached child.  */
      pid_t child_pid;
      int status;
      while ((read(si->ErrorPipe[0], &child_pid, sizeof(child_pid)) < 0) &&
             (errno == EINTR))
        ;

      /* Wait for the intermediate process to exit and clean it up.  */
      while ((waitpid(middle_pid, &status, 0) < 0) && (errno == EINTR))
        ;
      return child_pid;
    }
  } else {
    /* Not creating a detached process.  Use normal fork.  */
    return fork();
  }
}
#endif

/* We try to obtain process information by invoking the ps command.
   Here we define the command to call on each platform and the
   corresponding parsing format string.  The parsing format should
   have two integers to store: the pid and then the ppid.  */
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||       \
  defined(__OpenBSD__) || defined(__GLIBC__) || defined(__GNU__)
#  define KWSYSPE_PS_COMMAND "ps axo pid,ppid"
#  define KWSYSPE_PS_FORMAT "%d %d\n"
#elif defined(__sun) && (defined(__SVR4) || defined(__svr4__)) /* Solaris */
#  define KWSYSPE_PS_COMMAND "ps -e -o pid,ppid"
#  define KWSYSPE_PS_FORMAT "%d %d\n"
#elif defined(__hpux) || defined(__sun__) || defined(__sgi) ||                \
  defined(_AIX) || defined(__sparc)
#  define KWSYSPE_PS_COMMAND "ps -ef"
#  define KWSYSPE_PS_FORMAT "%*s %d %d %*[^\n]\n"
#elif defined(__QNX__)
#  define KWSYSPE_PS_COMMAND "ps -Af"
#  define KWSYSPE_PS_FORMAT "%*d %d %d %*[^\n]\n"
#elif defined(__CYGWIN__)
#  define KWSYSPE_PS_COMMAND "ps aux"
#  define KWSYSPE_PS_FORMAT "%d %d %*[^\n]\n"
#endif

void kwsysProcess_KillPID(unsigned long process_id)
{
  kwsysProcessKill((pid_t)process_id);
}

static void kwsysProcessKill(pid_t process_id)
{
#if defined(__linux__) || defined(__CYGWIN__)
  DIR* procdir;
#endif

  /* Suspend the process to be sure it will not create more children.  */
  kill(process_id, SIGSTOP);

#if defined(__CYGWIN__)
  /* Some Cygwin versions seem to need help here.  Give up our time slice
     so that the child can process SIGSTOP before we send SIGKILL.  */
  usleep(1);
#endif

/* Kill all children if we can find them.  */
#if defined(__linux__) || defined(__CYGWIN__)
  /* First try using the /proc filesystem.  */
  if ((procdir = opendir("/proc")) != NULL) {
#  if defined(MAXPATHLEN)
    char fname[MAXPATHLEN];
#  elif defined(PATH_MAX)
    char fname[PATH_MAX];
#  else
    char fname[4096];
#  endif
    char buffer[KWSYSPE_PIPE_BUFFER_SIZE + 1];
    struct dirent* d;

    /* Each process has a directory in /proc whose name is the pid.
       Within this directory is a file called stat that has the
       following format:

         pid (command line) status ppid ...

       We want to get the ppid for all processes.  Those that have
       process_id as their parent should be recursively killed.  */
    for (d = readdir(procdir); d; d = readdir(procdir)) {
      int pid;
      if (sscanf(d->d_name, "%d", &pid) == 1 && pid != 0) {
        struct stat finfo;
        sprintf(fname, "/proc/%d/stat", pid);
        if (stat(fname, &finfo) == 0) {
          FILE* f = fopen(fname, "r");
          if (f) {
            size_t nread = fread(buffer, 1, KWSYSPE_PIPE_BUFFER_SIZE, f);
            fclose(f);
            buffer[nread] = '\0';
            if (nread > 0) {
              const char* rparen = strrchr(buffer, ')');
              int ppid;
              if (rparen && (sscanf(rparen + 1, "%*s %d", &ppid) == 1)) {
                if (ppid == process_id) {
                  /* Recursively kill this child and its children.  */
                  kwsysProcessKill(pid);
                }
              }
            }
          }
        }
      }
    }
    closedir(procdir);
  } else
#endif
  {
#if defined(KWSYSPE_PS_COMMAND)
    /* Try running "ps" to get the process information.  */
    FILE* ps = popen(KWSYSPE_PS_COMMAND, "r");

    /* Make sure the process started and provided a valid header.  */
    if (ps && fscanf(ps, "%*[^\n]\n") != EOF) {
      /* Look for processes whose parent is the process being killed.  */
      int pid, ppid;
      while (fscanf(ps, KWSYSPE_PS_FORMAT, &pid, &ppid) == 2) {
        if (ppid == process_id) {
          /* Recursively kill this child and its children.  */
          kwsysProcessKill(pid);
        }
      }
    }

    /* We are done with the ps process.  */
    if (ps) {
      pclose(ps);
    }
#endif
  }

  /* Kill the process.  */
  kill(process_id, SIGKILL);

#if defined(__APPLE__)
  /* On OS X 10.3 the above SIGSTOP occasionally prevents the SIGKILL
     from working.  Just in case, we resume the child and kill it
     again.  There is a small race condition in this obscure case.  If
     the child manages to fork again between these two signals, we
     will not catch its children.  */
  kill(process_id, SIGCONT);
  kill(process_id, SIGKILL);
#endif
}

#if defined(__VMS)
int decc$feature_get_index(const char* name);
int decc$feature_set_value(int index, int mode, int value);
static int kwsysProcessSetVMSFeature(const char* name, int value)
{
  int i;
  errno = 0;
  i = decc$feature_get_index(name);
  return i >= 0 && (decc$feature_set_value(i, 1, value) >= 0 || errno == 0);
}
#endif

/* Global set of executing processes for use by the signal handler.
   This global instance will be zero-initialized by the compiler.  */
typedef struct kwsysProcessInstances_s
{
  int Count;
  int Size;
  kwsysProcess** Processes;
} kwsysProcessInstances;
static kwsysProcessInstances kwsysProcesses;

/* The old SIGCHLD / SIGINT / SIGTERM handlers.  */
static struct sigaction kwsysProcessesOldSigChldAction;
static struct sigaction kwsysProcessesOldSigIntAction;
static struct sigaction kwsysProcessesOldSigTermAction;

static void kwsysProcessesUpdate(kwsysProcessInstances* newProcesses)
{
  /* Block signals while we update the set of pipes to check.
     TODO: sigprocmask is undefined for threaded apps.  See
     pthread_sigmask.  */
  sigset_t newset;
  sigset_t oldset;
  sigemptyset(&newset);
  sigaddset(&newset, SIGCHLD);
  sigaddset(&newset, SIGINT);
  sigaddset(&newset, SIGTERM);
  sigprocmask(SIG_BLOCK, &newset, &oldset);

  /* Store the new set in that seen by the signal handler.  */
  kwsysProcesses = *newProcesses;

  /* Restore the signal mask to the previous setting.  */
  sigprocmask(SIG_SETMASK, &oldset, 0);
}

static int kwsysProcessesAdd(kwsysProcess* cp)
{
  /* Create a pipe through which the signal handler can notify the
     given process object that a child has exited.  */
  {
    /* Create the pipe.  */
    int p[2];
    if (pipe(p KWSYSPE_VMS_NONBLOCK) < 0) {
      return 0;
    }

    /* Store the pipes now to be sure they are cleaned up later.  */
    cp->PipeReadEnds[KWSYSPE_PIPE_SIGNAL] = p[0];
    cp->SignalPipe = p[1];

    /* Switch the pipe to non-blocking mode so that reading a byte can
       be an atomic test-and-set.  */
    if (!kwsysProcessSetNonBlocking(p[0]) ||
        !kwsysProcessSetNonBlocking(p[1])) {
      return 0;
    }

    /* The children do not need this pipe.  Set close-on-exec flag on
       the pipe's ends.  */
    if ((fcntl(p[0], F_SETFD, FD_CLOEXEC) < 0) ||
        (fcntl(p[1], F_SETFD, FD_CLOEXEC) < 0)) {
      return 0;
    }
  }

  /* Attempt to add the given signal pipe to the signal handler set.  */
  {

    /* Make sure there is enough space for the new signal pipe.  */
    kwsysProcessInstances oldProcesses = kwsysProcesses;
    kwsysProcessInstances newProcesses = oldProcesses;
    if (oldProcesses.Count == oldProcesses.Size) {
      /* Start with enough space for a small number of process instances
         and double the size each time more is needed.  */
      newProcesses.Size = oldProcesses.Size ? oldProcesses.Size * 2 : 4;

      /* Try allocating the new block of memory.  */
      if ((newProcesses.Processes = ((kwsysProcess**)malloc(
             (size_t)(newProcesses.Size) * sizeof(kwsysProcess*))))) {
        /* Copy the old pipe set to the new memory.  */
        if (oldProcesses.Count > 0) {
          memcpy(newProcesses.Processes, oldProcesses.Processes,
                 ((size_t)(oldProcesses.Count) * sizeof(kwsysProcess*)));
        }
      } else {
        /* Failed to allocate memory for the new signal pipe set.  */
        return 0;
      }
    }

    /* Append the new signal pipe to the set.  */
    newProcesses.Processes[newProcesses.Count++] = cp;

    /* Store the new set in that seen by the signal handler.  */
    kwsysProcessesUpdate(&newProcesses);

    /* Free the original pipes if new ones were allocated.  */
    if (newProcesses.Processes != oldProcesses.Processes) {
      free(oldProcesses.Processes);
    }

    /* If this is the first process, enable the signal handler.  */
    if (newProcesses.Count == 1) {
      /* Install our handler for SIGCHLD.  Repeat call until it is not
         interrupted.  */
      struct sigaction newSigAction;
      memset(&newSigAction, 0, sizeof(struct sigaction));
#if KWSYSPE_USE_SIGINFO
      newSigAction.sa_sigaction = kwsysProcessesSignalHandler;
      newSigAction.sa_flags = SA_NOCLDSTOP | SA_SIGINFO;
#  ifdef SA_RESTART
      newSigAction.sa_flags |= SA_RESTART;
#  endif
#else
      newSigAction.sa_handler = kwsysProcessesSignalHandler;
      newSigAction.sa_flags = SA_NOCLDSTOP;
#endif
      sigemptyset(&newSigAction.sa_mask);
      while ((sigaction(SIGCHLD, &newSigAction,
                        &kwsysProcessesOldSigChldAction) < 0) &&
             (errno == EINTR))
        ;

      /* Install our handler for SIGINT / SIGTERM.  Repeat call until
         it is not interrupted.  */
      sigemptyset(&newSigAction.sa_mask);
      sigaddset(&newSigAction.sa_mask, SIGTERM);
      while ((sigaction(SIGINT, &newSigAction,
                        &kwsysProcessesOldSigIntAction) < 0) &&
             (errno == EINTR))
        ;

      sigemptyset(&newSigAction.sa_mask);
      sigaddset(&newSigAction.sa_mask, SIGINT);
      while ((sigaction(SIGTERM, &newSigAction,
                        &kwsysProcessesOldSigIntAction) < 0) &&
             (errno == EINTR))
        ;
    }
  }

  return 1;
}

static void kwsysProcessesRemove(kwsysProcess* cp)
{
  /* Attempt to remove the given signal pipe from the signal handler set.  */
  {
    /* Find the given process in the set.  */
    kwsysProcessInstances newProcesses = kwsysProcesses;
    int i;
    for (i = 0; i < newProcesses.Count; ++i) {
      if (newProcesses.Processes[i] == cp) {
        break;
      }
    }
    if (i < newProcesses.Count) {
      /* Remove the process from the set.  */
      --newProcesses.Count;
      for (; i < newProcesses.Count; ++i) {
        newProcesses.Processes[i] = newProcesses.Processes[i + 1];
      }

      /* If this was the last process, disable the signal handler.  */
      if (newProcesses.Count == 0) {
        /* Restore the signal handlers.  Repeat call until it is not
           interrupted.  */
        while ((sigaction(SIGCHLD, &kwsysProcessesOldSigChldAction, 0) < 0) &&
               (errno == EINTR))
          ;
        while ((sigaction(SIGINT, &kwsysProcessesOldSigIntAction, 0) < 0) &&
               (errno == EINTR))
          ;
        while ((sigaction(SIGTERM, &kwsysProcessesOldSigTermAction, 0) < 0) &&
               (errno == EINTR))
          ;

        /* Free the table of process pointers since it is now empty.
           This is safe because the signal handler has been removed.  */
        newProcesses.Size = 0;
        free(newProcesses.Processes);
        newProcesses.Processes = 0;
      }

      /* Store the new set in that seen by the signal handler.  */
      kwsysProcessesUpdate(&newProcesses);
    }
  }

  /* Close the pipe through which the signal handler may have notified
     the given process object that a child has exited.  */
  kwsysProcessCleanupDescriptor(&cp->SignalPipe);
}

static void kwsysProcessesSignalHandler(int signum
#if KWSYSPE_USE_SIGINFO
                                        ,
                                        siginfo_t* info, void* ucontext
#endif
)
{
  int i, j, procStatus, old_errno = errno;
#if KWSYSPE_USE_SIGINFO
  (void)info;
  (void)ucontext;
#endif

  /* Signal all process objects that a child has terminated.  */
  switch (signum) {
    case SIGCHLD:
      for (i = 0; i < kwsysProcesses.Count; ++i) {
        /* Set the pipe in a signalled state.  */
        char buf = 1;
        kwsysProcess* cp = kwsysProcesses.Processes[i];
        kwsysProcess_ssize_t pipeStatus =
          read(cp->PipeReadEnds[KWSYSPE_PIPE_SIGNAL], &buf, 1);
        (void)pipeStatus;
        pipeStatus = write(cp->SignalPipe, &buf, 1);
        (void)pipeStatus;
      }
      break;
    case SIGINT:
    case SIGTERM:
      /* Signal child processes that are running in new process groups.  */
      for (i = 0; i < kwsysProcesses.Count; ++i) {
        kwsysProcess* cp = kwsysProcesses.Processes[i];
        /* Check Killed to avoid data race condition when killing.
           Check State to avoid data race condition in kwsysProcessCleanup
           when there is an error (it leaves a reaped PID).  */
        if (cp->CreateProcessGroup && !cp->Killed &&
            cp->State != kwsysProcess_State_Error && cp->ForkPIDs) {
          for (j = 0; j < cp->NumberOfCommands; ++j) {
            /* Make sure the PID is still valid. */
            if (cp->ForkPIDs[j]) {
              /* The user created a process group for this process.  The group
                 ID
                 is the process ID for the original process in the group.  */
              kill(-cp->ForkPIDs[j], SIGINT);
            }
          }
        }
      }

      /* Wait for all processes to terminate.  */
      while (wait(&procStatus) >= 0 || errno != ECHILD) {
      }

      /* Terminate the process, which is now in an inconsistent state
         because we reaped all the PIDs that it may have been reaping
         or may have reaped in the future.  Reraise the signal so that
         the proper exit code is returned.  */
      {
        /* Install default signal handler.  */
        struct sigaction defSigAction;
        sigset_t unblockSet;
        memset(&defSigAction, 0, sizeof(defSigAction));
        defSigAction.sa_handler = SIG_DFL;
        sigemptyset(&defSigAction.sa_mask);
        while ((sigaction(signum, &defSigAction, 0) < 0) && (errno == EINTR))
          ;
        /* Unmask the signal.  */
        sigemptyset(&unblockSet);
        sigaddset(&unblockSet, signum);
        sigprocmask(SIG_UNBLOCK, &unblockSet, 0);
        /* Raise the signal again.  */
        raise(signum);
        /* We shouldn't get here... but if we do... */
        _exit(1);
      }
      /* break omitted to silence unreachable code clang compiler warning.  */
  }

#if !KWSYSPE_USE_SIGINFO
  /* Re-Install our handler.  Repeat call until it is not interrupted.  */
  {
    struct sigaction newSigAction;
    struct sigaction& oldSigAction;
    memset(&newSigAction, 0, sizeof(struct sigaction));
    newSigChldAction.sa_handler = kwsysProcessesSignalHandler;
    newSigChldAction.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&newSigAction.sa_mask);
    switch (signum) {
      case SIGCHLD:
        oldSigAction = &kwsysProcessesOldSigChldAction;
        break;
      case SIGINT:
        sigaddset(&newSigAction.sa_mask, SIGTERM);
        oldSigAction = &kwsysProcessesOldSigIntAction;
        break;
      case SIGTERM:
        sigaddset(&newSigAction.sa_mask, SIGINT);
        oldSigAction = &kwsysProcessesOldSigTermAction;
        break;
      default:
        return 0;
    }
    while ((sigaction(signum, &newSigAction, oldSigAction) < 0) &&
           (errno == EINTR))
      ;
  }
#endif

  errno = old_errno;
}

void kwsysProcess_ResetStartTime(kwsysProcess* cp)
{
  if (!cp) {
    return;
  }
  /* Reset start time. */
  cp->StartTime = kwsysProcessTimeGetCurrent();
}
