#include "cmCPluginAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  char* LibraryName;
  int Argc;
  char** Argv;
} cmVTKWrapTclData;

/* do almost everything in the initial pass */
static int CCONV InitialPass(void* inf, void* mf, int argc, char* argv[])
{
  char* file;
  char* str;
  char* srcs;
  const char* cstr;
  char buffer[1024];
  void* source_file;
  char* args[2];
  char* ccArgs[4];
  char* ccDep[1];
  char* ccOut[1];
  cmLoadedCommandInfo* info = (cmLoadedCommandInfo*)inf;

  cmVTKWrapTclData* cdata =
    (cmVTKWrapTclData*)malloc(sizeof(cmVTKWrapTclData));
  cdata->LibraryName = "BOO";
  cdata->Argc = argc;
  cdata->Argv = argv;
  info->CAPI->SetClientData(info, cdata);

  /* Now check and see if the value has been stored in the cache */
  /* already, if so use that value and don't look for the program */
  if (!info->CAPI->IsOn(mf, "TEST_COMMAND_TEST1")) {
    info->CAPI->AddDefinition(mf, "TEST_DEF", "HOO");
    return 1;
  }

  info->CAPI->AddDefinition(mf, "TEST_DEF", "HOO");
  cdata->LibraryName = "HOO";

  info->CAPI->AddCacheDefinition(mf, "SOME_CACHE_VARIABLE", "ON",
                                 "Test cache variable", CM_CACHE_BOOL);
  info->CAPI->AddCacheDefinition(mf, "SOME_CACHE_VARIABLE1", "",
                                 "Test cache variable 1", CM_CACHE_PATH);
  info->CAPI->AddCacheDefinition(mf, "SOME_CACHE_VARIABLE2", "",
                                 "Test cache variable 2", CM_CACHE_FILEPATH);
  info->CAPI->AddCacheDefinition(mf, "SOME_CACHE_VARIABLE3", "",
                                 "Test cache variable 3", CM_CACHE_STRING);
  info->CAPI->AddCacheDefinition(mf, "SOME_CACHE_VARIABLE4", "",
                                 "Test cache variable 4", CM_CACHE_INTERNAL);
  info->CAPI->AddCacheDefinition(mf, "SOME_CACHE_VARIABLE5", "",
                                 "Test cache variable 5", CM_CACHE_STATIC);

  file = info->CAPI->ExpandVariablesInString(mf, "${CMAKE_COMMAND}", 0, 0);

  str = info->CAPI->GetFilenameWithoutExtension(file);
  info->CAPI->DisplaySatus(mf, str);
  info->CAPI->Free(str);
  str = info->CAPI->GetFilenamePath(file);
  info->CAPI->DisplaySatus(mf, str);
  info->CAPI->Free(str);
  str = info->CAPI->Capitalized("cmake");
  info->CAPI->DisplaySatus(mf, str);
  info->CAPI->Free(str);

  info->CAPI->DisplaySatus(mf, info->CAPI->GetProjectName(mf));
  info->CAPI->DisplaySatus(mf, info->CAPI->GetHomeDirectory(mf));
  info->CAPI->DisplaySatus(mf, info->CAPI->GetHomeOutputDirectory(mf));
  info->CAPI->DisplaySatus(mf, info->CAPI->GetStartDirectory(mf));
  info->CAPI->DisplaySatus(mf, info->CAPI->GetStartOutputDirectory(mf));
  info->CAPI->DisplaySatus(mf, info->CAPI->GetCurrentDirectory(mf));
  info->CAPI->DisplaySatus(mf, info->CAPI->GetCurrentOutputDirectory(mf));
  sprintf(buffer, "Cache version: %d.%d, CMake version: %d.%d",
          info->CAPI->GetCacheMajorVersion(mf),
          info->CAPI->GetCacheMinorVersion(mf),
          info->CAPI->GetMajorVersion(mf), info->CAPI->GetMinorVersion(mf));
  info->CAPI->DisplaySatus(mf, buffer);
  if (info->CAPI->CommandExists(mf, "SET")) {
    info->CAPI->DisplaySatus(mf, "Command SET exists");
  }
  if (info->CAPI->CommandExists(mf, "SET_FOO_BAR")) {
    info->CAPI->SetError(mf, "Command SET_FOO_BAR should not exists");
    return 0;
  }
  info->CAPI->AddDefineFlag(mf, "-DADDED_DEFINITION");

  source_file = info->CAPI->CreateNewSourceFile(mf);
  cstr = info->CAPI->SourceFileGetSourceName(source_file);
  sprintf(buffer, "Shold be empty (source file name): [%s]", cstr);
  info->CAPI->DisplaySatus(mf, buffer);
  cstr = info->CAPI->SourceFileGetFullPath(source_file);
  sprintf(buffer, "Should be empty (source file full path): [%s]", cstr);
  info->CAPI->DisplaySatus(mf, buffer);
  info->CAPI->DefineSourceFileProperty(mf, "SOME_PROPERTY", "unused old prop",
                                       "This property is no longer used", 0);
  if (info->CAPI->SourceFileGetPropertyAsBool(source_file, "SOME_PROPERTY")) {
    info->CAPI->SetError(mf, "Property SOME_PROPERTY should not be defined");
    return 0;
  }
  info->CAPI->DefineSourceFileProperty(mf, "SOME_PROPERTY2", "nice prop",
                                       "This property is for testing.", 0);
  info->CAPI->SourceFileSetProperty(source_file, "SOME_PROPERTY2", "HERE");
  cstr = info->CAPI->SourceFileGetProperty(source_file, "ABSTRACT");
  sprintf(buffer, "Should be 0 (source file abstract property): [%p]", cstr);
  info->CAPI->DisplaySatus(mf, buffer);

  info->CAPI->DestroySourceFile(source_file);

  srcs = argv[2];
  info->CAPI->AddExecutable(mf, "LoadedCommand", 1, &srcs, 0);

  /* add customs commands to generate the source file */
  ccArgs[0] = "-E";
  ccArgs[1] = "copy";
  ccArgs[2] = argv[0];
  ccArgs[3] = argv[1];
  ccDep[0] = ccArgs[2];
  ccOut[0] = ccArgs[3];
  info->CAPI->AddCustomCommand(mf, "LoadedCommand.cxx.in", file, 4, ccArgs, 1,
                               ccDep, 1, ccOut, "LoadedCommand");

  ccArgs[2] = argv[1];
  ccArgs[3] = argv[2];
  ccDep[0] = ccArgs[2];
  ccOut[0] = ccArgs[3];
  info->CAPI->AddCustomCommandToOutput(mf, ccOut[0], file, 4, ccArgs, ccDep[0],
                                       0, 0);

  ccArgs[1] = "echo";
  ccArgs[2] = "Build has finished";
  info->CAPI->AddCustomCommandToTarget(mf, "LoadedCommand", file, 3, ccArgs,
                                       CM_POST_BUILD);

  info->CAPI->Free(file);

  args[0] = "TEST_EXEC";
  args[1] = "TRUE";

  /* code coverage */
  if (info->CAPI->GetTotalArgumentSize(2, args) != 13) {
    return 0;
  }
  info->CAPI->ExecuteCommand(mf, "SET", 2, args);

  /* make sure we can find the source file */
  if (!info->CAPI->GetSource(mf, argv[1])) {
    info->CAPI->SetError(mf, "Source file could not be found!");
    return 0;
  }

  return 1;
}

