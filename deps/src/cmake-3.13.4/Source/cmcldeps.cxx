// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Wrapper around cl that adds /showIncludes to command line, and uses that to
// generate .d files that match the style from gcc -MD.
//
// /showIncludes is equivalent to -MD, not -MMD, that is, system headers are
// included.

#include "cmSystemTools.h"
#include "cmsys/Encoding.hxx"

#include <algorithm>
#include <sstream>
#include <windows.h>

// We don't want any wildcard expansion.
// See http://msdn.microsoft.com/en-us/library/zay8tzh6(v=vs.85).aspx
void _setargv()
{
}

static void Fatal(const char* msg, ...)
{
  va_list ap;
  fprintf(stderr, "ninja: FATAL: ");
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  // On Windows, some tools may inject extra threads.
  // exit() may block on locks held by those threads, so forcibly exit.
  fflush(stderr);
  fflush(stdout);
  ExitProcess(1);
}

static void usage(const char* msg)
{
  Fatal("%s\n\nusage:\n    "
        "cmcldeps "
        "<language C, CXX or RC>  "
        "<source file path>  "
        "<output path for *.d file>  "
        "<output path for *.obj file>  "
        "<prefix of /showIncludes>  "
        "<path to cl.exe>  "
        "<path to tool (cl or rc)>  "
        "<rest of command ...>\n",
        msg);
}

static std::string trimLeadingSpace(const std::string& cmdline)
{
  int i = 0;
  for (; cmdline[i] == ' '; ++i)
    ;
  return cmdline.substr(i);
}

static void replaceAll(std::string& str, const std::string& search,
                       const std::string& repl)
{
  std::string::size_type pos = 0;
  while ((pos = str.find(search, pos)) != std::string::npos) {
    str.replace(pos, search.size(), repl);
    pos += repl.size();
  }
}

bool startsWith(const std::string& str, const std::string& what)
{
  return str.compare(0, what.size(), what) == 0;
}

// Strips one argument from the cmdline and returns it. "surrounding quotes"
// are removed from the argument if there were any.
static std::string getArg(std::string& cmdline)
{
  std::string ret;
  bool in_quoted = false;
  unsigned int i = 0;

  cmdline = trimLeadingSpace(cmdline);

  for (;; ++i) {
    if (i >= cmdline.size())
      usage("Couldn't parse arguments.");
    if (!in_quoted && cmdline[i] == ' ')
      break; // "a b" "x y"
    if (cmdline[i] == '"')
      in_quoted = !in_quoted;
  }

  ret = cmdline.substr(0, i);
  if (ret[0] == '"' && ret[i - 1] == '"')
    ret = ret.substr(1, ret.size() - 2);
  cmdline = cmdline.substr(i);
  return ret;
}

static void parseCommandLine(LPWSTR wincmdline, std::string& lang,
                             std::string& srcfile, std::string& dfile,
                             std::string& objfile, std::string& prefix,
                             std::string& clpath, std::string& binpath,
                             std::string& rest)
{
  std::string cmdline = cmsys::Encoding::ToNarrow(wincmdline);
  /* self */ getArg(cmdline);
  lang = getArg(cmdline);
  srcfile = getArg(cmdline);
  dfile = getArg(cmdline);
  objfile = getArg(cmdline);
  prefix = getArg(cmdline);
  clpath = getArg(cmdline);
  binpath = getArg(cmdline);
  rest = trimLeadingSpace(cmdline);
}

// Not all backslashes need to be escaped in a depfile, but it's easier that
// way.  See the re2c grammar in ninja's source code for more info.
static void escapePath(std::string& path)
{
  replaceAll(path, "\\", "\\\\");
  replaceAll(path, " ", "\\ ");
}

static void outputDepFile(const std::string& dfile, const std::string& objfile,
                          std::vector<std::string>& incs)
{

  if (dfile.empty())
    return;

  // strip duplicates
  std::sort(incs.begin(), incs.end());
  incs.erase(std::unique(incs.begin(), incs.end()), incs.end());

  FILE* out = cmsys::SystemTools::Fopen(dfile.c_str(), "wb");

  // FIXME should this be fatal or not? delete obj? delete d?
  if (!out)
    return;
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  replaceAll(cwd, "/", "\\");
  cwd += "\\";

  std::string tmp = objfile;
  escapePath(tmp);
  fprintf(out, "%s: \\\n", tmp.c_str());

  std::vector<std::string>::iterator it = incs.begin();
  for (; it != incs.end(); ++it) {
    tmp = *it;
    // The paths need to match the ones used to identify build artifacts in the
    // build.ninja file.  Therefore we need to canonicalize the path to use
    // backward slashes and relativize the path to the build directory.
    replaceAll(tmp, "/", "\\");
    if (startsWith(tmp, cwd))
      tmp = tmp.substr(cwd.size());
    escapePath(tmp);
    fprintf(out, "%s \\\n", tmp.c_str());
  }

  fprintf(out, "\n");
  fclose(out);
}

