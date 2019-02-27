/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmProcess.h"

#include "cmCTest.h"
#include "cmCTestRunTest.h"
#include "cmCTestTestHandler.h"
#include "cmsys/Process.h"

#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <string>
#if !defined(_WIN32)
#  include <unistd.h>
#endif

#define CM_PROCESS_BUF_SIZE 65536

#if defined(_WIN32) && !defined(__CYGWIN__)
#  include <io.h>

static int cmProcessGetPipes(int* fds)
{
  SECURITY_ATTRIBUTES attr;
  HANDLE readh, writeh;
  attr.nLength = sizeof(attr);
  attr.lpSecurityDescriptor = nullptr;
  attr.bInheritHandle = FALSE;
  if (!CreatePipe(&readh, &writeh, &attr, 0))
    return uv_translate_sys_error(GetLastError());
  fds[0] = _open_osfhandle((intptr_t)readh, 0);
  fds[1] = _open_osfhandle((intptr_t)writeh, 0);
  if (fds[0] == -1 || fds[1] == -1) {
    CloseHandle(readh);
    CloseHandle(writeh);
    return uv_translate_sys_error(GetLastError());
  }
  return 0;
}
#else
#  include <errno.h>

static int cmProcessGetPipes(int* fds)
{
  if (pipe(fds) == -1) {
    return uv_translate_sys_error(errno);
  }

  if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1 ||
      fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1) {
    close(fds[0]);
    close(fds[1]);
    return uv_translate_sys_error(errno);
  }
  return 0;
}
#endif

cmProcess::cmProcess(cmCTestRunTest& runner)
  : Runner(runner)
  , Conv(cmProcessOutput::UTF8, CM_PROCESS_BUF_SIZE)
{
  this->Timeout = cmDuration::zero();
  this->TotalTime = cmDuration::zero();
  this->ExitValue = 0;
  this->Id = 0;
  this->StartTime = std::chrono::steady_clock::time_point();
}

cmProcess::~cmProcess()
{
}

void cmProcess::SetCommand(const char* command)
{
  this->Command = command;
}

void cmProcess::SetCommandArguments(std::vector<std::string> const& args)
{
  this->Arguments = args;
}

bool cmProcess::StartProcess(uv_loop_t& loop, std::vector<size_t>* affinity)
{
  this->ProcessState = cmProcess::State::Error;
  if (this->Command.empty()) {
    return false;
  }
  this->StartTime = std::chrono::steady_clock::now();
  this->ProcessArgs.clear();
  // put the command as arg0
  this->ProcessArgs.push_back(this->Command.c_str());
  // now put the command arguments in
  for (std::string const& arg : this->Arguments) {
    this->ProcessArgs.push_back(arg.c_str());
  }
  this->ProcessArgs.push_back(nullptr); // null terminate the list

  cm::uv_timer_ptr timer;
  int status = timer.init(loop, this);
  if (status != 0) {
    cmCTestLog(this->Runner.GetCTest(), ERROR_MESSAGE,
               "Error initializing timer: " << uv_strerror(status)
                                            << std::endl);
    return false;
  }

  cm::uv_pipe_ptr pipe_writer;
  cm::uv_pipe_ptr pipe_reader;

  pipe_writer.init(loop, 0);
  pipe_reader.init(loop, 0, this);

  int fds[2] = { -1, -1 };
  status = cmProcessGetPipes(fds);
  if (status != 0) {
    cmCTestLog(this->Runner.GetCTest(), ERROR_MESSAGE,
               "Error initializing pipe: " << uv_strerror(status)
                                           << std::endl);
    return false;
  }

  uv_pipe_open(pipe_reader, fds[0]);
  uv_pipe_open(pipe_writer, fds[1]);

  uv_stdio_container_t stdio[3];
  stdio[0].flags = UV_INHERIT_FD;
  stdio[0].data.fd = 0;
  stdio[1].flags = UV_INHERIT_STREAM;
  stdio[1].data.stream = pipe_writer;
  stdio[2] = stdio[1];

  uv_process_options_t options = uv_process_options_t();
  options.file = this->Command.data();
  options.args = const_cast<char**>(this->ProcessArgs.data());
  options.stdio_count = 3; // in, out and err
  options.exit_cb = &cmProcess::OnExitCB;
  options.stdio = stdio;
#if !defined(CMAKE_USE_SYSTEM_LIBUV)
  std::vector<char> cpumask;
  if (affinity && !affinity->empty()) {
    cpumask.resize(static_cast<size_t>(uv_cpumask_size()), 0);
    for (auto p : *affinity) {
      cpumask[p] = 1;
    }
    options.cpumask = cpumask.data();
    options.cpumask_size = cpumask.size();
  } else {
    options.cpumask = nullptr;
    options.cpumask_size = 0;
  }
#else
  static_cast<void>(affinity);
#endif

  status =
    uv_read_start(pipe_reader, &cmProcess::OnAllocateCB, &cmProcess::OnReadCB);

  if (status != 0) {
    cmCTestLog(this->Runner.GetCTest(), ERROR_MESSAGE,
               "Error starting read events: " << uv_strerror(status)
                                              << std::endl);
    return false;
  }

  status = this->Process.spawn(loop, options, this);
  if (status != 0) {
    cmCTestLog(this->Runner.GetCTest(), ERROR_MESSAGE,
               "Process not started\n " << this->Command << "\n["
                                        << uv_strerror(status) << "]\n");
    return false;
  }

  this->PipeReader = std::move(pipe_reader);
  this->Timer = std::move(timer);

  this->StartTimer();

  this->ProcessState = cmProcess::State::Executing;
  return true;
}

