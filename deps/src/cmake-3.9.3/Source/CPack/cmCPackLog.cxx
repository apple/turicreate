/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackLog.h"

#include "cmConfigure.h"
#include <iostream>

#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"

cmCPackLog::cmCPackLog()
{
  this->Verbose = false;
  this->Debug = false;
  this->Quiet = false;
  this->NewLine = true;

  this->LastTag = cmCPackLog::NOTAG;
  this->DefaultOutput = &std::cout;
  this->DefaultError = &std::cerr;

  this->LogOutput = CM_NULLPTR;
  this->LogOutputCleanup = false;
}

cmCPackLog::~cmCPackLog()
{
  this->SetLogOutputStream(CM_NULLPTR);
}

void cmCPackLog::SetLogOutputStream(std::ostream* os)
{
  if (this->LogOutputCleanup && this->LogOutput) {
    delete this->LogOutput;
  }
  this->LogOutputCleanup = false;
  this->LogOutput = os;
}

bool cmCPackLog::SetLogOutputFile(const char* fname)
{
  cmGeneratedFileStream* cg = CM_NULLPTR;
  if (fname) {
    cg = new cmGeneratedFileStream(fname);
  }
  if (cg && !*cg) {
    delete cg;
    cg = CM_NULLPTR;
  }
  this->SetLogOutputStream(cg);
  if (!cg) {
    return false;
  }
  this->LogOutputCleanup = true;
  return true;
}

void cmCPackLog::Log(int tag, const char* file, int line, const char* msg,
                     size_t length)
{
  // By default no logging
  bool display = false;

  // Display file and line number if debug
  bool useFileAndLine = this->Debug;

  bool output = false;
  bool debug = false;
  bool warning = false;
  bool error = false;
  bool verbose = false;

  // When writing in file, add list of tags whenever tag changes.
  std::string tagString;
  bool needTagString = false;
  if (this->LogOutput && this->LastTag != tag) {
    needTagString = true;
  }

  if (tag & LOG_OUTPUT) {
    output = true;
    display = true;
    if (needTagString) {
      if (!tagString.empty()) {
        tagString += ",";
      }
      tagString = "VERBOSE";
    }
  }
  if (tag & LOG_WARNING) {
    warning = true;
    display = true;
    if (needTagString) {
      if (!tagString.empty()) {
        tagString += ",";
      }
      tagString = "WARNING";
    }
  }
  if (tag & LOG_ERROR) {
    error = true;
    display = true;
    if (needTagString) {
      if (!tagString.empty()) {
        tagString += ",";
      }
      tagString = "ERROR";
    }
  }
  if (tag & LOG_DEBUG && this->Debug) {
    debug = true;
    display = true;
    if (needTagString) {
      if (!tagString.empty()) {
        tagString += ",";
      }
      tagString = "DEBUG";
    }
    useFileAndLine = true;
  }
  if (tag & LOG_VERBOSE && this->Verbose) {
    verbose = true;
    display = true;
    if (needTagString) {
      if (!tagString.empty()) {
        tagString += ",";
      }
      tagString = "VERBOSE";
    }
  }
  if (this->Quiet) {
    display = false;
  }
  if (this->LogOutput) {
    if (needTagString) {
      *this->LogOutput << "[" << file << ":" << line << " " << tagString
                       << "] ";
    }
    this->LogOutput->write(msg, length);
  }
  this->LastTag = tag;
  if (!display) {
    return;
  }
  if (this->NewLine) {
    if (error && !this->ErrorPrefix.empty()) {
      *this->DefaultError << this->ErrorPrefix;
    } else if (warning && !this->WarningPrefix.empty()) {
      *this->DefaultError << this->WarningPrefix;
    } else if (output && !this->OutputPrefix.empty()) {
      *this->DefaultOutput << this->OutputPrefix;
    } else if (verbose && !this->VerbosePrefix.empty()) {
      *this->DefaultOutput << this->VerbosePrefix;
    } else if (debug && !this->DebugPrefix.empty()) {
      *this->DefaultOutput << this->DebugPrefix;
    } else if (!this->Prefix.empty()) {
      *this->DefaultOutput << this->Prefix;
    }
    if (useFileAndLine) {
      if (error || warning) {
        *this->DefaultError << file << ":" << line << " ";
      } else {
        *this->DefaultOutput << file << ":" << line << " ";
      }
    }
  }
  if (error || warning) {
    this->DefaultError->write(msg, length);
    this->DefaultError->flush();
  } else {
    this->DefaultOutput->write(msg, length);
    this->DefaultOutput->flush();
  }
  if (msg[length - 1] == '\n' || length > 2) {
    this->NewLine = true;
  }

  if (error) {
    cmSystemTools::SetErrorOccured();
  }
}
