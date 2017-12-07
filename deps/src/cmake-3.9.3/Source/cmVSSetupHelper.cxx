/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVSSetupHelper.h"
#include "cmSystemTools.h"

#ifndef VSSetupConstants
#define VSSetupConstants
/* clang-format off */
const IID IID_ISetupConfiguration = {
  0x42843719, 0xDB4C, 0x46C2,
  { 0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B }
};
const IID IID_ISetupConfiguration2 = {
  0x26AAB78C, 0x4A60, 0x49D6,
  { 0xAF, 0x3B, 0x3C, 0x35, 0xBC, 0x93, 0x36, 0x5D }
};
const IID IID_ISetupPackageReference = {
  0xda8d8a16, 0xb2b6, 0x4487,
  { 0xa2, 0xf1, 0x59, 0x4c, 0xcc, 0xcd, 0x6b, 0xf5 }
};
const IID IID_ISetupHelper = {
  0x42b21b78, 0x6192, 0x463e,
  { 0x87, 0xbf, 0xd5, 0x77, 0x83, 0x8f, 0x1d, 0x5c }
};
const IID IID_IEnumSetupInstances = {
  0x6380BCFF, 0x41D3, 0x4B2E,
  { 0x8B, 0x2E, 0xBF, 0x8A, 0x68, 0x10, 0xC8, 0x48 }
};
const IID IID_ISetupInstance2 = {
  0x89143C9A, 0x05AF, 0x49B0,
  { 0xB7, 0x17, 0x72, 0xE2, 0x18, 0xA2, 0x18, 0x5C }
};
const IID IID_ISetupInstance = {
  0xB41463C3, 0x8866, 0x43B5,
  { 0xBC, 0x33, 0x2B, 0x06, 0x76, 0xF7, 0xF4, 0x2E }
};
const CLSID CLSID_SetupConfiguration = {
  0x177F0C4A, 0x1CD3, 0x4DE7,
  { 0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D }
};
/* clang-format on */
#endif

const WCHAR* VCToolsetComponent =
  L"Microsoft.VisualStudio.Component.VC.Tools.x86.x64";
const WCHAR* Win10SDKComponent =
  L"Microsoft.VisualStudio.Component.Windows10SDK";
const WCHAR* Win81SDKComponent =
  L"Microsoft.VisualStudio.Component.Windows81SDK";
const WCHAR* ComponentType = L"Component";

cmVSSetupAPIHelper::cmVSSetupAPIHelper()
  : setupConfig(NULL)
  , setupConfig2(NULL)
  , setupHelper(NULL)
  , initializationFailure(false)
{
  comInitialized = CoInitializeEx(NULL, 0);
  if (SUCCEEDED(comInitialized)) {
    Initialize();
  } else {
    initializationFailure = true;
  }
}

cmVSSetupAPIHelper::~cmVSSetupAPIHelper()
{
  setupHelper = NULL;
  setupConfig2 = NULL;
  setupConfig = NULL;
  if (SUCCEEDED(comInitialized))
    CoUninitialize();
}

bool cmVSSetupAPIHelper::IsVS2017Installed()
{
  return this->EnumerateAndChooseVSInstance();
}

bool cmVSSetupAPIHelper::IsWin10SDKInstalled()
{
  return (this->EnumerateAndChooseVSInstance() &&
          chosenInstanceInfo.IsWin10SDKInstalled);
}

bool cmVSSetupAPIHelper::IsWin81SDKInstalled()
{
  return (this->EnumerateAndChooseVSInstance() &&
          chosenInstanceInfo.IsWin81SDKInstalled);
}

bool cmVSSetupAPIHelper::CheckInstalledComponent(
  SmartCOMPtr<ISetupPackageReference> package, bool& bVCToolset,
  bool& bWin10SDK, bool& bWin81SDK)
{
  bool ret = false;
  bVCToolset = bWin10SDK = bWin81SDK = false;
  SmartBSTR bstrId;
  if (FAILED(package->GetId(&bstrId))) {
    return ret;
  }

  SmartBSTR bstrType;
  if (FAILED(package->GetType(&bstrType))) {
    return ret;
  }

  std::wstring id = std::wstring(bstrId);
  std::wstring type = std::wstring(bstrType);
  if (id.compare(VCToolsetComponent) == 0 &&
      type.compare(ComponentType) == 0) {
    bVCToolset = true;
    ret = true;
  }

  // Checks for any version of Win10 SDK. The version is appended at the end of
  // the
  // component name ex: Microsoft.VisualStudio.Component.Windows10SDK.10240
  if (id.find(Win10SDKComponent) != std::wstring::npos &&
      type.compare(ComponentType) == 0) {
    bWin10SDK = true;
    ret = true;
  }

  if (id.compare(Win81SDKComponent) == 0 && type.compare(ComponentType) == 0) {
    bWin81SDK = true;
    ret = true;
  }

  return ret;
}

