/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#if defined(_WIN32)
#define NOMINMAX // use our min,max
#if !defined(_WIN32_WINNT) && !(defined(_MSC_VER) && _MSC_VER < 1300)
#define _WIN32_WINNT 0x0501
#endif
#include <winsock.h> // WSADATA, include before sys/types.h
#endif

#if (defined(__GNUC__) || defined(__PGI)) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

// TODO:
// We need an alternative implementation for many functions in this file
// when USE_ASM_INSTRUCTIONS gets defined as 0.
//
// Consider using these on Win32/Win64 for some of them:
//
// IsProcessorFeaturePresent
// http://msdn.microsoft.com/en-us/library/ms724482(VS.85).aspx
//
// GetProcessMemoryInfo
// http://msdn.microsoft.com/en-us/library/ms683219(VS.85).aspx

#include "kwsysPrivate.h"
#include KWSYS_HEADER(SystemInformation.hxx)
#include KWSYS_HEADER(Process.h)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "Process.h.in"
#include "SystemInformation.hxx.in"
#endif

#include <algorithm>
#include <bitset>
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#if defined(_MSC_VER) && _MSC_VER >= 1800
#define KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#endif
#include <errno.h>
#if defined(KWSYS_SYS_HAS_PSAPI)
#include <psapi.h>
#endif
#if !defined(siginfo_t)
typedef int siginfo_t;
#endif
#else
#include <sys/types.h>

#include <errno.h> // extern int errno;
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h> // getrlimit
#include <sys/time.h>
#include <sys/utsname.h> // int uname(struct utsname *buf);
#include <unistd.h>
#endif

#if defined(__CYGWIN__) && !defined(_WIN32)
#include <windows.h>
#undef _WIN32
#endif

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) ||    \
  defined(__DragonFly__)
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#if defined(KWSYS_SYS_HAS_IFADDRS_H)
#include <ifaddrs.h>
#include <net/if.h>
#define KWSYS_SYSTEMINFORMATION_IMPLEMENT_FQDN
#endif
#endif

#if defined(KWSYS_SYS_HAS_MACHINE_CPU_H)
#include <machine/cpu.h>
#endif

#ifdef __APPLE__
#include <fenv.h>
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_types.h>
#include <mach/vm_statistics.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#if defined(KWSYS_SYS_HAS_IFADDRS_H)
#include <ifaddrs.h>
#include <net/if.h>
#define KWSYS_SYSTEMINFORMATION_IMPLEMENT_FQDN
#endif
#if !(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ - 0 >= 1050)
#undef KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE
#endif
#endif

#if defined(__linux) || defined(__sun) || defined(_SCO_DS)
#include <fenv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#if defined(KWSYS_SYS_HAS_IFADDRS_H)
#include <ifaddrs.h>
#include <net/if.h>
#if !defined(__LSB_VERSION__) /* LSB has no getifaddrs */
#define KWSYS_SYSTEMINFORMATION_IMPLEMENT_FQDN
#endif
#endif
#if defined(KWSYS_CXX_HAS_RLIMIT64)
typedef struct rlimit64 ResourceLimitType;
#define GetResourceLimit getrlimit64
#else
typedef struct rlimit ResourceLimitType;
#define GetResourceLimit getrlimit
#endif
#elif defined(__hpux)
#include <sys/param.h>
#include <sys/pstat.h>
#if defined(KWSYS_SYS_HAS_MPCTL_H)
#include <sys/mpctl.h>
#endif
#endif

#ifdef __HAIKU__
#include <OS.h>
#endif

#if defined(KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE)
#include <execinfo.h>
#if defined(KWSYS_SYSTEMINFORMATION_HAS_CPP_DEMANGLE)
#include <cxxabi.h>
#endif
#if defined(KWSYS_SYSTEMINFORMATION_HAS_SYMBOL_LOOKUP)
#include <dlfcn.h>
#endif
#else
#undef KWSYS_SYSTEMINFORMATION_HAS_CPP_DEMANGLE
#undef KWSYS_SYSTEMINFORMATION_HAS_SYMBOL_LOOKUP
#endif

#include <ctype.h> // int isdigit(int c);
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(KWSYS_USE_LONG_LONG)
#if defined(KWSYS_IOS_HAS_OSTREAM_LONG_LONG)
#define iostreamLongLong(x) (x)
#else
#define iostreamLongLong(x) ((long)(x))
#endif
#elif defined(KWSYS_USE___INT64)
#if defined(KWSYS_IOS_HAS_OSTREAM___INT64)
#define iostreamLongLong(x) (x)
#else
#define iostreamLongLong(x) ((long)(x))
#endif
#else
#error "No Long Long"
#endif

#if defined(KWSYS_CXX_HAS_ATOLL)
#define atoLongLong atoll
#else
#if defined(KWSYS_CXX_HAS__ATOI64)
#define atoLongLong _atoi64
#elif defined(KWSYS_CXX_HAS_ATOL)
#define atoLongLong atol
#else
#define atoLongLong atoi
#endif
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && !defined(_WIN64) &&            \
  !defined(__clang__)
#define USE_ASM_INSTRUCTIONS 1
#else
#define USE_ASM_INSTRUCTIONS 0
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1400) && !defined(__clang__)
#include <intrin.h>
#define USE_CPUID_INTRINSICS 1
#else
#define USE_CPUID_INTRINSICS 0
#endif

#if USE_ASM_INSTRUCTIONS || USE_CPUID_INTRINSICS ||                           \
  defined(KWSYS_CXX_HAS_BORLAND_ASM_CPUID)
#define USE_CPUID 1
#else
#define USE_CPUID 0
#endif

#if USE_CPUID

#define CPUID_AWARE_COMPILER

/**
 * call CPUID instruction
 *
 * Will return false if the instruction failed.
 */
static bool call_cpuid(int select, int result[4])
{
#if USE_CPUID_INTRINSICS
  __cpuid(result, select);
  return true;
#else
  int tmp[4];
#if defined(_MSC_VER)
  // Use SEH to determine CPUID presence
  __try {
    _asm {
#ifdef CPUID_AWARE_COMPILER
      ; we must push/pop the registers <<CPUID>> writes to, as the
      ; optimiser does not know about <<CPUID>>, and so does not expect
      ; these registers to change.
      push eax
      push ebx
      push ecx
      push edx
#endif
      ; <<CPUID>>
      mov eax, select
#ifdef CPUID_AWARE_COMPILER
      cpuid
#else
      _asm _emit 0x0f
      _asm _emit 0xa2
#endif
      mov tmp[0 * TYPE int], eax
      mov tmp[1 * TYPE int], ebx
      mov tmp[2 * TYPE int], ecx
      mov tmp[3 * TYPE int], edx

#ifdef CPUID_AWARE_COMPILER
      pop edx
      pop ecx
      pop ebx
      pop eax
#endif
    }
  } __except (1) {
    return false;
  }

  memcpy(result, tmp, sizeof(tmp));
#elif defined(KWSYS_CXX_HAS_BORLAND_ASM_CPUID)
  unsigned int a, b, c, d;
  __asm {
    mov EAX, select;
    cpuid
    mov a, EAX;
    mov b, EBX;
    mov c, ECX;
    mov d, EDX;
  }

  result[0] = a;
  result[1] = b;
  result[2] = c;
  result[3] = d;
#endif

  // The cpuid instruction succeeded.
  return true;
#endif
}
#endif

namespace KWSYS_NAMESPACE {
template <typename T>
T min(T a, T b)
{
  return a < b ? a : b;
}

extern "C" {
typedef void (*SigAction)(int, siginfo_t*, void*);
}

//  Define SystemInformationImplementation class
typedef void (*DELAY_FUNC)(unsigned int uiMS);

class SystemInformationImplementation
{
public:
  typedef SystemInformation::LongLong LongLong;
  SystemInformationImplementation();
  ~SystemInformationImplementation();

  const char* GetVendorString();
  const char* GetVendorID();
  std::string GetTypeID();
  std::string GetFamilyID();
  std::string GetModelID();
  std::string GetModelName();
  std::string GetSteppingCode();
  const char* GetExtendedProcessorName();
  const char* GetProcessorSerialNumber();
  int GetProcessorCacheSize();
  unsigned int GetLogicalProcessorsPerPhysical();
  float GetProcessorClockFrequency();
  int GetProcessorAPICID();
  int GetProcessorCacheXSize(long int);
  bool DoesCPUSupportFeature(long int);

  const char* GetOSName();
  const char* GetHostname();
  int GetFullyQualifiedDomainName(std::string& fqdn);
  const char* GetOSRelease();
  const char* GetOSVersion();
  const char* GetOSPlatform();

  bool Is64Bits();

  unsigned int GetNumberOfLogicalCPU(); // per physical cpu
  unsigned int GetNumberOfPhysicalCPU();

  bool DoesCPUSupportCPUID();

  // Retrieve memory information in megabyte.
  size_t GetTotalVirtualMemory();
  size_t GetAvailableVirtualMemory();
  size_t GetTotalPhysicalMemory();
  size_t GetAvailablePhysicalMemory();

  LongLong GetProcessId();

  // Retrieve memory information in kib
  LongLong GetHostMemoryTotal();
  LongLong GetHostMemoryAvailable(const char* envVarName);
  LongLong GetHostMemoryUsed();

  LongLong GetProcMemoryAvailable(const char* hostLimitEnvVarName,
                                  const char* procLimitEnvVarName);
  LongLong GetProcMemoryUsed();

  double GetLoadAverage();

  // enable/disable stack trace signal handler.
  static void SetStackTraceOnError(int enable);

  // get current stack
  static std::string GetProgramStack(int firstFrame, int wholePath);

  /** Run the different checks */
  void RunCPUCheck();
  void RunOSCheck();
  void RunMemoryCheck();

public:
  typedef struct tagID
  {
    int Type;
    int Family;
    int Model;
    int Revision;
    int ExtendedFamily;
    int ExtendedModel;
    std::string ProcessorName;
    std::string Vendor;
    std::string SerialNumber;
    std::string ModelName;
  } ID;

  typedef struct tagCPUPowerManagement
  {
    bool HasVoltageID;
    bool HasFrequencyID;
    bool HasTempSenseDiode;
  } CPUPowerManagement;

  typedef struct tagCPUExtendedFeatures
  {
    bool Has3DNow;
    bool Has3DNowPlus;
    bool SupportsMP;
    bool HasMMXPlus;
    bool HasSSEMMX;
    unsigned int LogicalProcessorsPerPhysical;
    int APIC_ID;
    CPUPowerManagement PowerManagement;
  } CPUExtendedFeatures;

  typedef struct CPUtagFeatures
  {
    bool HasFPU;
    bool HasTSC;
    bool HasMMX;
    bool HasSSE;
    bool HasSSEFP;
    bool HasSSE2;
    bool HasIA64;
    bool HasAPIC;
    bool HasCMOV;
    bool HasMTRR;
    bool HasACPI;
    bool HasSerial;
    bool HasThermal;
    int CPUSpeed;
    int L1CacheSize;
    int L2CacheSize;
    int L3CacheSize;
    CPUExtendedFeatures ExtendedFeatures;
  } CPUFeatures;

  enum Manufacturer
  {
    AMD,
    Intel,
    NSC,
    UMC,
    Cyrix,
    NexGen,
    IDT,
    Rise,
    Transmeta,
    Sun,
    IBM,
    Motorola,
    HP,
    UnknownManufacturer
  };

protected:
  // For windows
  bool RetrieveCPUFeatures();
  bool RetrieveCPUIdentity();
  bool RetrieveCPUCacheDetails();
  bool RetrieveClassicalCPUCacheDetails();
  bool RetrieveCPUClockSpeed();
  bool RetrieveClassicalCPUClockSpeed();
  bool RetrieveCPUExtendedLevelSupport(int);
  bool RetrieveExtendedCPUFeatures();
  bool RetrieveProcessorSerialNumber();
  bool RetrieveCPUPowerManagement();
  bool RetrieveClassicalCPUIdentity();
  bool RetrieveExtendedCPUIdentity();

  // Processor information
  Manufacturer ChipManufacturer;
  CPUFeatures Features;
  ID ChipID;
  float CPUSpeedInMHz;
  unsigned int NumberOfLogicalCPU;
  unsigned int NumberOfPhysicalCPU;

  void CPUCountWindows();    // For windows
  unsigned char GetAPICId(); // For windows
  bool IsSMTSupported();
  static LongLong GetCyclesDifference(DELAY_FUNC, unsigned int); // For windows

  // For Linux and Cygwin, /proc/cpuinfo formats are slightly different
  bool RetreiveInformationFromCpuInfoFile();
  std::string ExtractValueFromCpuInfoFile(std::string buffer, const char* word,
                                          size_t init = 0);

  bool QueryLinuxMemory();
  bool QueryCygwinMemory();

  static void Delay(unsigned int);
  static void DelayOverhead(unsigned int);

  void FindManufacturer(const std::string& family = "");

  // For Mac
  bool ParseSysCtl();
  int CallSwVers(const char* arg, std::string& ver);
  void TrimNewline(std::string&);
  std::string ExtractValueFromSysCtl(const char* word);
  std::string SysCtlBuffer;

  // For Solaris
  bool QuerySolarisMemory();
  bool QuerySolarisProcessor();
  std::string ParseValueFromKStat(const char* arguments);
  std::string RunProcess(std::vector<const char*> args);

  // For Haiku OS
  bool QueryHaikuInfo();

  // For QNX
  bool QueryQNXMemory();
  bool QueryQNXProcessor();

  // For OpenBSD, FreeBSD, NetBSD, DragonFly
  bool QueryBSDMemory();
  bool QueryBSDProcessor();

  // For HP-UX
  bool QueryHPUXMemory();
  bool QueryHPUXProcessor();

  // For Microsoft Windows
  bool QueryWindowsMemory();

  // For AIX
  bool QueryAIXMemory();

  bool QueryProcessorBySysconf();
  bool QueryProcessor();

  // Evaluate the memory information.
  bool QueryMemoryBySysconf();
  bool QueryMemory();
  size_t TotalVirtualMemory;
  size_t AvailableVirtualMemory;
  size_t TotalPhysicalMemory;
  size_t AvailablePhysicalMemory;

  size_t CurrentPositionInFile;

