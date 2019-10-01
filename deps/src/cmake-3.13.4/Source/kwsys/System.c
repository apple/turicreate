/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(System.h)

/* Work-around CMake dependency scanning limitation.  This must
   duplicate the above list of headers.  */
#if 0
#  include "System.h.in"
#endif

#include <ctype.h>  /* isspace */
#include <stddef.h> /* ptrdiff_t */
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memcpy */

#include <stdio.h>

#if defined(KWSYS_C_HAS_PTRDIFF_T) && KWSYS_C_HAS_PTRDIFF_T
typedef ptrdiff_t kwsysSystem_ptrdiff_t;
#else
typedef int kwsysSystem_ptrdiff_t;
#endif

static int kwsysSystem__AppendByte(char* local, char** begin, char** end,
                                   int* size, char c)
{
  /* Allocate space for the character.  */
  if ((*end - *begin) >= *size) {
    kwsysSystem_ptrdiff_t length = *end - *begin;
    char* newBuffer = (char*)malloc((size_t)(*size * 2));
    if (!newBuffer) {
      return 0;
    }
    memcpy(newBuffer, *begin, (size_t)(length) * sizeof(char));
    if (*begin != local) {
      free(*begin);
    }
    *begin = newBuffer;
    *end = *begin + length;
    *size *= 2;
  }

  /* Store the character.  */
  *(*end)++ = c;
  return 1;
}

static int kwsysSystem__AppendArgument(char** local, char*** begin,
                                       char*** end, int* size, char* arg_local,
                                       char** arg_begin, char** arg_end,
                                       int* arg_size)
{
  /* Append a null-terminator to the argument string.  */
  if (!kwsysSystem__AppendByte(arg_local, arg_begin, arg_end, arg_size,
                               '\0')) {
    return 0;
  }

  /* Allocate space for the argument pointer.  */
  if ((*end - *begin) >= *size) {
    kwsysSystem_ptrdiff_t length = *end - *begin;
    char** newPointers = (char**)malloc((size_t)(*size) * 2 * sizeof(char*));
    if (!newPointers) {
      return 0;
    }
    memcpy(newPointers, *begin, (size_t)(length) * sizeof(char*));
    if (*begin != local) {
      free(*begin);
    }
    *begin = newPointers;
    *end = *begin + length;
    *size *= 2;
  }

  /* Allocate space for the argument string.  */
  **end = (char*)malloc((size_t)(*arg_end - *arg_begin));
  if (!**end) {
    return 0;
  }

  /* Store the argument in the command array.  */
  memcpy(**end, *arg_begin, (size_t)(*arg_end - *arg_begin));
  ++(*end);

  /* Reset the argument to be empty.  */
  *arg_end = *arg_begin;

  return 1;
}

#define KWSYSPE_LOCAL_BYTE_COUNT 1024
#define KWSYSPE_LOCAL_ARGS_COUNT 32
static char** kwsysSystem__ParseUnixCommand(const char* command, int flags)
{
  /* Create a buffer for argument pointers during parsing.  */
  char* local_pointers[KWSYSPE_LOCAL_ARGS_COUNT];
  int pointers_size = KWSYSPE_LOCAL_ARGS_COUNT;
  char** pointer_begin = local_pointers;
  char** pointer_end = pointer_begin;

  /* Create a buffer for argument strings during parsing.  */
  char local_buffer[KWSYSPE_LOCAL_BYTE_COUNT];
  int buffer_size = KWSYSPE_LOCAL_BYTE_COUNT;
  char* buffer_begin = local_buffer;
  char* buffer_end = buffer_begin;

  /* Parse the command string.  Try to behave like a UNIX shell.  */
  char** newCommand = 0;
  const char* c = command;
  int in_argument = 0;
  int in_escape = 0;
  int in_single = 0;
  int in_double = 0;
  int failed = 0;
  for (; *c; ++c) {
    if (in_escape) {
      /* This character is escaped so do no special handling.  */
      if (!in_argument) {
        in_argument = 1;
      }
      if (!kwsysSystem__AppendByte(local_buffer, &buffer_begin, &buffer_end,
                                   &buffer_size, *c)) {
        failed = 1;
        break;
      }
      in_escape = 0;
    } else if (*c == '\\') {
      /* The next character should be escaped.  */
      in_escape = 1;
    } else if (*c == '\'' && !in_double) {
      /* Enter or exit single-quote state.  */
      if (in_single) {
        in_single = 0;
      } else {
        in_single = 1;
        if (!in_argument) {
          in_argument = 1;
        }
      }
    } else if (*c == '"' && !in_single) {
      /* Enter or exit double-quote state.  */
      if (in_double) {
        in_double = 0;
      } else {
        in_double = 1;
        if (!in_argument) {
          in_argument = 1;
        }
      }
    } else if (isspace((unsigned char)*c)) {
      if (in_argument) {
        if (in_single || in_double) {
          /* This space belongs to a quoted argument.  */
          if (!kwsysSystem__AppendByte(local_buffer, &buffer_begin,
                                       &buffer_end, &buffer_size, *c)) {
            failed = 1;
            break;
          }
        } else {
          /* This argument has been terminated by whitespace.  */
          if (!kwsysSystem__AppendArgument(
                local_pointers, &pointer_begin, &pointer_end, &pointers_size,
                local_buffer, &buffer_begin, &buffer_end, &buffer_size)) {
            failed = 1;
            break;
          }
          in_argument = 0;
        }
      }
    } else {
      /* This character belong to an argument.  */
      if (!in_argument) {
        in_argument = 1;
      }
      if (!kwsysSystem__AppendByte(local_buffer, &buffer_begin, &buffer_end,
                                   &buffer_size, *c)) {
        failed = 1;
        break;
      }
    }
  }

  /* Finish the last argument.  */
  if (in_argument) {
    if (!kwsysSystem__AppendArgument(
          local_pointers, &pointer_begin, &pointer_end, &pointers_size,
          local_buffer, &buffer_begin, &buffer_end, &buffer_size)) {
      failed = 1;
    }
  }

  /* If we still have memory allocate space for the new command
     buffer.  */
  if (!failed) {
    kwsysSystem_ptrdiff_t n = pointer_end - pointer_begin;
    newCommand = (char**)malloc((size_t)(n + 1) * sizeof(char*));
  }

  if (newCommand) {
    /* Copy the arguments into the new command buffer.  */
    kwsysSystem_ptrdiff_t n = pointer_end - pointer_begin;
    memcpy(newCommand, pointer_begin, sizeof(char*) * (size_t)(n));
    newCommand[n] = 0;
  } else {
    /* Free arguments already allocated.  */
    while (pointer_end != pointer_begin) {
      free(*(--pointer_end));
    }
  }

  /* Free temporary buffers.  */
  if (pointer_begin != local_pointers) {
    free(pointer_begin);
  }
  if (buffer_begin != local_buffer) {
    free(buffer_begin);
  }

  /* The flags argument is currently unused.  */
  (void)flags;

  /* Return the final command buffer.  */
  return newCommand;
}

char** kwsysSystem_Parse_CommandForUnix(const char* command, int flags)
{
  /* Validate the flags.  */
  if (flags != 0) {
    return 0;
  }

  /* Forward to our internal implementation.  */
  return kwsysSystem__ParseUnixCommand(command, flags);
}
