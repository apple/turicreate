/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(Process.h)
#include KWSYS_HEADER(Encoding.h)

/* Work-around CMake dependency scanning limitation.  This must
   duplicate the above list of headers.  */
#if 0
#include "Encoding.h.in"
#include "Process.h.in"
#endif

/*

Implementation for Windows

On windows, a thread is created to wait for data on each pipe.  The
threads are synchronized with the main thread to simulate the use of
a UNIX-style select system call.

*/

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#include <windows.h> /* Windows API */
#if defined(_MSC_VER) && _MSC_VER >= 1800
#define KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#endif
#include <io.h>     /* _unlink */
#include <stdio.h>  /* sprintf */
#include <string.h> /* strlen, strdup */
#ifdef __WATCOMC__
#define _unlink unlink
#endif

#ifndef _MAX_FNAME
#define _MAX_FNAME 4096
#endif
#ifndef _MAX_PATH
#define _MAX_PATH 4096
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(disable : 4514)
#pragma warning(disable : 4706)
#endif

#if defined(__BORLANDC__)
#pragma warn - 8004 /* assigned a value that is never used  */
#pragma warn - 8060 /* Assignment inside if() condition.  */
#endif

/* There are pipes for the process pipeline's stdout and stderr.  */
#define KWSYSPE_PIPE_COUNT 2
#define KWSYSPE_PIPE_STDOUT 0
#define KWSYSPE_PIPE_STDERR 1

/* The maximum amount to read from a pipe at a time.  */
#define KWSYSPE_PIPE_BUFFER_SIZE 1024

/* Debug output macro.  */
#if 0
#define KWSYSPE_DEBUG(x)                                                      \
  ((void*)cp == (void*)0x00226DE0                                             \
     ? (fprintf(stderr, "%d/%p/%d ", (int)GetCurrentProcessId(), cp,          \
                __LINE__),                                                    \
        fprintf x, fflush(stderr), 1)                                         \
     : (1))
#else
#define KWSYSPE_DEBUG(x) (void)1
#endif

typedef LARGE_INTEGER kwsysProcessTime;

typedef struct kwsysProcessCreateInformation_s
{
  /* Windows child startup control data.  */
  STARTUPINFOW StartupInfo;

  /* Original handles before making inherited duplicates.  */
  HANDLE hStdInput;
  HANDLE hStdOutput;
  HANDLE hStdError;
} kwsysProcessCreateInformation;

typedef struct kwsysProcessPipeData_s kwsysProcessPipeData;
static DWORD WINAPI kwsysProcessPipeThreadRead(LPVOID ptd);
static void kwsysProcessPipeThreadReadPipe(kwsysProcess* cp,
                                           kwsysProcessPipeData* td);
static DWORD WINAPI kwsysProcessPipeThreadWake(LPVOID ptd);
static void kwsysProcessPipeThreadWakePipe(kwsysProcess* cp,
                                           kwsysProcessPipeData* td);
static int kwsysProcessInitialize(kwsysProcess* cp);
static DWORD kwsysProcessCreate(kwsysProcess* cp, int index,
                                kwsysProcessCreateInformation* si);
static void kwsysProcessDestroy(kwsysProcess* cp, int event);
static DWORD kwsysProcessSetupOutputPipeFile(PHANDLE handle, const char* name);
static void kwsysProcessSetupSharedPipe(DWORD nStdHandle, PHANDLE handle);
static void kwsysProcessSetupPipeNative(HANDLE native, PHANDLE handle);
static void kwsysProcessCleanupHandle(PHANDLE h);
static void kwsysProcessCleanup(kwsysProcess* cp, DWORD error);
static void kwsysProcessCleanErrorMessage(kwsysProcess* cp);
static int kwsysProcessGetTimeoutTime(kwsysProcess* cp, double* userTimeout,
                                      kwsysProcessTime* timeoutTime);
static int kwsysProcessGetTimeoutLeft(kwsysProcessTime* timeoutTime,
                                      double* userTimeout,
                                      kwsysProcessTime* timeoutLength);
static kwsysProcessTime kwsysProcessTimeGetCurrent(void);
static DWORD kwsysProcessTimeToDWORD(kwsysProcessTime t);
static double kwsysProcessTimeToDouble(kwsysProcessTime t);
static kwsysProcessTime kwsysProcessTimeFromDouble(double d);
static int kwsysProcessTimeLess(kwsysProcessTime in1, kwsysProcessTime in2);
static kwsysProcessTime kwsysProcessTimeAdd(kwsysProcessTime in1,
                                            kwsysProcessTime in2);
static kwsysProcessTime kwsysProcessTimeSubtract(kwsysProcessTime in1,
                                                 kwsysProcessTime in2);
static void kwsysProcessSetExitException(kwsysProcess* cp, int code);
static void kwsysProcessSetExitExceptionByIndex(kwsysProcess* cp, int code,
                                                int idx);
static void kwsysProcessKillTree(int pid);
static void kwsysProcessDisablePipeThreads(kwsysProcess* cp);
static int kwsysProcessesInitialize(void);
static int kwsysTryEnterCreateProcessSection(void);
static void kwsysLeaveCreateProcessSection(void);
static int kwsysProcessesAdd(HANDLE hProcess, DWORD dwProcessId,
                             int newProcessGroup);
static void kwsysProcessesRemove(HANDLE hProcess);
static BOOL WINAPI kwsysCtrlHandler(DWORD dwCtrlType);

/* A structure containing synchronization data for each thread.  */
typedef struct kwsysProcessPipeSync_s kwsysProcessPipeSync;
struct kwsysProcessPipeSync_s
{
  /* Handle to the thread.  */
  HANDLE Thread;

  /* Semaphore indicating to the thread that a process has started.  */
  HANDLE Ready;

  /* Semaphore indicating to the thread that it should begin work.  */
  HANDLE Go;

  /* Semaphore indicating thread has reset for another process.  */
  HANDLE Reset;
};

/* A structure containing data for each pipe's threads.  */
struct kwsysProcessPipeData_s
{
  /* ------------- Data managed per instance of kwsysProcess ------------- */

  /* Synchronization data for reading thread.  */
  kwsysProcessPipeSync Reader;

  /* Synchronization data for waking thread.  */
  kwsysProcessPipeSync Waker;

  /* Index of this pipe.  */
  int Index;

  /* The kwsysProcess instance owning this pipe.  */
  kwsysProcess* Process;

  /* ------------- Data managed per call to Execute ------------- */

  /* Buffer for data read in this pipe's thread.  */
  char DataBuffer[KWSYSPE_PIPE_BUFFER_SIZE];

  /* The length of the data stored in the buffer.  */
  DWORD DataLength;

  /* Whether the pipe has been closed.  */
  int Closed;

  /* Handle for the read end of this pipe. */
  HANDLE Read;

  /* Handle for the write end of this pipe. */
  HANDLE Write;
};

/* A structure containing results data for each process.  */
typedef struct kwsysProcessResults_s kwsysProcessResults;
struct kwsysProcessResults_s
{
  /* The status of the process.  */
  int State;

  /* The exceptional behavior that terminated the process, if any.  */
  int ExitException;

  /* The process exit code.  */
  DWORD ExitCode;

  /* The process return code, if any.  */
  int ExitValue;

  /* Description for the ExitException.  */
  char ExitExceptionString[KWSYSPE_PIPE_BUFFER_SIZE + 1];
};

/* Structure containing data used to implement the child's execution.  */
struct kwsysProcess_s
{
  /* ------------- Data managed per instance of kwsysProcess ------------- */

  /* The status of the process structure.  */
  int State;

  /* The command lines to execute.  */
  wchar_t** Commands;
  int NumberOfCommands;

  /* The exit code of each command.  */
  DWORD* CommandExitCodes;

  /* The working directory for the child process.  */
  wchar_t* WorkingDirectory;

  /* Whether to create the child as a detached process.  */
  int OptionDetach;

  /* Whether the child was created as a detached process.  */
  int Detached;

  /* Whether to hide the child process's window.  */
  int HideWindow;

  /* Whether to treat command lines as verbatim.  */
  int Verbatim;

  /* Whether to merge stdout/stderr of the child.  */
  int MergeOutput;

  /* Whether to create the process in a new process group.  */
  int CreateProcessGroup;

  /* Mutex to protect the shared index used by threads to report data.  */
  HANDLE SharedIndexMutex;

  /* Semaphore used by threads to signal data ready.  */
  HANDLE Full;

  /* Whether we are currently deleting this kwsysProcess instance.  */
  int Deleting;

  /* Data specific to each pipe and its thread.  */
  kwsysProcessPipeData Pipe[KWSYSPE_PIPE_COUNT];

  /* Name of files to which stdin and stdout pipes are attached.  */
  char* PipeFileSTDIN;
  char* PipeFileSTDOUT;
  char* PipeFileSTDERR;

  /* Whether each pipe is shared with the parent process.  */
  int PipeSharedSTDIN;
  int PipeSharedSTDOUT;
  int PipeSharedSTDERR;

  /* Native pipes provided by the user.  */
  HANDLE PipeNativeSTDIN[2];
  HANDLE PipeNativeSTDOUT[2];
  HANDLE PipeNativeSTDERR[2];

  /* ------------- Data managed per call to Execute ------------- */

  /* Index of last pipe to report data, if any.  */
  int CurrentIndex;

  /* Index shared by threads to report data.  */
  int SharedIndex;

  /* The timeout length.  */
  double Timeout;

  /* Time at which the child started.  */
  kwsysProcessTime StartTime;

  /* Time at which the child will timeout.  Negative for no timeout.  */
  kwsysProcessTime TimeoutTime;

  /* Flag for whether the process was killed.  */
  int Killed;

  /* Flag for whether the timeout expired.  */
  int TimeoutExpired;

  /* Flag for whether the process has terminated.  */
  int Terminated;

  /* The number of pipes still open during execution and while waiting
     for pipes to close after process termination.  */
  int PipesLeft;

  /* Buffer for error messages.  */
  char ErrorMessage[KWSYSPE_PIPE_BUFFER_SIZE + 1];

  /* process results.  */
  kwsysProcessResults* ProcessResults;

  /* Windows process information data.  */
  PROCESS_INFORMATION* ProcessInformation;

  /* Data and process termination events for which to wait.  */
  PHANDLE ProcessEvents;
  int ProcessEventsLength;

  /* Real working directory of our own process.  */
  DWORD RealWorkingDirectoryLength;
  wchar_t* RealWorkingDirectory;

  /* Own handles for the child's ends of the pipes in the parent process.
     Used temporarily during process creation.  */
  HANDLE PipeChildStd[3];
};