  // Operating System information
  bool QueryOSInformation();
  std::string OSName;
  std::string Hostname;
  std::string OSRelease;
  std::string OSVersion;
  std::string OSPlatform;
  bool OSIs64Bit;
};

SystemInformation::SystemInformation()
{
  this->Implementation = new SystemInformationImplementation;
}

SystemInformation::~SystemInformation()
{
  delete this->Implementation;
}

const char* SystemInformation::GetVendorString()
{
  return this->Implementation->GetVendorString();
}

const char* SystemInformation::GetVendorID()
{
  return this->Implementation->GetVendorID();
}

std::string SystemInformation::GetTypeID()
{
  return this->Implementation->GetTypeID();
}

std::string SystemInformation::GetFamilyID()
{
  return this->Implementation->GetFamilyID();
}

std::string SystemInformation::GetModelID()
{
  return this->Implementation->GetModelID();
}

std::string SystemInformation::GetModelName()
{
  return this->Implementation->GetModelName();
}

std::string SystemInformation::GetSteppingCode()
{
  return this->Implementation->GetSteppingCode();
}

const char* SystemInformation::GetExtendedProcessorName()
{
  return this->Implementation->GetExtendedProcessorName();
}

const char* SystemInformation::GetProcessorSerialNumber()
{
  return this->Implementation->GetProcessorSerialNumber();
}

int SystemInformation::GetProcessorCacheSize()
{
  return this->Implementation->GetProcessorCacheSize();
}

unsigned int SystemInformation::GetLogicalProcessorsPerPhysical()
{
  return this->Implementation->GetLogicalProcessorsPerPhysical();
}

float SystemInformation::GetProcessorClockFrequency()
{
  return this->Implementation->GetProcessorClockFrequency();
}

int SystemInformation::GetProcessorAPICID()
{
  return this->Implementation->GetProcessorAPICID();
}

int SystemInformation::GetProcessorCacheXSize(long int l)
{
  return this->Implementation->GetProcessorCacheXSize(l);
}

bool SystemInformation::DoesCPUSupportFeature(long int i)
{
  return this->Implementation->DoesCPUSupportFeature(i);
}

std::string SystemInformation::GetCPUDescription()
{
  std::ostringstream oss;
  oss << this->GetNumberOfPhysicalCPU() << " core ";
  if (this->GetModelName().empty()) {
    oss << this->GetProcessorClockFrequency() << " MHz "
        << this->GetVendorString() << " " << this->GetExtendedProcessorName();
  } else {
    oss << this->GetModelName();
  }

  // remove extra spaces
  std::string tmp = oss.str();
  size_t pos;
  while ((pos = tmp.find("  ")) != std::string::npos) {
    tmp.replace(pos, 2, " ");
  }

  return tmp;
}

const char* SystemInformation::GetOSName()
{
  return this->Implementation->GetOSName();
}

const char* SystemInformation::GetHostname()
{
  return this->Implementation->GetHostname();
}

std::string SystemInformation::GetFullyQualifiedDomainName()
{
  std::string fqdn;
  this->Implementation->GetFullyQualifiedDomainName(fqdn);
  return fqdn;
}

const char* SystemInformation::GetOSRelease()
{
  return this->Implementation->GetOSRelease();
}

const char* SystemInformation::GetOSVersion()
{
  return this->Implementation->GetOSVersion();
}

const char* SystemInformation::GetOSPlatform()
{
  return this->Implementation->GetOSPlatform();
}

int SystemInformation::GetOSIsWindows()
{
#if defined(_WIN32)
  return 1;
#else
  return 0;
#endif
}

int SystemInformation::GetOSIsLinux()
{
#if defined(__linux)
  return 1;
#else
  return 0;
#endif
}

int SystemInformation::GetOSIsApple()
{
#if defined(__APPLE__)
  return 1;
#else
  return 0;
#endif
}

std::string SystemInformation::GetOSDescription()
{
  std::ostringstream oss;
  oss << this->GetOSName() << " " << this->GetOSRelease() << " "
      << this->GetOSVersion();

  return oss.str();
}

bool SystemInformation::Is64Bits()
{
  return this->Implementation->Is64Bits();
}

unsigned int SystemInformation::GetNumberOfLogicalCPU() // per physical cpu
{
  return this->Implementation->GetNumberOfLogicalCPU();
}

unsigned int SystemInformation::GetNumberOfPhysicalCPU()
{
  return this->Implementation->GetNumberOfPhysicalCPU();
}

bool SystemInformation::DoesCPUSupportCPUID()
{
  return this->Implementation->DoesCPUSupportCPUID();
}

// Retrieve memory information in megabyte.
size_t SystemInformation::GetTotalVirtualMemory()
{
  return this->Implementation->GetTotalVirtualMemory();
}

size_t SystemInformation::GetAvailableVirtualMemory()
{
  return this->Implementation->GetAvailableVirtualMemory();
}

size_t SystemInformation::GetTotalPhysicalMemory()
{
  return this->Implementation->GetTotalPhysicalMemory();
}

size_t SystemInformation::GetAvailablePhysicalMemory()
{
  return this->Implementation->GetAvailablePhysicalMemory();
}

std::string SystemInformation::GetMemoryDescription(
  const char* hostLimitEnvVarName, const char* procLimitEnvVarName)
{
  std::ostringstream oss;
  oss << "Host Total: " << iostreamLongLong(this->GetHostMemoryTotal())
      << " KiB, Host Available: "
      << iostreamLongLong(this->GetHostMemoryAvailable(hostLimitEnvVarName))
      << " KiB, Process Available: "
      << iostreamLongLong(this->GetProcMemoryAvailable(hostLimitEnvVarName,
                                                       procLimitEnvVarName))
      << " KiB";
  return oss.str();
}

// host memory info in units of KiB.
SystemInformation::LongLong SystemInformation::GetHostMemoryTotal()
{
  return this->Implementation->GetHostMemoryTotal();
}

SystemInformation::LongLong SystemInformation::GetHostMemoryAvailable(
  const char* hostLimitEnvVarName)
{
  return this->Implementation->GetHostMemoryAvailable(hostLimitEnvVarName);
}

SystemInformation::LongLong SystemInformation::GetHostMemoryUsed()
{
  return this->Implementation->GetHostMemoryUsed();
}

// process memory info in units of KiB.
SystemInformation::LongLong SystemInformation::GetProcMemoryAvailable(
  const char* hostLimitEnvVarName, const char* procLimitEnvVarName)
{
  return this->Implementation->GetProcMemoryAvailable(hostLimitEnvVarName,
                                                      procLimitEnvVarName);
}

SystemInformation::LongLong SystemInformation::GetProcMemoryUsed()
{
  return this->Implementation->GetProcMemoryUsed();
}

double SystemInformation::GetLoadAverage()
{
  return this->Implementation->GetLoadAverage();
}

SystemInformation::LongLong SystemInformation::GetProcessId()
{
  return this->Implementation->GetProcessId();
}

void SystemInformation::SetStackTraceOnError(int enable)
{
  SystemInformationImplementation::SetStackTraceOnError(enable);
}

std::string SystemInformation::GetProgramStack(int firstFrame, int wholePath)
{
  return SystemInformationImplementation::GetProgramStack(firstFrame,
                                                          wholePath);
}

/** Run the different checks */
void SystemInformation::RunCPUCheck()
{
  this->Implementation->RunCPUCheck();
}

void SystemInformation::RunOSCheck()
{
  this->Implementation->RunOSCheck();
}

void SystemInformation::RunMemoryCheck()
{
  this->Implementation->RunMemoryCheck();
}

// SystemInformationImplementation starts here

#define STORE_TLBCACHE_INFO(x, y) x = (x < (y)) ? (y) : x
#define TLBCACHE_INFO_UNITS (15)
#define CLASSICAL_CPU_FREQ_LOOP 10000000
#define RDTSC_INSTRUCTION _asm _emit 0x0f _asm _emit 0x31

// Status Flag
#define HT_NOT_CAPABLE 0
#define HT_ENABLED 1
#define HT_DISABLED 2
#define HT_SUPPORTED_NOT_ENABLED 3
#define HT_CANNOT_DETECT 4

// EDX[28]  Bit 28 is set if HT is supported
#define HT_BIT 0x10000000

// EAX[11:8] Bit 8-11 contains family processor ID.
#define FAMILY_ID 0x0F00
#define PENTIUM4_ID 0x0F00
// EAX[23:20] Bit 20-23 contains extended family processor ID
#define EXT_FAMILY_ID 0x0F00000
// EBX[23:16] Bit 16-23 in ebx contains the number of logical
#define NUM_LOGICAL_BITS 0x00FF0000
// processors per physical processor when execute cpuid with
// eax set to 1
// EBX[31:24] Bits 24-31 (8 bits) return the 8-bit unique
#define INITIAL_APIC_ID_BITS 0xFF000000
// initial APIC ID for the processor this code is running on.
// Default value = 0xff if HT is not supported

// Hide implementation details in an anonymous namespace.
namespace {
// *****************************************************************************
#if defined(__linux) || defined(__APPLE__)
int LoadLines(FILE* file, std::vector<std::string>& lines)
{
  // Load each line in the given file into a the vector.
  int nRead = 0;
  const int bufSize = 1024;
  char buf[bufSize] = { '\0' };
  while (!feof(file) && !ferror(file)) {
    errno = 0;
    if (fgets(buf, bufSize, file) == 0) {
      if (ferror(file) && (errno == EINTR)) {
        clearerr(file);
      }
      continue;
    }
    char* pBuf = buf;
    while (*pBuf) {
      if (*pBuf == '\n')
        *pBuf = '\0';
      pBuf += 1;
    }
    lines.push_back(buf);
    ++nRead;
  }
  if (ferror(file)) {
    return 0;
  }
  return nRead;
}

#if defined(__linux)
// *****************************************************************************
int LoadLines(const char* fileName, std::vector<std::string>& lines)
{
  FILE* file = fopen(fileName, "r");
  if (file == 0) {
    return 0;
  }
  int nRead = LoadLines(file, lines);
  fclose(file);
  return nRead;
}
#endif

// ****************************************************************************
template <typename T>
int NameValue(std::vector<std::string> const& lines, std::string const& name,
              T& value)
{
  size_t nLines = lines.size();
  for (size_t i = 0; i < nLines; ++i) {
    size_t at = lines[i].find(name);
    if (at == std::string::npos) {
      continue;
    }
    std::istringstream is(lines[i].substr(at + name.size()));
    is >> value;
    return 0;
  }
  return -1;
}
#endif

#if defined(__linux)
// ****************************************************************************
template <typename T>
int GetFieldsFromFile(const char* fileName, const char** fieldNames, T* values)
{
  std::vector<std::string> fields;
  if (!LoadLines(fileName, fields)) {
    return -1;
  }
  int i = 0;
  while (fieldNames[i] != NULL) {
    int ierr = NameValue(fields, fieldNames[i], values[i]);
    if (ierr) {
      return -(i + 2);
    }
    i += 1;
  }
  return 0;
}

// ****************************************************************************
template <typename T>
int GetFieldFromFile(const char* fileName, const char* fieldName, T& value)
{
  const char* fieldNames[2] = { fieldName, NULL };
  T values[1] = { T(0) };
  int ierr = GetFieldsFromFile(fileName, fieldNames, values);
  if (ierr) {
    return ierr;
  }
  value = values[0];
  return 0;
}
#endif

// ****************************************************************************
#if defined(__APPLE__)
template <typename T>
int GetFieldsFromCommand(const char* command, const char** fieldNames,
                         T* values)
{
  FILE* file = popen(command, "r");
  if (file == 0) {
    return -1;
  }
  std::vector<std::string> fields;
  int nl = LoadLines(file, fields);
  pclose(file);
  if (nl == 0) {
    return -1;
  }
  int i = 0;
  while (fieldNames[i] != NULL) {
    int ierr = NameValue(fields, fieldNames[i], values[i]);
    if (ierr) {
      return -(i + 2);
    }
    i += 1;
  }
  return 0;
}
#endif

// ****************************************************************************
#if !defined(_WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
void StacktraceSignalHandler(int sigNo, siginfo_t* sigInfo,
                             void* /*sigContext*/)
{
#if defined(__linux) || defined(__APPLE__)
  std::ostringstream oss;
  oss << std::endl
      << "========================================================="
      << std::endl
      << "Process id " << getpid() << " ";
  switch (sigNo) {
    case SIGINT:
      oss << "Caught SIGINT";
      break;

    case SIGTERM:
      oss << "Caught SIGTERM";
      break;

    case SIGABRT:
      oss << "Caught SIGABRT";
      break;

    case SIGFPE:
      oss << "Caught SIGFPE at " << (sigInfo->si_addr == 0 ? "0x" : "")
          << sigInfo->si_addr << " ";
      switch (sigInfo->si_code) {
#if defined(FPE_INTDIV)
        case FPE_INTDIV:
          oss << "integer division by zero";
          break;
#endif

#if defined(FPE_INTOVF)
        case FPE_INTOVF:
          oss << "integer overflow";
          break;
#endif

        case FPE_FLTDIV:
          oss << "floating point divide by zero";
          break;

        case FPE_FLTOVF:
          oss << "floating point overflow";
          break;

        case FPE_FLTUND:
          oss << "floating point underflow";
          break;

        case FPE_FLTRES:
          oss << "floating point inexact result";
          break;

        case FPE_FLTINV:
          oss << "floating point invalid operation";
          break;

#if defined(FPE_FLTSUB)
        case FPE_FLTSUB:
          oss << "floating point subscript out of range";
          break;
#endif

        default:
          oss << "code " << sigInfo->si_code;
          break;
      }
      break;

    case SIGSEGV:
      oss << "Caught SIGSEGV at " << (sigInfo->si_addr == 0 ? "0x" : "")
          << sigInfo->si_addr << " ";
      switch (sigInfo->si_code) {
        case SEGV_MAPERR:
          oss << "address not mapped to object";
          break;

        case SEGV_ACCERR:
          oss << "invalid permission for mapped object";
          break;

        default:
          oss << "code " << sigInfo->si_code;
          break;
      }
      break;

    case SIGBUS:
      oss << "Caught SIGBUS at " << (sigInfo->si_addr == 0 ? "0x" : "")
          << sigInfo->si_addr << " ";
      switch (sigInfo->si_code) {
        case BUS_ADRALN:
          oss << "invalid address alignment";
          break;

#if defined(BUS_ADRERR)
        case BUS_ADRERR:
          oss << "nonexistent physical address";
          break;
#endif

#if defined(BUS_OBJERR)
        case BUS_OBJERR:
          oss << "object-specific hardware error";
          break;
#endif

#if defined(BUS_MCEERR_AR)
        case BUS_MCEERR_AR:
          oss << "Hardware memory error consumed on a machine check; action "
                 "required.";
          break;
#endif

#if defined(BUS_MCEERR_AO)
        case BUS_MCEERR_AO:
          oss << "Hardware memory error detected in process but not consumed; "
                 "action optional.";
          break;
#endif

        default:
          oss << "code " << sigInfo->si_code;
          break;
      }
      break;

    case SIGILL:
      oss << "Caught SIGILL at " << (sigInfo->si_addr == 0 ? "0x" : "")
          << sigInfo->si_addr << " ";
      switch (sigInfo->si_code) {
        case ILL_ILLOPC:
          oss << "illegal opcode";
          break;

#if defined(ILL_ILLOPN)
        case ILL_ILLOPN:
          oss << "illegal operand";
          break;
#endif

#if defined(ILL_ILLADR)
        case ILL_ILLADR:
          oss << "illegal addressing mode.";
          break;
#endif

        case ILL_ILLTRP:
          oss << "illegal trap";
          break;

        case ILL_PRVOPC:
          oss << "privileged opcode";
          break;

#if defined(ILL_PRVREG)
        case ILL_PRVREG:
          oss << "privileged register";
          break;
#endif

#if defined(ILL_COPROC)
        case ILL_COPROC:
          oss << "co-processor error";
          break;
#endif

#if defined(ILL_BADSTK)
        case ILL_BADSTK:
          oss << "internal stack error";
          break;
#endif

        default:
          oss << "code " << sigInfo->si_code;
          break;
      }
      break;

    default:
      oss << "Caught " << sigNo << " code " << sigInfo->si_code;
      break;
  }
  oss << std::endl
      << "Program Stack:" << std::endl
      << SystemInformationImplementation::GetProgramStack(2, 0)
      << "========================================================="
      << std::endl;
  std::cerr << oss.str() << std::endl;

  // restore the previously registered handlers
  // and abort
  SystemInformationImplementation::SetStackTraceOnError(0);
  abort();
#else
  // avoid warning C4100
  (void)sigNo;
  (void)sigInfo;
#endif
}
#endif

#if defined(KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE)
#define safes(_arg) ((_arg) ? (_arg) : "???")

// Description:
// A container for symbol properties. Each instance
// must be Initialized.
class SymbolProperties
{
public:
  SymbolProperties();

  // Description:
  // The SymbolProperties instance must be initialized by
  // passing a stack address.
  void Initialize(void* address);

  // Description:
  // Get the symbol's stack address.
  void* GetAddress() const { return this->Address; }

  // Description:
  // If not set paths will be removed. eg, from a binary
  // or source file.
  void SetReportPath(int rp) { this->ReportPath = rp; }

  // Description:
  // Set/Get the name of the binary file that the symbol
  // is found in.
  void SetBinary(const char* binary) { this->Binary = safes(binary); }

  std::string GetBinary() const;

  // Description:
  // Set the name of the function that the symbol is found in.
  // If c++ demangling is supported it will be demangled.
  void SetFunction(const char* function)
  {
    this->Function = this->Demangle(function);
  }

  std::string GetFunction() const { return this->Function; }

  // Description:
  // Set/Get the name of the source file where the symbol
  // is defined.
  void SetSourceFile(const char* sourcefile)
  {
    this->SourceFile = safes(sourcefile);
  }

  std::string GetSourceFile() const
  {
    return this->GetFileName(this->SourceFile);
  }

  // Description:
  // Set/Get the line number where the symbol is defined
  void SetLineNumber(long linenumber) { this->LineNumber = linenumber; }
  long GetLineNumber() const { return this->LineNumber; }

  // Description:
  // Set the address where the biinary image is mapped
  // into memory.
  void SetBinaryBaseAddress(void* address)
  {
    this->BinaryBaseAddress = address;
  }

private:
  void* GetRealAddress() const
  {
    return (void*)((char*)this->Address - (char*)this->BinaryBaseAddress);
  }

  std::string GetFileName(const std::string& path) const;
  std::string Demangle(const char* symbol) const;

private:
  std::string Binary;
  void* BinaryBaseAddress;
  void* Address;
  std::string SourceFile;
  std::string Function;
  long LineNumber;
  int ReportPath;
};

std::ostream& operator<<(std::ostream& os, const SymbolProperties& sp)
{
#if defined(KWSYS_SYSTEMINFORMATION_HAS_SYMBOL_LOOKUP)
  os << std::hex << sp.GetAddress() << " : " << sp.GetFunction() << " [("
     << sp.GetBinary() << ") " << sp.GetSourceFile() << ":" << std::dec
     << sp.GetLineNumber() << "]";
#elif defined(KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE)
  void* addr = sp.GetAddress();
  char** syminfo = backtrace_symbols(&addr, 1);
  os << safes(syminfo[0]);
  free(syminfo);
#else
  (void)os;
  (void)sp;
#endif
  return os;
}

SymbolProperties::SymbolProperties()
{
  // not using an initializer list
  // to avoid some PGI compiler warnings
  this->SetBinary("???");
  this->SetBinaryBaseAddress(NULL);
  this->Address = NULL;
  this->SetSourceFile("???");
  this->SetFunction("???");
  this->SetLineNumber(-1);
  this->SetReportPath(0);
  // avoid PGI compiler warnings
  this->GetRealAddress();
  this->GetFunction();
  this->GetSourceFile();
  this->GetLineNumber();
}

std::string SymbolProperties::GetFileName(const std::string& path) const
{
  std::string file(path);
  if (!this->ReportPath) {
    size_t at = file.rfind("/");
    if (at != std::string::npos) {
      file = file.substr(at + 1);
    }
  }
  return file;
}

std::string SymbolProperties::GetBinary() const
{
// only linux has proc fs
#if defined(__linux__)
  if (this->Binary == "/proc/self/exe") {
    std::string binary;
    char buf[1024] = { '\0' };
    ssize_t ll = 0;
    if ((ll = readlink("/proc/self/exe", buf, 1024)) > 0) {
      buf[ll] = '\0';
      binary = buf;
    } else {
      binary = "/proc/self/exe";
    }
    return this->GetFileName(binary);
  }
#endif
  return this->GetFileName(this->Binary);
}

std::string SymbolProperties::Demangle(const char* symbol) const
{
  std::string result = safes(symbol);
#if defined(KWSYS_SYSTEMINFORMATION_HAS_CPP_DEMANGLE)
  int status = 0;
  size_t bufferLen = 1024;
  char* buffer = (char*)malloc(1024);
  char* demangledSymbol =
    abi::__cxa_demangle(symbol, buffer, &bufferLen, &status);
  if (!status) {
    result = demangledSymbol;
  }
  free(buffer);
#else
  (void)symbol;
#endif
  return result;
}

void SymbolProperties::Initialize(void* address)
{
  this->Address = address;
#if defined(KWSYS_SYSTEMINFORMATION_HAS_SYMBOL_LOOKUP)
  // first fallback option can demangle c++ functions
  Dl_info info;
  int ierr = dladdr(this->Address, &info);
  if (ierr && info.dli_sname && info.dli_saddr) {
    this->SetBinary(info.dli_fname);
    this->SetFunction(info.dli_sname);
  }
#else
// second fallback use builtin backtrace_symbols
// to decode the bactrace.
#endif
}
#endif // don't define this class if we're not using it

#if defined(_WIN32) || defined(__CYGWIN__)
#define KWSYS_SYSTEMINFORMATION_USE_GetSystemTimes
#endif
#if defined(_MSC_VER) && _MSC_VER < 1310
#undef KWSYS_SYSTEMINFORMATION_USE_GetSystemTimes
#endif
#if defined(KWSYS_SYSTEMINFORMATION_USE_GetSystemTimes)
double calculateCPULoad(unsigned __int64 idleTicks,
                        unsigned __int64 totalTicks)
{
  static double previousLoad = -0.0;
  static unsigned __int64 previousIdleTicks = 0;
  static unsigned __int64 previousTotalTicks = 0;

  unsigned __int64 const idleTicksSinceLastTime =
    idleTicks - previousIdleTicks;
  unsigned __int64 const totalTicksSinceLastTime =
    totalTicks - previousTotalTicks;

  double load;
  if (previousTotalTicks == 0 || totalTicksSinceLastTime == 0) {
    // No new information.  Use previous result.
    load = previousLoad;
  } else {
    // Calculate load since last time.
    load = 1.0 - double(idleTicksSinceLastTime) / totalTicksSinceLastTime;

    // Smooth if possible.
    if (previousLoad > 0) {
      load = 0.25 * load + 0.75 * previousLoad;
    }
  }

  previousLoad = load;
  previousIdleTicks = idleTicks;
  previousTotalTicks = totalTicks;

  return load;
}

unsigned __int64 fileTimeToUInt64(FILETIME const& ft)
{
  LARGE_INTEGER out;
  out.HighPart = ft.dwHighDateTime;
  out.LowPart = ft.dwLowDateTime;
  return out.QuadPart;
}
#endif

} // anonymous namespace

SystemInformationImplementation::SystemInformationImplementation()
{
  this->TotalVirtualMemory = 0;
  this->AvailableVirtualMemory = 0;
  this->TotalPhysicalMemory = 0;
  this->AvailablePhysicalMemory = 0;
  this->CurrentPositionInFile = 0;
  this->ChipManufacturer = UnknownManufacturer;
  memset(&this->Features, 0, sizeof(CPUFeatures));
  this->ChipID.Type = 0;
  this->ChipID.Family = 0;
  this->ChipID.Model = 0;
  this->ChipID.Revision = 0;
  this->ChipID.ExtendedFamily = 0;
  this->ChipID.ExtendedModel = 0;
  this->CPUSpeedInMHz = 0;
  this->NumberOfLogicalCPU = 0;
  this->NumberOfPhysicalCPU = 0;
  this->OSName = "";
  this->Hostname = "";
  this->OSRelease = "";
  this->OSVersion = "";
  this->OSPlatform = "";
  this->OSIs64Bit = (sizeof(void*) == 8);
}

SystemInformationImplementation::~SystemInformationImplementation()
{
}

void SystemInformationImplementation::RunCPUCheck()
{
#ifdef _WIN32
  // Check to see if this processor supports CPUID.
  bool supportsCPUID = DoesCPUSupportCPUID();

  if (supportsCPUID) {
    // Retrieve the CPU details.
    RetrieveCPUIdentity();
    this->FindManufacturer();
    RetrieveCPUFeatures();
  }

  // These two may be called without support for the CPUID instruction.
  // (But if the instruction is there, they should be called *after*
  // the above call to RetrieveCPUIdentity... that's why the two if
  // blocks exist with the same "if (supportsCPUID)" logic...
  //
  if (!RetrieveCPUClockSpeed()) {
    RetrieveClassicalCPUClockSpeed();
  }

  if (supportsCPUID) {
    // Retrieve cache information.
    if (!RetrieveCPUCacheDetails()) {
      RetrieveClassicalCPUCacheDetails();
    }

    // Retrieve the extended CPU details.
    if (!RetrieveExtendedCPUIdentity()) {
      RetrieveClassicalCPUIdentity();
    }

    RetrieveExtendedCPUFeatures();
    RetrieveCPUPowerManagement();

    // Now attempt to retrieve the serial number (if possible).
    RetrieveProcessorSerialNumber();
  }

  this->CPUCountWindows();

#elif defined(__APPLE__)
  this->ParseSysCtl();
#elif defined(__SVR4) && defined(__sun)
  this->QuerySolarisProcessor();
#elif defined(__HAIKU__)
  this->QueryHaikuInfo();
#elif defined(__QNX__)
  this->QueryQNXProcessor();
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) ||  \
  defined(__DragonFly__)
  this->QueryBSDProcessor();
#elif defined(__hpux)
  this->QueryHPUXProcessor();
#elif defined(__linux) || defined(__CYGWIN__)
  this->RetreiveInformationFromCpuInfoFile();
#else
  this->QueryProcessor();
#endif
}

void SystemInformationImplementation::RunOSCheck()
{
  this->QueryOSInformation();
}