// Gather additional info such as if VCToolset, WinSDKs are installed, location
// of VS and version information.
bool cmVSSetupAPIHelper::GetVSInstanceInfo(
  SmartCOMPtr<ISetupInstance2> pInstance, VSInstanceInfo& vsInstanceInfo)
{
  bool isVCToolSetInstalled = false;
  if (pInstance == NULL)
    return false;

  SmartBSTR bstrId;
  if (SUCCEEDED(pInstance->GetInstanceId(&bstrId))) {
    vsInstanceInfo.InstanceId = std::wstring(bstrId);
  } else {
    return false;
  }

  InstanceState state;
  if (FAILED(pInstance->GetState(&state))) {
    return false;
  }

  ULONGLONG ullVersion = 0;
  SmartBSTR bstrVersion;
  if (FAILED(pInstance->GetInstallationVersion(&bstrVersion))) {
    return false;
  } else {
    vsInstanceInfo.Version = std::wstring(bstrVersion);
    if (FAILED(setupHelper->ParseVersion(bstrVersion, &ullVersion))) {
      vsInstanceInfo.ullVersion = 0;
    } else {
      vsInstanceInfo.ullVersion = ullVersion;
    }
  }

  // Reboot may have been required before the installation path was created.
  SmartBSTR bstrInstallationPath;
  if ((eLocal & state) == eLocal) {
    if (FAILED(pInstance->GetInstallationPath(&bstrInstallationPath))) {
      return false;
    } else {
      vsInstanceInfo.VSInstallLocation = std::wstring(bstrInstallationPath);
    }
  }

  // Reboot may have been required before the product package was registered
  // (last).
  if ((eRegistered & state) == eRegistered) {
    SmartCOMPtr<ISetupPackageReference> product;
    if (FAILED(pInstance->GetProduct(&product)) || !product) {
      return false;
    }

    LPSAFEARRAY lpsaPackages;
    if (FAILED(pInstance->GetPackages(&lpsaPackages)) ||
        lpsaPackages == NULL) {
      return false;
    }

    int lower = lpsaPackages->rgsabound[0].lLbound;
    int upper = lpsaPackages->rgsabound[0].cElements + lower;

    IUnknown** ppData = (IUnknown**)lpsaPackages->pvData;
    for (int i = lower; i < upper; i++) {
      SmartCOMPtr<ISetupPackageReference> package = NULL;
      if (FAILED(ppData[i]->QueryInterface(IID_ISetupPackageReference,
                                           (void**)&package)) ||
          package == NULL)
        continue;

      bool vcToolsetInstalled = false, win10SDKInstalled = false,
           win81SDkInstalled = false;
      bool ret = CheckInstalledComponent(package, vcToolsetInstalled,
                                         win10SDKInstalled, win81SDkInstalled);
      if (ret) {
        isVCToolSetInstalled |= vcToolsetInstalled;
        vsInstanceInfo.IsWin10SDKInstalled |= win10SDKInstalled;
        vsInstanceInfo.IsWin81SDKInstalled |= win81SDkInstalled;
      }
    }

    SafeArrayDestroy(lpsaPackages);
  }

  return isVCToolSetInstalled;
}

bool cmVSSetupAPIHelper::GetVSInstanceInfo(std::string& vsInstallLocation)
{
  vsInstallLocation = "";
  bool isInstalled = this->EnumerateAndChooseVSInstance();

  if (isInstalled) {
    std::string str(chosenInstanceInfo.VSInstallLocation.begin(),
                    chosenInstanceInfo.VSInstallLocation.end());
    vsInstallLocation = str;
  }

  return isInstalled;
}