void cmProcess::StartTimer()
{
  auto properties = this->Runner.GetTestProperties();
  auto msec =
    std::chrono::duration_cast<std::chrono::milliseconds>(this->Timeout);

  if (msec != std::chrono::milliseconds(0) || !properties->ExplicitTimeout) {
    this->Timer.start(&cmProcess::OnTimeoutCB,
                      static_cast<uint64_t>(msec.count()), 0);
  }
}

bool cmProcess::Buffer::GetLine(std::string& line)
{
  // Scan for the next newline.
  for (size_type sz = this->size(); this->Last != sz; ++this->Last) {
    if ((*this)[this->Last] == '\n' || (*this)[this->Last] == '\0') {
      // Extract the range first..last as a line.
      const char* text = &*this->begin() + this->First;
      size_type length = this->Last - this->First;
      while (length && text[length - 1] == '\r') {
        length--;
      }
      line.assign(text, length);

      // Start a new range for the next line.
      ++this->Last;
      this->First = Last;

      // Return the line extracted.
      return true;
    }
  }

  // Available data have been exhausted without a newline.
  if (this->First != 0) {
    // Move the partial line to the beginning of the buffer.
    this->erase(this->begin(), this->begin() + this->First);
    this->First = 0;
    this->Last = this->size();
  }
  return false;
}

bool cmProcess::Buffer::GetLast(std::string& line)
{
  // Return the partial last line, if any.
  if (!this->empty()) {
    line.assign(&*this->begin(), this->size());
    this->First = this->Last = 0;
    this->clear();
    return true;
  }
  return false;
}

void cmProcess::OnReadCB(uv_stream_t* stream, ssize_t nread,
                         const uv_buf_t* buf)
{
  auto self = static_cast<cmProcess*>(stream->data);
  self->OnRead(nread, buf);
}

void cmProcess::OnRead(ssize_t nread, const uv_buf_t* buf)
{
  std::string line;
  if (nread > 0) {
    std::string strdata;
    this->Conv.DecodeText(buf->base, static_cast<size_t>(nread), strdata);
    this->Output.insert(this->Output.end(), strdata.begin(), strdata.end());

    while (this->Output.GetLine(line)) {
      this->Runner.CheckOutput(line);
      line.clear();
    }

    return;
  }

  if (nread == 0) {
    return;
  }

  // The process will provide no more data.
  if (nread != UV_EOF) {
    auto error = static_cast<int>(nread);
    cmCTestLog(this->Runner.GetCTest(), ERROR_MESSAGE,
               "Error reading stream: " << uv_strerror(error) << std::endl);
  }

  // Look for partial last lines.
  if (this->Output.GetLast(line)) {
    this->Runner.CheckOutput(line);
  }

  this->ReadHandleClosed = true;
  this->PipeReader.reset();
  if (this->ProcessHandleClosed) {
    uv_timer_stop(this->Timer);
    this->Runner.FinalizeTest();
  }
}

void cmProcess::OnAllocateCB(uv_handle_t* handle, size_t suggested_size,
                             uv_buf_t* buf)
{
  auto self = static_cast<cmProcess*>(handle->data);
  self->OnAllocate(suggested_size, buf);
}