void SystemInformationImplementation::RunMemoryCheck()
{
#if defined(__APPLE__)
  this->ParseSysCtl();
#elif defined(__SVR4) && defined(__sun)
  this->QuerySolarisMemory();
#elif defined(__HAIKU__)
  this->QueryHaikuInfo();
#elif defined(__QNX__)
  this->QueryQNXMemory();
#elif defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) ||  \
  defined(__DragonFly__)
  this->QueryBSDMemory();
#elif defined(__CYGWIN__)
  this->QueryCygwinMemory();
#elif defined(_WIN32)
  this->QueryWindowsMemory();
#elif defined(__hpux)
  this->QueryHPUXMemory();
#elif defined(__linux)
  this->QueryLinuxMemory();
#elif defined(_AIX)
  this->QueryAIXMemory();
#else
  this->QueryMemory();
#endif
}

/** Get the vendor string */
const char* SystemInformationImplementation::GetVendorString()
{
  return this->ChipID.Vendor.c_str();
}

/** Get the OS Name */
const char* SystemInformationImplementation::GetOSName()
{
  return this->OSName.c_str();
}

/** Get the hostname */
const char* SystemInformationImplementation::GetHostname()
{
  if (this->Hostname.empty()) {
    this->Hostname = "localhost";
#if defined(_WIN32)
    WORD wVersionRequested;
    WSADATA wsaData;
    char name[255];
    wVersionRequested = MAKEWORD(2, 0);
    if (WSAStartup(wVersionRequested, &wsaData) == 0) {
      gethostname(name, sizeof(name));
      WSACleanup();
    }
    this->Hostname = name;
#else
    struct utsname unameInfo;
    int errorFlag = uname(&unameInfo);
    if (errorFlag == 0) {
      this->Hostname = unameInfo.nodename;
    }
#endif
  }
  return this->Hostname.c_str();
}

/** Get the FQDN */
int SystemInformationImplementation::GetFullyQualifiedDomainName(
  std::string& fqdn)
{
  // in the event of absolute failure return localhost.
  fqdn = "localhost";

#if defined(_WIN32)
  int ierr;
  // TODO - a more robust implementation for windows, see comments
  // in unix implementation.
  WSADATA wsaData;
  WORD ver = MAKEWORD(2, 0);
  ierr = WSAStartup(ver, &wsaData);
  if (ierr) {
    return -1;
  }

  char base[256] = { '\0' };
  ierr = gethostname(base, 256);
  if (ierr) {
    WSACleanup();
    return -2;
  }
  fqdn = base;

  HOSTENT* hent = gethostbyname(base);
  if (hent) {
    fqdn = hent->h_name;
  }

  WSACleanup();
  return 0;

#elif defined(KWSYS_SYSTEMINFORMATION_IMPLEMENT_FQDN)
  // gethostname typical returns an alias for loopback interface
  // we want the fully qualified domain name. Because there are
  // any number of interfaces on this system we look for the
  // first of these that contains the name returned by gethostname
  // and is longer. failing that we return gethostname and indicate
  // with a failure code. Return of a failure code is not necessarilly
  // an indication of an error. for instance gethostname may return
  // the fully qualified domain name, or there may not be one if the
  // system lives on a private network such as in the case of a cluster
  // node.

  int ierr = 0;
  char base[NI_MAXHOST];
  ierr = gethostname(base, NI_MAXHOST);
  if (ierr) {
    return -1;
  }
  size_t baseSize = strlen(base);
  fqdn = base;

  struct ifaddrs* ifas;
  struct ifaddrs* ifa;
  ierr = getifaddrs(&ifas);
  if (ierr) {
    return -2;
  }

  for (ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
    int fam = ifa->ifa_addr ? ifa->ifa_addr->sa_family : -1;
    // Skip Loopback interfaces
    if (((fam == AF_INET) || (fam == AF_INET6)) &&
        !(ifa->ifa_flags & IFF_LOOPBACK)) {
      char host[NI_MAXHOST] = { '\0' };

      const size_t addrlen = (fam == AF_INET ? sizeof(struct sockaddr_in)
                                             : sizeof(struct sockaddr_in6));

      ierr = getnameinfo(ifa->ifa_addr, static_cast<socklen_t>(addrlen), host,
                         NI_MAXHOST, NULL, 0, NI_NAMEREQD);
      if (ierr) {
        // don't report the failure now since we may succeed on another
        // interface. If all attempts fail then return the failure code.
        ierr = -3;
        continue;
      }

      std::string candidate = host;
      if ((candidate.find(base) != std::string::npos) &&
          baseSize < candidate.size()) {
        // success, stop now.
        ierr = 0;
        fqdn = candidate;
        break;
      }
    }
  }
  freeifaddrs(ifas);

  return ierr;
#else
  /* TODO: Implement on more platforms.  */
  fqdn = this->GetHostname();
  return -1;
#endif
}

/** Get the OS release */
const char* SystemInformationImplementation::GetOSRelease()
{
  return this->OSRelease.c_str();
}

/** Get the OS version */
const char* SystemInformationImplementation::GetOSVersion()
{
  return this->OSVersion.c_str();
}

/** Get the OS platform */
const char* SystemInformationImplementation::GetOSPlatform()
{
  return this->OSPlatform.c_str();
}

/** Get the vendor ID */
const char* SystemInformationImplementation::GetVendorID()
{
  // Return the vendor ID.
  switch (this->ChipManufacturer) {
    case Intel:
      return "Intel Corporation";
    case AMD:
      return "Advanced Micro Devices";
    case NSC:
      return "National Semiconductor";
    case Cyrix:
      return "Cyrix Corp., VIA Inc.";
    case NexGen:
      return "NexGen Inc., Advanced Micro Devices";
    case IDT:
      return "IDT\\Centaur, Via Inc.";
    case UMC:
      return "United Microelectronics Corp.";
    case Rise:
      return "Rise";
    case Transmeta:
      return "Transmeta";
    case Sun:
      return "Sun Microelectronics";
    case IBM:
      return "IBM";
    case Motorola:
      return "Motorola";
    case HP:
      return "Hewlett-Packard";
    case UnknownManufacturer:
    default:
      return "Unknown Manufacturer";
  }
}

/** Return the type ID of the CPU */
std::string SystemInformationImplementation::GetTypeID()
{
  std::ostringstream str;
  str << this->ChipID.Type;
  return str.str();
}

/** Return the family of the CPU present */
std::string SystemInformationImplementation::GetFamilyID()
{
  std::ostringstream str;
  str << this->ChipID.Family;
  return str.str();
}

// Return the model of CPU present */
std::string SystemInformationImplementation::GetModelID()
{
  std::ostringstream str;
  str << this->ChipID.Model;
  return str.str();
}

// Return the model name of CPU present */
std::string SystemInformationImplementation::GetModelName()
{
  return this->ChipID.ModelName;
}

/** Return the stepping code of the CPU present. */
std::string SystemInformationImplementation::GetSteppingCode()
{
  std::ostringstream str;
  str << this->ChipID.Revision;
  return str.str();
}

/** Return the stepping code of the CPU present. */
const char* SystemInformationImplementation::GetExtendedProcessorName()
{
  return this->ChipID.ProcessorName.c_str();
}

/** Return the serial number of the processor
 *  in hexadecimal: xxxx-xxxx-xxxx-xxxx-xxxx-xxxx. */
const char* SystemInformationImplementation::GetProcessorSerialNumber()
{
  return this->ChipID.SerialNumber.c_str();
}

/** Return the logical processors per physical */
unsigned int SystemInformationImplementation::GetLogicalProcessorsPerPhysical()
{
  return this->Features.ExtendedFeatures.LogicalProcessorsPerPhysical;
}

/** Return the processor clock frequency. */
float SystemInformationImplementation::GetProcessorClockFrequency()
{
  return this->CPUSpeedInMHz;
}

/**  Return the APIC ID. */
int SystemInformationImplementation::GetProcessorAPICID()
{
  return this->Features.ExtendedFeatures.APIC_ID;
}

/** Return the L1 cache size. */
int SystemInformationImplementation::GetProcessorCacheSize()
{
  return this->Features.L1CacheSize;
}

/** Return the chosen cache size. */
int SystemInformationImplementation::GetProcessorCacheXSize(long int dwCacheID)
{
  switch (dwCacheID) {
    case SystemInformation::CPU_FEATURE_L1CACHE:
      return this->Features.L1CacheSize;
    case SystemInformation::CPU_FEATURE_L2CACHE:
      return this->Features.L2CacheSize;
    case SystemInformation::CPU_FEATURE_L3CACHE:
      return this->Features.L3CacheSize;
  }
  return -1;
}

bool SystemInformationImplementation::DoesCPUSupportFeature(long int dwFeature)
{
  bool bHasFeature = false;

  // Check for MMX instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_MMX) != 0) &&
      this->Features.HasMMX)
    bHasFeature = true;

  // Check for MMX+ instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_MMX_PLUS) != 0) &&
      this->Features.ExtendedFeatures.HasMMXPlus)
    bHasFeature = true;

  // Check for SSE FP instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_SSE) != 0) &&
      this->Features.HasSSE)
    bHasFeature = true;

  // Check for SSE FP instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_SSE_FP) != 0) &&
      this->Features.HasSSEFP)
    bHasFeature = true;

  // Check for SSE MMX instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_SSE_MMX) != 0) &&
      this->Features.ExtendedFeatures.HasSSEMMX)
    bHasFeature = true;

  // Check for SSE2 instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_SSE2) != 0) &&
      this->Features.HasSSE2)
    bHasFeature = true;

  // Check for 3DNow! instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_AMD_3DNOW) != 0) &&
      this->Features.ExtendedFeatures.Has3DNow)
    bHasFeature = true;

  // Check for 3DNow+ instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_AMD_3DNOW_PLUS) != 0) &&
      this->Features.ExtendedFeatures.Has3DNowPlus)
    bHasFeature = true;

  // Check for IA64 instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_IA64) != 0) &&
      this->Features.HasIA64)
    bHasFeature = true;

  // Check for MP capable.
  if (((dwFeature & SystemInformation::CPU_FEATURE_MP_CAPABLE) != 0) &&
      this->Features.ExtendedFeatures.SupportsMP)
    bHasFeature = true;

  // Check for a serial number for the processor.
  if (((dwFeature & SystemInformation::CPU_FEATURE_SERIALNUMBER) != 0) &&
      this->Features.HasSerial)
    bHasFeature = true;

  // Check for a local APIC in the processor.
  if (((dwFeature & SystemInformation::CPU_FEATURE_APIC) != 0) &&
      this->Features.HasAPIC)
    bHasFeature = true;

  // Check for CMOV instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_CMOV) != 0) &&
      this->Features.HasCMOV)
    bHasFeature = true;

  // Check for MTRR instructions.
  if (((dwFeature & SystemInformation::CPU_FEATURE_MTRR) != 0) &&
      this->Features.HasMTRR)
    bHasFeature = true;

  // Check for L1 cache size.
  if (((dwFeature & SystemInformation::CPU_FEATURE_L1CACHE) != 0) &&
      (this->Features.L1CacheSize != -1))
    bHasFeature = true;

  // Check for L2 cache size.
  if (((dwFeature & SystemInformation::CPU_FEATURE_L2CACHE) != 0) &&
      (this->Features.L2CacheSize != -1))
    bHasFeature = true;

  // Check for L3 cache size.
  if (((dwFeature & SystemInformation::CPU_FEATURE_L3CACHE) != 0) &&
      (this->Features.L3CacheSize != -1))
    bHasFeature = true;

  // Check for ACPI capability.
  if (((dwFeature & SystemInformation::CPU_FEATURE_ACPI) != 0) &&
      this->Features.HasACPI)
    bHasFeature = true;

  // Check for thermal monitor support.
  if (((dwFeature & SystemInformation::CPU_FEATURE_THERMALMONITOR) != 0) &&
      this->Features.HasThermal)
    bHasFeature = true;

  // Check for temperature sensing diode support.
  if (((dwFeature & SystemInformation::CPU_FEATURE_TEMPSENSEDIODE) != 0) &&
      this->Features.ExtendedFeatures.PowerManagement.HasTempSenseDiode)
    bHasFeature = true;

  // Check for frequency ID support.
  if (((dwFeature & SystemInformation::CPU_FEATURE_FREQUENCYID) != 0) &&
      this->Features.ExtendedFeatures.PowerManagement.HasFrequencyID)
    bHasFeature = true;

  // Check for voltage ID support.
  if (((dwFeature & SystemInformation::CPU_FEATURE_VOLTAGEID_FREQUENCY) !=
       0) &&
      this->Features.ExtendedFeatures.PowerManagement.HasVoltageID)
    bHasFeature = true;

  // Check for FPU support.
  if (((dwFeature & SystemInformation::CPU_FEATURE_FPU) != 0) &&
      this->Features.HasFPU)
    bHasFeature = true;

  return bHasFeature;
}

void SystemInformationImplementation::Delay(unsigned int uiMS)
{
#ifdef _WIN32
  LARGE_INTEGER Frequency, StartCounter, EndCounter;
  __int64 x;

  // Get the frequency of the high performance counter.
  if (!QueryPerformanceFrequency(&Frequency))
    return;
  x = Frequency.QuadPart / 1000 * uiMS;

  // Get the starting position of the counter.
  QueryPerformanceCounter(&StartCounter);

  do {
    // Get the ending position of the counter.
    QueryPerformanceCounter(&EndCounter);
  } while (EndCounter.QuadPart - StartCounter.QuadPart < x);
#endif
  (void)uiMS;
}

bool SystemInformationImplementation::DoesCPUSupportCPUID()
{
#if USE_CPUID
  int dummy[4] = { 0, 0, 0, 0 };

#if USE_ASM_INSTRUCTIONS
  return call_cpuid(0, dummy);
#else
  call_cpuid(0, dummy);
  return dummy[0] || dummy[1] || dummy[2] || dummy[3];
#endif
#else
  // Assume no cpuid instruction.
  return false;
#endif
}

bool SystemInformationImplementation::RetrieveCPUFeatures()
{
#if USE_CPUID
  int cpuinfo[4] = { 0, 0, 0, 0 };

  if (!call_cpuid(1, cpuinfo)) {
    return false;
  }

  // Retrieve the features of CPU present.
  this->Features.HasFPU =
    ((cpuinfo[3] & 0x00000001) != 0); // FPU Present --> Bit 0
  this->Features.HasTSC =
    ((cpuinfo[3] & 0x00000010) != 0); // TSC Present --> Bit 4
  this->Features.HasAPIC =
    ((cpuinfo[3] & 0x00000200) != 0); // APIC Present --> Bit 9
  this->Features.HasMTRR =
    ((cpuinfo[3] & 0x00001000) != 0); // MTRR Present --> Bit 12
  this->Features.HasCMOV =
    ((cpuinfo[3] & 0x00008000) != 0); // CMOV Present --> Bit 15
  this->Features.HasSerial =
    ((cpuinfo[3] & 0x00040000) != 0); // Serial Present --> Bit 18
  this->Features.HasACPI =
    ((cpuinfo[3] & 0x00400000) != 0); // ACPI Capable --> Bit 22
  this->Features.HasMMX =
    ((cpuinfo[3] & 0x00800000) != 0); // MMX Present --> Bit 23
  this->Features.HasSSE =
    ((cpuinfo[3] & 0x02000000) != 0); // SSE Present --> Bit 25
  this->Features.HasSSE2 =
    ((cpuinfo[3] & 0x04000000) != 0); // SSE2 Present --> Bit 26
  this->Features.HasThermal =
    ((cpuinfo[3] & 0x20000000) != 0); // Thermal Monitor Present --> Bit 29
  this->Features.HasIA64 =
    ((cpuinfo[3] & 0x40000000) != 0); // IA64 Present --> Bit 30

#if USE_ASM_INSTRUCTIONS
  // Retrieve extended SSE capabilities if SSE is available.
  if (this->Features.HasSSE) {

    // Attempt to __try some SSE FP instructions.
    __try {
      // Perform: orps xmm0, xmm0
      _asm
      {
        _emit 0x0f
        _emit 0x56
        _emit 0xc0
      }

      // SSE FP capable processor.
      this->Features.HasSSEFP = true;
    } __except (1) {
      // bad instruction - processor or OS cannot handle SSE FP.
      this->Features.HasSSEFP = false;
    }
  } else {
    // Set the advanced SSE capabilities to not available.
    this->Features.HasSSEFP = false;
  }
#else
  this->Features.HasSSEFP = false;
#endif

  // Retrieve Intel specific extended features.
  if (this->ChipManufacturer == Intel) {
    bool SupportsSMT =
      ((cpuinfo[3] & 0x10000000) != 0); // Intel specific: SMT --> Bit 28

    if ((SupportsSMT) && (this->Features.HasAPIC)) {
      // Retrieve APIC information if there is one present.
      this->Features.ExtendedFeatures.APIC_ID =
        ((cpuinfo[1] & 0xFF000000) >> 24);
    }
  }

  return true;

#else
  return false;
#endif
}

/** Find the manufacturer given the vendor id */
void SystemInformationImplementation::FindManufacturer(
  const std::string& family)
{
  if (this->ChipID.Vendor == "GenuineIntel")
    this->ChipManufacturer = Intel; // Intel Corp.
  else if (this->ChipID.Vendor == "UMC UMC UMC ")
    this->ChipManufacturer = UMC; // United Microelectronics Corp.
  else if (this->ChipID.Vendor == "AuthenticAMD")
    this->ChipManufacturer = AMD; // Advanced Micro Devices
  else if (this->ChipID.Vendor == "AMD ISBETTER")
    this->ChipManufacturer = AMD; // Advanced Micro Devices (1994)
  else if (this->ChipID.Vendor == "CyrixInstead")
    this->ChipManufacturer = Cyrix; // Cyrix Corp., VIA Inc.
  else if (this->ChipID.Vendor == "NexGenDriven")
    this->ChipManufacturer = NexGen; // NexGen Inc. (now AMD)
  else if (this->ChipID.Vendor == "CentaurHauls")
    this->ChipManufacturer = IDT; // IDT/Centaur (now VIA)
  else if (this->ChipID.Vendor == "RiseRiseRise")
    this->ChipManufacturer = Rise; // Rise
  else if (this->ChipID.Vendor == "GenuineTMx86")
    this->ChipManufacturer = Transmeta; // Transmeta
  else if (this->ChipID.Vendor == "TransmetaCPU")
    this->ChipManufacturer = Transmeta; // Transmeta
  else if (this->ChipID.Vendor == "Geode By NSC")
    this->ChipManufacturer = NSC; // National Semiconductor
  else if (this->ChipID.Vendor == "Sun")
    this->ChipManufacturer = Sun; // Sun Microelectronics
  else if (this->ChipID.Vendor == "IBM")
    this->ChipManufacturer = IBM; // IBM Microelectronics
  else if (this->ChipID.Vendor == "Hewlett-Packard")
    this->ChipManufacturer = HP; // Hewlett-Packard
  else if (this->ChipID.Vendor == "Motorola")
    this->ChipManufacturer = Motorola; // Motorola Microelectronics
  else if (family.substr(0, 7) == "PA-RISC")
    this->ChipManufacturer = HP; // Hewlett-Packard
  else
    this->ChipManufacturer = UnknownManufacturer; // Unknown manufacturer
}

/** */
bool SystemInformationImplementation::RetrieveCPUIdentity()
{
#if USE_CPUID
  int localCPUVendor[4];
  int localCPUSignature[4];

  if (!call_cpuid(0, localCPUVendor)) {
    return false;
  }
  if (!call_cpuid(1, localCPUSignature)) {
    return false;
  }

  // Process the returned information.
  //    ; eax = 0 --> eax: maximum value of CPUID instruction.
  //    ;        ebx: part 1 of 3; CPU signature.
  //    ;        edx: part 2 of 3; CPU signature.
  //    ;        ecx: part 3 of 3; CPU signature.
  char vbuf[13];
  memcpy(&(vbuf[0]), &(localCPUVendor[1]), sizeof(int));
  memcpy(&(vbuf[4]), &(localCPUVendor[3]), sizeof(int));
  memcpy(&(vbuf[8]), &(localCPUVendor[2]), sizeof(int));
  vbuf[12] = '\0';
  this->ChipID.Vendor = vbuf;

  // Retrieve the family of CPU present.
  //    ; eax = 1 --> eax: CPU ID - bits 31..16 - unused, bits 15..12 - type,
  //    bits 11..8 - family, bits 7..4 - model, bits 3..0 - mask revision
  //    ;        ebx: 31..24 - default APIC ID, 23..16 - logical processor ID,
  //    15..8 - CFLUSH chunk size , 7..0 - brand ID
  //    ;        edx: CPU feature flags
  this->ChipID.ExtendedFamily =
    ((localCPUSignature[0] & 0x0FF00000) >> 20); // Bits 27..20 Used
  this->ChipID.ExtendedModel =
    ((localCPUSignature[0] & 0x000F0000) >> 16); // Bits 19..16 Used
  this->ChipID.Type =
    ((localCPUSignature[0] & 0x0000F000) >> 12); // Bits 15..12 Used
  this->ChipID.Family =
    ((localCPUSignature[0] & 0x00000F00) >> 8); // Bits 11..8 Used
  this->ChipID.Model =
    ((localCPUSignature[0] & 0x000000F0) >> 4); // Bits 7..4 Used
  this->ChipID.Revision =
    ((localCPUSignature[0] & 0x0000000F) >> 0); // Bits 3..0 Used

  return true;

#else
  return false;
#endif
}

