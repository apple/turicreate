/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <core/parallel/pthread_h.h>
#include <core/logging/backtrace.hpp>
#include <core/export.hpp>

EXPORT file_logger& global_logger() {
  static file_logger l;
  return l;
}

void streambuffdestructor(void* v){
  logger_impl::streambuff_tls_entry* t =
    reinterpret_cast<logger_impl::streambuff_tls_entry*>(v);
  delete t;
}

const char* messages[] = {  "DEBUG:    ",
                            "DEBUG:    ",
                            "INFO:     ",
                            "INFO:     ",
                            "PROGRESS: ",
                            "WARNING:  ",
                            "ERROR:    ",
                            "FATAL:    "};


file_logger::file_logger() {
  log_file = "";
  log_to_console = true;
  log_level = LOG_EMPH;
  for (size_t i = 0;i < LOG_NONE; ++i) {
    has_callback[i] = 0;
  }
  pthread_mutex_init(&mut, NULL);
  pthread_key_create(&streambuffkey, streambuffdestructor);
}

file_logger::~file_logger() {
  pthread_mutex_lock(&mut);
  if (fout.good()) {
    fout.flush();
    fout.close();
  }
  pthread_mutex_unlock(&mut);

  pthread_mutex_destroy(&mut);
}

bool file_logger::set_log_file(std::string file) {
  // close the file if it is open
  pthread_mutex_lock(&mut);
  if (fout.good()) {
    fout.flush();
    fout.close();
    log_file = "";
  }
  // if file is not an empty string, open the new file
  if (file.length() > 0) {
    fout.open(file.c_str());
    if (fout.fail()) {
      pthread_mutex_unlock(&mut);
      return false;
    }
    log_file = file;
  }
  pthread_mutex_unlock(&mut);
  return true;
}


void textcolor(FILE* handle, int attr, int fg)
{
  char command[13];
  /* Command is the control command to the terminal */
  sprintf(command, "%c[%d;%dm", 0x1B, attr, fg + 30);
  fprintf(handle, "%s", command);
}

std::string textcolor(int attr, int fg)
{
  char command[13];
  /* Command is the control command to the terminal */
  sprintf(command, "%c[%d;%dm", 0x1B, attr, fg + 30);
  return command;
}

void reset_color(FILE* handle)
{
  char command[20];
  /* Command is the control command to the terminal */
  sprintf(command, "%c[0m", 0x1B);
  fprintf(handle, "%s", command);
}


std::string reset_color()
{
  char command[20];
  /* Command is the control command to the terminal */
  sprintf(command, "%c[0m", 0x1B);
  return command;
}



void file_logger::_log(int lineloglevel,const char* file,const char* function,
                       int line,const char* fmt, va_list ap ) {
  // if the logger level fits
  if (lineloglevel >= log_level){
    // get just the filename. this line found on a forum on line.
    // claims to be from google.
    file = ((strrchr(file, '/') ? : file- 1) + 1);

    char str[1024];

    // write the actual header (only show file in debug build)
#ifndef NDEBUG
    int head_bytes_written = snprintf(str,1024, "%s%s(%s:%d): ",
                                  messages[lineloglevel],file,function,line);
#else
    int head_bytes_written = snprintf(str,1024, "%s(%s:%d): ",
                                  messages[lineloglevel],function,line);
#endif
    // write the actual logger

    int byteswritten = head_bytes_written +
        vsnprintf(str + head_bytes_written, 1024 - head_bytes_written,fmt,ap);

    str[byteswritten] = '\n';
    str[byteswritten+1] = 0;
    // write the output
    if (has_callback[lineloglevel]) {
      pthread_mutex_lock(&mut);
      if (callback[lineloglevel]) {
        // only emit the message not the header
        callback[lineloglevel](lineloglevel,
                               str + head_bytes_written,
                               byteswritten - head_bytes_written);
      }
      pthread_mutex_unlock(&mut);
    }
    _lograw(lineloglevel, str, byteswritten);
  }
}