void cmProcess::OnAllocate(size_t /*suggested_size*/, uv_buf_t* buf)
{
  if (this->Buf.size() != CM_PROCESS_BUF_SIZE) {
    this->Buf.resize(CM_PROCESS_BUF_SIZE);
  }

  *buf =
    uv_buf_init(this->Buf.data(), static_cast<unsigned int>(this->Buf.size()));
}

void cmProcess::OnTimeoutCB(uv_timer_t* timer)
{
  auto self = static_cast<cmProcess*>(timer->data);
  self->OnTimeout();
}

void cmProcess::OnTimeout()
{
  if (this->ProcessState != cmProcess::State::Executing) {
    return;
  }
  this->ProcessState = cmProcess::State::Expired;
  bool const was_still_reading = !this->ReadHandleClosed;
  if (!this->ReadHandleClosed) {
    this->ReadHandleClosed = true;
    this->PipeReader.reset();
  }
  if (!this->ProcessHandleClosed) {
    // Kill the child and let our on-exit handler finish the test.
    cmsysProcess_KillPID(static_cast<unsigned long>(this->Process->pid));
  } else if (was_still_reading) {
    // Our on-exit handler already ran but did not finish the test
    // because we were still reading output.  We've just dropped
    // our read handler, so we need to finish the test now.
    this->Runner.FinalizeTest();
  }
}

void cmProcess::OnExitCB(uv_process_t* process, int64_t exit_status,
                         int term_signal)
{
  auto self = static_cast<cmProcess*>(process->data);
  self->OnExit(exit_status, term_signal);
}

void cmProcess::OnExit(int64_t exit_status, int term_signal)
{
  if (this->ProcessState != cmProcess::State::Expired) {
    if (
#if defined(_WIN32)
      ((DWORD)exit_status & 0xF0000000) == 0xC0000000
#else
      term_signal != 0
#endif
    ) {
      this->ProcessState = cmProcess::State::Exception;
    } else {
      this->ProcessState = cmProcess::State::Exited;
    }
  }

  // Record exit information.
  this->ExitValue = static_cast<int>(exit_status);
  this->Signal = term_signal;
  this->TotalTime = std::chrono::steady_clock::now() - this->StartTime;
  // Because of a processor clock scew the runtime may become slightly
  // negative. If someone changed the system clock while the process was
  // running this may be even more. Make sure not to report a negative
  // duration here.
  if (this->TotalTime <= cmDuration::zero()) {
    this->TotalTime = cmDuration::zero();
  }

  this->ProcessHandleClosed = true;
  if (this->ReadHandleClosed) {
    uv_timer_stop(this->Timer);
    this->Runner.FinalizeTest();
  }
}

cmProcess::State cmProcess::GetProcessStatus()
{
  return this->ProcessState;
}

void cmProcess::ChangeTimeout(cmDuration t)
{
  this->Timeout = t;
  this->StartTimer();
}

void cmProcess::ResetStartTime()
{
  this->StartTime = std::chrono::steady_clock::now();
}

cmProcess::Exception cmProcess::GetExitException()
{
  auto exception = Exception::None;
#if defined(_WIN32) && !defined(__CYGWIN__)
  auto exit_code = (DWORD)this->ExitValue;
  if ((exit_code & 0xF0000000) != 0xC0000000) {
    return exception;
  }

  if (exit_code) {
    switch (exit_code) {
      case STATUS_DATATYPE_MISALIGNMENT:
      case STATUS_ACCESS_VIOLATION:
      case STATUS_IN_PAGE_ERROR:
      case STATUS_INVALID_HANDLE:
      case STATUS_NONCONTINUABLE_EXCEPTION:
      case STATUS_INVALID_DISPOSITION:
      case STATUS_ARRAY_BOUNDS_EXCEEDED:
      case STATUS_STACK_OVERFLOW:
        exception = Exception::Fault;
        break;
      case STATUS_FLOAT_DENORMAL_OPERAND:
      case STATUS_FLOAT_DIVIDE_BY_ZERO:
      case STATUS_FLOAT_INEXACT_RESULT:
      case STATUS_FLOAT_INVALID_OPERATION:
      case STATUS_FLOAT_OVERFLOW:
      case STATUS_FLOAT_STACK_CHECK:
      case STATUS_FLOAT_UNDERFLOW:
#  ifdef STATUS_FLOAT_MULTIPLE_FAULTS
      case STATUS_FLOAT_MULTIPLE_FAULTS:
#  endif
#  ifdef STATUS_FLOAT_MULTIPLE_TRAPS
      case STATUS_FLOAT_MULTIPLE_TRAPS:
#  endif
      case STATUS_INTEGER_DIVIDE_BY_ZERO:
      case STATUS_INTEGER_OVERFLOW:
        exception = Exception::Numerical;
        break;
      case STATUS_CONTROL_C_EXIT:
        exception = Exception::Interrupt;
        break;
      case STATUS_ILLEGAL_INSTRUCTION:
      case STATUS_PRIVILEGED_INSTRUCTION:
        exception = Exception::Illegal;
        break;
      default:
        exception = Exception::Other;
    }
  }
#else
  if (this->Signal) {
    switch (this->Signal) {
      case SIGSEGV:
        exception = Exception::Fault;
        break;
      case SIGFPE:
        exception = Exception::Numerical;
        break;
      case SIGINT:
        exception = Exception::Interrupt;
        break;
      case SIGILL:
        exception = Exception::Illegal;
        break;
      default:
        exception = Exception::Other;
    }
  }
#endif
  return exception;
}