/** */
bool SystemInformationImplementation::RetrieveCPUCacheDetails()
{
#if USE_CPUID
  int L1Cache[4] = { 0, 0, 0, 0 };
  int L2Cache[4] = { 0, 0, 0, 0 };

  // Check to see if what we are about to do is supported...
  if (RetrieveCPUExtendedLevelSupport(0x80000005)) {
    if (!call_cpuid(0x80000005, L1Cache)) {
      return false;
    }
    // Save the L1 data cache size (in KB) from ecx: bits 31..24 as well as
    // data cache size from edx: bits 31..24.
    this->Features.L1CacheSize = ((L1Cache[2] & 0xFF000000) >> 24);
    this->Features.L1CacheSize += ((L1Cache[3] & 0xFF000000) >> 24);
  } else {
    // Store -1 to indicate the cache could not be queried.
    this->Features.L1CacheSize = -1;
  }

  // Check to see if what we are about to do is supported...
  if (RetrieveCPUExtendedLevelSupport(0x80000006)) {
    if (!call_cpuid(0x80000006, L2Cache)) {
      return false;
    }
    // Save the L2 unified cache size (in KB) from ecx: bits 31..16.
    this->Features.L2CacheSize = ((L2Cache[2] & 0xFFFF0000) >> 16);
  } else {
    // Store -1 to indicate the cache could not be queried.
    this->Features.L2CacheSize = -1;
  }

  // Define L3 as being not present as we cannot test for it.
  this->Features.L3CacheSize = -1;

#endif

  // Return failure if we cannot detect either cache with this method.
  return ((this->Features.L1CacheSize == -1) &&
          (this->Features.L2CacheSize == -1))
    ? false
    : true;
}

/** */
bool SystemInformationImplementation::RetrieveClassicalCPUCacheDetails()
{
#if USE_CPUID
  int TLBCode = -1, TLBData = -1, L1Code = -1, L1Data = -1, L1Trace = -1,
      L2Unified = -1, L3Unified = -1;
  int TLBCacheData[4] = { 0, 0, 0, 0 };
  int TLBPassCounter = 0;
  int TLBCacheUnit = 0;

  do {
    if (!call_cpuid(2, TLBCacheData)) {
      return false;
    }

    int bob = ((TLBCacheData[0] & 0x00FF0000) >> 16);
    (void)bob;
    // Process the returned TLB and cache information.
    for (int nCounter = 0; nCounter < TLBCACHE_INFO_UNITS; nCounter++) {
      // First of all - decide which unit we are dealing with.
      switch (nCounter) {
        // eax: bits 8..15 : bits 16..23 : bits 24..31
        case 0:
          TLBCacheUnit = ((TLBCacheData[0] & 0x0000FF00) >> 8);
          break;
        case 1:
          TLBCacheUnit = ((TLBCacheData[0] & 0x00FF0000) >> 16);
          break;
        case 2:
          TLBCacheUnit = ((TLBCacheData[0] & 0xFF000000) >> 24);
          break;

        // ebx: bits 0..7 : bits 8..15 : bits 16..23 : bits 24..31
        case 3:
          TLBCacheUnit = ((TLBCacheData[1] & 0x000000FF) >> 0);
          break;
        case 4:
          TLBCacheUnit = ((TLBCacheData[1] & 0x0000FF00) >> 8);
          break;
        case 5:
          TLBCacheUnit = ((TLBCacheData[1] & 0x00FF0000) >> 16);
          break;
        case 6:
          TLBCacheUnit = ((TLBCacheData[1] & 0xFF000000) >> 24);
          break;

        // ecx: bits 0..7 : bits 8..15 : bits 16..23 : bits 24..31
        case 7:
          TLBCacheUnit = ((TLBCacheData[2] & 0x000000FF) >> 0);
          break;
        case 8:
          TLBCacheUnit = ((TLBCacheData[2] & 0x0000FF00) >> 8);
          break;
        case 9:
          TLBCacheUnit = ((TLBCacheData[2] & 0x00FF0000) >> 16);
          break;
        case 10:
          TLBCacheUnit = ((TLBCacheData[2] & 0xFF000000) >> 24);
          break;

        // edx: bits 0..7 : bits 8..15 : bits 16..23 : bits 24..31
        case 11:
          TLBCacheUnit = ((TLBCacheData[3] & 0x000000FF) >> 0);
          break;
        case 12:
          TLBCacheUnit = ((TLBCacheData[3] & 0x0000FF00) >> 8);
          break;
        case 13:
          TLBCacheUnit = ((TLBCacheData[3] & 0x00FF0000) >> 16);
          break;
        case 14:
          TLBCacheUnit = ((TLBCacheData[3] & 0xFF000000) >> 24);
          break;

        // Default case - an error has occurred.
        default:
          return false;
      }

      // Now process the resulting unit to see what it means....
      switch (TLBCacheUnit) {
        case 0x00:
          break;
        case 0x01:
          STORE_TLBCACHE_INFO(TLBCode, 4);
          break;
        case 0x02:
          STORE_TLBCACHE_INFO(TLBCode, 4096);
          break;
        case 0x03:
          STORE_TLBCACHE_INFO(TLBData, 4);
          break;
        case 0x04:
          STORE_TLBCACHE_INFO(TLBData, 4096);
          break;
        case 0x06:
          STORE_TLBCACHE_INFO(L1Code, 8);
          break;
        case 0x08:
          STORE_TLBCACHE_INFO(L1Code, 16);
          break;
        case 0x0a:
          STORE_TLBCACHE_INFO(L1Data, 8);
          break;
        case 0x0c:
          STORE_TLBCACHE_INFO(L1Data, 16);
          break;
        case 0x10:
          STORE_TLBCACHE_INFO(L1Data, 16);
          break; // <-- FIXME: IA-64 Only
        case 0x15:
          STORE_TLBCACHE_INFO(L1Code, 16);
          break; // <-- FIXME: IA-64 Only
        case 0x1a:
          STORE_TLBCACHE_INFO(L2Unified, 96);
          break; // <-- FIXME: IA-64 Only
        case 0x22:
          STORE_TLBCACHE_INFO(L3Unified, 512);
          break;
        case 0x23:
          STORE_TLBCACHE_INFO(L3Unified, 1024);
          break;
        case 0x25:
          STORE_TLBCACHE_INFO(L3Unified, 2048);
          break;
        case 0x29:
          STORE_TLBCACHE_INFO(L3Unified, 4096);
          break;
        case 0x39:
          STORE_TLBCACHE_INFO(L2Unified, 128);
          break;
        case 0x3c:
          STORE_TLBCACHE_INFO(L2Unified, 256);
          break;
        case 0x40:
          STORE_TLBCACHE_INFO(L2Unified, 0);
          break; // <-- FIXME: No integrated L2 cache (P6 core) or L3 cache (P4
                 // core).
        case 0x41:
          STORE_TLBCACHE_INFO(L2Unified, 128);
          break;
        case 0x42:
          STORE_TLBCACHE_INFO(L2Unified, 256);
          break;
        case 0x43:
          STORE_TLBCACHE_INFO(L2Unified, 512);
          break;
        case 0x44:
          STORE_TLBCACHE_INFO(L2Unified, 1024);
          break;
        case 0x45:
          STORE_TLBCACHE_INFO(L2Unified, 2048);
          break;
        case 0x50:
          STORE_TLBCACHE_INFO(TLBCode, 4096);
          break;
        case 0x51:
          STORE_TLBCACHE_INFO(TLBCode, 4096);
          break;
        case 0x52:
          STORE_TLBCACHE_INFO(TLBCode, 4096);
          break;
        case 0x5b:
          STORE_TLBCACHE_INFO(TLBData, 4096);
          break;
        case 0x5c:
          STORE_TLBCACHE_INFO(TLBData, 4096);
          break;
        case 0x5d:
          STORE_TLBCACHE_INFO(TLBData, 4096);
          break;
        case 0x66:
          STORE_TLBCACHE_INFO(L1Data, 8);
          break;
        case 0x67:
          STORE_TLBCACHE_INFO(L1Data, 16);
          break;
        case 0x68:
          STORE_TLBCACHE_INFO(L1Data, 32);
          break;
        case 0x70:
          STORE_TLBCACHE_INFO(L1Trace, 12);
          break;
        case 0x71:
          STORE_TLBCACHE_INFO(L1Trace, 16);
          break;
        case 0x72:
          STORE_TLBCACHE_INFO(L1Trace, 32);
          break;
        case 0x77:
          STORE_TLBCACHE_INFO(L1Code, 16);
          break; // <-- FIXME: IA-64 Only
        case 0x79:
          STORE_TLBCACHE_INFO(L2Unified, 128);
          break;
        case 0x7a:
          STORE_TLBCACHE_INFO(L2Unified, 256);
          break;
        case 0x7b:
          STORE_TLBCACHE_INFO(L2Unified, 512);
          break;
        case 0x7c:
          STORE_TLBCACHE_INFO(L2Unified, 1024);
          break;
        case 0x7e:
          STORE_TLBCACHE_INFO(L2Unified, 256);
          break;
        case 0x81:
          STORE_TLBCACHE_INFO(L2Unified, 128);
          break;
        case 0x82:
          STORE_TLBCACHE_INFO(L2Unified, 256);
          break;
        case 0x83:
          STORE_TLBCACHE_INFO(L2Unified, 512);
          break;
        case 0x84:
          STORE_TLBCACHE_INFO(L2Unified, 1024);
          break;
        case 0x85:
          STORE_TLBCACHE_INFO(L2Unified, 2048);
          break;
        case 0x88:
          STORE_TLBCACHE_INFO(L3Unified, 2048);
          break; // <-- FIXME: IA-64 Only
        case 0x89:
          STORE_TLBCACHE_INFO(L3Unified, 4096);
          break; // <-- FIXME: IA-64 Only
        case 0x8a:
          STORE_TLBCACHE_INFO(L3Unified, 8192);
          break; // <-- FIXME: IA-64 Only
        case 0x8d:
          STORE_TLBCACHE_INFO(L3Unified, 3096);
          break; // <-- FIXME: IA-64 Only
        case 0x90:
          STORE_TLBCACHE_INFO(TLBCode, 262144);
          break; // <-- FIXME: IA-64 Only
        case 0x96:
          STORE_TLBCACHE_INFO(TLBCode, 262144);
          break; // <-- FIXME: IA-64 Only
        case 0x9b:
          STORE_TLBCACHE_INFO(TLBCode, 262144);
          break; // <-- FIXME: IA-64 Only

        // Default case - an error has occurred.
        default:
          return false;
      }
    }

    // Increment the TLB pass counter.
    TLBPassCounter++;
  } while ((TLBCacheData[0] & 0x000000FF) > TLBPassCounter);

  // Ok - we now have the maximum TLB, L1, L2, and L3 sizes...
  if ((L1Code == -1) && (L1Data == -1) && (L1Trace == -1)) {
    this->Features.L1CacheSize = -1;
  } else if ((L1Code == -1) && (L1Data == -1) && (L1Trace != -1)) {
    this->Features.L1CacheSize = L1Trace;
  } else if ((L1Code != -1) && (L1Data == -1)) {
    this->Features.L1CacheSize = L1Code;
  } else if ((L1Code == -1) && (L1Data != -1)) {
    this->Features.L1CacheSize = L1Data;
  } else if ((L1Code != -1) && (L1Data != -1)) {
    this->Features.L1CacheSize = L1Code + L1Data;
  } else {
    this->Features.L1CacheSize = -1;
  }

  // Ok - we now have the maximum TLB, L1, L2, and L3 sizes...
  if (L2Unified == -1) {
    this->Features.L2CacheSize = -1;
  } else {
    this->Features.L2CacheSize = L2Unified;
  }

  // Ok - we now have the maximum TLB, L1, L2, and L3 sizes...
  if (L3Unified == -1) {
    this->Features.L3CacheSize = -1;
  } else {
    this->Features.L3CacheSize = L3Unified;
  }

  return true;

#else
  return false;
#endif
}

/** */
bool SystemInformationImplementation::RetrieveCPUClockSpeed()
{
  bool retrieved = false;

#if defined(_WIN32)
  unsigned int uiRepetitions = 1;
  unsigned int uiMSecPerRepetition = 50;
  __int64 i64Total = 0;
  __int64 i64Overhead = 0;

  // Check if the TSC implementation works at all
  if (this->Features.HasTSC &&
      GetCyclesDifference(SystemInformationImplementation::Delay,
                          uiMSecPerRepetition) > 0) {
    for (unsigned int nCounter = 0; nCounter < uiRepetitions; nCounter++) {
      i64Total += GetCyclesDifference(SystemInformationImplementation::Delay,
                                      uiMSecPerRepetition);
      i64Overhead += GetCyclesDifference(
        SystemInformationImplementation::DelayOverhead, uiMSecPerRepetition);
    }

    // Calculate the MHz speed.
    i64Total -= i64Overhead;
    i64Total /= uiRepetitions;
    i64Total /= uiMSecPerRepetition;
    i64Total /= 1000;

    // Save the CPU speed.
    this->CPUSpeedInMHz = (float)i64Total;

    retrieved = true;
  }

  // If RDTSC is not supported, we fallback to trying to read this value
  // from the registry:
  if (!retrieved) {
    HKEY hKey = NULL;
    LONG err =
      RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
                    KEY_READ, &hKey);

    if (ERROR_SUCCESS == err) {
      DWORD dwType = 0;
      DWORD data = 0;
      DWORD dwSize = sizeof(DWORD);

      err =
        RegQueryValueExW(hKey, L"~MHz", 0, &dwType, (LPBYTE)&data, &dwSize);

      if (ERROR_SUCCESS == err) {
        this->CPUSpeedInMHz = (float)data;
        retrieved = true;
      }

      RegCloseKey(hKey);
      hKey = NULL;
    }
  }
#endif

  return retrieved;
}

/** */
bool SystemInformationImplementation::RetrieveClassicalCPUClockSpeed()
{
#if USE_ASM_INSTRUCTIONS
  LARGE_INTEGER liStart, liEnd, liCountsPerSecond;
  double dFrequency, dDifference;

  // Attempt to get a starting tick count.
  QueryPerformanceCounter(&liStart);

  __try {
    _asm {
      mov eax, 0x80000000
      mov ebx, CLASSICAL_CPU_FREQ_LOOP
      Timer_Loop:
      bsf ecx,eax
      dec ebx
      jnz Timer_Loop
    }
  } __except (1) {
    return false;
  }

  // Attempt to get a starting tick count.
  QueryPerformanceCounter(&liEnd);

  // Get the difference...  NB: This is in seconds....
  QueryPerformanceFrequency(&liCountsPerSecond);
  dDifference = (((double)liEnd.QuadPart - (double)liStart.QuadPart) /
                 (double)liCountsPerSecond.QuadPart);

  // Calculate the clock speed.
  if (this->ChipID.Family == 3) {
    // 80386 processors....  Loop time is 115 cycles!
    dFrequency = (((CLASSICAL_CPU_FREQ_LOOP * 115) / dDifference) / 1000000);
  } else if (this->ChipID.Family == 4) {
    // 80486 processors....  Loop time is 47 cycles!
    dFrequency = (((CLASSICAL_CPU_FREQ_LOOP * 47) / dDifference) / 1000000);
  } else if (this->ChipID.Family == 5) {
    // Pentium processors....  Loop time is 43 cycles!
    dFrequency = (((CLASSICAL_CPU_FREQ_LOOP * 43) / dDifference) / 1000000);
  }

  // Save the clock speed.
  this->Features.CPUSpeed = (int)dFrequency;

  return true;

#else
  return false;
#endif
}

/** */
bool SystemInformationImplementation::RetrieveCPUExtendedLevelSupport(
  int CPULevelToCheck)
{
  int cpuinfo[4] = { 0, 0, 0, 0 };

  // The extended CPUID is supported by various vendors starting with the
  // following CPU models:
  //
  //    Manufacturer & Chip Name      |    Family     Model    Revision
  //
  //    AMD K6, K6-2                  |       5       6      x
  //    Cyrix GXm, Cyrix III "Joshua" |       5       4      x
  //    IDT C6-2                      |       5       8      x
  //    VIA Cyrix III                 |       6       5      x
  //    Transmeta Crusoe              |       5       x      x
  //    Intel Pentium 4               |       f       x      x
  //

  // We check to see if a supported processor is present...
  if (this->ChipManufacturer == AMD) {
    if (this->ChipID.Family < 5)
      return false;
    if ((this->ChipID.Family == 5) && (this->ChipID.Model < 6))
      return false;
  } else if (this->ChipManufacturer == Cyrix) {
    if (this->ChipID.Family < 5)
      return false;
    if ((this->ChipID.Family == 5) && (this->ChipID.Model < 4))
      return false;
    if ((this->ChipID.Family == 6) && (this->ChipID.Model < 5))
      return false;
  } else if (this->ChipManufacturer == IDT) {
    if (this->ChipID.Family < 5)
      return false;
    if ((this->ChipID.Family == 5) && (this->ChipID.Model < 8))
      return false;
  } else if (this->ChipManufacturer == Transmeta) {
    if (this->ChipID.Family < 5)
      return false;
  } else if (this->ChipManufacturer == Intel) {
    if (this->ChipID.Family < 0xf) {
      return false;
    }
  }

#if USE_CPUID
  if (!call_cpuid(0x80000000, cpuinfo)) {
    return false;
  }
#endif

  // Now we have to check the level wanted vs level returned...
  int nLevelWanted = (CPULevelToCheck & 0x7FFFFFFF);
  int nLevelReturn = (cpuinfo[0] & 0x7FFFFFFF);

  // Check to see if the level provided is supported...
  if (nLevelWanted > nLevelReturn) {
    return false;
  }

  return true;
}

/** */
bool SystemInformationImplementation::RetrieveExtendedCPUFeatures()
{

  // Check that we are not using an Intel processor as it does not support
  // this.
  if (this->ChipManufacturer == Intel) {
    return false;
  }

  // Check to see if what we are about to do is supported...
  if (!RetrieveCPUExtendedLevelSupport(static_cast<int>(0x80000001))) {
    return false;
  }

#if USE_CPUID
  int localCPUExtendedFeatures[4] = { 0, 0, 0, 0 };

  if (!call_cpuid(0x80000001, localCPUExtendedFeatures)) {
    return false;
  }

  // Retrieve the extended features of CPU present.
  this->Features.ExtendedFeatures.Has3DNow =
    ((localCPUExtendedFeatures[3] & 0x80000000) !=
     0); // 3DNow Present --> Bit 31.
  this->Features.ExtendedFeatures.Has3DNowPlus =
    ((localCPUExtendedFeatures[3] & 0x40000000) !=
     0); // 3DNow+ Present -- > Bit 30.
  this->Features.ExtendedFeatures.HasSSEMMX =
    ((localCPUExtendedFeatures[3] & 0x00400000) !=
     0); // SSE MMX Present --> Bit 22.
  this->Features.ExtendedFeatures.SupportsMP =
    ((localCPUExtendedFeatures[3] & 0x00080000) !=
     0); // MP Capable -- > Bit 19.

  // Retrieve AMD specific extended features.
  if (this->ChipManufacturer == AMD) {
    this->Features.ExtendedFeatures.HasMMXPlus =
      ((localCPUExtendedFeatures[3] & 0x00400000) !=
       0); // AMD specific: MMX-SSE --> Bit 22
  }

  // Retrieve Cyrix specific extended features.
  if (this->ChipManufacturer == Cyrix) {
    this->Features.ExtendedFeatures.HasMMXPlus =
      ((localCPUExtendedFeatures[3] & 0x01000000) !=
       0); // Cyrix specific: Extended MMX --> Bit 24
  }

  return true;

#else
  return false;
#endif
}