bool cmVSSetupAPIHelper::EnumerateAndChooseVSInstance()
{
  bool isVSInstanceExists = false;
  if (chosenInstanceInfo.VSInstallLocation.compare(L"") != 0) {
    return true;
  }

  if (initializationFailure || setupConfig == NULL || setupConfig2 == NULL ||
      setupHelper == NULL)
    return false;

  std::string envVSCommonToolsDir;

  // FIXME: When we support VS versions beyond 2017, the version
  // to choose will be passed in by the caller.  We need to map that
  // to a per-version name of this environment variable.
  if (cmSystemTools::GetEnv("VS150COMNTOOLS", envVSCommonToolsDir)) {
    cmSystemTools::ConvertToUnixSlashes(envVSCommonToolsDir);
  }
  // FIXME: If the environment variable value changes between runs
  // of CMake within a given build tree the results are not defined.
  // Instead we should save a CMAKE_GENERATOR_INSTANCE value in the cache
  // (similar to CMAKE_GENERATOR_TOOLSET) to hold it persistently.
  // Unfortunately doing so will require refactoring elsewhere in
  // order to make sure the value is available in time to create
  // the generator.

  std::vector<VSInstanceInfo> vecVSInstances;
  SmartCOMPtr<IEnumSetupInstances> enumInstances = NULL;
  if (FAILED(
        setupConfig2->EnumInstances((IEnumSetupInstances**)&enumInstances)) ||
      !enumInstances) {
    return false;
  }

  SmartCOMPtr<ISetupInstance> instance;
  while (SUCCEEDED(enumInstances->Next(1, &instance, NULL)) && instance) {
    SmartCOMPtr<ISetupInstance2> instance2 = NULL;
    if (FAILED(
          instance->QueryInterface(IID_ISetupInstance2, (void**)&instance2)) ||
        !instance2) {
      instance = NULL;
      continue;
    }

    VSInstanceInfo instanceInfo;
    bool isInstalled = GetVSInstanceInfo(instance2, instanceInfo);
    instance = instance2 = NULL;

    if (isInstalled) {
      if (!envVSCommonToolsDir.empty()) {
        std::string currentVSLocation(instanceInfo.VSInstallLocation.begin(),
                                      instanceInfo.VSInstallLocation.end());
        cmSystemTools::ConvertToUnixSlashes(currentVSLocation);
        currentVSLocation += "/Common7/Tools";
        if (cmSystemTools::ComparePath(currentVSLocation,
                                       envVSCommonToolsDir)) {
          chosenInstanceInfo = instanceInfo;
          return true;
        }
      }
      vecVSInstances.push_back(instanceInfo);
    }
  }

  if (vecVSInstances.size() > 0) {
    isVSInstanceExists = true;
    int index = ChooseVSInstance(vecVSInstances);
    chosenInstanceInfo = vecVSInstances[index];
  }

  return isVSInstanceExists;
}

int cmVSSetupAPIHelper::ChooseVSInstance(
  const std::vector<VSInstanceInfo>& vecVSInstances)
{
  if (vecVSInstances.size() == 0)
    return -1;

  if (vecVSInstances.size() == 1)
    return 0;

  unsigned int chosenIndex = 0;
  for (unsigned int i = 1; i < vecVSInstances.size(); i++) {
    // If the current has Win10 SDK but not the chosen one, then choose the
    // current VS instance
    if (!vecVSInstances[chosenIndex].IsWin10SDKInstalled &&
        vecVSInstances[i].IsWin10SDKInstalled) {
      chosenIndex = i;
      continue;
    }

    // If the chosen one has Win10 SDK but the current one is not, then look at
    // the next VS instance even the current
    // instance version may be higher
    if (vecVSInstances[chosenIndex].IsWin10SDKInstalled &&
        !vecVSInstances[i].IsWin10SDKInstalled) {
      continue;
    }

    // If both chosen one and current one doesn't have Win10 SDK but the
    // current one has Win8.1 SDK installed,
    // then choose the current one
    if (!vecVSInstances[chosenIndex].IsWin10SDKInstalled &&
        !vecVSInstances[i].IsWin10SDKInstalled &&
        !vecVSInstances[chosenIndex].IsWin81SDKInstalled &&
        vecVSInstances[i].IsWin81SDKInstalled) {
      chosenIndex = i;
      continue;
    }

    // If there is no difference in WinSDKs then look for the highest version
    // of installed VS
    if ((vecVSInstances[chosenIndex].IsWin10SDKInstalled ==
         vecVSInstances[i].IsWin10SDKInstalled) &&
        (vecVSInstances[chosenIndex].IsWin81SDKInstalled ==
         vecVSInstances[i].IsWin81SDKInstalled) &&
        vecVSInstances[chosenIndex].Version < vecVSInstances[i].Version) {
      chosenIndex = i;
      continue;
    }
  }

  return chosenIndex;
}

bool cmVSSetupAPIHelper::Initialize()
{
  if (initializationFailure)
    return false;

  if (FAILED(comInitialized)) {
    initializationFailure = true;
    return false;
  }

  if (FAILED(setupConfig.CoCreateInstance(CLSID_SetupConfiguration, NULL,
                                          IID_ISetupConfiguration,
                                          CLSCTX_INPROC_SERVER)) ||
      setupConfig == NULL) {
    initializationFailure = true;
    return false;
  }

  if (FAILED(setupConfig.QueryInterface(IID_ISetupConfiguration2,
                                        (void**)&setupConfig2)) ||
      setupConfig2 == NULL) {
    initializationFailure = true;
    return false;
  }

  if (FAILED(
        setupConfig.QueryInterface(IID_ISetupHelper, (void**)&setupHelper)) ||
      setupHelper == NULL) {
    initializationFailure = true;
    return false;
  }

  initializationFailure = false;
  return true;
}