kwsysProcess* kwsysProcess_New(void)
{
  int i;

  /* Process control structure.  */
  kwsysProcess* cp;

  /* Windows version number data.  */
  OSVERSIONINFO osv;

  /* Initialize list of processes before we get any farther.  It's especially
     important that the console Ctrl handler be added BEFORE starting the
     first process.  This prevents the risk of an orphaned process being
     started by the main thread while the default Ctrl handler is in
     progress.  */
  if (!kwsysProcessesInitialize()) {
    return 0;
  }

  /* Allocate a process control structure.  */
  cp = (kwsysProcess*)malloc(sizeof(kwsysProcess));
  if (!cp) {
    /* Could not allocate memory for the control structure.  */
    return 0;
  }
  ZeroMemory(cp, sizeof(*cp));

  /* Share stdin with the parent process by default.  */
  cp->PipeSharedSTDIN = 1;

  /* Set initial status.  */
  cp->State = kwsysProcess_State_Starting;

  /* Choose a method of running the child based on version of
     windows.  */
  ZeroMemory(&osv, sizeof(osv));
  osv.dwOSVersionInfoSize = sizeof(osv);
#ifdef KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#pragma warning(push)
#ifdef __INTEL_COMPILER
#pragma warning(disable : 1478)
#else
#pragma warning(disable : 4996)
#endif
#endif
  GetVersionEx(&osv);
#ifdef KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#pragma warning(pop)
#endif
  if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
    /* Win9x no longer supported.  */
    kwsysProcess_Delete(cp);
    return 0;
  }

  /* Initially no thread owns the mutex.  Initialize semaphore to 1.  */
  if (!(cp->SharedIndexMutex = CreateSemaphore(0, 1, 1, 0))) {
    kwsysProcess_Delete(cp);
    return 0;
  }

  /* Initially no data are available.  Initialize semaphore to 0.  */
  if (!(cp->Full = CreateSemaphore(0, 0, 1, 0))) {
    kwsysProcess_Delete(cp);
    return 0;
  }

  /* Create the thread to read each pipe.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    DWORD dummy = 0;

    /* Assign the thread its index.  */
    cp->Pipe[i].Index = i;

    /* Give the thread a pointer back to the kwsysProcess instance.  */
    cp->Pipe[i].Process = cp;

    /* No process is yet running.  Initialize semaphore to 0.  */
    if (!(cp->Pipe[i].Reader.Ready = CreateSemaphore(0, 0, 1, 0))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* The pipe is not yet reset.  Initialize semaphore to 0.  */
    if (!(cp->Pipe[i].Reader.Reset = CreateSemaphore(0, 0, 1, 0))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* The thread's buffer is initially empty.  Initialize semaphore to 1.  */
    if (!(cp->Pipe[i].Reader.Go = CreateSemaphore(0, 1, 1, 0))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* Create the reading thread.  It will block immediately.  The
       thread will not make deeply nested calls, so we need only a
       small stack.  */
    if (!(cp->Pipe[i].Reader.Thread = CreateThread(
            0, 1024, kwsysProcessPipeThreadRead, &cp->Pipe[i], 0, &dummy))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* No process is yet running.  Initialize semaphore to 0.  */
    if (!(cp->Pipe[i].Waker.Ready = CreateSemaphore(0, 0, 1, 0))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* The pipe is not yet reset.  Initialize semaphore to 0.  */
    if (!(cp->Pipe[i].Waker.Reset = CreateSemaphore(0, 0, 1, 0))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* The waker should not wake immediately.  Initialize semaphore to 0.  */
    if (!(cp->Pipe[i].Waker.Go = CreateSemaphore(0, 0, 1, 0))) {
      kwsysProcess_Delete(cp);
      return 0;
    }

    /* Create the waking thread.  It will block immediately.  The
       thread will not make deeply nested calls, so we need only a
       small stack.  */
    if (!(cp->Pipe[i].Waker.Thread = CreateThread(
            0, 1024, kwsysProcessPipeThreadWake, &cp->Pipe[i], 0, &dummy))) {
      kwsysProcess_Delete(cp);
      return 0;
    }
  }
  for (i = 0; i < 3; ++i) {
    cp->PipeChildStd[i] = INVALID_HANDLE_VALUE;
  }

  return cp;
}

void kwsysProcess_Delete(kwsysProcess* cp)
{
  int i;

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

  /* We are deleting the kwsysProcess instance.  */
  cp->Deleting = 1;

  /* Terminate each of the threads.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    /* Terminate this reading thread.  */
    if (cp->Pipe[i].Reader.Thread) {
      /* Signal the thread we are ready for it.  It will terminate
         immediately since Deleting is set.  */
      ReleaseSemaphore(cp->Pipe[i].Reader.Ready, 1, 0);

      /* Wait for the thread to exit.  */
      WaitForSingleObject(cp->Pipe[i].Reader.Thread, INFINITE);

      /* Close the handle to the thread. */
      kwsysProcessCleanupHandle(&cp->Pipe[i].Reader.Thread);
    }

    /* Terminate this waking thread.  */
    if (cp->Pipe[i].Waker.Thread) {
      /* Signal the thread we are ready for it.  It will terminate
         immediately since Deleting is set.  */
      ReleaseSemaphore(cp->Pipe[i].Waker.Ready, 1, 0);

      /* Wait for the thread to exit.  */
      WaitForSingleObject(cp->Pipe[i].Waker.Thread, INFINITE);

      /* Close the handle to the thread. */
      kwsysProcessCleanupHandle(&cp->Pipe[i].Waker.Thread);
    }

    /* Cleanup the pipe's semaphores.  */
    kwsysProcessCleanupHandle(&cp->Pipe[i].Reader.Ready);
    kwsysProcessCleanupHandle(&cp->Pipe[i].Reader.Go);
    kwsysProcessCleanupHandle(&cp->Pipe[i].Reader.Reset);
    kwsysProcessCleanupHandle(&cp->Pipe[i].Waker.Ready);
    kwsysProcessCleanupHandle(&cp->Pipe[i].Waker.Go);
    kwsysProcessCleanupHandle(&cp->Pipe[i].Waker.Reset);
  }

  /* Close the shared semaphores.  */
  kwsysProcessCleanupHandle(&cp->SharedIndexMutex);
  kwsysProcessCleanupHandle(&cp->Full);

  /* Free memory.  */
  kwsysProcess_SetCommand(cp, 0);
  kwsysProcess_SetWorkingDirectory(cp, 0);
  kwsysProcess_SetPipeFile(cp, kwsysProcess_Pipe_STDIN, 0);
  kwsysProcess_SetPipeFile(cp, kwsysProcess_Pipe_STDOUT, 0);
  kwsysProcess_SetPipeFile(cp, kwsysProcess_Pipe_STDERR, 0);
  if (cp->CommandExitCodes) {
    free(cp->CommandExitCodes);
  }
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
  wchar_t** newCommands;

  /* Make sure we have a command to add.  */
  if (!cp || !command || !*command) {
    return 0;
  }

  /* Allocate a new array for command pointers.  */
  newNumberOfCommands = cp->NumberOfCommands + 1;
  if (!(newCommands =
          (wchar_t**)malloc(sizeof(wchar_t*) * newNumberOfCommands))) {
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

  if (cp->Verbatim) {
    /* Copy the verbatim command line into the buffer.  */
    newCommands[cp->NumberOfCommands] = kwsysEncoding_DupToWide(*command);
  } else {
    /* Encode the arguments so CommandLineToArgvW can decode
       them from the command line string in the child.  */
    char buffer[32768]; /* CreateProcess max command-line length.  */
    char* end = buffer + sizeof(buffer);
    char* out = buffer;
    char const* const* a;
    for (a = command; *a; ++a) {
      int quote = !**a; /* Quote the empty string.  */
      int slashes = 0;
      char const* c;
      if (a != command && out != end) {
        *out++ = ' ';
      }
      for (c = *a; !quote && *c; ++c) {
        quote = (*c == ' ' || *c == '\t');
      }
      if (quote && out != end) {
        *out++ = '"';
      }
      for (c = *a; *c; ++c) {
        if (*c == '\\') {
          ++slashes;
        } else {
          if (*c == '"') {
            // Add n+1 backslashes to total 2n+1 before internal '"'.
            while (slashes-- >= 0 && out != end) {
              *out++ = '\\';
            }
          }
          slashes = 0;
        }
        if (out != end) {
          *out++ = *c;
        }
      }
      if (quote) {
        // Add n backslashes to total 2n before ending '"'.
        while (slashes-- > 0 && out != end) {
          *out++ = '\\';
        }
        if (out != end) {
          *out++ = '"';
        }
      }
    }
    if (out != end) {
      *out = '\0';
      newCommands[cp->NumberOfCommands] = kwsysEncoding_DupToWide(buffer);
    } else {
      newCommands[cp->NumberOfCommands] = 0;
    }
  }
  if (!newCommands[cp->NumberOfCommands]) {
    /* Out of memory or command line too long.  */
    free(newCommands);
    return 0;
  }

  /* Save the new array of commands.  */
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
  cp->TimeoutTime.QuadPart = -1;
}

int kwsysProcess_SetWorkingDirectory(kwsysProcess* cp, const char* dir)
{
  if (!cp) {
    return 0;
  }
  if (cp->WorkingDirectory) {
    free(cp->WorkingDirectory);
    cp->WorkingDirectory = 0;
  }
  if (dir && dir[0]) {
    wchar_t* wdir = kwsysEncoding_DupToWide(dir);
    /* We must convert the working directory to a full path.  */
    DWORD length = GetFullPathNameW(wdir, 0, 0, 0);
    if (length > 0) {
      wchar_t* work_dir = malloc(length * sizeof(wchar_t));
      if (!work_dir) {
        free(wdir);
        return 0;
      }
      if (!GetFullPathNameW(wdir, length, work_dir, 0)) {
        free(work_dir);
        free(wdir);
        return 0;
      }
      cp->WorkingDirectory = work_dir;
    }
    free(wdir);
  }
  return 1;
}

int kwsysProcess_SetPipeFile(kwsysProcess* cp, int pipe, const char* file)
{
  char** pfile;
  if (!cp) {
    return 0;
  }
  switch (pipe) {
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
    *pfile = (char*)malloc(strlen(file) + 1);
    if (!*pfile) {
      return 0;
    }
    strcpy(*pfile, file);
  }

  /* If we are redirecting the pipe, do not share it or use a native
     pipe.  */
  if (*pfile) {
    kwsysProcess_SetPipeNative(cp, pipe, 0);
    kwsysProcess_SetPipeShared(cp, pipe, 0);
  }

  return 1;
}

void kwsysProcess_SetPipeShared(kwsysProcess* cp, int pipe, int shared)
{
  if (!cp) {
    return;
  }

  switch (pipe) {
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
    kwsysProcess_SetPipeFile(cp, pipe, 0);
    kwsysProcess_SetPipeNative(cp, pipe, 0);
  }
}

void kwsysProcess_SetPipeNative(kwsysProcess* cp, int pipe, HANDLE p[2])
{
  HANDLE* pPipeNative = 0;

  if (!cp) {
    return;
  }

  switch (pipe) {
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

  /* Copy the native pipe handles provided.  */
  if (p) {
    pPipeNative[0] = p[0];
    pPipeNative[1] = p[1];
  } else {
    pPipeNative[0] = 0;
    pPipeNative[1] = 0;
  }

  /* If we are using a native pipe, do not share it or redirect it to
     a file.  */
  if (p) {
    kwsysProcess_SetPipeFile(cp, pipe, 0);
    kwsysProcess_SetPipeShared(cp, pipe, 0);
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
    case kwsysProcess_Option_HideWindow:
      return cp->HideWindow;
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
    case kwsysProcess_Option_HideWindow:
      cp->HideWindow = value;
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

int kwsysProcess_GetExitValue(kwsysProcess* cp)
{
  return (cp && cp->ProcessResults && (cp->NumberOfCommands > 0))
    ? cp->ProcessResults[cp->NumberOfCommands - 1].ExitValue
    : -1;
}

int kwsysProcess_GetExitCode(kwsysProcess* cp)
{
  return (cp && cp->ProcessResults && (cp->NumberOfCommands > 0))
    ? cp->ProcessResults[cp->NumberOfCommands - 1].ExitCode
    : 0;
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
    KWSYSPE_DEBUG((stderr, "array index out of bound\n"));                    \
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

  /* Do not execute a second time.  */
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

  /* Save the real working directory of this process and change to
     the working directory for the child processes.  This is needed
     to make pipe file paths evaluate correctly.  */
  if (cp->WorkingDirectory) {
    if (!GetCurrentDirectoryW(cp->RealWorkingDirectoryLength,
                              cp->RealWorkingDirectory)) {
      kwsysProcessCleanup(cp, GetLastError());
      return;
    }
    SetCurrentDirectoryW(cp->WorkingDirectory);
  }

  /* Setup the stdin pipe for the first process.  */
  if (cp->PipeFileSTDIN) {
    /* Create a handle to read a file for stdin.  */
    wchar_t* wstdin = kwsysEncoding_DupToWide(cp->PipeFileSTDIN);
    DWORD error;
    cp->PipeChildStd[0] =
      CreateFileW(wstdin, GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    error = GetLastError(); /* Check now in case free changes this.  */
    free(wstdin);
    if (cp->PipeChildStd[0] == INVALID_HANDLE_VALUE) {
      kwsysProcessCleanup(cp, error);
      return;
    }
  } else if (cp->PipeSharedSTDIN) {
    /* Share this process's stdin with the child.  */
    kwsysProcessSetupSharedPipe(STD_INPUT_HANDLE, &cp->PipeChildStd[0]);
  } else if (cp->PipeNativeSTDIN[0]) {
    /* Use the provided native pipe.  */
    kwsysProcessSetupPipeNative(cp->PipeNativeSTDIN[0], &cp->PipeChildStd[0]);
  } else {
    /* Explicitly give the child no stdin.  */
    cp->PipeChildStd[0] = INVALID_HANDLE_VALUE;
  }

  /* Create the output pipe for the last process.
     We always create this so the pipe thread can run even if we
     do not end up giving the write end to the child below.  */
  if (!CreatePipe(&cp->Pipe[KWSYSPE_PIPE_STDOUT].Read,
                  &cp->Pipe[KWSYSPE_PIPE_STDOUT].Write, 0, 0)) {
    kwsysProcessCleanup(cp, GetLastError());
    return;
  }

  if (cp->PipeFileSTDOUT) {
    /* Use a file for stdout.  */
    DWORD error = kwsysProcessSetupOutputPipeFile(&cp->PipeChildStd[1],
                                                  cp->PipeFileSTDOUT);
    if (error) {
      kwsysProcessCleanup(cp, error);
      return;
    }
  } else if (cp->PipeSharedSTDOUT) {
    /* Use the parent stdout.  */
    kwsysProcessSetupSharedPipe(STD_OUTPUT_HANDLE, &cp->PipeChildStd[1]);
  } else if (cp->PipeNativeSTDOUT[1]) {
    /* Use the given handle for stdout.  */
    kwsysProcessSetupPipeNative(cp->PipeNativeSTDOUT[1], &cp->PipeChildStd[1]);
  } else {
    /* Use our pipe for stdout.  Duplicate the handle since our waker
       thread will use the original.  Do not make it inherited yet.  */
    if (!DuplicateHandle(GetCurrentProcess(),
                         cp->Pipe[KWSYSPE_PIPE_STDOUT].Write,
                         GetCurrentProcess(), &cp->PipeChildStd[1], 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
      kwsysProcessCleanup(cp, GetLastError());
      return;
    }
  }

  /* Create stderr pipe to be shared by all processes in the pipeline.
     We always create this so the pipe thread can run even if we do not
     end up giving the write end to the child below.  */
  if (!CreatePipe(&cp->Pipe[KWSYSPE_PIPE_STDERR].Read,
                  &cp->Pipe[KWSYSPE_PIPE_STDERR].Write, 0, 0)) {
    kwsysProcessCleanup(cp, GetLastError());
    return;
  }

  if (cp->PipeFileSTDERR) {
    /* Use a file for stderr.  */
    DWORD error = kwsysProcessSetupOutputPipeFile(&cp->PipeChildStd[2],
                                                  cp->PipeFileSTDERR);
    if (error) {
      kwsysProcessCleanup(cp, error);
      return;
    }
  } else if (cp->PipeSharedSTDERR) {
    /* Use the parent stderr.  */
    kwsysProcessSetupSharedPipe(STD_ERROR_HANDLE, &cp->PipeChildStd[2]);
  } else if (cp->PipeNativeSTDERR[1]) {
    /* Use the given handle for stderr.  */
    kwsysProcessSetupPipeNative(cp->PipeNativeSTDERR[1], &cp->PipeChildStd[2]);
  } else {
    /* Use our pipe for stderr.  Duplicate the handle since our waker
       thread will use the original.  Do not make it inherited yet.  */
    if (!DuplicateHandle(GetCurrentProcess(),
                         cp->Pipe[KWSYSPE_PIPE_STDERR].Write,
                         GetCurrentProcess(), &cp->PipeChildStd[2], 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
      kwsysProcessCleanup(cp, GetLastError());
      return;
    }
  }

  /* Create the pipeline of processes.  */
  {
    /* Child startup control data.  */
    kwsysProcessCreateInformation si;
    HANDLE nextStdInput = cp->PipeChildStd[0];

    /* Initialize startup info data.  */
    ZeroMemory(&si, sizeof(si));
    si.StartupInfo.cb = sizeof(si.StartupInfo);

    /* Decide whether a child window should be shown.  */
    si.StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    si.StartupInfo.wShowWindow =
      (unsigned short)(cp->HideWindow ? SW_HIDE : SW_SHOWDEFAULT);

    /* Connect the child's output pipes to the threads.  */
    si.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

    for (i = 0; i < cp->NumberOfCommands; ++i) {
      /* Setup the process's pipes.  */
      si.hStdInput = nextStdInput;
      if (i == cp->NumberOfCommands - 1) {
        /* The last child gets the overall stdout.  */
        nextStdInput = INVALID_HANDLE_VALUE;
        si.hStdOutput = cp->PipeChildStd[1];
      } else {
        /* Create a pipe to sit between the children.  */
        HANDLE p[2] = { INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE };
        if (!CreatePipe(&p[0], &p[1], 0, 0)) {
          DWORD error = GetLastError();
          if (nextStdInput != cp->PipeChildStd[0]) {
            kwsysProcessCleanupHandle(&nextStdInput);
          }
          kwsysProcessCleanup(cp, error);
          return;
        }
        nextStdInput = p[0];
        si.hStdOutput = p[1];
      }
      si.hStdError =
        cp->MergeOutput ? cp->PipeChildStd[1] : cp->PipeChildStd[2];

      {
        DWORD error = kwsysProcessCreate(cp, i, &si);

        /* Close our copies of pipes used between children.  */
        if (si.hStdInput != cp->PipeChildStd[0]) {
          kwsysProcessCleanupHandle(&si.hStdInput);
        }
        if (si.hStdOutput != cp->PipeChildStd[1]) {
          kwsysProcessCleanupHandle(&si.hStdOutput);
        }
        if (si.hStdError != cp->PipeChildStd[2] && !cp->MergeOutput) {
          kwsysProcessCleanupHandle(&si.hStdError);
        }
        if (!error) {
          cp->ProcessEvents[i + 1] = cp->ProcessInformation[i].hProcess;
        } else {
          if (nextStdInput != cp->PipeChildStd[0]) {
            kwsysProcessCleanupHandle(&nextStdInput);
          }
          kwsysProcessCleanup(cp, error);
          return;
        }
      }
    }
  }

  /* The parent process does not need the child's pipe ends.  */
  for (i = 0; i < 3; ++i) {
    kwsysProcessCleanupHandle(&cp->PipeChildStd[i]);
  }

  /* Restore the working directory.  */
  if (cp->RealWorkingDirectory) {
    SetCurrentDirectoryW(cp->RealWorkingDirectory);
    free(cp->RealWorkingDirectory);
    cp->RealWorkingDirectory = 0;
  }

  /* The timeout period starts now.  */
  cp->StartTime = kwsysProcessTimeGetCurrent();
  cp->TimeoutTime = kwsysProcessTimeFromDouble(-1);

  /* All processes in the pipeline have been started in suspended
     mode.  Resume them all now.  */
  for (i = 0; i < cp->NumberOfCommands; ++i) {
    ResumeThread(cp->ProcessInformation[i].hThread);
  }

  /* ---- It is no longer safe to call kwsysProcessCleanup. ----- */
  /* Tell the pipe threads that a process has started.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    ReleaseSemaphore(cp->Pipe[i].Reader.Ready, 1, 0);
    ReleaseSemaphore(cp->Pipe[i].Waker.Ready, 1, 0);
  }

  /* We don't care about the children's main threads.  */
  for (i = 0; i < cp->NumberOfCommands; ++i) {
    kwsysProcessCleanupHandle(&cp->ProcessInformation[i].hThread);
  }

  /* No pipe has reported data.  */
  cp->CurrentIndex = KWSYSPE_PIPE_COUNT;
  cp->PipesLeft = KWSYSPE_PIPE_COUNT;

  /* The process has now started.  */
  cp->State = kwsysProcess_State_Executing;
  cp->Detached = cp->OptionDetach;
}

void kwsysProcess_Disown(kwsysProcess* cp)
{
  int i;

  /* Make sure we are executing a detached process.  */
  if (!cp || !cp->Detached || cp->State != kwsysProcess_State_Executing ||
      cp->TimeoutExpired || cp->Killed || cp->Terminated) {
    return;
  }

  /* Disable the reading threads.  */
  kwsysProcessDisablePipeThreads(cp);

  /* Wait for all pipe threads to reset.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    WaitForSingleObject(cp->Pipe[i].Reader.Reset, INFINITE);
    WaitForSingleObject(cp->Pipe[i].Waker.Reset, INFINITE);
  }

  /* We will not wait for exit, so cleanup now.  */
  kwsysProcessCleanup(cp, 0);

  /* The process has been disowned.  */
  cp->State = kwsysProcess_State_Disowned;
}

int kwsysProcess_WaitForData(kwsysProcess* cp, char** data, int* length,
                             double* userTimeout)
{
  kwsysProcessTime userStartTime;
  kwsysProcessTime timeoutLength;
  kwsysProcessTime timeoutTime;
  DWORD timeout;
  int user;
  int done = 0;
  int expired = 0;
  int pipeId = kwsysProcess_Pipe_None;
  DWORD w;

  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing || cp->Killed ||
      cp->TimeoutExpired) {
    return kwsysProcess_Pipe_None;
  }

  /* Record the time at which user timeout period starts.  */
  userStartTime = kwsysProcessTimeGetCurrent();

  /* Calculate the time at which a timeout will expire, and whether it
     is the user or process timeout.  */
  user = kwsysProcessGetTimeoutTime(cp, userTimeout, &timeoutTime);

  /* Loop until we have a reason to return.  */
  while (!done && cp->PipesLeft > 0) {
    /* If we previously got data from a thread, let it know we are
       done with the data.  */
    if (cp->CurrentIndex < KWSYSPE_PIPE_COUNT) {
      KWSYSPE_DEBUG((stderr, "releasing reader %d\n", cp->CurrentIndex));
      ReleaseSemaphore(cp->Pipe[cp->CurrentIndex].Reader.Go, 1, 0);
      cp->CurrentIndex = KWSYSPE_PIPE_COUNT;
    }

    /* Setup a timeout if required.  */
    if (kwsysProcessGetTimeoutLeft(&timeoutTime, user ? userTimeout : 0,
                                   &timeoutLength)) {
      /* Timeout has already expired.  */
      expired = 1;
      break;
    }
    if (timeoutTime.QuadPart < 0) {
      timeout = INFINITE;
    } else {
      timeout = kwsysProcessTimeToDWORD(timeoutLength);
    }

    /* Wait for a pipe's thread to signal or a process to terminate.  */
    w = WaitForMultipleObjects(cp->ProcessEventsLength, cp->ProcessEvents, 0,
                               timeout);
    if (w == WAIT_TIMEOUT) {
      /* Timeout has expired.  */
      expired = 1;
      done = 1;
    } else if (w == WAIT_OBJECT_0) {
      /* Save the index of the reporting thread and release the mutex.
         The thread will block until we signal its Empty mutex.  */
      cp->CurrentIndex = cp->SharedIndex;
      ReleaseSemaphore(cp->SharedIndexMutex, 1, 0);

      /* Data are available or a pipe closed.  */
      if (cp->Pipe[cp->CurrentIndex].Closed) {
        /* The pipe closed at the write end.  Close the read end and
           inform the wakeup thread it is done with this process.  */
        kwsysProcessCleanupHandle(&cp->Pipe[cp->CurrentIndex].Read);
        ReleaseSemaphore(cp->Pipe[cp->CurrentIndex].Waker.Go, 1, 0);
        KWSYSPE_DEBUG((stderr, "wakeup %d\n", cp->CurrentIndex));
        --cp->PipesLeft;
      } else if (data && length) {
        /* Report this data.  */
        *data = cp->Pipe[cp->CurrentIndex].DataBuffer;
        *length = cp->Pipe[cp->CurrentIndex].DataLength;
        switch (cp->CurrentIndex) {
          case KWSYSPE_PIPE_STDOUT:
            pipeId = kwsysProcess_Pipe_STDOUT;
            break;
          case KWSYSPE_PIPE_STDERR:
            pipeId = kwsysProcess_Pipe_STDERR;
            break;
        }
        done = 1;
      }
    } else {
      /* A process has terminated.  */
      kwsysProcessDestroy(cp, w - WAIT_OBJECT_0);
    }
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
  if (pipeId) {
    /* Data are ready on a pipe.  */
    return pipeId;
  } else if (expired) {
    /* A timeout has expired.  */
    if (user) {
      /* The user timeout has expired.  It has no time left.  */
      return kwsysProcess_Pipe_Timeout;
    } else {
      /* The process timeout has expired.  Kill the child now.  */
      KWSYSPE_DEBUG((stderr, "killing child because timeout expired\n"));
      kwsysProcess_Kill(cp);
      cp->TimeoutExpired = 1;
      cp->Killed = 0;
      return kwsysProcess_Pipe_None;
    }
  } else {
    /* The children have terminated and no more data are available.  */
    return kwsysProcess_Pipe_None;
  }
}

int kwsysProcess_WaitForExit(kwsysProcess* cp, double* userTimeout)
{
  int i;
  int pipe;

  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing) {
    return 1;
  }

  /* Wait for the process to terminate.  Ignore all data.  */
  while ((pipe = kwsysProcess_WaitForData(cp, 0, 0, userTimeout)) > 0) {
    if (pipe == kwsysProcess_Pipe_Timeout) {
      /* The user timeout has expired.  */
      return 0;
    }
  }

  KWSYSPE_DEBUG((stderr, "no more data\n"));

  /* When the last pipe closes in WaitForData, the loop terminates
     without releasing the pipe's thread.  Release it now.  */
  if (cp->CurrentIndex < KWSYSPE_PIPE_COUNT) {
    KWSYSPE_DEBUG((stderr, "releasing reader %d\n", cp->CurrentIndex));
    ReleaseSemaphore(cp->Pipe[cp->CurrentIndex].Reader.Go, 1, 0);
    cp->CurrentIndex = KWSYSPE_PIPE_COUNT;
  }

  /* Wait for all pipe threads to reset.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    KWSYSPE_DEBUG((stderr, "waiting reader reset %d\n", i));
    WaitForSingleObject(cp->Pipe[i].Reader.Reset, INFINITE);
    KWSYSPE_DEBUG((stderr, "waiting waker reset %d\n", i));
    WaitForSingleObject(cp->Pipe[i].Waker.Reset, INFINITE);
  }

  /* ---- It is now safe again to call kwsysProcessCleanup. ----- */
  /* Close all the pipes.  */
  kwsysProcessCleanup(cp, 0);

  /* Determine the outcome.  */
  if (cp->Killed) {
    /* We killed the child.  */
    cp->State = kwsysProcess_State_Killed;
  } else if (cp->TimeoutExpired) {
    /* The timeout expired.  */
    cp->State = kwsysProcess_State_Expired;
  } else {
    /* The children exited.  Report the outcome of the child processes.  */
    for (i = 0; i < cp->NumberOfCommands; ++i) {
      cp->ProcessResults[i].ExitCode = cp->CommandExitCodes[i];
      if ((cp->ProcessResults[i].ExitCode & 0xF0000000) == 0xC0000000) {
        /* Child terminated due to exceptional behavior.  */
        cp->ProcessResults[i].State = kwsysProcess_StateByIndex_Exception;
        cp->ProcessResults[i].ExitValue = 1;
        kwsysProcessSetExitExceptionByIndex(cp, cp->ProcessResults[i].ExitCode,
                                            i);
      } else {
        /* Child exited without exception.  */
        cp->ProcessResults[i].State = kwsysProcess_StateByIndex_Exited;
        cp->ProcessResults[i].ExitException = kwsysProcess_Exception_None;
        cp->ProcessResults[i].ExitValue = cp->ProcessResults[i].ExitCode;
      }
    }
    /* support legacy state status value */
    cp->State = cp->ProcessResults[cp->NumberOfCommands - 1].State;
  }

  return 1;
}

void kwsysProcess_Interrupt(kwsysProcess* cp)
{
  int i;
  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing || cp->TimeoutExpired ||
      cp->Killed) {
    KWSYSPE_DEBUG((stderr, "interrupt: child not executing\n"));
    return;
  }

  /* Skip actually interrupting the child if it has already terminated.  */
  if (cp->Terminated) {
    KWSYSPE_DEBUG((stderr, "interrupt: child already terminated\n"));
    return;
  }

  /* Interrupt the children.  */
  if (cp->CreateProcessGroup) {
    if (cp->ProcessInformation) {
      for (i = 0; i < cp->NumberOfCommands; ++i) {
        /* Make sure the process handle isn't closed (e.g. from disowning). */
        if (cp->ProcessInformation[i].hProcess) {
          /* The user created a process group for this process.  The group ID
             is the process ID for the original process in the group.  Note
             that we have to use Ctrl+Break: Ctrl+C is not allowed for process
             groups.  */
          GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT,
                                   cp->ProcessInformation[i].dwProcessId);
        }
      }
    }
  } else {
    /* No process group was created.  Kill our own process group...  */
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
  }
}

void kwsysProcess_Kill(kwsysProcess* cp)
{
  int i;
  /* Make sure we are executing a process.  */
  if (!cp || cp->State != kwsysProcess_State_Executing || cp->TimeoutExpired ||
      cp->Killed) {
    KWSYSPE_DEBUG((stderr, "kill: child not executing\n"));
    return;
  }

  /* Disable the reading threads.  */
  KWSYSPE_DEBUG((stderr, "kill: disabling pipe threads\n"));
  kwsysProcessDisablePipeThreads(cp);

  /* Skip actually killing the child if it has already terminated.  */
  if (cp->Terminated) {
    KWSYSPE_DEBUG((stderr, "kill: child already terminated\n"));
    return;
  }

  /* Kill the children.  */
  cp->Killed = 1;
  for (i = 0; i < cp->NumberOfCommands; ++i) {
    kwsysProcessKillTree(cp->ProcessInformation[i].dwProcessId);
    /* Remove from global list of processes and close handles.  */
    kwsysProcessesRemove(cp->ProcessInformation[i].hProcess);
    kwsysProcessCleanupHandle(&cp->ProcessInformation[i].hThread);
    kwsysProcessCleanupHandle(&cp->ProcessInformation[i].hProcess);
  }

  /* We are killing the children and ignoring all data.  Do not wait
     for them to exit.  */
}

/*
  Function executed for each pipe's thread.  Argument is a pointer to
  the kwsysProcessPipeData instance for this thread.
*/
DWORD WINAPI kwsysProcessPipeThreadRead(LPVOID ptd)
{
  kwsysProcessPipeData* td = (kwsysProcessPipeData*)ptd;
  kwsysProcess* cp = td->Process;

  /* Wait for a process to be ready.  */
  while ((WaitForSingleObject(td->Reader.Ready, INFINITE), !cp->Deleting)) {
    /* Read output from the process for this thread's pipe.  */
    kwsysProcessPipeThreadReadPipe(cp, td);

    /* Signal the main thread we have reset for a new process.  */
    ReleaseSemaphore(td->Reader.Reset, 1, 0);
  }
  return 0;
}

/*
  Function called in each pipe's thread to handle data for one
  execution of a subprocess.
*/
void kwsysProcessPipeThreadReadPipe(kwsysProcess* cp, kwsysProcessPipeData* td)
{
  /* Wait for space in the thread's buffer. */
  while ((KWSYSPE_DEBUG((stderr, "wait for read %d\n", td->Index)),
          WaitForSingleObject(td->Reader.Go, INFINITE), !td->Closed)) {
    KWSYSPE_DEBUG((stderr, "reading %d\n", td->Index));

    /* Read data from the pipe.  This may block until data are available.  */
    if (!ReadFile(td->Read, td->DataBuffer, KWSYSPE_PIPE_BUFFER_SIZE,
                  &td->DataLength, 0)) {
      if (GetLastError() != ERROR_BROKEN_PIPE) {
        /* UNEXPECTED failure to read the pipe.  */
      }

      /* The pipe closed.  There are no more data to read.  */
      td->Closed = 1;
      KWSYSPE_DEBUG((stderr, "read closed %d\n", td->Index));
    }

    KWSYSPE_DEBUG((stderr, "read %d\n", td->Index));

    /* Wait for our turn to be handled by the main thread.  */
    WaitForSingleObject(cp->SharedIndexMutex, INFINITE);

    KWSYSPE_DEBUG((stderr, "reporting read %d\n", td->Index));

    /* Tell the main thread we have something to report.  */
    cp->SharedIndex = td->Index;
    ReleaseSemaphore(cp->Full, 1, 0);
  }

  /* We were signalled to exit with our buffer empty.  Reset the
     mutex for a new process.  */
  KWSYSPE_DEBUG((stderr, "self releasing reader %d\n", td->Index));
  ReleaseSemaphore(td->Reader.Go, 1, 0);
}

/*
  Function executed for each pipe's thread.  Argument is a pointer to
  the kwsysProcessPipeData instance for this thread.
*/
DWORD WINAPI kwsysProcessPipeThreadWake(LPVOID ptd)
{
  kwsysProcessPipeData* td = (kwsysProcessPipeData*)ptd;
  kwsysProcess* cp = td->Process;

  /* Wait for a process to be ready.  */
  while ((WaitForSingleObject(td->Waker.Ready, INFINITE), !cp->Deleting)) {
    /* Wait for a possible wakeup.  */
    kwsysProcessPipeThreadWakePipe(cp, td);

    /* Signal the main thread we have reset for a new process.  */
    ReleaseSemaphore(td->Waker.Reset, 1, 0);
  }
  return 0;
}

/*
  Function called in each pipe's thread to handle reading thread
  wakeup for one execution of a subprocess.
*/
void kwsysProcessPipeThreadWakePipe(kwsysProcess* cp, kwsysProcessPipeData* td)
{
  (void)cp;

  /* Wait for a possible wake command. */
  KWSYSPE_DEBUG((stderr, "wait for wake %d\n", td->Index));
  WaitForSingleObject(td->Waker.Go, INFINITE);
  KWSYSPE_DEBUG((stderr, "waking %d\n", td->Index));

  /* If the pipe is not closed, we need to wake up the reading thread.  */
  if (!td->Closed) {
    DWORD dummy;
    KWSYSPE_DEBUG((stderr, "waker %d writing byte\n", td->Index));
    WriteFile(td->Write, "", 1, &dummy, 0);
    KWSYSPE_DEBUG((stderr, "waker %d wrote byte\n", td->Index));
  }
}

/* Initialize a process control structure for kwsysProcess_Execute.  */
int kwsysProcessInitialize(kwsysProcess* cp)
{
  int i;
  /* Reset internal status flags.  */
  cp->TimeoutExpired = 0;
  cp->Terminated = 0;
  cp->Killed = 0;

  free(cp->ProcessResults);
  /* Allocate process result information for each process.  */
  cp->ProcessResults = (kwsysProcessResults*)malloc(
    sizeof(kwsysProcessResults) * (cp->NumberOfCommands));
  if (!cp->ProcessResults) {
    return 0;
  }
  ZeroMemory(cp->ProcessResults,
             sizeof(kwsysProcessResults) * cp->NumberOfCommands);
  for (i = 0; i < cp->NumberOfCommands; i++) {
    cp->ProcessResults[i].ExitException = kwsysProcess_Exception_None;
    cp->ProcessResults[i].State = kwsysProcess_StateByIndex_Starting;
    cp->ProcessResults[i].ExitCode = 1;
    cp->ProcessResults[i].ExitValue = 1;
    strcpy(cp->ProcessResults[i].ExitExceptionString, "No exception");
  }

  /* Allocate process information for each process.  */
  free(cp->ProcessInformation);
  cp->ProcessInformation = (PROCESS_INFORMATION*)malloc(
    sizeof(PROCESS_INFORMATION) * cp->NumberOfCommands);
  if (!cp->ProcessInformation) {
    return 0;
  }
  ZeroMemory(cp->ProcessInformation,
             sizeof(PROCESS_INFORMATION) * cp->NumberOfCommands);
  if (cp->CommandExitCodes) {
    free(cp->CommandExitCodes);
  }
  cp->CommandExitCodes = (DWORD*)malloc(sizeof(DWORD) * cp->NumberOfCommands);
  if (!cp->CommandExitCodes) {
    return 0;
  }
  ZeroMemory(cp->CommandExitCodes, sizeof(DWORD) * cp->NumberOfCommands);

  /* Allocate event wait array.  The first event is cp->Full, the rest
     are the process termination events.  */
  cp->ProcessEvents =
    (PHANDLE)malloc(sizeof(HANDLE) * (cp->NumberOfCommands + 1));
  if (!cp->ProcessEvents) {
    return 0;
  }
  ZeroMemory(cp->ProcessEvents, sizeof(HANDLE) * (cp->NumberOfCommands + 1));
  cp->ProcessEvents[0] = cp->Full;
  cp->ProcessEventsLength = cp->NumberOfCommands + 1;

  /* Allocate space to save the real working directory of this process.  */
  if (cp->WorkingDirectory) {
    cp->RealWorkingDirectoryLength = GetCurrentDirectoryW(0, 0);
    if (cp->RealWorkingDirectoryLength > 0) {
      cp->RealWorkingDirectory =
        malloc(cp->RealWorkingDirectoryLength * sizeof(wchar_t));
      if (!cp->RealWorkingDirectory) {
        return 0;
      }
    }
  }
  {
    for (i = 0; i < 3; ++i) {
      cp->PipeChildStd[i] = INVALID_HANDLE_VALUE;
    }
  }

  return 1;
}

static DWORD kwsysProcessCreateChildHandle(PHANDLE out, HANDLE in, int isStdIn)
{
  DWORD flags;

  /* Check whether the handle is valid for this process.  */
  if (in != INVALID_HANDLE_VALUE && GetHandleInformation(in, &flags)) {
    /* Use the handle as-is if it is already inherited.  */
    if (flags & HANDLE_FLAG_INHERIT) {
      *out = in;
      return ERROR_SUCCESS;
    }

    /* Create an inherited copy of this handle.  */
    if (DuplicateHandle(GetCurrentProcess(), in, GetCurrentProcess(), out, 0,
                        TRUE, DUPLICATE_SAME_ACCESS)) {
      return ERROR_SUCCESS;
    } else {
      return GetLastError();
    }
  } else {
    /* The given handle is not valid for this process.  Some child
       processes may break if they do not have a valid standard handle,
       so open NUL to give to the child.  */
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = (DWORD)sizeof(sa);
    sa.bInheritHandle = 1;
    *out = CreateFileW(
      L"NUL",
      (isStdIn ? GENERIC_READ : (GENERIC_WRITE | FILE_READ_ATTRIBUTES)),
      FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, 0);
    return (*out != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
  }
}

DWORD kwsysProcessCreate(kwsysProcess* cp, int index,
                         kwsysProcessCreateInformation* si)
{
  DWORD creationFlags;
  DWORD error = ERROR_SUCCESS;

  /* Check if we are currently exiting.  */
  if (!kwsysTryEnterCreateProcessSection()) {
    /* The Ctrl handler is currently working on exiting our process.  Rather
    than return an error code, which could cause incorrect conclusions to be
    reached by the caller, we simply hang.  (For example, a CMake try_run
    configure step might cause the project to configure wrong.)  */
    Sleep(INFINITE);
  }

  /* Create the child in a suspended state so we can wait until all
     children have been created before running any one.  */
  creationFlags = CREATE_SUSPENDED;
  if (cp->CreateProcessGroup) {
    creationFlags |= CREATE_NEW_PROCESS_GROUP;
  }

  /* Create inherited copies of the handles.  */
  (error = kwsysProcessCreateChildHandle(&si->StartupInfo.hStdInput,
                                         si->hStdInput, 1)) ||
    (error = kwsysProcessCreateChildHandle(&si->StartupInfo.hStdOutput,
                                           si->hStdOutput, 0)) ||
    (error = kwsysProcessCreateChildHandle(&si->StartupInfo.hStdError,
                                           si->hStdError, 0)) ||
    /* Create the process.  */
    (!CreateProcessW(0, cp->Commands[index], 0, 0, TRUE, creationFlags, 0, 0,
                     &si->StartupInfo, &cp->ProcessInformation[index]) &&
     (error = GetLastError()));

  /* Close the inherited copies of the handles. */
  if (si->StartupInfo.hStdInput != si->hStdInput) {
    kwsysProcessCleanupHandle(&si->StartupInfo.hStdInput);
  }
  if (si->StartupInfo.hStdOutput != si->hStdOutput) {
    kwsysProcessCleanupHandle(&si->StartupInfo.hStdOutput);
  }
  if (si->StartupInfo.hStdError != si->hStdError) {
    kwsysProcessCleanupHandle(&si->StartupInfo.hStdError);
  }

  /* Add the process to the global list of processes. */
  if (!error &&
      !kwsysProcessesAdd(cp->ProcessInformation[index].hProcess,
                         cp->ProcessInformation[index].dwProcessId,
                         cp->CreateProcessGroup)) {
    /* This failed for some reason.  Kill the suspended process. */
    TerminateProcess(cp->ProcessInformation[index].hProcess, 1);
    /* And clean up... */
    kwsysProcessCleanupHandle(&cp->ProcessInformation[index].hProcess);
    kwsysProcessCleanupHandle(&cp->ProcessInformation[index].hThread);
    strcpy(cp->ErrorMessage, "kwsysProcessesAdd function failed");
    error = ERROR_NOT_ENOUGH_MEMORY; /* Most likely reason.  */
  }

  /* If the console Ctrl handler is waiting for us, this will release it... */
  kwsysLeaveCreateProcessSection();
  return error;
}

void kwsysProcessDestroy(kwsysProcess* cp, int event)
{
  int i;
  int index;

  /* Find the process index for the termination event.  */
  for (index = 0; index < cp->NumberOfCommands; ++index) {
    if (cp->ProcessInformation[index].hProcess == cp->ProcessEvents[event]) {
      break;
    }
  }

  /* Check the exit code of the process.  */
  GetExitCodeProcess(cp->ProcessInformation[index].hProcess,
                     &cp->CommandExitCodes[index]);

  /* Remove from global list of processes.  */
  kwsysProcessesRemove(cp->ProcessInformation[index].hProcess);

  /* Close the process handle for the terminated process.  */
  kwsysProcessCleanupHandle(&cp->ProcessInformation[index].hProcess);

  /* Remove the process from the available events.  */
  cp->ProcessEventsLength -= 1;
  for (i = event; i < cp->ProcessEventsLength; ++i) {
    cp->ProcessEvents[i] = cp->ProcessEvents[i + 1];
  }

  /* Check if all processes have terminated.  */
  if (cp->ProcessEventsLength == 1) {
    cp->Terminated = 1;

    /* Close our copies of the pipe write handles so the pipe threads
       can detect end-of-data.  */
    for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
      /* TODO: If the child created its own child (our grandchild)
         which inherited a copy of the pipe write-end then the pipe
         may not close and we will still need the waker write pipe.
         However we still want to be able to detect end-of-data in the
         normal case.  The reader thread will have to switch to using
         PeekNamedPipe to read the last bit of data from the pipe
         without blocking.  This is equivalent to using a non-blocking
         read on posix.  */
      KWSYSPE_DEBUG((stderr, "closing wakeup write %d\n", i));
      kwsysProcessCleanupHandle(&cp->Pipe[i].Write);
    }
  }
}

DWORD kwsysProcessSetupOutputPipeFile(PHANDLE phandle, const char* name)
{
  HANDLE fout;
  wchar_t* wname;
  DWORD error;
  if (!name) {
    return ERROR_INVALID_PARAMETER;
  }

  /* Close the existing handle.  */
  kwsysProcessCleanupHandle(phandle);

  /* Create a handle to write a file for the pipe.  */
  wname = kwsysEncoding_DupToWide(name);
  fout =
    CreateFileW(wname, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
  error = GetLastError();
  free(wname);
  if (fout == INVALID_HANDLE_VALUE) {
    return error;
  }

  /* Assign the replacement handle.  */
  *phandle = fout;
  return ERROR_SUCCESS;
}

void kwsysProcessSetupSharedPipe(DWORD nStdHandle, PHANDLE handle)
{
  /* Close the existing handle.  */
  kwsysProcessCleanupHandle(handle);
  /* Store the new standard handle.  */
  *handle = GetStdHandle(nStdHandle);
}

void kwsysProcessSetupPipeNative(HANDLE native, PHANDLE handle)
{
  /* Close the existing handle.  */
  kwsysProcessCleanupHandle(handle);
  /* Store the new given handle.  */
  *handle = native;
}

/* Close the given handle if it is open.  Reset its value to 0.  */
void kwsysProcessCleanupHandle(PHANDLE h)
{
  if (h && *h && *h != INVALID_HANDLE_VALUE &&
      *h != GetStdHandle(STD_INPUT_HANDLE) &&
      *h != GetStdHandle(STD_OUTPUT_HANDLE) &&
      *h != GetStdHandle(STD_ERROR_HANDLE)) {
    CloseHandle(*h);
    *h = INVALID_HANDLE_VALUE;
  }
}

/* Close all handles created by kwsysProcess_Execute.  */
void kwsysProcessCleanup(kwsysProcess* cp, DWORD error)
{
  int i;
  /* If this is an error case, report the error.  */
  if (error) {
    /* Construct an error message if one has not been provided already.  */
    if (cp->ErrorMessage[0] == 0) {
      /* Format the error message.  */
      wchar_t err_msg[KWSYSPE_PIPE_BUFFER_SIZE];
      DWORD length = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err_msg,
        KWSYSPE_PIPE_BUFFER_SIZE, 0);
      if (length < 1) {
        /* FormatMessage failed.  Use a default message.  */
        _snprintf(cp->ErrorMessage, KWSYSPE_PIPE_BUFFER_SIZE,
                  "Process execution failed with error 0x%X.  "
                  "FormatMessage failed with error 0x%X",
                  error, GetLastError());
      }
      if (!WideCharToMultiByte(CP_UTF8, 0, err_msg, -1, cp->ErrorMessage,
                               KWSYSPE_PIPE_BUFFER_SIZE, NULL, NULL)) {
        /* WideCharToMultiByte failed.  Use a default message.  */
        _snprintf(cp->ErrorMessage, KWSYSPE_PIPE_BUFFER_SIZE,
                  "Process execution failed with error 0x%X.  "
                  "WideCharToMultiByte failed with error 0x%X",
                  error, GetLastError());
      }
    }

    /* Remove trailing period and newline, if any.  */
    kwsysProcessCleanErrorMessage(cp);

    /* Set the error state.  */
    cp->State = kwsysProcess_State_Error;

    /* Cleanup any processes already started in a suspended state.  */
    if (cp->ProcessInformation) {
      for (i = 0; i < cp->NumberOfCommands; ++i) {
        if (cp->ProcessInformation[i].hProcess) {
          TerminateProcess(cp->ProcessInformation[i].hProcess, 255);
          WaitForSingleObject(cp->ProcessInformation[i].hProcess, INFINITE);
        }
      }
      for (i = 0; i < cp->NumberOfCommands; ++i) {
        /* Remove from global list of processes and close handles.  */
        kwsysProcessesRemove(cp->ProcessInformation[i].hProcess);
        kwsysProcessCleanupHandle(&cp->ProcessInformation[i].hThread);
        kwsysProcessCleanupHandle(&cp->ProcessInformation[i].hProcess);
      }
    }

    /* Restore the working directory.  */
    if (cp->RealWorkingDirectory) {
      SetCurrentDirectoryW(cp->RealWorkingDirectory);
    }
  }

  /* Free memory.  */
  if (cp->ProcessInformation) {
    free(cp->ProcessInformation);
    cp->ProcessInformation = 0;
  }
  if (cp->ProcessEvents) {
    free(cp->ProcessEvents);
    cp->ProcessEvents = 0;
  }
  if (cp->RealWorkingDirectory) {
    free(cp->RealWorkingDirectory);
    cp->RealWorkingDirectory = 0;
  }

  /* Close each pipe.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    kwsysProcessCleanupHandle(&cp->Pipe[i].Write);
    kwsysProcessCleanupHandle(&cp->Pipe[i].Read);
    cp->Pipe[i].Closed = 0;
  }
  for (i = 0; i < 3; ++i) {
    kwsysProcessCleanupHandle(&cp->PipeChildStd[i]);
  }
}

void kwsysProcessCleanErrorMessage(kwsysProcess* cp)
{
  /* Remove trailing period and newline, if any.  */
  size_t length = strlen(cp->ErrorMessage);
  if (cp->ErrorMessage[length - 1] == '\n') {
    cp->ErrorMessage[length - 1] = 0;
    --length;
    if (length > 0 && cp->ErrorMessage[length - 1] == '\r') {
      cp->ErrorMessage[length - 1] = 0;
      --length;
    }
  }
  if (length > 0 && cp->ErrorMessage[length - 1] == '.') {
    cp->ErrorMessage[length - 1] = 0;
  }
}

/* Get the time at which either the process or user timeout will
   expire.  Returns 1 if the user timeout is first, and 0 otherwise.  */
int kwsysProcessGetTimeoutTime(kwsysProcess* cp, double* userTimeout,
                               kwsysProcessTime* timeoutTime)
{
  /* The first time this is called, we need to calculate the time at
     which the child will timeout.  */
  if (cp->Timeout && cp->TimeoutTime.QuadPart < 0) {
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
    if (timeoutTime->QuadPart < 0 ||
        kwsysProcessTimeLess(userTimeoutTime, *timeoutTime)) {
      *timeoutTime = userTimeoutTime;
      return 1;
    }
  }
  return 0;
}

/* Get the length of time before the given timeout time arrives.
   Returns 1 if the time has already arrived, and 0 otherwise.  */
int kwsysProcessGetTimeoutLeft(kwsysProcessTime* timeoutTime,
                               double* userTimeout,
                               kwsysProcessTime* timeoutLength)
{
  if (timeoutTime->QuadPart < 0) {
    /* No timeout time has been requested.  */
    return 0;
  } else {
    /* Calculate the remaining time.  */
    kwsysProcessTime currentTime = kwsysProcessTimeGetCurrent();
    *timeoutLength = kwsysProcessTimeSubtract(*timeoutTime, currentTime);

    if (timeoutLength->QuadPart < 0 && userTimeout && *userTimeout <= 0) {
      /* Caller has explicitly requested a zero timeout.  */
      timeoutLength->QuadPart = 0;
    }

    if (timeoutLength->QuadPart < 0) {
      /* Timeout has already expired.  */
      return 1;
    } else {
      /* There is some time left.  */
      return 0;
    }
  }
}

kwsysProcessTime kwsysProcessTimeGetCurrent()
{
  kwsysProcessTime current;
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  current.LowPart = ft.dwLowDateTime;
  current.HighPart = ft.dwHighDateTime;
  return current;
}

DWORD kwsysProcessTimeToDWORD(kwsysProcessTime t)
{
  return (DWORD)(t.QuadPart * 0.0001);
}

double kwsysProcessTimeToDouble(kwsysProcessTime t)
{
  return t.QuadPart * 0.0000001;
}

kwsysProcessTime kwsysProcessTimeFromDouble(double d)
{
  kwsysProcessTime t;
  t.QuadPart = (LONGLONG)(d * 10000000);
  return t;
}

int kwsysProcessTimeLess(kwsysProcessTime in1, kwsysProcessTime in2)
{
  return in1.QuadPart < in2.QuadPart;
}

kwsysProcessTime kwsysProcessTimeAdd(kwsysProcessTime in1,
                                     kwsysProcessTime in2)
{
  kwsysProcessTime out;
  out.QuadPart = in1.QuadPart + in2.QuadPart;
  return out;
}

kwsysProcessTime kwsysProcessTimeSubtract(kwsysProcessTime in1,
                                          kwsysProcessTime in2)
{
  kwsysProcessTime out;
  out.QuadPart = in1.QuadPart - in2.QuadPart;
  return out;
}

#define KWSYSPE_CASE(type, str)                                               \
  cp->ProcessResults[idx].ExitException = kwsysProcess_Exception_##type;      \
  strcpy(cp->ProcessResults[idx].ExitExceptionString, str)
static void kwsysProcessSetExitExceptionByIndex(kwsysProcess* cp, int code,
                                                int idx)
{
  switch (code) {
    case STATUS_CONTROL_C_EXIT:
      KWSYSPE_CASE(Interrupt, "User interrupt");
      break;

    case STATUS_FLOAT_DENORMAL_OPERAND:
      KWSYSPE_CASE(Numerical, "Floating-point exception (denormal operand)");
      break;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
      KWSYSPE_CASE(Numerical, "Divide-by-zero");
      break;
    case STATUS_FLOAT_INEXACT_RESULT:
      KWSYSPE_CASE(Numerical, "Floating-point exception (inexact result)");
      break;
    case STATUS_FLOAT_INVALID_OPERATION:
      KWSYSPE_CASE(Numerical, "Invalid floating-point operation");
      break;
    case STATUS_FLOAT_OVERFLOW:
      KWSYSPE_CASE(Numerical, "Floating-point overflow");
      break;
    case STATUS_FLOAT_STACK_CHECK:
      KWSYSPE_CASE(Numerical, "Floating-point stack check failed");
      break;
    case STATUS_FLOAT_UNDERFLOW:
      KWSYSPE_CASE(Numerical, "Floating-point underflow");
      break;
#ifdef STATUS_FLOAT_MULTIPLE_FAULTS
    case STATUS_FLOAT_MULTIPLE_FAULTS:
      KWSYSPE_CASE(Numerical, "Floating-point exception (multiple faults)");
      break;
#endif
#ifdef STATUS_FLOAT_MULTIPLE_TRAPS
    case STATUS_FLOAT_MULTIPLE_TRAPS:
      KWSYSPE_CASE(Numerical, "Floating-point exception (multiple traps)");
      break;
#endif
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      KWSYSPE_CASE(Numerical, "Integer divide-by-zero");
      break;
    case STATUS_INTEGER_OVERFLOW:
      KWSYSPE_CASE(Numerical, "Integer overflow");
      break;

    case STATUS_DATATYPE_MISALIGNMENT:
      KWSYSPE_CASE(Fault, "Datatype misalignment");
      break;
    case STATUS_ACCESS_VIOLATION:
      KWSYSPE_CASE(Fault, "Access violation");
      break;
    case STATUS_IN_PAGE_ERROR:
      KWSYSPE_CASE(Fault, "In-page error");
      break;
    case STATUS_INVALID_HANDLE:
      KWSYSPE_CASE(Fault, "Invalid hanlde");
      break;
    case STATUS_NONCONTINUABLE_EXCEPTION:
      KWSYSPE_CASE(Fault, "Noncontinuable exception");
      break;
    case STATUS_INVALID_DISPOSITION:
      KWSYSPE_CASE(Fault, "Invalid disposition");
      break;
    case STATUS_ARRAY_BOUNDS_EXCEEDED:
      KWSYSPE_CASE(Fault, "Array bounds exceeded");
      break;
    case STATUS_STACK_OVERFLOW:
      KWSYSPE_CASE(Fault, "Stack overflow");
      break;

    case STATUS_ILLEGAL_INSTRUCTION:
      KWSYSPE_CASE(Illegal, "Illegal instruction");
      break;
    case STATUS_PRIVILEGED_INSTRUCTION:
      KWSYSPE_CASE(Illegal, "Privileged instruction");
      break;

    case STATUS_NO_MEMORY:
    default:
      cp->ProcessResults[idx].ExitException = kwsysProcess_Exception_Other;
      _snprintf(cp->ProcessResults[idx].ExitExceptionString,
                KWSYSPE_PIPE_BUFFER_SIZE, "Exit code 0x%x\n", code);
      break;
  }
}
#undef KWSYSPE_CASE

typedef struct kwsysProcess_List_s kwsysProcess_List;
static kwsysProcess_List* kwsysProcess_List_New(void);
static void kwsysProcess_List_Delete(kwsysProcess_List* self);
static int kwsysProcess_List_Update(kwsysProcess_List* self);
static int kwsysProcess_List_NextProcess(kwsysProcess_List* self);
static int kwsysProcess_List_GetCurrentProcessId(kwsysProcess_List* self);
static int kwsysProcess_List_GetCurrentParentId(kwsysProcess_List* self);

/* Windows NT 4 API definitions.  */
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
typedef LONG NTSTATUS;
typedef LONG KPRIORITY;
typedef struct _UNICODE_STRING UNICODE_STRING;
struct _UNICODE_STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
};

/* The process information structure.  Declare only enough to get
   process identifiers.  The rest may be ignored because we use the
   NextEntryDelta to move through an array of instances.  */
typedef struct _SYSTEM_PROCESS_INFORMATION SYSTEM_PROCESS_INFORMATION;
typedef SYSTEM_PROCESS_INFORMATION* PSYSTEM_PROCESS_INFORMATION;
struct _SYSTEM_PROCESS_INFORMATION
{
  ULONG NextEntryDelta;
  ULONG ThreadCount;
  ULONG Reserved1[6];
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER KernelTime;
  UNICODE_STRING ProcessName;
  KPRIORITY BasePriority;
  ULONG ProcessId;
  ULONG InheritedFromProcessId;
};

/* Toolhelp32 API definitions.  */
#define TH32CS_SNAPPROCESS 0x00000002
#if defined(_WIN64)
typedef unsigned __int64 ProcessULONG_PTR;
#else
typedef unsigned long ProcessULONG_PTR;
#endif
typedef struct tagPROCESSENTRY32 PROCESSENTRY32;
typedef PROCESSENTRY32* LPPROCESSENTRY32;
struct tagPROCESSENTRY32
{
  DWORD dwSize;
  DWORD cntUsage;
  DWORD th32ProcessID;
  ProcessULONG_PTR th32DefaultHeapID;
  DWORD th32ModuleID;
  DWORD cntThreads;
  DWORD th32ParentProcessID;
  LONG pcPriClassBase;
  DWORD dwFlags;
  char szExeFile[MAX_PATH];
};

/* Windows API function types.  */
typedef HANDLE(WINAPI* CreateToolhelp32SnapshotType)(DWORD, DWORD);
typedef BOOL(WINAPI* Process32FirstType)(HANDLE, LPPROCESSENTRY32);
typedef BOOL(WINAPI* Process32NextType)(HANDLE, LPPROCESSENTRY32);
typedef NTSTATUS(WINAPI* ZwQuerySystemInformationType)(ULONG, PVOID, ULONG,
                                                       PULONG);

static int kwsysProcess_List__New_NT4(kwsysProcess_List* self);
static int kwsysProcess_List__New_Snapshot(kwsysProcess_List* self);
static void kwsysProcess_List__Delete_NT4(kwsysProcess_List* self);
static void kwsysProcess_List__Delete_Snapshot(kwsysProcess_List* self);
static int kwsysProcess_List__Update_NT4(kwsysProcess_List* self);
static int kwsysProcess_List__Update_Snapshot(kwsysProcess_List* self);
static int kwsysProcess_List__Next_NT4(kwsysProcess_List* self);
static int kwsysProcess_List__Next_Snapshot(kwsysProcess_List* self);
static int kwsysProcess_List__GetProcessId_NT4(kwsysProcess_List* self);
static int kwsysProcess_List__GetProcessId_Snapshot(kwsysProcess_List* self);
static int kwsysProcess_List__GetParentId_NT4(kwsysProcess_List* self);
static int kwsysProcess_List__GetParentId_Snapshot(kwsysProcess_List* self);

struct kwsysProcess_List_s
{
  /* Implementation switches at runtime based on version of Windows.  */
  int NT4;

  /* Implementation functions and data for NT 4.  */
  ZwQuerySystemInformationType P_ZwQuerySystemInformation;
  char* Buffer;
  int BufferSize;
  PSYSTEM_PROCESS_INFORMATION CurrentInfo;

  /* Implementation functions and data for other Windows versions.  */
  CreateToolhelp32SnapshotType P_CreateToolhelp32Snapshot;
  Process32FirstType P_Process32First;
  Process32NextType P_Process32Next;
  HANDLE Snapshot;
  PROCESSENTRY32 CurrentEntry;
};

static kwsysProcess_List* kwsysProcess_List_New(void)
{
  OSVERSIONINFO osv;
  kwsysProcess_List* self;

  /* Allocate and initialize the list object.  */
  if (!(self = (kwsysProcess_List*)malloc(sizeof(kwsysProcess_List)))) {
    return 0;
  }
  memset(self, 0, sizeof(*self));

  /* Select an implementation.  */
  ZeroMemory(&osv, sizeof(osv));
  osv.dwOSVersionInfoSize = sizeof(osv);
#ifdef KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#pragma warning(push)
#ifdef __INTEL_COMPILER
#pragma warning(disable : 1478)
#else
#pragma warning(disable : 4996)
#endif
#endif
  GetVersionEx(&osv);
#ifdef KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#pragma warning(pop)
#endif
  self->NT4 =
    (osv.dwPlatformId == VER_PLATFORM_WIN32_NT && osv.dwMajorVersion < 5) ? 1
                                                                          : 0;

  /* Initialize the selected implementation.  */
  if (!(self->NT4 ? kwsysProcess_List__New_NT4(self)
                  : kwsysProcess_List__New_Snapshot(self))) {
    kwsysProcess_List_Delete(self);
    return 0;
  }

  /* Update to the current set of processes.  */
  if (!kwsysProcess_List_Update(self)) {
    kwsysProcess_List_Delete(self);
    return 0;
  }
  return self;
}

static void kwsysProcess_List_Delete(kwsysProcess_List* self)
{
  if (self) {
    if (self->NT4) {
      kwsysProcess_List__Delete_NT4(self);
    } else {
      kwsysProcess_List__Delete_Snapshot(self);
    }
    free(self);
  }
}

static int kwsysProcess_List_Update(kwsysProcess_List* self)
{
  return self ? (self->NT4 ? kwsysProcess_List__Update_NT4(self)
                           : kwsysProcess_List__Update_Snapshot(self))
              : 0;
}

static int kwsysProcess_List_GetCurrentProcessId(kwsysProcess_List* self)
{
  return self ? (self->NT4 ? kwsysProcess_List__GetProcessId_NT4(self)
                           : kwsysProcess_List__GetProcessId_Snapshot(self))
              : -1;
}

static int kwsysProcess_List_GetCurrentParentId(kwsysProcess_List* self)
{
  return self ? (self->NT4 ? kwsysProcess_List__GetParentId_NT4(self)
                           : kwsysProcess_List__GetParentId_Snapshot(self))
              : -1;
}

static int kwsysProcess_List_NextProcess(kwsysProcess_List* self)
{
  return (self ? (self->NT4 ? kwsysProcess_List__Next_NT4(self)
                            : kwsysProcess_List__Next_Snapshot(self))
               : 0);
}

static int kwsysProcess_List__New_NT4(kwsysProcess_List* self)
{
  /* Get a handle to the NT runtime module that should already be
     loaded in this program.  This does not actually increment the
     reference count to the module so we do not need to close the
     handle.  */
  HMODULE hNT = GetModuleHandleW(L"ntdll.dll");
  if (hNT) {
    /* Get pointers to the needed API functions.  */
    self->P_ZwQuerySystemInformation =
      ((ZwQuerySystemInformationType)GetProcAddress(
        hNT, "ZwQuerySystemInformation"));
  }
  if (!self->P_ZwQuerySystemInformation) {
    return 0;
  }

  /* Allocate an initial process information buffer.  */
  self->BufferSize = 32768;
  self->Buffer = (char*)malloc(self->BufferSize);
  return self->Buffer ? 1 : 0;
}

static void kwsysProcess_List__Delete_NT4(kwsysProcess_List* self)
{
  /* Free the process information buffer.  */
  if (self->Buffer) {
    free(self->Buffer);
  }
}

static int kwsysProcess_List__Update_NT4(kwsysProcess_List* self)
{
  self->CurrentInfo = 0;
  for (;;) {
    /* Query number 5 is for system process list.  */
    NTSTATUS status =
      self->P_ZwQuerySystemInformation(5, self->Buffer, self->BufferSize, 0);
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
      /* The query requires a bigger buffer.  */
      int newBufferSize = self->BufferSize * 2;
      char* newBuffer = (char*)malloc(newBufferSize);
      if (newBuffer) {
        free(self->Buffer);
        self->Buffer = newBuffer;
        self->BufferSize = newBufferSize;
      } else {
        return 0;
      }
    } else if (status >= 0) {
      /* The query succeeded.  Initialize traversal of the process list.  */
      self->CurrentInfo = (PSYSTEM_PROCESS_INFORMATION)self->Buffer;
      return 1;
    } else {
      /* The query failed.  */
      return 0;
    }
  }
}

static int kwsysProcess_List__Next_NT4(kwsysProcess_List* self)
{
  if (self->CurrentInfo) {
    if (self->CurrentInfo->NextEntryDelta > 0) {
      self->CurrentInfo = ((PSYSTEM_PROCESS_INFORMATION)(
        (char*)self->CurrentInfo + self->CurrentInfo->NextEntryDelta));
      return 1;
    }
    self->CurrentInfo = 0;
  }
  return 0;
}

static int kwsysProcess_List__GetProcessId_NT4(kwsysProcess_List* self)
{
  return self->CurrentInfo ? self->CurrentInfo->ProcessId : -1;
}

static int kwsysProcess_List__GetParentId_NT4(kwsysProcess_List* self)
{
  return self->CurrentInfo ? self->CurrentInfo->InheritedFromProcessId : -1;
}

static int kwsysProcess_List__New_Snapshot(kwsysProcess_List* self)
{
  /* Get a handle to the Windows runtime module that should already be
     loaded in this program.  This does not actually increment the
     reference count to the module so we do not need to close the
     handle.  */
  HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
  if (hKernel) {
    self->P_CreateToolhelp32Snapshot =
      ((CreateToolhelp32SnapshotType)GetProcAddress(
        hKernel, "CreateToolhelp32Snapshot"));
    self->P_Process32First =
      ((Process32FirstType)GetProcAddress(hKernel, "Process32First"));
    self->P_Process32Next =
      ((Process32NextType)GetProcAddress(hKernel, "Process32Next"));
  }
  return (self->P_CreateToolhelp32Snapshot && self->P_Process32First &&
          self->P_Process32Next)
    ? 1
    : 0;
}

static void kwsysProcess_List__Delete_Snapshot(kwsysProcess_List* self)
{
  if (self->Snapshot) {
    CloseHandle(self->Snapshot);
  }
}

static int kwsysProcess_List__Update_Snapshot(kwsysProcess_List* self)
{
  if (self->Snapshot) {
    CloseHandle(self->Snapshot);
  }
  if (!(self->Snapshot =
          self->P_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0))) {
    return 0;
  }
  ZeroMemory(&self->CurrentEntry, sizeof(self->CurrentEntry));
  self->CurrentEntry.dwSize = sizeof(self->CurrentEntry);
  if (!self->P_Process32First(self->Snapshot, &self->CurrentEntry)) {
    CloseHandle(self->Snapshot);
    self->Snapshot = 0;
    return 0;
  }
  return 1;
}

static int kwsysProcess_List__Next_Snapshot(kwsysProcess_List* self)
{
  if (self->Snapshot) {
    if (self->P_Process32Next(self->Snapshot, &self->CurrentEntry)) {
      return 1;
    }
    CloseHandle(self->Snapshot);
    self->Snapshot = 0;
  }
  return 0;
}

static int kwsysProcess_List__GetProcessId_Snapshot(kwsysProcess_List* self)
{
  return self->Snapshot ? self->CurrentEntry.th32ProcessID : -1;
}

static int kwsysProcess_List__GetParentId_Snapshot(kwsysProcess_List* self)
{
  return self->Snapshot ? self->CurrentEntry.th32ParentProcessID : -1;
}

static void kwsysProcessKill(DWORD pid)
{
  HANDLE h = OpenProcess(PROCESS_TERMINATE, 0, pid);
  if (h) {
    TerminateProcess(h, 255);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
  }
}

static void kwsysProcessKillTree(int pid)
{
  kwsysProcess_List* plist = kwsysProcess_List_New();
  kwsysProcessKill(pid);
  if (plist) {
    do {
      if (kwsysProcess_List_GetCurrentParentId(plist) == pid) {
        int ppid = kwsysProcess_List_GetCurrentProcessId(plist);
        kwsysProcessKillTree(ppid);
      }
    } while (kwsysProcess_List_NextProcess(plist));
    kwsysProcess_List_Delete(plist);
  }
}

static void kwsysProcessDisablePipeThreads(kwsysProcess* cp)
{
  int i;

  /* If data were just reported data, release the pipe's thread.  */
  if (cp->CurrentIndex < KWSYSPE_PIPE_COUNT) {
    KWSYSPE_DEBUG((stderr, "releasing reader %d\n", cp->CurrentIndex));
    ReleaseSemaphore(cp->Pipe[cp->CurrentIndex].Reader.Go, 1, 0);
    cp->CurrentIndex = KWSYSPE_PIPE_COUNT;
  }

  /* Wakeup all reading threads that are not on closed pipes.  */
  for (i = 0; i < KWSYSPE_PIPE_COUNT; ++i) {
    /* The wakeup threads will write one byte to the pipe write ends.
       If there are no data in the pipe then this is enough to wakeup
       the reading threads.  If there are already data in the pipe
       this may block.  We cannot use PeekNamedPipe to check whether
       there are data because an outside process might still be
       writing data if we are disowning it.  Also, PeekNamedPipe will
       block if checking a pipe on which the reading thread is
       currently calling ReadPipe.  Therefore we need a separate
       thread to call WriteFile.  If it blocks, that is okay because
       it will unblock when we close the read end and break the pipe
       below.  */
    if (cp->Pipe[i].Read) {
      KWSYSPE_DEBUG((stderr, "releasing waker %d\n", i));
      ReleaseSemaphore(cp->Pipe[i].Waker.Go, 1, 0);
    }
  }

  /* Tell pipe threads to reset until we run another process.  */
  while (cp->PipesLeft > 0) {
    /* The waking threads will cause all reading threads to report.
       Wait for the next one and save its index.  */
    KWSYSPE_DEBUG((stderr, "waiting for reader\n"));
    WaitForSingleObject(cp->Full, INFINITE);
    cp->CurrentIndex = cp->SharedIndex;
    ReleaseSemaphore(cp->SharedIndexMutex, 1, 0);
    KWSYSPE_DEBUG((stderr, "got reader %d\n", cp->CurrentIndex));

    /* We are done reading this pipe.  Close its read handle.  */
    cp->Pipe[cp->CurrentIndex].Closed = 1;
    kwsysProcessCleanupHandle(&cp->Pipe[cp->CurrentIndex].Read);
    --cp->PipesLeft;

    /* Tell the reading thread we are done with the data.  It will
       reset immediately because the pipe is closed.  */
    ReleaseSemaphore(cp->Pipe[cp->CurrentIndex].Reader.Go, 1, 0);
  }
}

/* Global set of executing processes for use by the Ctrl handler.
   This global instance will be zero-initialized by the compiler.

   Note that the console Ctrl handler runs on a background thread and so
   everything it does must be thread safe.  Here, we track the hProcess
   HANDLEs directly instead of kwsysProcess instances, so that we don't have
   to make kwsysProcess thread safe.  */
typedef struct kwsysProcessInstance_s
{
  HANDLE hProcess;
  DWORD dwProcessId;
  int NewProcessGroup; /* Whether the process was created in a new group.  */
} kwsysProcessInstance;

typedef struct kwsysProcessInstances_s
{
  /* Whether we have initialized key fields below, like critical sections.  */
  int Initialized;

  /* Ctrl handler runs on a different thread, so we must sync access.  */
  CRITICAL_SECTION Lock;

  int Exiting;
  size_t Count;
  size_t Size;
  kwsysProcessInstance* Processes;
} kwsysProcessInstances;
static kwsysProcessInstances kwsysProcesses;

/* Initialize critial section and set up console Ctrl handler.  You MUST call
   this before using any other kwsysProcesses* functions below.  */
static int kwsysProcessesInitialize(void)
{
  /* Initialize everything if not done already.  */
  if (!kwsysProcesses.Initialized) {
    InitializeCriticalSection(&kwsysProcesses.Lock);

    /* Set up console ctrl handler.  */
    if (!SetConsoleCtrlHandler(kwsysCtrlHandler, TRUE)) {
      return 0;
    }

    kwsysProcesses.Initialized = 1;
  }
  return 1;
}

/* The Ctrl handler waits on the global list of processes.  To prevent an
   orphaned process, do not create a new process if the Ctrl handler is
   already running.  Do so by using this function to check if it is ok to
   create a process.  */
static int kwsysTryEnterCreateProcessSection(void)
{
  /* Enter main critical section; this means creating a process and the Ctrl
     handler are mutually exclusive.  */
  EnterCriticalSection(&kwsysProcesses.Lock);
  /* Indicate to the caller if they can create a process.  */
  if (kwsysProcesses.Exiting) {
    LeaveCriticalSection(&kwsysProcesses.Lock);
    return 0;
  } else {
    return 1;
  }
}

/* Matching function on successful kwsysTryEnterCreateProcessSection return.
   Make sure you called kwsysProcessesAdd if applicable before calling this.*/
static void kwsysLeaveCreateProcessSection(void)
{
  LeaveCriticalSection(&kwsysProcesses.Lock);
}

/* Add new process to global process list.  The Ctrl handler will wait for
   the process to exit before it returns.  Do not close the process handle
   until after calling kwsysProcessesRemove.  The newProcessGroup parameter
   must be set if the process was created with CREATE_NEW_PROCESS_GROUP.  */
static int kwsysProcessesAdd(HANDLE hProcess, DWORD dwProcessid,
                             int newProcessGroup)
{
  if (!kwsysProcessesInitialize() || !hProcess ||
      hProcess == INVALID_HANDLE_VALUE) {
    return 0;
  }

  /* Enter the critical section. */
  EnterCriticalSection(&kwsysProcesses.Lock);

  /* Make sure there is enough space for the new process handle.  */
  if (kwsysProcesses.Count == kwsysProcesses.Size) {
    size_t newSize;
    kwsysProcessInstance* newArray;
    /* Start with enough space for a small number of process handles
       and double the size each time more is needed.  */
    newSize = kwsysProcesses.Size ? kwsysProcesses.Size * 2 : 4;

    /* Try allocating the new block of memory.  */
    if (newArray = (kwsysProcessInstance*)malloc(
          newSize * sizeof(kwsysProcessInstance))) {
      /* Copy the old process handles to the new memory.  */
      if (kwsysProcesses.Count > 0) {
        memcpy(newArray, kwsysProcesses.Processes,
               kwsysProcesses.Count * sizeof(kwsysProcessInstance));
      }
    } else {
      /* Failed to allocate memory for the new process handle set.  */
      LeaveCriticalSection(&kwsysProcesses.Lock);
      return 0;
    }

    /* Free original array. */
    free(kwsysProcesses.Processes);

    /* Update original structure with new allocation. */
    kwsysProcesses.Size = newSize;
    kwsysProcesses.Processes = newArray;
  }

  /* Append the new process information to the set.  */
  kwsysProcesses.Processes[kwsysProcesses.Count].hProcess = hProcess;
  kwsysProcesses.Processes[kwsysProcesses.Count].dwProcessId = dwProcessid;
  kwsysProcesses.Processes[kwsysProcesses.Count++].NewProcessGroup =
    newProcessGroup;

  /* Leave critical section and return success. */
  LeaveCriticalSection(&kwsysProcesses.Lock);

  return 1;
}

/* Removes process to global process list.  */
static void kwsysProcessesRemove(HANDLE hProcess)
{
  size_t i;

  if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
    return;
  }

  EnterCriticalSection(&kwsysProcesses.Lock);

  /* Find the given process in the set.  */
  for (i = 0; i < kwsysProcesses.Count; ++i) {
    if (kwsysProcesses.Processes[i].hProcess == hProcess) {
      break;
    }
  }
  if (i < kwsysProcesses.Count) {
    /* Found it!  Remove the process from the set.  */
    --kwsysProcesses.Count;
    for (; i < kwsysProcesses.Count; ++i) {
      kwsysProcesses.Processes[i] = kwsysProcesses.Processes[i + 1];
    }

    /* If this was the last process, free the array.  */
    if (kwsysProcesses.Count == 0) {
      kwsysProcesses.Size = 0;
      free(kwsysProcesses.Processes);
      kwsysProcesses.Processes = 0;
    }
  }

  LeaveCriticalSection(&kwsysProcesses.Lock);
}

static BOOL WINAPI kwsysCtrlHandler(DWORD dwCtrlType)
{
  size_t i;
  (void)dwCtrlType;
  /* Enter critical section.  */
  EnterCriticalSection(&kwsysProcesses.Lock);

  /* Set flag indicating that we are exiting.  */
  kwsysProcesses.Exiting = 1;

  /* If some of our processes were created in a new process group, we must
     manually interrupt them.  They won't otherwise receive a Ctrl+C/Break. */
  for (i = 0; i < kwsysProcesses.Count; ++i) {
    if (kwsysProcesses.Processes[i].NewProcessGroup) {
      DWORD groupId = kwsysProcesses.Processes[i].dwProcessId;
      if (groupId) {
        GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, groupId);
      }
    }
  }

  /* Wait for each child process to exit.  This is the key step that prevents
     us from leaving several orphaned children processes running in the
     background when the user presses Ctrl+C.  */
  for (i = 0; i < kwsysProcesses.Count; ++i) {
    WaitForSingleObject(kwsysProcesses.Processes[i].hProcess, INFINITE);
  }

  /* Leave critical section.  */
  LeaveCriticalSection(&kwsysProcesses.Lock);

  /* Continue on to default Ctrl handler (which calls ExitProcess).  */
  return FALSE;
}

void kwsysProcess_ResetStartTime(kwsysProcess* cp)
{
  if (!cp) {
    return;
  }
  /* Reset start time. */
  cp->StartTime = kwsysProcessTimeGetCurrent();
}