/** */
bool SystemInformationImplementation::RetrieveProcessorSerialNumber()
{
  // Check to see if the processor supports the processor serial number.
  if (!this->Features.HasSerial) {
    return false;
  }

#if USE_CPUID
  int SerialNumber[4];

  if (!call_cpuid(3, SerialNumber)) {
    return false;
  }

  // Process the returned information.
  //    ; eax = 3 --> ebx: top 32 bits are the processor signature bits --> NB:
  //    Transmeta only ?!?
  //    ;        ecx: middle 32 bits are the processor signature bits
  //    ;        edx: bottom 32 bits are the processor signature bits
  char sn[128];
  sprintf(sn, "%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x-%.2x%.2x",
          ((SerialNumber[1] & 0xff000000) >> 24),
          ((SerialNumber[1] & 0x00ff0000) >> 16),
          ((SerialNumber[1] & 0x0000ff00) >> 8),
          ((SerialNumber[1] & 0x000000ff) >> 0),
          ((SerialNumber[2] & 0xff000000) >> 24),
          ((SerialNumber[2] & 0x00ff0000) >> 16),
          ((SerialNumber[2] & 0x0000ff00) >> 8),
          ((SerialNumber[2] & 0x000000ff) >> 0),
          ((SerialNumber[3] & 0xff000000) >> 24),
          ((SerialNumber[3] & 0x00ff0000) >> 16),
          ((SerialNumber[3] & 0x0000ff00) >> 8),
          ((SerialNumber[3] & 0x000000ff) >> 0));
  this->ChipID.SerialNumber = sn;
  return true;

#else
  return false;
#endif
}

/** */
bool SystemInformationImplementation::RetrieveCPUPowerManagement()
{
  // Check to see if what we are about to do is supported...
  if (!RetrieveCPUExtendedLevelSupport(static_cast<int>(0x80000007))) {
    this->Features.ExtendedFeatures.PowerManagement.HasFrequencyID = false;
    this->Features.ExtendedFeatures.PowerManagement.HasVoltageID = false;
    this->Features.ExtendedFeatures.PowerManagement.HasTempSenseDiode = false;
    return false;
  }

#if USE_CPUID
  int localCPUPowerManagement[4] = { 0, 0, 0, 0 };

  if (!call_cpuid(0x80000007, localCPUPowerManagement)) {
    return false;
  }

  // Check for the power management capabilities of the CPU.
  this->Features.ExtendedFeatures.PowerManagement.HasTempSenseDiode =
    ((localCPUPowerManagement[3] & 0x00000001) != 0);
  this->Features.ExtendedFeatures.PowerManagement.HasFrequencyID =
    ((localCPUPowerManagement[3] & 0x00000002) != 0);
  this->Features.ExtendedFeatures.PowerManagement.HasVoltageID =
    ((localCPUPowerManagement[3] & 0x00000004) != 0);

  return true;

#else
  return false;
#endif
}

#if USE_CPUID
// Used only in USE_CPUID implementation below.
static void SystemInformationStripLeadingSpace(std::string& str)
{
  // Because some manufacturers have leading white space - we have to
  // post-process the name.
  std::string::size_type pos = str.find_first_not_of(" ");
  if (pos != std::string::npos) {
    str = str.substr(pos);
  }
}
#endif

/** */
bool SystemInformationImplementation::RetrieveExtendedCPUIdentity()
{
  // Check to see if what we are about to do is supported...
  if (!RetrieveCPUExtendedLevelSupport(static_cast<int>(0x80000002)))
    return false;
  if (!RetrieveCPUExtendedLevelSupport(static_cast<int>(0x80000003)))
    return false;
  if (!RetrieveCPUExtendedLevelSupport(static_cast<int>(0x80000004)))
    return false;

#if USE_CPUID
  int CPUExtendedIdentity[12];

  if (!call_cpuid(0x80000002, CPUExtendedIdentity)) {
    return false;
  }
  if (!call_cpuid(0x80000003, CPUExtendedIdentity + 4)) {
    return false;
  }
  if (!call_cpuid(0x80000004, CPUExtendedIdentity + 8)) {
    return false;
  }

  // Process the returned information.
  char nbuf[49];
  memcpy(&(nbuf[0]), &(CPUExtendedIdentity[0]), sizeof(int));
  memcpy(&(nbuf[4]), &(CPUExtendedIdentity[1]), sizeof(int));
  memcpy(&(nbuf[8]), &(CPUExtendedIdentity[2]), sizeof(int));
  memcpy(&(nbuf[12]), &(CPUExtendedIdentity[3]), sizeof(int));
  memcpy(&(nbuf[16]), &(CPUExtendedIdentity[4]), sizeof(int));
  memcpy(&(nbuf[20]), &(CPUExtendedIdentity[5]), sizeof(int));
  memcpy(&(nbuf[24]), &(CPUExtendedIdentity[6]), sizeof(int));
  memcpy(&(nbuf[28]), &(CPUExtendedIdentity[7]), sizeof(int));
  memcpy(&(nbuf[32]), &(CPUExtendedIdentity[8]), sizeof(int));
  memcpy(&(nbuf[36]), &(CPUExtendedIdentity[9]), sizeof(int));
  memcpy(&(nbuf[40]), &(CPUExtendedIdentity[10]), sizeof(int));
  memcpy(&(nbuf[44]), &(CPUExtendedIdentity[11]), sizeof(int));
  nbuf[48] = '\0';
  this->ChipID.ProcessorName = nbuf;
  this->ChipID.ModelName = nbuf;

  // Because some manufacturers have leading white space - we have to
  // post-process the name.
  SystemInformationStripLeadingSpace(this->ChipID.ProcessorName);
  return true;
#else
  return false;
#endif
}

/** */
bool SystemInformationImplementation::RetrieveClassicalCPUIdentity()
{
  // Start by decided which manufacturer we are using....
  switch (this->ChipManufacturer) {
    case Intel:
      // Check the family / model / revision to determine the CPU ID.
      switch (this->ChipID.Family) {
        case 3:
          this->ChipID.ProcessorName = "Newer i80386 family";
          break;
        case 4:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "i80486DX-25/33";
              break;
            case 1:
              this->ChipID.ProcessorName = "i80486DX-50";
              break;
            case 2:
              this->ChipID.ProcessorName = "i80486SX";
              break;
            case 3:
              this->ChipID.ProcessorName = "i80486DX2";
              break;
            case 4:
              this->ChipID.ProcessorName = "i80486SL";
              break;
            case 5:
              this->ChipID.ProcessorName = "i80486SX2";
              break;
            case 7:
              this->ChipID.ProcessorName = "i80486DX2 WriteBack";
              break;
            case 8:
              this->ChipID.ProcessorName = "i80486DX4";
              break;
            case 9:
              this->ChipID.ProcessorName = "i80486DX4 WriteBack";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown 80486 family";
              return false;
          }
          break;
        case 5:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "P5 A-Step";
              break;
            case 1:
              this->ChipID.ProcessorName = "P5";
              break;
            case 2:
              this->ChipID.ProcessorName = "P54C";
              break;
            case 3:
              this->ChipID.ProcessorName = "P24T OverDrive";
              break;
            case 4:
              this->ChipID.ProcessorName = "P55C";
              break;
            case 7:
              this->ChipID.ProcessorName = "P54C";
              break;
            case 8:
              this->ChipID.ProcessorName = "P55C (0.25micron)";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown Pentium family";
              return false;
          }
          break;
        case 6:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "P6 A-Step";
              break;
            case 1:
              this->ChipID.ProcessorName = "P6";
              break;
            case 3:
              this->ChipID.ProcessorName = "Pentium II (0.28 micron)";
              break;
            case 5:
              this->ChipID.ProcessorName = "Pentium II (0.25 micron)";
              break;
            case 6:
              this->ChipID.ProcessorName = "Pentium II With On-Die L2 Cache";
              break;
            case 7:
              this->ChipID.ProcessorName = "Pentium III (0.25 micron)";
              break;
            case 8:
              this->ChipID.ProcessorName =
                "Pentium III (0.18 micron) With 256 KB On-Die L2 Cache ";
              break;
            case 0xa:
              this->ChipID.ProcessorName =
                "Pentium III (0.18 micron) With 1 Or 2 MB On-Die L2 Cache ";
              break;
            case 0xb:
              this->ChipID.ProcessorName = "Pentium III (0.13 micron) With "
                                           "256 Or 512 KB On-Die L2 Cache ";
              break;
            case 23:
              this->ChipID.ProcessorName =
                "Intel(R) Core(TM)2 Duo CPU     T9500  @ 2.60GHz";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown P6 family";
              return false;
          }
          break;
        case 7:
          this->ChipID.ProcessorName = "Intel Merced (IA-64)";
          break;
        case 0xf:
          // Check the extended family bits...
          switch (this->ChipID.ExtendedFamily) {
            case 0:
              switch (this->ChipID.Model) {
                case 0:
                  this->ChipID.ProcessorName = "Pentium IV (0.18 micron)";
                  break;
                case 1:
                  this->ChipID.ProcessorName = "Pentium IV (0.18 micron)";
                  break;
                case 2:
                  this->ChipID.ProcessorName = "Pentium IV (0.13 micron)";
                  break;
                default:
                  this->ChipID.ProcessorName = "Unknown Pentium 4 family";
                  return false;
              }
              break;
            case 1:
              this->ChipID.ProcessorName = "Intel McKinley (IA-64)";
              break;
            default:
              this->ChipID.ProcessorName = "Pentium";
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown Intel family";
          return false;
      }
      break;

    case AMD:
      // Check the family / model / revision to determine the CPU ID.
      switch (this->ChipID.Family) {
        case 4:
          switch (this->ChipID.Model) {
            case 3:
              this->ChipID.ProcessorName = "80486DX2";
              break;
            case 7:
              this->ChipID.ProcessorName = "80486DX2 WriteBack";
              break;
            case 8:
              this->ChipID.ProcessorName = "80486DX4";
              break;
            case 9:
              this->ChipID.ProcessorName = "80486DX4 WriteBack";
              break;
            case 0xe:
              this->ChipID.ProcessorName = "5x86";
              break;
            case 0xf:
              this->ChipID.ProcessorName = "5x86WB";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown 80486 family";
              return false;
          }
          break;
        case 5:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "SSA5 (PR75, PR90 =  PR100)";
              break;
            case 1:
              this->ChipID.ProcessorName = "5k86 (PR120 =  PR133)";
              break;
            case 2:
              this->ChipID.ProcessorName = "5k86 (PR166)";
              break;
            case 3:
              this->ChipID.ProcessorName = "5k86 (PR200)";
              break;
            case 6:
              this->ChipID.ProcessorName = "K6 (0.30 micron)";
              break;
            case 7:
              this->ChipID.ProcessorName = "K6 (0.25 micron)";
              break;
            case 8:
              this->ChipID.ProcessorName = "K6-2";
              break;
            case 9:
              this->ChipID.ProcessorName = "K6-III";
              break;
            case 0xd:
              this->ChipID.ProcessorName = "K6-2+ or K6-III+ (0.18 micron)";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown 80586 family";
              return false;
          }
          break;
        case 6:
          switch (this->ChipID.Model) {
            case 1:
              this->ChipID.ProcessorName = "Athlon- (0.25 micron)";
              break;
            case 2:
              this->ChipID.ProcessorName = "Athlon- (0.18 micron)";
              break;
            case 3:
              this->ChipID.ProcessorName = "Duron- (SF core)";
              break;
            case 4:
              this->ChipID.ProcessorName = "Athlon- (Thunderbird core)";
              break;
            case 6:
              this->ChipID.ProcessorName = "Athlon- (Palomino core)";
              break;
            case 7:
              this->ChipID.ProcessorName = "Duron- (Morgan core)";
              break;
            case 8:
              if (this->Features.ExtendedFeatures.SupportsMP)
                this->ChipID.ProcessorName = "Athlon - MP (Thoroughbred core)";
              else
                this->ChipID.ProcessorName = "Athlon - XP (Thoroughbred core)";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown K7 family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown AMD family";
          return false;
      }
      break;

    case Transmeta:
      switch (this->ChipID.Family) {
        case 5:
          switch (this->ChipID.Model) {
            case 4:
              this->ChipID.ProcessorName = "Crusoe TM3x00 and TM5x00";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown Crusoe family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown Transmeta family";
          return false;
      }
      break;

    case Rise:
      switch (this->ChipID.Family) {
        case 5:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "mP6 (0.25 micron)";
              break;
            case 2:
              this->ChipID.ProcessorName = "mP6 (0.18 micron)";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown Rise family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown Rise family";
          return false;
      }
      break;

    case UMC:
      switch (this->ChipID.Family) {
        case 4:
          switch (this->ChipID.Model) {
            case 1:
              this->ChipID.ProcessorName = "U5D";
              break;
            case 2:
              this->ChipID.ProcessorName = "U5S";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown UMC family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown UMC family";
          return false;
      }
      break;

    case IDT:
      switch (this->ChipID.Family) {
        case 5:
          switch (this->ChipID.Model) {
            case 4:
              this->ChipID.ProcessorName = "C6";
              break;
            case 8:
              this->ChipID.ProcessorName = "C2";
              break;
            case 9:
              this->ChipID.ProcessorName = "C3";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown IDT\\Centaur family";
              return false;
          }
          break;
        case 6:
          switch (this->ChipID.Model) {
            case 6:
              this->ChipID.ProcessorName = "VIA Cyrix III - Samuel";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown IDT\\Centaur family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown IDT\\Centaur family";
          return false;
      }
      break;

    case Cyrix:
      switch (this->ChipID.Family) {
        case 4:
          switch (this->ChipID.Model) {
            case 4:
              this->ChipID.ProcessorName = "MediaGX GX =  GXm";
              break;
            case 9:
              this->ChipID.ProcessorName = "5x86";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown Cx5x86 family";
              return false;
          }
          break;
        case 5:
          switch (this->ChipID.Model) {
            case 2:
              this->ChipID.ProcessorName = "Cx6x86";
              break;
            case 4:
              this->ChipID.ProcessorName = "MediaGX GXm";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown Cx6x86 family";
              return false;
          }
          break;
        case 6:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "6x86MX";
              break;
            case 5:
              this->ChipID.ProcessorName = "Cyrix M2 Core";
              break;
            case 6:
              this->ChipID.ProcessorName = "WinChip C5A Core";
              break;
            case 7:
              this->ChipID.ProcessorName = "WinChip C5B\\C5C Core";
              break;
            case 8:
              this->ChipID.ProcessorName = "WinChip C5C-T Core";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown 6x86MX\\Cyrix III family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown Cyrix family";
          return false;
      }
      break;

    case NexGen:
      switch (this->ChipID.Family) {
        case 5:
          switch (this->ChipID.Model) {
            case 0:
              this->ChipID.ProcessorName = "Nx586 or Nx586FPU";
              break;
            default:
              this->ChipID.ProcessorName = "Unknown NexGen family";
              return false;
          }
          break;
        default:
          this->ChipID.ProcessorName = "Unknown NexGen family";
          return false;
      }
      break;

    case NSC:
      this->ChipID.ProcessorName = "Cx486SLC \\ DLC \\ Cx486S A-Step";
      break;

    case Sun:
    case IBM:
    case Motorola:
    case HP:
    case UnknownManufacturer:
    default:
      this->ChipID.ProcessorName =
        "Unknown family"; // We cannot identify the processor.
      return false;
  }

  return true;
}

/** Extract a value from the CPUInfo file */
std::string SystemInformationImplementation::ExtractValueFromCpuInfoFile(
  std::string buffer, const char* word, size_t init)
{
  size_t pos = buffer.find(word, init);
  if (pos != std::string::npos) {
    this->CurrentPositionInFile = pos;
    pos = buffer.find(":", pos);
    size_t pos2 = buffer.find("\n", pos);
    if (pos != std::string::npos && pos2 != std::string::npos) {
      // It may happen that the beginning matches, but this is still not the
      // requested key.
      // An example is looking for "cpu" when "cpu family" comes first. So we
      // check that
      // we have only spaces from here to pos, otherwise we search again.
      for (size_t i = this->CurrentPositionInFile + strlen(word); i < pos;
           ++i) {
        if (buffer[i] != ' ' && buffer[i] != '\t') {
          return this->ExtractValueFromCpuInfoFile(buffer, word, pos2);
        }
      }
      return buffer.substr(pos + 2, pos2 - pos - 2);
    }
  }
  this->CurrentPositionInFile = std::string::npos;
  return "";
}

/** Query for the cpu status */
bool SystemInformationImplementation::RetreiveInformationFromCpuInfoFile()
{
  this->NumberOfLogicalCPU = 0;
  this->NumberOfPhysicalCPU = 0;
  std::string buffer;

  FILE* fd = fopen("/proc/cpuinfo", "r");
  if (!fd) {
    std::cout << "Problem opening /proc/cpuinfo" << std::endl;
    return false;
  }

  size_t fileSize = 0;
  while (!feof(fd)) {
    buffer += static_cast<char>(fgetc(fd));
    fileSize++;
  }
  fclose(fd);
  buffer.resize(fileSize - 2);
  // Number of logical CPUs (combination of multiple processors, multi-core
  // and SMT)
  size_t pos = buffer.find("processor\t");
  while (pos != std::string::npos) {
    this->NumberOfLogicalCPU++;
    pos = buffer.find("processor\t", pos + 1);
  }

#ifdef __linux
  // Count sockets.
  std::set<int> PhysicalIDs;
  std::string idc = this->ExtractValueFromCpuInfoFile(buffer, "physical id");
  while (this->CurrentPositionInFile != std::string::npos) {
    int id = atoi(idc.c_str());
    PhysicalIDs.insert(id);
    idc = this->ExtractValueFromCpuInfoFile(buffer, "physical id",
                                            this->CurrentPositionInFile + 1);
  }
  uint64_t NumberOfSockets = PhysicalIDs.size();
  NumberOfSockets = std::max(NumberOfSockets, (uint64_t)1);
  // Physical ids returned by Linux don't distinguish cores.
  // We want to record the total number of cores in this->NumberOfPhysicalCPU
  // (checking only the first proc)
  std::string Cores = this->ExtractValueFromCpuInfoFile(buffer, "cpu cores");
  unsigned int NumberOfCoresPerSocket = (unsigned int)atoi(Cores.c_str());
  NumberOfCoresPerSocket = std::max(NumberOfCoresPerSocket, 1u);
  this->NumberOfPhysicalCPU =
    NumberOfCoresPerSocket * (unsigned int)NumberOfSockets;

#else // __CYGWIN__
  // does not have "physical id" entries, neither "cpu cores"
  // this has to be fixed for hyper-threading.
  std::string cpucount =
    this->ExtractValueFromCpuInfoFile(buffer, "cpu count");
  this->NumberOfPhysicalCPU = this->NumberOfLogicalCPU =
    atoi(cpucount.c_str());
#endif
  // gotta have one, and if this is 0 then we get a / by 0n
  // better to have a bad answer than a crash
  if (this->NumberOfPhysicalCPU <= 0) {
    this->NumberOfPhysicalCPU = 1;
  }
  // LogicalProcessorsPerPhysical>1 => SMT.
  this->Features.ExtendedFeatures.LogicalProcessorsPerPhysical =
    this->NumberOfLogicalCPU / this->NumberOfPhysicalCPU;

  // CPU speed (checking only the first processor)
  std::string CPUSpeed = this->ExtractValueFromCpuInfoFile(buffer, "cpu MHz");
  if (!CPUSpeed.empty()) {
    this->CPUSpeedInMHz = static_cast<float>(atof(CPUSpeed.c_str()));
  }
#ifdef __linux
  else {
    // Linux Sparc: CPU speed is in Hz and encoded in hexadecimal
    CPUSpeed = this->ExtractValueFromCpuInfoFile(buffer, "Cpu0ClkTck");
    this->CPUSpeedInMHz =
      static_cast<float>(strtoull(CPUSpeed.c_str(), 0, 16)) / 1000000.0f;
  }
#endif

  // Chip family
  std::string familyStr =
    this->ExtractValueFromCpuInfoFile(buffer, "cpu family");
  if (familyStr.empty()) {
    familyStr = this->ExtractValueFromCpuInfoFile(buffer, "CPU architecture");
  }
  this->ChipID.Family = atoi(familyStr.c_str());

  // Chip Vendor
  this->ChipID.Vendor = this->ExtractValueFromCpuInfoFile(buffer, "vendor_id");
  this->FindManufacturer(familyStr);

  // second try for setting family
  if (this->ChipID.Family == 0 && this->ChipManufacturer == HP) {
    if (familyStr == "PA-RISC 1.1a")
      this->ChipID.Family = 0x11a;
    else if (familyStr == "PA-RISC 2.0")
      this->ChipID.Family = 0x200;
    // If you really get CMake to work on a machine not belonging to
    // any of those families I owe you a dinner if you get it to
    // contribute nightly builds regularly.
  }

  // Chip Model
  this->ChipID.Model =
    atoi(this->ExtractValueFromCpuInfoFile(buffer, "model").c_str());
  if (!this->RetrieveClassicalCPUIdentity()) {
    // Some platforms (e.g. PA-RISC) tell us their CPU name here.
    // Note: x86 does not.
    std::string cpuname = this->ExtractValueFromCpuInfoFile(buffer, "cpu");
    if (!cpuname.empty()) {
      this->ChipID.ProcessorName = cpuname;
    }
  }

  // Chip revision
  std::string cpurev = this->ExtractValueFromCpuInfoFile(buffer, "stepping");
  if (cpurev.empty()) {
    cpurev = this->ExtractValueFromCpuInfoFile(buffer, "CPU revision");
  }
  this->ChipID.Revision = atoi(cpurev.c_str());

  // Chip Model Name
  this->ChipID.ModelName =
    this->ExtractValueFromCpuInfoFile(buffer, "model name").c_str();

  // L1 Cache size
  // Different architectures may show different names for the caches.
  // Sum up everything we find.
  std::vector<const char*> cachename;
  cachename.clear();

  cachename.push_back("cache size"); // e.g. x86
  cachename.push_back("I-cache");    // e.g. PA-RISC
  cachename.push_back("D-cache");    // e.g. PA-RISC

  this->Features.L1CacheSize = 0;
  for (size_t index = 0; index < cachename.size(); index++) {
    std::string cacheSize =
      this->ExtractValueFromCpuInfoFile(buffer, cachename[index]);
    if (!cacheSize.empty()) {
      pos = cacheSize.find(" KB");
      if (pos != std::string::npos) {
        cacheSize = cacheSize.substr(0, pos);
      }
      this->Features.L1CacheSize += atoi(cacheSize.c_str());
    }
  }

  // processor feature flags (probably x86 specific)
  std::string cpuflags = this->ExtractValueFromCpuInfoFile(buffer, "flags");
  if (!cpurev.empty()) {
    // now we can match every flags as space + flag + space
    cpuflags = " " + cpuflags + " ";
    if ((cpuflags.find(" fpu ") != std::string::npos)) {
      this->Features.HasFPU = true;
    }
    if ((cpuflags.find(" tsc ") != std::string::npos)) {
      this->Features.HasTSC = true;
    }
    if ((cpuflags.find(" mmx ") != std::string::npos)) {
      this->Features.HasMMX = true;
    }
    if ((cpuflags.find(" sse ") != std::string::npos)) {
      this->Features.HasSSE = true;
    }
    if ((cpuflags.find(" sse2 ") != std::string::npos)) {
      this->Features.HasSSE2 = true;
    }
    if ((cpuflags.find(" apic ") != std::string::npos)) {
      this->Features.HasAPIC = true;
    }
    if ((cpuflags.find(" cmov ") != std::string::npos)) {
      this->Features.HasCMOV = true;
    }
    if ((cpuflags.find(" mtrr ") != std::string::npos)) {
      this->Features.HasMTRR = true;
    }
    if ((cpuflags.find(" acpi ") != std::string::npos)) {
      this->Features.HasACPI = true;
    }
    if ((cpuflags.find(" 3dnow ") != std::string::npos)) {
      this->Features.ExtendedFeatures.Has3DNow = true;
    }
  }

  return true;
}

bool SystemInformationImplementation::QueryProcessorBySysconf()
{
#if defined(_SC_NPROC_ONLN) && !defined(_SC_NPROCESSORS_ONLN)
// IRIX names this slightly different
#define _SC_NPROCESSORS_ONLN _SC_NPROC_ONLN
#endif

#ifdef _SC_NPROCESSORS_ONLN
  long c = sysconf(_SC_NPROCESSORS_ONLN);
  if (c <= 0) {
    return false;
  }

  this->NumberOfPhysicalCPU = static_cast<unsigned int>(c);
  this->NumberOfLogicalCPU = this->NumberOfPhysicalCPU;

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryProcessor()
{
  return this->QueryProcessorBySysconf();
}

/**
Get total system RAM in units of KiB.
*/
SystemInformation::LongLong
SystemInformationImplementation::GetHostMemoryTotal()
{
#if defined(_WIN32)
#if defined(_MSC_VER) && _MSC_VER < 1300
  MEMORYSTATUS stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatus(&stat);
  return stat.dwTotalPhys / 1024;
#else
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  return statex.ullTotalPhys / 1024;
#endif
#elif defined(__linux)
  SystemInformation::LongLong memTotal = 0;
  int ierr = GetFieldFromFile("/proc/meminfo", "MemTotal:", memTotal);
  if (ierr) {
    return -1;
  }
  return memTotal;
#elif defined(__APPLE__)
  uint64_t mem;
  size_t len = sizeof(mem);
  int ierr = sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
  if (ierr) {
    return -1;
  }
  return mem / 1024;
#else
  return 0;
#endif
}

/**
Get total system RAM in units of KiB. This may differ from the
host total if a host-wide resource limit is applied.
*/
SystemInformation::LongLong
SystemInformationImplementation::GetHostMemoryAvailable(
  const char* hostLimitEnvVarName)
{
  SystemInformation::LongLong memTotal = this->GetHostMemoryTotal();

  // the following mechanism is provided for systems that
  // apply resource limits across groups of processes.
  // this is of use on certain SMP systems (eg. SGI UV)
  // where the host has a large amount of ram but a given user's
  // access to it is severly restricted. The system will
  // apply a limit across a set of processes. Units are in KiB.
  if (hostLimitEnvVarName) {
    const char* hostLimitEnvVarValue = getenv(hostLimitEnvVarName);
    if (hostLimitEnvVarValue) {
      SystemInformation::LongLong hostLimit =
        atoLongLong(hostLimitEnvVarValue);
      if (hostLimit > 0) {
        memTotal = min(hostLimit, memTotal);
      }
    }
  }

  return memTotal;
}

/**
Get total system RAM in units of KiB. This may differ from the
host total if a per-process resource limit is applied.
*/
SystemInformation::LongLong
SystemInformationImplementation::GetProcMemoryAvailable(
  const char* hostLimitEnvVarName, const char* procLimitEnvVarName)
{
  SystemInformation::LongLong memAvail =
    this->GetHostMemoryAvailable(hostLimitEnvVarName);

  // the following mechanism is provide for systems where rlimits
  // are not employed. Units are in KiB.
  if (procLimitEnvVarName) {
    const char* procLimitEnvVarValue = getenv(procLimitEnvVarName);
    if (procLimitEnvVarValue) {
      SystemInformation::LongLong procLimit =
        atoLongLong(procLimitEnvVarValue);
      if (procLimit > 0) {
        memAvail = min(procLimit, memAvail);
      }
    }
  }

#if defined(__linux)
  int ierr;
  ResourceLimitType rlim;
  ierr = GetResourceLimit(RLIMIT_DATA, &rlim);
  if ((ierr == 0) && (rlim.rlim_cur != RLIM_INFINITY)) {
    memAvail =
      min((SystemInformation::LongLong)rlim.rlim_cur / 1024, memAvail);
  }

  ierr = GetResourceLimit(RLIMIT_AS, &rlim);
  if ((ierr == 0) && (rlim.rlim_cur != RLIM_INFINITY)) {
    memAvail =
      min((SystemInformation::LongLong)rlim.rlim_cur / 1024, memAvail);
  }
#elif defined(__APPLE__)
  struct rlimit rlim;
  int ierr;
  ierr = getrlimit(RLIMIT_DATA, &rlim);
  if ((ierr == 0) && (rlim.rlim_cur != RLIM_INFINITY)) {
    memAvail =
      min((SystemInformation::LongLong)rlim.rlim_cur / 1024, memAvail);
  }

  ierr = getrlimit(RLIMIT_RSS, &rlim);
  if ((ierr == 0) && (rlim.rlim_cur != RLIM_INFINITY)) {
    memAvail =
      min((SystemInformation::LongLong)rlim.rlim_cur / 1024, memAvail);
  }
#endif

  return memAvail;
}

/**
Get RAM used by all processes in the host, in units of KiB.
*/
SystemInformation::LongLong
SystemInformationImplementation::GetHostMemoryUsed()
{
#if defined(_WIN32)
#if defined(_MSC_VER) && _MSC_VER < 1300
  MEMORYSTATUS stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatus(&stat);
  return (stat.dwTotalPhys - stat.dwAvailPhys) / 1024;
#else
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);
  return (statex.ullTotalPhys - statex.ullAvailPhys) / 1024;
#endif
#elif defined(__linux)
  // First try to use MemAvailable, but it only works on newer kernels
  const char* names2[3] = { "MemTotal:", "MemAvailable:", NULL };
  SystemInformation::LongLong values2[2] = { SystemInformation::LongLong(0) };
  int ierr = GetFieldsFromFile("/proc/meminfo", names2, values2);
  if (ierr) {
    const char* names4[5] = { "MemTotal:", "MemFree:", "Buffers:", "Cached:",
                              NULL };
    SystemInformation::LongLong values4[4] = { SystemInformation::LongLong(
      0) };
    ierr = GetFieldsFromFile("/proc/meminfo", names4, values4);
    if (ierr) {
      return ierr;
    }
    SystemInformation::LongLong& memTotal = values4[0];
    SystemInformation::LongLong& memFree = values4[1];
    SystemInformation::LongLong& memBuffers = values4[2];
    SystemInformation::LongLong& memCached = values4[3];
    return memTotal - memFree - memBuffers - memCached;
  }
  SystemInformation::LongLong& memTotal = values2[0];
  SystemInformation::LongLong& memAvail = values2[1];
  return memTotal - memAvail;
#elif defined(__APPLE__)
  SystemInformation::LongLong psz = getpagesize();
  if (psz < 1) {
    return -1;
  }
  const char* names[3] = { "Pages wired down:", "Pages active:", NULL };
  SystemInformation::LongLong values[2] = { SystemInformation::LongLong(0) };
  int ierr = GetFieldsFromCommand("vm_stat", names, values);
  if (ierr) {
    return -1;
  }
  SystemInformation::LongLong& vmWired = values[0];
  SystemInformation::LongLong& vmActive = values[1];
  return ((vmActive + vmWired) * psz) / 1024;
#else
  return 0;
#endif
}

/**
Get system RAM used by the process associated with the given
process id in units of KiB.
*/
SystemInformation::LongLong
SystemInformationImplementation::GetProcMemoryUsed()
{
#if defined(_WIN32) && defined(KWSYS_SYS_HAS_PSAPI)
  long pid = GetCurrentProcessId();
  HANDLE hProc;
  hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid);
  if (hProc == 0) {
    return -1;
  }
  PROCESS_MEMORY_COUNTERS pmc;
  int ok = GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc));
  CloseHandle(hProc);
  if (!ok) {
    return -2;
  }
  return pmc.WorkingSetSize / 1024;