void file_logger::_logbuf(int lineloglevel,const char* file,const char* function,
                          int line,const char* buf, int len) {
  // if the logger level fits
  if (lineloglevel >= log_level){
    // get just the filename. this line found on a forum on line.
    // claims to be from google.
    file = ((strrchr(file, '/') ? : file- 1) + 1);

    // length of the 'head' of the string
#ifndef NDEBUG
    size_t headerlen = snprintf(NULL,0,"%s%s(%s:%d): ",
                                messages[lineloglevel],file,function,line);
#else
    size_t headerlen = snprintf(NULL,0,"%s(%s:%d): ",
                                messages[lineloglevel],function,line);
#endif

    if (headerlen> 2047) {
      std::cerr << "Header length exceed buffer length!";
    }
    else {
      char str[2048];
      const char *newline="\n";
#ifndef NDEBUG
      // write the actual header
      int byteswritten = snprintf(str,2047,"%s%s(%s:%d): ",
                                  messages[lineloglevel],file,function,line);
#else
      int byteswritten = snprintf(str,2047,"%s(%s:%d): ",
                                  messages[lineloglevel],function,line);
#endif

      char final_str[byteswritten + len + strlen(newline) + 1];
      final_str[0] = '\0';
      strncpy(final_str, str, byteswritten);
      strncat(final_str, buf, len);
      strncat(final_str, newline, strlen(newline));
      _lograw(lineloglevel,final_str, byteswritten + len + strlen(newline));
      // call the callback on the message
      if (has_callback[lineloglevel]) {
        pthread_mutex_lock(&mut);
        if (callback[lineloglevel]) {
          callback[lineloglevel](lineloglevel, buf, len);
        }
        pthread_mutex_unlock(&mut);
      }
    }
  }
}

void file_logger::_lograw(int lineloglevel, const char* buf, int len) {
  pthread_mutex_lock(&mut);
  if (fout.good()) {
    fout.write(buf,len);
    fout.flush();
  }
  pthread_mutex_unlock(&mut);
  if (log_to_console || log_to_stderr) {

#ifdef COLOROUTPUT

    pthread_mutex_lock(&mut);
    if (lineloglevel == LOG_FATAL) {
      textcolor(stderr, TEXTCOLOR_BRIGHT, TEXTCOLOR_RED);
    }
    else if (lineloglevel == LOG_ERROR) {
      textcolor(log_to_stderr ? stderr : stdout, TEXTCOLOR_BRIGHT, TEXTCOLOR_RED);
    }
    else if (lineloglevel == LOG_WARNING) {
      textcolor(log_to_stderr ? stderr : stdout, TEXTCOLOR_BRIGHT, TEXTCOLOR_MAGENTA);
    }
    else if (lineloglevel == LOG_DEBUG) {
      textcolor(log_to_stderr ? stderr : stdout, TEXTCOLOR_BRIGHT, TEXTCOLOR_YELLOW);
    }
    else if (lineloglevel == LOG_EMPH) {
      textcolor(log_to_stderr ? stderr : stdout, TEXTCOLOR_BRIGHT, TEXTCOLOR_GREEN);
    }
#endif
    if(lineloglevel >= LOG_FATAL) {
      std::cerr.write(buf,len);
    } else {
      (log_to_stderr ? std::cerr : std::cout).write(buf,len);
    }
#ifdef COLOROUTPUT

    pthread_mutex_unlock(&mut);
    if (lineloglevel >= LOG_FATAL) {
      reset_color(stderr);
    } else {
      reset_color(log_to_stderr ? stderr : stdout);
    }
#endif
  }
}

file_logger& file_logger::start_stream(int lineloglevel,const char* file,
                                       const char* function, int line, bool do_start) {
  // get the stream buffer
  logger_impl::streambuff_tls_entry* streambufentry =
        reinterpret_cast<logger_impl::streambuff_tls_entry*>(
                              pthread_getspecific(streambuffkey));
  // create the key if it doesn't exist
  if (streambufentry == NULL) {
    streambufentry = new logger_impl::streambuff_tls_entry;
    pthread_setspecific(streambuffkey, streambufentry);
  }
  if (streambufentry->streambuffer.str().length() > 0) {
    (*this) << std::endl;
  }
  std::stringstream& streambuffer = streambufentry->streambuffer;
  bool& streamactive = streambufentry->streamactive;
  size_t& header_len = streambufentry->header_len;
  streambufentry->loglevel = lineloglevel;

  if (lineloglevel >= log_level){
    // get the stream buffer
    // if do not start the stream, just quit
    if (do_start == false) {
      streamactive = false;
      return *this;
    }

#ifndef NDEBUG
    file = ((strrchr(file, '/') ? : file- 1) + 1);

    if (streambuffer.str().length() == 0) {
      streambuffer << int(time(NULL)) << " : " << messages[lineloglevel] << file
                   << "(" << function << ":" <<line<<"): ";
    }
#else
    if (streambuffer.str().length() == 0) {
      streambuffer << int(time(NULL)) << " : " << messages[lineloglevel] << "(" << function << ":" <<line<<"): ";
    }
#endif
    streamactive = true;
    streamloglevel = lineloglevel;
    header_len = streambuffer.tellp();
  } else {
    streamactive = false;
  }
  return *this;
}