static void CCONV FinalPass(void* inf, void* mf)
{
  cmLoadedCommandInfo* info = (cmLoadedCommandInfo*)inf;
  /* get our client data from initial pass */
  cmVTKWrapTclData* cdata = (cmVTKWrapTclData*)info->CAPI->GetClientData(info);
  if (strcmp(info->CAPI->GetDefinition(mf, "TEST_DEF"), "HOO") ||
      strcmp(cdata->LibraryName, "HOO")) {
    fprintf(stderr, "*** Failed LOADED COMMAND Final Pass\n");
  }
}
static void CCONV Destructor(void* inf)
{
  cmLoadedCommandInfo* info = (cmLoadedCommandInfo*)inf;
  /* get our client data from initial pass */
  cmVTKWrapTclData* cdata = (cmVTKWrapTclData*)info->CAPI->GetClientData(info);
  free(cdata);
}

#ifdef MUCHO_MUDSLIDE
void CM_PLUGIN_EXPORT CCONV CMAKE_TEST_COMMANDInit(cmLoadedCommandInfo* info)
{
  info->InitialPass = InitialPass;
  info->FinalPass = FinalPass;
  info->Destructor = Destructor;
  info->m_Inherited = 0;
  info->Name = "CMAKE_TEST_COMMAND";
}
#endif