bool contains(const std::string& str, const std::string& what)
{
  return str.find(what) != std::string::npos;
}

std::string replace(const std::string& str, const std::string& what,
                    const std::string& replacement)
{
  size_t pos = str.find(what);
  if (pos == std::string::npos)
    return str;
  std::string replaced = str;
  return replaced.replace(pos, what.size(), replacement);
}

static int process(const std::string& srcfilename, const std::string& dfile,
                   const std::string& objfile, const std::string& prefix,
                   const std::string& cmd, const std::string& dir = "",
                   bool quiet = false)
{
  std::string output;
  // break up command line into a vector
  std::vector<std::string> args;
  cmSystemTools::ParseWindowsCommandLine(cmd.c_str(), args);
  // convert to correct vector type for RunSingleCommand
  std::vector<std::string> command;
  for (std::vector<std::string>::iterator i = args.begin(); i != args.end();
       ++i) {
    command.push_back(*i);
  }
  // run the command
  int exit_code = 0;
  bool run =
    cmSystemTools::RunSingleCommand(command, &output, &output, &exit_code,
                                    dir.c_str(), cmSystemTools::OUTPUT_NONE);

  // process the include directives and output everything else
  std::istringstream ss(output);
  std::string line;
  std::vector<std::string> includes;
  bool isFirstLine = true; // cl prints always first the source filename
  while (std::getline(ss, line)) {
    if (startsWith(line, prefix)) {
      std::string inc = trimLeadingSpace(line.substr(prefix.size()).c_str());
      if (inc[inc.size() - 1] == '\r') // blech, stupid \r\n
        inc = inc.substr(0, inc.size() - 1);
      includes.push_back(inc);
    } else {
      if (!isFirstLine || !startsWith(line, srcfilename)) {
        if (!quiet || exit_code != 0) {
          fprintf(stdout, "%s\n", line.c_str());
        }
      } else {
        isFirstLine = false;
      }
    }
  }

  // don't update .d until/unless we succeed compilation
  if (run && exit_code == 0)
    outputDepFile(dfile, objfile, includes);

  return exit_code;
}

int main()
{

  // Use the Win32 API instead of argc/argv so we can avoid interpreting the
  // rest of command line after the .d and .obj. Custom parsing seemed
  // preferable to the ugliness you get into in trying to re-escape quotes for
  // subprocesses, so by avoiding argc/argv, the subprocess is called with
  // the same command line verbatim.

  std::string lang, srcfile, dfile, objfile, prefix, cl, binpath, rest;
  parseCommandLine(GetCommandLineW(), lang, srcfile, dfile, objfile, prefix,
                   cl, binpath, rest);

  // needed to suppress filename output of msvc tools
  std::string srcfilename;
  {
    std::string::size_type pos = srcfile.rfind('\\');
    if (pos == std::string::npos) {
      srcfilename = srcfile;
    } else {
      srcfilename = srcfile.substr(pos + 1);
    }
  }

  std::string nol = " /nologo ";
  std::string show = " /showIncludes ";
  if (lang == "C" || lang == "CXX") {
    return process(srcfilename, dfile, objfile, prefix,
                   binpath + nol + show + rest);
  } else if (lang == "RC") {
    // "misuse" cl.exe to get headers from .rc files

    std::string clrest = rest;
    // rc: /fo x.dir\x.rc.res  ->  cl: /out:x.dir\x.rc.res.dep.obj
    clrest = replace(clrest, "/fo", "/out:");
    clrest = replace(clrest, objfile, objfile + ".dep.obj ");

    cl = "\"" + cl + "\" /P /DRC_INVOKED /TC ";

    // call cl in object dir so the .i is generated there
    std::string objdir;
    {
      std::string::size_type pos = objfile.rfind("\\");
      if (pos != std::string::npos) {
        objdir = objfile.substr(0, pos);
      }
    }

    // extract dependencies with cl.exe
    int exit_code = process(srcfilename, dfile, objfile, prefix,
                            cl + nol + show + clrest, objdir, true);

    if (exit_code != 0)
      return exit_code;

    // compile rc file with rc.exe
    return process(srcfilename, "", objfile, prefix, binpath + " " + rest);
  }

  usage("Invalid language specified.");
  return 1;
}