#elif defined(__linux)
  SystemInformation::LongLong memUsed = 0;
  int ierr = GetFieldFromFile("/proc/self/status", "VmRSS:", memUsed);
  if (ierr) {
    return -1;
  }
  return memUsed;
#elif defined(__APPLE__)
  SystemInformation::LongLong memUsed = 0;
  pid_t pid = getpid();
  std::ostringstream oss;
  oss << "ps -o rss= -p " << pid;
  FILE* file = popen(oss.str().c_str(), "r");
  if (file == 0) {
    return -1;
  }
  oss.str("");
  while (!feof(file) && !ferror(file)) {
    char buf[256] = { '\0' };
    errno = 0;
    size_t nRead = fread(buf, 1, 256, file);
    if (ferror(file) && (errno == EINTR)) {
      clearerr(file);
    }
    if (nRead)
      oss << buf;
  }
  int ierr = ferror(file);
  pclose(file);
  if (ierr) {
    return -2;
  }
  std::istringstream iss(oss.str());
  iss >> memUsed;
  return memUsed;
#else
  return 0;
#endif
}

double SystemInformationImplementation::GetLoadAverage()
{
#if defined(KWSYS_CXX_HAS_GETLOADAVG)
  double loadavg[3] = { 0.0, 0.0, 0.0 };
  if (getloadavg(loadavg, 3) > 0) {
    return loadavg[0];
  }
  return -0.0;
#elif defined(KWSYS_SYSTEMINFORMATION_USE_GetSystemTimes)
  // Old windows.h headers do not provide GetSystemTimes.
  typedef BOOL(WINAPI * GetSystemTimesType)(LPFILETIME, LPFILETIME,
                                            LPFILETIME);
  static GetSystemTimesType pGetSystemTimes =
    (GetSystemTimesType)GetProcAddress(GetModuleHandleW(L"kernel32"),
                                       "GetSystemTimes");
  FILETIME idleTime, kernelTime, userTime;
  if (pGetSystemTimes && pGetSystemTimes(&idleTime, &kernelTime, &userTime)) {
    unsigned __int64 const idleTicks = fileTimeToUInt64(idleTime);
    unsigned __int64 const totalTicks =
      fileTimeToUInt64(kernelTime) + fileTimeToUInt64(userTime);
    return calculateCPULoad(idleTicks, totalTicks) * GetNumberOfPhysicalCPU();
  }
  return -0.0;
#else
  // Not implemented on this platform.
  return -0.0;
#endif
}

/**
Get the process id of the running process.
*/
SystemInformation::LongLong SystemInformationImplementation::GetProcessId()
{
#if defined(_WIN32)
  return GetCurrentProcessId();
#elif defined(__linux) || defined(__APPLE__)
  return getpid();
#else
  return -1;
#endif
}

/**
return current program stack in a string
demangle cxx symbols if possible.
*/
std::string SystemInformationImplementation::GetProgramStack(int firstFrame,
                                                             int wholePath)
{
  std::string programStack = ""
#if !defined(KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE)
                             "WARNING: The stack could not be examined "
                             "because backtrace is not supported.\n"
#elif !defined(KWSYS_SYSTEMINFORMATION_HAS_DEBUG_BUILD)
                             "WARNING: The stack trace will not use advanced "
                             "capabilities because this is a release build.\n"
#else
#if !defined(KWSYS_SYSTEMINFORMATION_HAS_SYMBOL_LOOKUP)
                             "WARNING: Function names will not be demangled "
                             "because "
                             "dladdr is not available.\n"
#endif
#if !defined(KWSYS_SYSTEMINFORMATION_HAS_CPP_DEMANGLE)
                             "WARNING: Function names will not be demangled "
                             "because cxxabi is not available.\n"
#endif
#endif
    ;

  std::ostringstream oss;
#if defined(KWSYS_SYSTEMINFORMATION_HAS_BACKTRACE)
  void* stackSymbols[256];
  int nFrames = backtrace(stackSymbols, 256);
  for (int i = firstFrame; i < nFrames; ++i) {
    SymbolProperties symProps;
    symProps.SetReportPath(wholePath);
    symProps.Initialize(stackSymbols[i]);
    oss << symProps << std::endl;
  }
#else
  (void)firstFrame;
  (void)wholePath;
#endif
  programStack += oss.str();

  return programStack;
}

/**
when set print stack trace in response to common signals.
*/
void SystemInformationImplementation::SetStackTraceOnError(int enable)
{
#if !defined(_WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
  static int saOrigValid = 0;
  static struct sigaction saABRTOrig;
  static struct sigaction saSEGVOrig;
  static struct sigaction saTERMOrig;
  static struct sigaction saINTOrig;
  static struct sigaction saILLOrig;
  static struct sigaction saBUSOrig;
  static struct sigaction saFPEOrig;

  if (enable && !saOrigValid) {
    // save the current actions
    sigaction(SIGABRT, 0, &saABRTOrig);
    sigaction(SIGSEGV, 0, &saSEGVOrig);
    sigaction(SIGTERM, 0, &saTERMOrig);
    sigaction(SIGINT, 0, &saINTOrig);
    sigaction(SIGILL, 0, &saILLOrig);
    sigaction(SIGBUS, 0, &saBUSOrig);
    sigaction(SIGFPE, 0, &saFPEOrig);

    // enable read, disable write
    saOrigValid = 1;

    // install ours
    struct sigaction sa;
    sa.sa_sigaction = (SigAction)StacktraceSignalHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
#ifdef SA_RESTART
    sa.sa_flags |= SA_RESTART;
#endif
    sigemptyset(&sa.sa_mask);

    sigaction(SIGABRT, &sa, 0);
    sigaction(SIGSEGV, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGILL, &sa, 0);
    sigaction(SIGBUS, &sa, 0);
    sigaction(SIGFPE, &sa, 0);
  } else if (!enable && saOrigValid) {
    // restore previous actions
    sigaction(SIGABRT, &saABRTOrig, 0);
    sigaction(SIGSEGV, &saSEGVOrig, 0);
    sigaction(SIGTERM, &saTERMOrig, 0);
    sigaction(SIGINT, &saINTOrig, 0);
    sigaction(SIGILL, &saILLOrig, 0);
    sigaction(SIGBUS, &saBUSOrig, 0);
    sigaction(SIGFPE, &saFPEOrig, 0);

    // enable write, disable read
    saOrigValid = 0;
  }
#else
  // avoid warning C4100
  (void)enable;
#endif
}