std::string cmProcess::GetExitExceptionString()
{
  std::string exception_str;
#if defined(_WIN32)
  switch (this->ExitValue) {
    case STATUS_CONTROL_C_EXIT:
      exception_str = "User interrupt";
      break;
    case STATUS_FLOAT_DENORMAL_OPERAND:
      exception_str = "Floating-point exception (denormal operand)";
      break;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:
      exception_str = "Divide-by-zero";
      break;
    case STATUS_FLOAT_INEXACT_RESULT:
      exception_str = "Floating-point exception (inexact result)";
      break;
    case STATUS_FLOAT_INVALID_OPERATION:
      exception_str = "Invalid floating-point operation";
      break;
    case STATUS_FLOAT_OVERFLOW:
      exception_str = "Floating-point overflow";
      break;
    case STATUS_FLOAT_STACK_CHECK:
      exception_str = "Floating-point stack check failed";
      break;
    case STATUS_FLOAT_UNDERFLOW:
      exception_str = "Floating-point underflow";
      break;
#  ifdef STATUS_FLOAT_MULTIPLE_FAULTS
    case STATUS_FLOAT_MULTIPLE_FAULTS:
      exception_str = "Floating-point exception (multiple faults)";
      break;
#  endif
#  ifdef STATUS_FLOAT_MULTIPLE_TRAPS
    case STATUS_FLOAT_MULTIPLE_TRAPS:
      exception_str = "Floating-point exception (multiple traps)";
      break;
#  endif
    case STATUS_INTEGER_DIVIDE_BY_ZERO:
      exception_str = "Integer divide-by-zero";
      break;
    case STATUS_INTEGER_OVERFLOW:
      exception_str = "Integer overflow";
      break;

    case STATUS_DATATYPE_MISALIGNMENT:
      exception_str = "Datatype misalignment";
      break;
    case STATUS_ACCESS_VIOLATION:
      exception_str = "Access violation";
      break;
    case STATUS_IN_PAGE_ERROR:
      exception_str = "In-page error";
      break;
    case STATUS_INVALID_HANDLE:
      exception_str = "Invalid handle";
      break;
    case STATUS_NONCONTINUABLE_EXCEPTION:
      exception_str = "Noncontinuable exception";
      break;
    case STATUS_INVALID_DISPOSITION:
      exception_str = "Invalid disposition";
      break;
    case STATUS_ARRAY_BOUNDS_EXCEEDED:
      exception_str = "Array bounds exceeded";
      break;
    case STATUS_STACK_OVERFLOW:
      exception_str = "Stack overflow";
      break;

    case STATUS_ILLEGAL_INSTRUCTION:
      exception_str = "Illegal instruction";
      break;
    case STATUS_PRIVILEGED_INSTRUCTION:
      exception_str = "Privileged instruction";
      break;
    case STATUS_NO_MEMORY:
    default:
      char buf[1024];
      _snprintf(buf, 1024, "Exit code 0x%x\n", this->ExitValue);
      exception_str.assign(buf);
  }
#else
  switch (this->Signal) {
#  ifdef SIGSEGV
    case SIGSEGV:
      exception_str = "Segmentation fault";
      break;
#  endif
#  ifdef SIGBUS
#    if !defined(SIGSEGV) || SIGBUS != SIGSEGV
    case SIGBUS:
      exception_str = "Bus error";
      break;
#    endif
#  endif
#  ifdef SIGFPE
    case SIGFPE:
      exception_str = "Floating-point exception";
      break;
#  endif
#  ifdef SIGILL
    case SIGILL:
      exception_str = "Illegal instruction";
      break;
#  endif
#  ifdef SIGINT
    case SIGINT:
      exception_str = "User interrupt";
      break;
#  endif
#  ifdef SIGABRT
    case SIGABRT:
      exception_str = "Child aborted";
      break;
#  endif
#  ifdef SIGKILL
    case SIGKILL:
      exception_str = "Child killed";
      break;
#  endif
#  ifdef SIGTERM
    case SIGTERM:
      exception_str = "Child terminated";
      break;
#  endif
#  ifdef SIGHUP
    case SIGHUP:
      exception_str = "SIGHUP";
      break;
#  endif
#  ifdef SIGQUIT
    case SIGQUIT:
      exception_str = "SIGQUIT";
      break;
#  endif
#  ifdef SIGTRAP
    case SIGTRAP:
      exception_str = "SIGTRAP";
      break;
#  endif
#  ifdef SIGIOT
#    if !defined(SIGABRT) || SIGIOT != SIGABRT
    case SIGIOT:
      exception_str = "SIGIOT";
      break;
#    endif
#  endif
#  ifdef SIGUSR1
    case SIGUSR1:
      exception_str = "SIGUSR1";
      break;
#  endif
#  ifdef SIGUSR2
    case SIGUSR2:
      exception_str = "SIGUSR2";
      break;
#  endif
#  ifdef SIGPIPE
    case SIGPIPE:
      exception_str = "SIGPIPE";
      break;
#  endif
#  ifdef SIGALRM
    case SIGALRM:
      exception_str = "SIGALRM";
      break;
#  endif
#  ifdef SIGSTKFLT
    case SIGSTKFLT:
      exception_str = "SIGSTKFLT";
      break;
#  endif
#  ifdef SIGCHLD
    case SIGCHLD:
      exception_str = "SIGCHLD";
      break;
#  elif defined(SIGCLD)
    case SIGCLD:
      exception_str = "SIGCLD";
      break;
#  endif
#  ifdef SIGCONT
    case SIGCONT:
      exception_str = "SIGCONT";
      break;
#  endif
#  ifdef SIGSTOP
    case SIGSTOP:
      exception_str = "SIGSTOP";
      break;
#  endif
#  ifdef SIGTSTP
    case SIGTSTP:
      exception_str = "SIGTSTP";
      break;
#  endif
#  ifdef SIGTTIN
    case SIGTTIN:
      exception_str = "SIGTTIN";
      break;
#  endif
#  ifdef SIGTTOU
    case SIGTTOU:
      exception_str = "SIGTTOU";
      break;
#  endif
#  ifdef SIGURG
    case SIGURG:
      exception_str = "SIGURG";
      break;
#  endif
#  ifdef SIGXCPU
    case SIGXCPU:
      exception_str = "SIGXCPU";
      break;
#  endif
#  ifdef SIGXFSZ
    case SIGXFSZ:
      exception_str = "SIGXFSZ";
      break;
#  endif
#  ifdef SIGVTALRM
    case SIGVTALRM:
      exception_str = "SIGVTALRM";
      break;
#  endif
#  ifdef SIGPROF
    case SIGPROF:
      exception_str = "SIGPROF";
      break;
#  endif
#  ifdef SIGWINCH
    case SIGWINCH:
      exception_str = "SIGWINCH";
      break;
#  endif
#  ifdef SIGPOLL
    case SIGPOLL:
      exception_str = "SIGPOLL";
      break;
#  endif
#  ifdef SIGIO
#    if !defined(SIGPOLL) || SIGIO != SIGPOLL
    case SIGIO:
      exception_str = "SIGIO";
      break;
#    endif
#  endif
#  ifdef SIGPWR
    case SIGPWR:
      exception_str = "SIGPWR";
      break;
#  endif
#  ifdef SIGSYS
    case SIGSYS:
      exception_str = "SIGSYS";
      break;
#  endif
#  ifdef SIGUNUSED
#    if !defined(SIGSYS) || SIGUNUSED != SIGSYS
    case SIGUNUSED:
      exception_str = "SIGUNUSED";
      break;
#    endif
#  endif
    default:
      exception_str = "Signal ";
      exception_str += std::to_string(this->Signal);
  }
#endif
  return exception_str;
}
