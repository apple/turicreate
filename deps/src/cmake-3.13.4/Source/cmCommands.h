/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCommands_h
#define cmCommands_h

class cmState;

/**
 * Global function to register all compiled in commands.
 * To add a new command edit cmCommands.cxx and add your command.
 * It is up to the caller to delete the commands created by this call.
 */
void GetScriptingCommands(cmState* state);
void GetProjectCommands(cmState* state);
void GetProjectCommandsInScriptMode(cmState* state);

#endif