bool SystemInformationImplementation::QueryWindowsMemory()
{
#if defined(_WIN32)
#if defined(_MSC_VER) && _MSC_VER < 1300
  MEMORYSTATUS ms;
  unsigned long tv, tp, av, ap;
  ms.dwLength = sizeof(ms);
  GlobalMemoryStatus(&ms);
#define MEM_VAL(value) dw##value
#else
  MEMORYSTATUSEX ms;
  DWORDLONG tv, tp, av, ap;
  ms.dwLength = sizeof(ms);
  if (0 == GlobalMemoryStatusEx(&ms)) {
    return 0;
  }
#define MEM_VAL(value) ull##value
#endif
  tv = ms.MEM_VAL(TotalPageFile);
  tp = ms.MEM_VAL(TotalPhys);
  av = ms.MEM_VAL(AvailPageFile);
  ap = ms.MEM_VAL(AvailPhys);
  this->TotalVirtualMemory = tv >> 10 >> 10;
  this->TotalPhysicalMemory = tp >> 10 >> 10;
  this->AvailableVirtualMemory = av >> 10 >> 10;
  this->AvailablePhysicalMemory = ap >> 10 >> 10;
  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryLinuxMemory()
{
#if defined(__linux)
  unsigned long tv = 0;
  unsigned long tp = 0;
  unsigned long av = 0;
  unsigned long ap = 0;

  char buffer[1024]; // for reading lines

  int linuxMajor = 0;
  int linuxMinor = 0;

  // Find the Linux kernel version first
  struct utsname unameInfo;
  int errorFlag = uname(&unameInfo);
  if (errorFlag != 0) {
    std::cout << "Problem calling uname(): " << strerror(errno) << std::endl;
    return false;
  }

  if (strlen(unameInfo.release) >= 3) {
    // release looks like "2.6.3-15mdk-i686-up-4GB"
    char majorChar = unameInfo.release[0];
    char minorChar = unameInfo.release[2];

    if (isdigit(majorChar)) {
      linuxMajor = majorChar - '0';
    }

    if (isdigit(minorChar)) {
      linuxMinor = minorChar - '0';
    }
  }

  FILE* fd = fopen("/proc/meminfo", "r");
  if (!fd) {
    std::cout << "Problem opening /proc/meminfo" << std::endl;
    return false;
  }

  if (linuxMajor >= 3 || ((linuxMajor >= 2) && (linuxMinor >= 6))) {
    // new /proc/meminfo format since kernel 2.6.x
    // Rigorously, this test should check from the developping version 2.5.x
    // that introduced the new format...

    enum
    {
      mMemTotal,
      mMemFree,
      mBuffers,
      mCached,
      mSwapTotal,
      mSwapFree
    };
    const char* format[6] = { "MemTotal:%lu kB",  "MemFree:%lu kB",
                              "Buffers:%lu kB",   "Cached:%lu kB",
                              "SwapTotal:%lu kB", "SwapFree:%lu kB" };
    bool have[6] = { false, false, false, false, false, false };
    unsigned long value[6];
    int count = 0;
    while (fgets(buffer, static_cast<int>(sizeof(buffer)), fd)) {
      for (int i = 0; i < 6; ++i) {
        if (!have[i] && sscanf(buffer, format[i], &value[i]) == 1) {
          have[i] = true;
          ++count;
        }
      }
    }
    if (count == 6) {
      this->TotalPhysicalMemory = value[mMemTotal] / 1024;
      this->AvailablePhysicalMemory =
        (value[mMemFree] + value[mBuffers] + value[mCached]) / 1024;
      this->TotalVirtualMemory = value[mSwapTotal] / 1024;
      this->AvailableVirtualMemory = value[mSwapFree] / 1024;
    } else {
      std::cout << "Problem parsing /proc/meminfo" << std::endl;
      fclose(fd);
      return false;
    }
  } else {
    // /proc/meminfo format for kernel older than 2.6.x

    unsigned long temp;
    unsigned long cachedMem;
    unsigned long buffersMem;
    // Skip "total: used:..."
    char* r = fgets(buffer, static_cast<int>(sizeof(buffer)), fd);
    int status = 0;
    if (r == buffer) {
      status += fscanf(fd, "Mem: %lu %lu %lu %lu %lu %lu\n", &tp, &temp, &ap,
                       &temp, &buffersMem, &cachedMem);
    }
    if (status == 6) {
      status += fscanf(fd, "Swap: %lu %lu %lu\n", &tv, &temp, &av);
    }
    if (status == 9) {
      this->TotalVirtualMemory = tv >> 10 >> 10;
      this->TotalPhysicalMemory = tp >> 10 >> 10;
      this->AvailableVirtualMemory = av >> 10 >> 10;
      this->AvailablePhysicalMemory =
        (ap + buffersMem + cachedMem) >> 10 >> 10;
    } else {
      std::cout << "Problem parsing /proc/meminfo" << std::endl;
      fclose(fd);
      return false;
    }
  }
  fclose(fd);

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryCygwinMemory()
{
#ifdef __CYGWIN__
  // _SC_PAGE_SIZE does return the mmap() granularity on Cygwin,
  // see http://cygwin.com/ml/cygwin/2006-06/msg00350.html
  // Therefore just use 4096 as the page size of Windows.
  long m = sysconf(_SC_PHYS_PAGES);
  if (m < 0) {
    return false;
  }
  this->TotalPhysicalMemory = m >> 8;
  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryAIXMemory()
{
#if defined(_AIX) && defined(_SC_AIX_REALMEM)
  long c = sysconf(_SC_AIX_REALMEM);
  if (c <= 0) {
    return false;
  }

  this->TotalPhysicalMemory = c / 1024;

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryMemoryBySysconf()
{
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
  // Assume the mmap() granularity as returned by _SC_PAGESIZE is also
  // the system page size. The only known system where this isn't true
  // is Cygwin.
  long p = sysconf(_SC_PHYS_PAGES);
  long m = sysconf(_SC_PAGESIZE);

  if (p < 0 || m < 0) {
    return false;
  }

  // assume pagesize is a power of 2 and smaller 1 MiB
  size_t pagediv = (1024 * 1024 / m);

  this->TotalPhysicalMemory = p;
  this->TotalPhysicalMemory /= pagediv;

#if defined(_SC_AVPHYS_PAGES)
  p = sysconf(_SC_AVPHYS_PAGES);
  if (p < 0) {
    return false;
  }

  this->AvailablePhysicalMemory = p;
  this->AvailablePhysicalMemory /= pagediv;
#endif

  return true;
#else
  return false;
#endif
}

/** Query for the memory status */
bool SystemInformationImplementation::QueryMemory()
{
  return this->QueryMemoryBySysconf();
}

/** */
size_t SystemInformationImplementation::GetTotalVirtualMemory()
{
  return this->TotalVirtualMemory;
}

/** */
size_t SystemInformationImplementation::GetAvailableVirtualMemory()
{
  return this->AvailableVirtualMemory;
}

size_t SystemInformationImplementation::GetTotalPhysicalMemory()
{
  return this->TotalPhysicalMemory;
}

/** */
size_t SystemInformationImplementation::GetAvailablePhysicalMemory()
{
  return this->AvailablePhysicalMemory;
}

/** Get Cycle differences */
SystemInformation::LongLong
SystemInformationImplementation::GetCyclesDifference(DELAY_FUNC DelayFunction,
                                                     unsigned int uiParameter)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
  unsigned __int64 stamp1, stamp2;

  stamp1 = __rdtsc();
  DelayFunction(uiParameter);
  stamp2 = __rdtsc();

  return stamp2 - stamp1;
#elif USE_ASM_INSTRUCTIONS

  unsigned int edx1, eax1;
  unsigned int edx2, eax2;

  // Calculate the frequency of the CPU instructions.
  __try {
    _asm {
      push uiParameter ; push parameter param
      mov ebx, DelayFunction ; store func in ebx

      RDTSC_INSTRUCTION

      mov esi, eax ; esi = eax
      mov edi, edx ; edi = edx

      call ebx ; call the delay functions

      RDTSC_INSTRUCTION

      pop ebx

      mov edx2, edx      ; edx2 = edx
      mov eax2, eax      ; eax2 = eax

      mov edx1, edi      ; edx2 = edi
      mov eax1, esi      ; eax2 = esi
    }
  } __except (1) {
    return -1;
  }

  return ((((__int64)edx2 << 32) + eax2) - (((__int64)edx1 << 32) + eax1));

#else
  (void)DelayFunction;
  (void)uiParameter;
  return -1;
#endif
}

/** Compute the delay overhead */
void SystemInformationImplementation::DelayOverhead(unsigned int uiMS)
{
#if defined(_WIN32)
  LARGE_INTEGER Frequency, StartCounter, EndCounter;
  __int64 x;

  // Get the frequency of the high performance counter.
  if (!QueryPerformanceFrequency(&Frequency)) {
    return;
  }
  x = Frequency.QuadPart / 1000 * uiMS;

  // Get the starting position of the counter.
  QueryPerformanceCounter(&StartCounter);

  do {
    // Get the ending position of the counter.
    QueryPerformanceCounter(&EndCounter);
  } while (EndCounter.QuadPart - StartCounter.QuadPart == x);
#endif
  (void)uiMS;
}

/** Works only for windows */
bool SystemInformationImplementation::IsSMTSupported()
{
  return this->Features.ExtendedFeatures.LogicalProcessorsPerPhysical > 1;
}

/** Return the APIC Id. Works only for windows. */
unsigned char SystemInformationImplementation::GetAPICId()
{
  int Regs[4] = { 0, 0, 0, 0 };

#if USE_CPUID
  if (!this->IsSMTSupported()) {
    return static_cast<unsigned char>(-1); // HT not supported
  }                                        // Logical processor = 1
  call_cpuid(1, Regs);
#endif

  return static_cast<unsigned char>((Regs[1] & INITIAL_APIC_ID_BITS) >> 24);
}

/** Count the number of CPUs. Works only on windows. */
void SystemInformationImplementation::CPUCountWindows()
{
#if defined(_WIN32)
  this->NumberOfPhysicalCPU = 0;
  this->NumberOfLogicalCPU = 0;

  typedef BOOL(WINAPI * GetLogicalProcessorInformationType)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
  static GetLogicalProcessorInformationType pGetLogicalProcessorInformation =
    (GetLogicalProcessorInformationType)GetProcAddress(
      GetModuleHandleW(L"kernel32"), "GetLogicalProcessorInformation");

  if (!pGetLogicalProcessorInformation) {
    // Fallback to approximate implementation on ancient Windows versions.
    SYSTEM_INFO info;
    ZeroMemory(&info, sizeof(info));
    GetSystemInfo(&info);
    this->NumberOfPhysicalCPU =
      static_cast<unsigned int>(info.dwNumberOfProcessors);
    this->NumberOfLogicalCPU = this->NumberOfPhysicalCPU;
    return;
  }

  std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> ProcInfo;
  {
    DWORD Length = 0;
    DWORD rc = pGetLogicalProcessorInformation(NULL, &Length);
    assert(FALSE == rc);
    (void)rc; // Silence unused variable warning in Borland C++ 5.81
    assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    ProcInfo.resize(Length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    rc = pGetLogicalProcessorInformation(&ProcInfo[0], &Length);
    assert(rc != FALSE);
    (void)rc; // Silence unused variable warning in Borland C++ 5.81
  }

  typedef std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION>::iterator
    pinfoIt_t;
  for (pinfoIt_t it = ProcInfo.begin(); it != ProcInfo.end(); ++it) {
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION PInfo = *it;
    if (PInfo.Relationship != RelationProcessorCore) {
      continue;
    }

    std::bitset<std::numeric_limits<ULONG_PTR>::digits> ProcMask(
      (unsigned long long)PInfo.ProcessorMask);
    unsigned int count = (unsigned int)ProcMask.count();
    if (count == 0) { // I think this should never happen, but just to be safe.
      continue;
    }
    this->NumberOfPhysicalCPU++;
    this->NumberOfLogicalCPU += (unsigned int)count;
    this->Features.ExtendedFeatures.LogicalProcessorsPerPhysical = count;
  }
  this->NumberOfPhysicalCPU = std::max(1u, this->NumberOfPhysicalCPU);
  this->NumberOfLogicalCPU = std::max(1u, this->NumberOfLogicalCPU);
#else
#endif
}

/** Return the number of logical CPUs on the system */
unsigned int SystemInformationImplementation::GetNumberOfLogicalCPU()
{
  return this->NumberOfLogicalCPU;
}

/** Return the number of physical CPUs on the system */
unsigned int SystemInformationImplementation::GetNumberOfPhysicalCPU()
{
  return this->NumberOfPhysicalCPU;
}

/** For Mac use sysctlbyname calls to find system info */
bool SystemInformationImplementation::ParseSysCtl()
{
#if defined(__APPLE__)
  char retBuf[128];
  int err = 0;
  uint64_t value = 0;
  size_t len = sizeof(value);
  sysctlbyname("hw.memsize", &value, &len, NULL, 0);
  this->TotalPhysicalMemory = static_cast<size_t>(value / 1048576);

  // Parse values for Mac
  this->AvailablePhysicalMemory = 0;
  vm_statistics_data_t vmstat;
  mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
  if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat,
                      &count) == KERN_SUCCESS) {
    len = sizeof(value);
    err = sysctlbyname("hw.pagesize", &value, &len, NULL, 0);
    int64_t available_memory = vmstat.free_count * value;
    this->AvailablePhysicalMemory =
      static_cast<size_t>(available_memory / 1048576);
  }

#ifdef VM_SWAPUSAGE
  // Virtual memory.
  int mib[2] = { CTL_VM, VM_SWAPUSAGE };
  size_t miblen = sizeof(mib) / sizeof(mib[0]);
  struct xsw_usage swap;
  len = sizeof(swap);
  err = sysctl(mib, miblen, &swap, &len, NULL, 0);
  if (err == 0) {
    this->AvailableVirtualMemory =
      static_cast<size_t>(swap.xsu_avail / 1048576);
    this->TotalVirtualMemory = static_cast<size_t>(swap.xsu_total / 1048576);
  }
#else
  this->AvailableVirtualMemory = 0;
  this->TotalVirtualMemory = 0;
#endif

  // CPU Info
  len = sizeof(this->NumberOfPhysicalCPU);
  sysctlbyname("hw.physicalcpu", &this->NumberOfPhysicalCPU, &len, NULL, 0);
  len = sizeof(this->NumberOfLogicalCPU);
  sysctlbyname("hw.logicalcpu", &this->NumberOfLogicalCPU, &len, NULL, 0);

  int cores_per_package = 0;
  len = sizeof(cores_per_package);
  err = sysctlbyname("machdep.cpu.cores_per_package", &cores_per_package, &len,
                     NULL, 0);
  // That name was not found, default to 1
  this->Features.ExtendedFeatures.LogicalProcessorsPerPhysical =
    err != 0 ? 1 : static_cast<unsigned char>(cores_per_package);

  len = sizeof(value);
  sysctlbyname("hw.cpufrequency", &value, &len, NULL, 0);
  this->CPUSpeedInMHz = static_cast<float>(value) / 1000000;

  // Chip family
  len = sizeof(this->ChipID.Family);
  // Seems only the intel chips will have this name so if this fails it is
  // probably a PPC machine
  err =
    sysctlbyname("machdep.cpu.family", &this->ChipID.Family, &len, NULL, 0);
  if (err != 0) // Go back to names we know but are less descriptive
  {
    this->ChipID.Family = 0;
    ::memset(retBuf, 0, 128);
    len = 32;
    err = sysctlbyname("hw.machine", &retBuf, &len, NULL, 0);
    std::string machineBuf(retBuf);
    if (machineBuf.find_first_of("Power") != std::string::npos) {
      this->ChipID.Vendor = "IBM";
      len = sizeof(this->ChipID.Family);
      err = sysctlbyname("hw.cputype", &this->ChipID.Family, &len, NULL, 0);
      len = sizeof(this->ChipID.Model);
      err = sysctlbyname("hw.cpusubtype", &this->ChipID.Model, &len, NULL, 0);
      this->FindManufacturer();
    }
  } else // Should be an Intel Chip.
  {
    len = sizeof(this->ChipID.Family);
    err =
      sysctlbyname("machdep.cpu.family", &this->ChipID.Family, &len, NULL, 0);

    ::memset(retBuf, 0, 128);
    len = 128;
    err = sysctlbyname("machdep.cpu.vendor", retBuf, &len, NULL, 0);
    // Chip Vendor
    this->ChipID.Vendor = retBuf;
    this->FindManufacturer();

    // Chip Model
    len = sizeof(value);
    err = sysctlbyname("machdep.cpu.model", &value, &len, NULL, 0);
    this->ChipID.Model = static_cast<int>(value);

    // Chip Stepping
    len = sizeof(value);
    value = 0;
    err = sysctlbyname("machdep.cpu.stepping", &value, &len, NULL, 0);
    if (!err) {
      this->ChipID.Revision = static_cast<int>(value);
    }

    // feature string
    char* buf = 0;
    size_t allocSize = 128;

    err = 0;
    len = 0;

    // sysctlbyname() will return with err==0 && len==0 if the buffer is too
    // small
    while (err == 0 && len == 0) {
      delete[] buf;
      allocSize *= 2;
      buf = new char[allocSize];
      if (!buf) {
        break;
      }
      buf[0] = ' ';
      len = allocSize - 2; // keep space for leading and trailing space
      err = sysctlbyname("machdep.cpu.features", buf + 1, &len, NULL, 0);
    }
    if (!err && buf && len) {
      // now we can match every flags as space + flag + space
      buf[len + 1] = ' ';
      std::string cpuflags(buf, len + 2);

      if ((cpuflags.find(" FPU ") != std::string::npos)) {
        this->Features.HasFPU = true;
      }
      if ((cpuflags.find(" TSC ") != std::string::npos)) {
        this->Features.HasTSC = true;
      }
      if ((cpuflags.find(" MMX ") != std::string::npos)) {
        this->Features.HasMMX = true;
      }
      if ((cpuflags.find(" SSE ") != std::string::npos)) {
        this->Features.HasSSE = true;
      }
      if ((cpuflags.find(" SSE2 ") != std::string::npos)) {
        this->Features.HasSSE2 = true;
      }
      if ((cpuflags.find(" APIC ") != std::string::npos)) {
        this->Features.HasAPIC = true;
      }
      if ((cpuflags.find(" CMOV ") != std::string::npos)) {
        this->Features.HasCMOV = true;
      }
      if ((cpuflags.find(" MTRR ") != std::string::npos)) {
        this->Features.HasMTRR = true;
      }
      if ((cpuflags.find(" ACPI ") != std::string::npos)) {
        this->Features.HasACPI = true;
      }
    }
    delete[] buf;
  }

  // brand string
  ::memset(retBuf, 0, sizeof(retBuf));
  len = sizeof(retBuf);
  err = sysctlbyname("machdep.cpu.brand_string", retBuf, &len, NULL, 0);
  if (!err) {
    this->ChipID.ProcessorName = retBuf;
    this->ChipID.ModelName = retBuf;
  }

  // Cache size
  len = sizeof(value);
  err = sysctlbyname("hw.l1icachesize", &value, &len, NULL, 0);
  this->Features.L1CacheSize = static_cast<int>(value);
  len = sizeof(value);
  err = sysctlbyname("hw.l2cachesize", &value, &len, NULL, 0);
  this->Features.L2CacheSize = static_cast<int>(value);

  return true;
#else
  return false;
#endif
}

/** Extract a value from sysctl command */
std::string SystemInformationImplementation::ExtractValueFromSysCtl(
  const char* word)
{
  size_t pos = this->SysCtlBuffer.find(word);
  if (pos != std::string::npos) {
    pos = this->SysCtlBuffer.find(": ", pos);
    size_t pos2 = this->SysCtlBuffer.find("\n", pos);
    if (pos != std::string::npos && pos2 != std::string::npos) {
      return this->SysCtlBuffer.substr(pos + 2, pos2 - pos - 2);
    }
  }
  return "";
}

/** Run a given process */
std::string SystemInformationImplementation::RunProcess(
  std::vector<const char*> args)
{
  std::string buffer = "";

  // Run the application
  kwsysProcess* gp = kwsysProcess_New();
  kwsysProcess_SetCommand(gp, &*args.begin());
  kwsysProcess_SetOption(gp, kwsysProcess_Option_HideWindow, 1);

  kwsysProcess_Execute(gp);

  char* data = NULL;
  int length;
  double timeout = 255;
  int pipe; // pipe id as returned by kwsysProcess_WaitForData()

  while ((static_cast<void>(
            pipe = kwsysProcess_WaitForData(gp, &data, &length, &timeout)),
          (pipe == kwsysProcess_Pipe_STDOUT ||
           pipe == kwsysProcess_Pipe_STDERR))) // wait for 1s
  {
    buffer.append(data, length);
  }
  kwsysProcess_WaitForExit(gp, 0);

  int result = 0;
  switch (kwsysProcess_GetState(gp)) {
    case kwsysProcess_State_Exited: {
      result = kwsysProcess_GetExitValue(gp);
    } break;
    case kwsysProcess_State_Error: {
      std::cerr << "Error: Could not run " << args[0] << ":\n";
      std::cerr << kwsysProcess_GetErrorString(gp) << "\n";
    } break;
    case kwsysProcess_State_Exception: {
      std::cerr << "Error: " << args[0] << " terminated with an exception: "
                << kwsysProcess_GetExceptionString(gp) << "\n";
    } break;
    case kwsysProcess_State_Starting:
    case kwsysProcess_State_Executing:
    case kwsysProcess_State_Expired:
    case kwsysProcess_State_Killed: {
      // Should not get here.
      std::cerr << "Unexpected ending state after running " << args[0]
                << std::endl;
    } break;
  }
  kwsysProcess_Delete(gp);
  if (result) {
    std::cerr << "Error " << args[0] << " returned :" << result << "\n";
  }
  return buffer;
}

std::string SystemInformationImplementation::ParseValueFromKStat(
  const char* arguments)
{
  std::vector<const char*> args;
  args.clear();
  args.push_back("kstat");
  args.push_back("-p");

  std::string command = arguments;
  size_t start = std::string::npos;
  size_t pos = command.find(' ', 0);
  while (pos != std::string::npos) {
    bool inQuotes = false;
    // Check if we are between quotes
    size_t b0 = command.find('"', 0);
    size_t b1 = command.find('"', b0 + 1);
    while (b0 != std::string::npos && b1 != std::string::npos && b1 > b0) {
      if (pos > b0 && pos < b1) {
        inQuotes = true;
        break;
      }
      b0 = command.find('"', b1 + 1);
      b1 = command.find('"', b0 + 1);
    }

    if (!inQuotes) {
      std::string arg = command.substr(start + 1, pos - start - 1);

      // Remove the quotes if any
      size_t quotes = arg.find('"');
      while (quotes != std::string::npos) {
        arg.erase(quotes, 1);
        quotes = arg.find('"');
      }
      args.push_back(arg.c_str());
      start = pos;
    }
    pos = command.find(' ', pos + 1);
  }
  std::string lastArg = command.substr(start + 1, command.size() - start - 1);
  args.push_back(lastArg.c_str());

  args.push_back(0);

  std::string buffer = this->RunProcess(args);

  std::string value = "";
  for (size_t i = buffer.size() - 1; i > 0; i--) {
    if (buffer[i] == ' ' || buffer[i] == '\t') {
      break;
    }
    if (buffer[i] != '\n' && buffer[i] != '\r') {
      std::string val = value;
      value = buffer[i];
      value += val;
    }
  }
  return value;
}

/** Querying for system information from Solaris */
bool SystemInformationImplementation::QuerySolarisMemory()
{
#if defined(__SVR4) && defined(__sun)
// Solaris allows querying this value by sysconf, but if this is
// a 32 bit process on a 64 bit host the returned memory will be
// limited to 4GiB. So if this is a 32 bit process or if the sysconf
// method fails use the kstat interface.
#if SIZEOF_VOID_P == 8
  if (this->QueryMemoryBySysconf()) {
    return true;
  }
#endif

  char* tail;
  unsigned long totalMemory =
    strtoul(this->ParseValueFromKStat("-s physmem").c_str(), &tail, 0);
  this->TotalPhysicalMemory = totalMemory / 128;

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QuerySolarisProcessor()
{
  if (!this->QueryProcessorBySysconf()) {
    return false;
  }

  // Parse values
  this->CPUSpeedInMHz = static_cast<float>(
    atoi(this->ParseValueFromKStat("-s clock_MHz").c_str()));

  // Chip family
  this->ChipID.Family = 0;

  // Chip Model
  this->ChipID.ProcessorName = this->ParseValueFromKStat("-s cpu_type");
  this->ChipID.Model = 0;

  // Chip Vendor
  if (this->ChipID.ProcessorName != "i386") {
    this->ChipID.Vendor = "Sun";
    this->FindManufacturer();
  }

  return true;
}

/** Querying for system information from Haiku OS */
bool SystemInformationImplementation::QueryHaikuInfo()
{
#if defined(__HAIKU__)

  // CPU count
  system_info info;
  get_system_info(&info);
  this->NumberOfPhysicalCPU = info.cpu_count;

  // CPU speed
  uint32 topologyNodeCount = 0;
  cpu_topology_node_info* topology = 0;
  get_cpu_topology_info(0, &topologyNodeCount);
  if (topologyNodeCount != 0)
    topology = new cpu_topology_node_info[topologyNodeCount];
  get_cpu_topology_info(topology, &topologyNodeCount);

  for (uint32 i = 0; i < topologyNodeCount; i++) {
    if (topology[i].type == B_TOPOLOGY_CORE) {
      this->CPUSpeedInMHz =
        topology[i].data.core.default_frequency / 1000000.0f;
      break;
    }
  }

  delete[] topology;

  // Physical Memory
  this->TotalPhysicalMemory = (info.max_pages * B_PAGE_SIZE) / (1024 * 1024);
  this->AvailablePhysicalMemory = this->TotalPhysicalMemory -
    ((info.used_pages * B_PAGE_SIZE) / (1024 * 1024));

  // NOTE: get_system_info_etc is currently a private call so just set to 0
  // until it becomes public
  this->TotalVirtualMemory = 0;
  this->AvailableVirtualMemory = 0;

  // Retrieve cpuid_info union for cpu 0
  cpuid_info cpu_info;
  get_cpuid(&cpu_info, 0, 0);

  // Chip Vendor
  // Use a temporary buffer so that we can add NULL termination to the string
  char vbuf[13];
  strncpy(vbuf, cpu_info.eax_0.vendor_id, 12);
  vbuf[12] = '\0';
  this->ChipID.Vendor = vbuf;

  this->FindManufacturer();

  // Retrieve cpuid_info union for cpu 0 this time using a register value of 1
  get_cpuid(&cpu_info, 1, 0);

  this->NumberOfLogicalCPU = cpu_info.eax_1.logical_cpus;

  // Chip type
  this->ChipID.Type = cpu_info.eax_1.type;

  // Chip family
  this->ChipID.Family = cpu_info.eax_1.family;

  // Chip Model
  this->ChipID.Model = cpu_info.eax_1.model;

  // Chip Revision
  this->ChipID.Revision = cpu_info.eax_1.stepping;

  // Chip Extended Family
  this->ChipID.ExtendedFamily = cpu_info.eax_1.extended_family;

  // Chip Extended Model
  this->ChipID.ExtendedModel = cpu_info.eax_1.extended_model;

  // Get ChipID.ProcessorName from other information already gathered
  this->RetrieveClassicalCPUIdentity();

  // Cache size
  this->Features.L1CacheSize = 0;
  this->Features.L2CacheSize = 0;

  return true;

#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryQNXMemory()
{
#if defined(__QNX__)
  std::string buffer;
  std::vector<const char*> args;
  args.clear();

  args.push_back("showmem");
  args.push_back("-S");
  args.push_back(0);
  buffer = this->RunProcess(args);
  args.clear();

  size_t pos = buffer.find("System RAM:");
  if (pos == std::string::npos)
    return false;
  pos = buffer.find(":", pos);
  size_t pos2 = buffer.find("M (", pos);
  if (pos2 == std::string::npos)
    return false;

  pos++;
  while (buffer[pos] == ' ')
    pos++;

  this->TotalPhysicalMemory = atoi(buffer.substr(pos, pos2 - pos).c_str());
  return true;
#endif
  return false;
}

bool SystemInformationImplementation::QueryBSDMemory()
{
#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) ||    \
  defined(__DragonFly__)
  int ctrl[2] = { CTL_HW, HW_PHYSMEM };
#if defined(HW_PHYSMEM64)
  int64_t k;
  ctrl[1] = HW_PHYSMEM64;
#else
  int k;
#endif
  size_t sz = sizeof(k);

  if (sysctl(ctrl, 2, &k, &sz, NULL, 0) != 0) {
    return false;
  }

  this->TotalPhysicalMemory = k >> 10 >> 10;

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryQNXProcessor()
{
#if defined(__QNX__)
  // the output on my QNX 6.4.1 looks like this:
  // Processor1: 686 Pentium II Stepping 3 2175MHz FPU
  std::string buffer;
  std::vector<const char*> args;
  args.clear();

  args.push_back("pidin");
  args.push_back("info");
  args.push_back(0);
  buffer = this->RunProcess(args);
  args.clear();

  size_t pos = buffer.find("Processor1:");
  if (pos == std::string::npos)
    return false;

  size_t pos2 = buffer.find("MHz", pos);
  if (pos2 == std::string::npos)
    return false;

  size_t pos3 = pos2;
  while (buffer[pos3] != ' ')
    --pos3;

  this->CPUSpeedInMHz = atoi(buffer.substr(pos3 + 1, pos2 - pos3 - 1).c_str());

  pos2 = buffer.find(" Stepping", pos);
  if (pos2 != std::string::npos) {
    pos2 = buffer.find(" ", pos2 + 1);
    if (pos2 != std::string::npos && pos2 < pos3) {
      this->ChipID.Revision =
        atoi(buffer.substr(pos2 + 1, pos3 - pos2).c_str());
    }
  }

  this->NumberOfPhysicalCPU = 0;
  do {
    pos = buffer.find("\nProcessor", pos + 1);
    ++this->NumberOfPhysicalCPU;
  } while (pos != std::string::npos);
  this->NumberOfLogicalCPU = 1;

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryBSDProcessor()
{
#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__) ||    \
  defined(__DragonFly__)
  int k;
  size_t sz = sizeof(k);
  int ctrl[2] = { CTL_HW, HW_NCPU };

  if (sysctl(ctrl, 2, &k, &sz, NULL, 0) != 0) {
    return false;
  }

  this->NumberOfPhysicalCPU = k;
  this->NumberOfLogicalCPU = this->NumberOfPhysicalCPU;

#if defined(HW_CPUSPEED)
  ctrl[1] = HW_CPUSPEED;

  if (sysctl(ctrl, 2, &k, &sz, NULL, 0) != 0) {
    return false;
  }

  this->CPUSpeedInMHz = (float)k;
#endif

#if defined(CPU_SSE)
  ctrl[0] = CTL_MACHDEP;
  ctrl[1] = CPU_SSE;

  if (sysctl(ctrl, 2, &k, &sz, NULL, 0) != 0) {
    return false;
  }

  this->Features.HasSSE = (k > 0);
#endif

#if defined(CPU_SSE2)
  ctrl[0] = CTL_MACHDEP;
  ctrl[1] = CPU_SSE2;

  if (sysctl(ctrl, 2, &k, &sz, NULL, 0) != 0) {
    return false;
  }

  this->Features.HasSSE2 = (k > 0);
#endif

#if defined(CPU_CPUVENDOR)
  ctrl[0] = CTL_MACHDEP;
  ctrl[1] = CPU_CPUVENDOR;
  char vbuf[25];
  ::memset(vbuf, 0, sizeof(vbuf));
  sz = sizeof(vbuf) - 1;
  if (sysctl(ctrl, 2, vbuf, &sz, NULL, 0) != 0) {
    return false;
  }

  this->ChipID.Vendor = vbuf;
  this->FindManufacturer();
#endif

  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryHPUXMemory()
{
#if defined(__hpux)
  unsigned long tv = 0;
  unsigned long tp = 0;
  unsigned long av = 0;
  unsigned long ap = 0;
  struct pst_static pst;
  struct pst_dynamic pdy;

  unsigned long ps = 0;
  if (pstat_getstatic(&pst, sizeof(pst), (size_t)1, 0) == -1) {
    return false;
  }

  ps = pst.page_size;
  tp = pst.physical_memory * ps;
  tv = (pst.physical_memory + pst.pst_maxmem) * ps;
  if (pstat_getdynamic(&pdy, sizeof(pdy), (size_t)1, 0) == -1) {
    return false;
  }

  ap = tp - pdy.psd_rm * ps;
  av = tv - pdy.psd_vm;
  this->TotalVirtualMemory = tv >> 10 >> 10;
  this->TotalPhysicalMemory = tp >> 10 >> 10;
  this->AvailableVirtualMemory = av >> 10 >> 10;
  this->AvailablePhysicalMemory = ap >> 10 >> 10;
  return true;
#else
  return false;
#endif
}

bool SystemInformationImplementation::QueryHPUXProcessor()
{
#if defined(__hpux)
#if defined(KWSYS_SYS_HAS_MPCTL_H)
  int c = mpctl(MPC_GETNUMSPUS_SYS, 0, 0);
  if (c <= 0) {
    return false;
  }

  this->NumberOfPhysicalCPU = c;
  this->NumberOfLogicalCPU = this->NumberOfPhysicalCPU;

  long t = sysconf(_SC_CPU_VERSION);

  if (t == -1) {
    return false;
  }

  switch (t) {
    case CPU_PA_RISC1_0:
      this->ChipID.Vendor = "Hewlett-Packard";
      this->ChipID.Family = 0x100;
      break;
    case CPU_PA_RISC1_1:
      this->ChipID.Vendor = "Hewlett-Packard";
      this->ChipID.Family = 0x110;
      break;
    case CPU_PA_RISC2_0:
      this->ChipID.Vendor = "Hewlett-Packard";
      this->ChipID.Family = 0x200;
      break;
#if defined(CPU_HP_INTEL_EM_1_0) || defined(CPU_IA64_ARCHREV_0)
#ifdef CPU_HP_INTEL_EM_1_0
    case CPU_HP_INTEL_EM_1_0:
#endif
#ifdef CPU_IA64_ARCHREV_0
    case CPU_IA64_ARCHREV_0:
#endif
      this->ChipID.Vendor = "GenuineIntel";
      this->Features.HasIA64 = true;
      break;
#endif
    default:
      return false;
  }

  this->FindManufacturer();

  return true;
#else
  return false;
#endif
#else
  return false;
#endif
}

/** Query the operating system information */
bool SystemInformationImplementation::QueryOSInformation()
{
#if defined(_WIN32)

  this->OSName = "Windows";

  OSVERSIONINFOEXW osvi;
  BOOL bIsWindows64Bit;
  BOOL bOsVersionInfoEx;
  char operatingSystem[256];

  // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
#ifdef KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#pragma warning(push)
#ifdef __INTEL_COMPILER
#pragma warning(disable : 1478)
#else
#pragma warning(disable : 4996)
#endif
#endif
  bOsVersionInfoEx = GetVersionExW((OSVERSIONINFOW*)&osvi);
  if (!bOsVersionInfoEx) {
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    if (!GetVersionExW((OSVERSIONINFOW*)&osvi)) {
      return false;
    }
  }
#ifdef KWSYS_WINDOWS_DEPRECATED_GetVersionEx
#pragma warning(pop)
#endif

  switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
      // Test for the product.
      if (osvi.dwMajorVersion <= 4) {
        this->OSRelease = "NT";
      }
      if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
        this->OSRelease = "2000";
      }
      if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
        this->OSRelease = "XP";
      }
      // XP Professional x64
      if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2) {
        this->OSRelease = "XP";
      }
#ifdef VER_NT_WORKSTATION
      // Test for product type.
      if (bOsVersionInfoEx) {
        if (osvi.wProductType == VER_NT_WORKSTATION) {
          if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0) {
            this->OSRelease = "Vista";
          }
          if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) {
            this->OSRelease = "7";
          }
// VER_SUITE_PERSONAL may not be defined
#ifdef VER_SUITE_PERSONAL
          else {
            if (osvi.wSuiteMask & VER_SUITE_PERSONAL) {
              this->OSRelease += " Personal";
            } else {
              this->OSRelease += " Professional";
            }
          }
#endif
        } else if (osvi.wProductType == VER_NT_SERVER) {
          // Check for .NET Server instead of Windows XP.
          if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
            this->OSRelease = ".NET";
          }

          // Continue with the type detection.
          if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
            this->OSRelease += " DataCenter Server";
          } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
            this->OSRelease += " Advanced Server";
          } else {
            this->OSRelease += " Server";
          }
        }

        sprintf(operatingSystem, "%ls (Build %ld)", osvi.szCSDVersion,
                osvi.dwBuildNumber & 0xFFFF);
        this->OSVersion = operatingSystem;
      } else
