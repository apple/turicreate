# This file is configured by CMake automatically as DartConfiguration.tcl
# If you choose not to use CMake, this file may be hand configured, by
# filling in the required variables.


# Configuration directories and files
SourceDirectory: /Users/hoytak/workspace/turicreate-4/deps/src/cmake-3.13.4/Utilities/cmlibarchive
BuildDirectory: /Users/hoytak/workspace/turicreate-4/deps/src/cmake-3.13.4/Utilities/cmlibarchive

# Where to place the cost data store
CostDataFile:

# Site is something like machine.domain, i.e. pragmatic.crd
Site: MacBook-Pro-7.local

# Build name is osname-revision-compiler, i.e. Linux-2.4.2-2smp-c++
BuildName: Darwin-clang++

# Subprojects
LabelsForSubprojects:

# Submission information
IsCDash: TRUE
CDashVersion: 1.6
QueryCDashVersion: TRUE
DropSite: open.cdash.org
DropLocation: /submit.php?project=CMake
DropSiteUser:
DropSitePassword:
DropSiteMode:
DropMethod: http
TriggerSite:
ScpCommand: /usr/bin/scp

# Dashboard start time
NightlyStartTime: 1:00:00 UTC

# Commands for the build/test/submit cycle
ConfigureCommand: "/Users/hoytak/workspace/turicreate-4/deps/src/cmake-3.13.4/Bootstrap.cmk/cmake" "/Users/hoytak/workspace/turicreate-4/deps/src/cmake-3.13.4/Utilities/cmlibarchive"
MakeCommand: /Users/hoytak/workspace/turicreate-4/deps/src/cmake-3.13.4/Bootstrap.cmk/cmake --build . --config "${CTEST_CONFIGURATION_TYPE}" -- -i
DefaultCTestConfigurationType: Release

# version control
UpdateVersionOnly:

# CVS options
# Default is "-d -P -A"
CVSCommand: CVSCOMMAND-NOTFOUND
CVSUpdateOptions: -d -A -P

# Subversion options
SVNCommand: /usr/bin/svn
SVNOptions:
SVNUpdateOptions:

# Git options
GITCommand: /usr/bin/git
GITInitSubmodules:
GITUpdateOptions:
GITUpdateCustom:

# Perforce options
P4Command: P4COMMAND-NOTFOUND
P4Client:
P4Options:
P4UpdateOptions:
P4UpdateCustom:

# Generic update command
UpdateCommand:
UpdateOptions:
UpdateType:

# Compiler info
Compiler: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++
CompilerVersion: 10.0.1.10010037

# Dynamic analysis (MemCheck)
PurifyCommand:
ValgrindCommand:
ValgrindCommandOptions:
MemoryCheckType:
MemoryCheckSanitizerOptions:
MemoryCheckCommand: MEMORYCHECK_COMMAND-NOTFOUND
MemoryCheckCommandOptions:
MemoryCheckSuppressionFile:

# Coverage
CoverageCommand: /usr/bin/gcov
CoverageExtraFlags: -l

# Cluster commands
SlurmBatchCommand: SLURM_SBATCH_COMMAND-NOTFOUND
SlurmRunCommand: SLURM_SRUN_COMMAND-NOTFOUND

# Testing options
# TimeOut is the amount of time in seconds to wait for processes
# to complete during testing.  After TimeOut seconds, the
# process will be summarily terminated.
# Currently set to 25 minutes
TimeOut: 1500

# During parallel testing CTest will not start a new test if doing
# so would cause the system load to exceed this value.
TestLoad:

UseLaunchers:
CurlOptions:
# warning, if you add new options here that have to do with submit,
# you have to update cmCTestSubmitCommand.cxx

# For CTest submissions that timeout, these options
# specify behavior for retrying the submission
CTestSubmitRetryDelay: 5
CTestSubmitRetryCount: 3
