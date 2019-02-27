#include <windows.h>

#include <msi.h>
#include <msiquery.h>

#include <string>
#include <vector>

std::wstring get_property(MSIHANDLE msi_handle, std::wstring const& name)
{
  DWORD size = 0;

  UINT status = MsiGetPropertyW(msi_handle, name.c_str(), L"", &size);

  if (status == ERROR_MORE_DATA) {
    std::vector<wchar_t> buffer(size + 1);
    MsiGetPropertyW(msi_handle, name.c_str(), &buffer[0], &size);
    return std::wstring(&buffer[0]);
  } else {
    return std::wstring();
  }
}

void set_property(MSIHANDLE msi_handle, std::wstring const& name,
                  std::wstring const& value)
{
  MsiSetPropertyW(msi_handle, name.c_str(), value.c_str());
}

extern "C" UINT __stdcall DetectNsisOverwrite(MSIHANDLE msi_handle)
{
  std::wstring install_root = get_property(msi_handle, L"INSTALL_ROOT");

  std::wstring uninstall_exe = install_root + L"\\uninstall.exe";

  bool uninstall_exe_exists =
    GetFileAttributesW(uninstall_exe.c_str()) != INVALID_FILE_ATTRIBUTES;

  set_property(msi_handle, L"CMAKE_NSIS_OVERWRITE_DETECTED",
               uninstall_exe_exists ? L"1" : L"0");

  return ERROR_SUCCESS;
}