#endif // VER_NT_WORKSTATION
      {
        HKEY hKey;
        wchar_t szProductType[80];
        DWORD dwBufLen;

        // Query the registry to retrieve information.
        RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0,
                      KEY_QUERY_VALUE, &hKey);
        RegQueryValueExW(hKey, L"ProductType", NULL, NULL,
                         (LPBYTE)szProductType, &dwBufLen);
        RegCloseKey(hKey);

        if (lstrcmpiW(L"WINNT", szProductType) == 0) {
          this->OSRelease += " Professional";
        }
        if (lstrcmpiW(L"LANMANNT", szProductType) == 0) {
          // Decide between Windows 2000 Advanced Server and Windows .NET
          // Enterprise Server.
          if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
            this->OSRelease += " Standard Server";
          } else {
            this->OSRelease += " Server";
          }
        }
        if (lstrcmpiW(L"SERVERNT", szProductType) == 0) {
          // Decide between Windows 2000 Advanced Server and Windows .NET
          // Enterprise Server.
          if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
            this->OSRelease += " Enterprise Server";
          } else {
            this->OSRelease += " Advanced Server";
          }
        }
      }

      // Display version, service pack (if any), and build number.
      if (osvi.dwMajorVersion <= 4) {
        // NB: NT 4.0 and earlier.
        sprintf(operatingSystem, "version %ld.%ld %ls (Build %ld)",
                osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.szCSDVersion,
                osvi.dwBuildNumber & 0xFFFF);
        this->OSVersion = operatingSystem;
      } else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
        // Windows XP and .NET server.
        typedef BOOL(CALLBACK * LPFNPROC)(HANDLE, BOOL*);
        HINSTANCE hKernelDLL;
        LPFNPROC DLLProc;

        // Load the Kernel32 DLL.
        hKernelDLL = LoadLibraryW(L"kernel32");
        if (hKernelDLL != NULL) {
          // Only XP and .NET Server support IsWOW64Process so... Load
          // dynamically!
          DLLProc = (LPFNPROC)GetProcAddress(hKernelDLL, "IsWow64Process");

          // If the function address is valid, call the function.
          if (DLLProc != NULL)
            (DLLProc)(GetCurrentProcess(), &bIsWindows64Bit);
          else
            bIsWindows64Bit = false;

          // Free the DLL module.
          FreeLibrary(hKernelDLL);
        }
      } else {
        // Windows 2000 and everything else.
        sprintf(operatingSystem, "%ls (Build %ld)", osvi.szCSDVersion,
                osvi.dwBuildNumber & 0xFFFF);
        this->OSVersion = operatingSystem;
      }
      break;

    case VER_PLATFORM_WIN32_WINDOWS:
      // Test for the product.
      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
        this->OSRelease = "95";
        if (osvi.szCSDVersion[1] == 'C') {
          this->OSRelease += "OSR 2.5";
        } else if (osvi.szCSDVersion[1] == 'B') {
          this->OSRelease += "OSR 2";
        }
      }

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
        this->OSRelease = "98";
        if (osvi.szCSDVersion[1] == 'A') {
          this->OSRelease += "SE";
        }
      }

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
        this->OSRelease = "Me";
      }
      break;

    case VER_PLATFORM_WIN32s:
      this->OSRelease = "Win32s";
      break;

    default:
      this->OSRelease = "Unknown";
      break;
  }

  // Get the hostname
  WORD wVersionRequested;
  WSADATA wsaData;
  char name[255];
  wVersionRequested = MAKEWORD(2, 0);

  if (WSAStartup(wVersionRequested, &wsaData) == 0) {
    gethostname(name, sizeof(name));
    WSACleanup();
  }
  this->Hostname = name;

  const char* arch = getenv("PROCESSOR_ARCHITECTURE");
  const char* wow64 = getenv("PROCESSOR_ARCHITEW6432");
  if (arch) {
    this->OSPlatform = arch;
  }

  if (wow64) {
    // the PROCESSOR_ARCHITEW6432 is only defined when running 32bit programs
    // on 64bit OS
    this->OSIs64Bit = true;
  } else if (arch) {
    // all values other than x86 map to 64bit architectures
    this->OSIs64Bit = (strncmp(arch, "x86", 3) != 0);
  }

#else

  struct utsname unameInfo;
  int errorFlag = uname(&unameInfo);
  if (errorFlag == 0) {
    this->OSName = unameInfo.sysname;
    this->Hostname = unameInfo.nodename;
    this->OSRelease = unameInfo.release;
    this->OSVersion = unameInfo.version;
    this->OSPlatform = unameInfo.machine;

    // This is still insufficient to capture 64bit architecture such
    // powerpc and possible mips and sparc
    if (this->OSPlatform.find_first_of("64") != std::string::npos) {
      this->OSIs64Bit = true;
    }
  }

#ifdef __APPLE__
  this->OSName = "Unknown Apple OS";
  this->OSRelease = "Unknown product version";
  this->OSVersion = "Unknown build version";

  this->CallSwVers("-productName", this->OSName);
  this->CallSwVers("-productVersion", this->OSRelease);
  this->CallSwVers("-buildVersion", this->OSVersion);
#endif

#endif

  return true;
}

int SystemInformationImplementation::CallSwVers(const char* arg,
                                                std::string& ver)
{
#ifdef __APPLE__
  std::vector<const char*> args;
  args.push_back("sw_vers");
  args.push_back(arg);
  args.push_back(0);
  ver = this->RunProcess(args);
  this->TrimNewline(ver);
#else
  // avoid C4100
  (void)arg;
  (void)ver;
#endif
  return 0;
}

void SystemInformationImplementation::TrimNewline(std::string& output)
{
  // remove \r
  std::string::size_type pos = 0;
  while ((pos = output.find("\r", pos)) != std::string::npos) {
    output.erase(pos);
  }

  // remove \n
  pos = 0;
  while ((pos = output.find("\n", pos)) != std::string::npos) {
    output.erase(pos);
  }
}

/** Return true if the machine is 64 bits */
bool SystemInformationImplementation::Is64Bits()
{
  return this->OSIs64Bit;
}
}
