#include <kwsys/DynamicLoader.hxx>

#include <example.h>

#include <example_exe.h>

#include <iostream>
#include <string>

#include <stdio.h>

// Implement the ABI used by plugins.
extern "C" int example_exe_function()
{
  std::cout << "hello" << std::endl;
  return 123;
}

#ifdef CMAKE_INTDIR
#define CONFIG_DIR "/" CMAKE_INTDIR
#else
#define CONFIG_DIR ""
#endif

int main()
{
  std::string libName = EXAMPLE_EXE_PLUGIN_DIR CONFIG_DIR "/";
  libName += kwsys::DynamicLoader::LibPrefix();
  libName += "example_mod_1";
  libName += kwsys::DynamicLoader::LibExtension();
  kwsys::DynamicLoader::LibraryHandle handle =
    kwsys::DynamicLoader::OpenLibrary(libName.c_str());
  if (!handle) {
    // Leave the .c_str() on this one.  It is needed on OpenWatcom.
    std::cerr << "Could not open plugin \"" << libName.c_str() << "\"!"
              << std::endl;
    return 1;
  }
  kwsys::DynamicLoader::SymbolPointer sym =
    kwsys::DynamicLoader::GetSymbolAddress(handle, "example_mod_1_function");
  if (!sym) {
    std::cerr << "Could not get plugin symbol \"example_mod_1_function\"!"
              << std::endl;
    return 1;
  }
#ifdef __WATCOMC__
  int(__cdecl * f)(int) = (int(__cdecl*)(int))(sym);
#else
  int (*f)(int) = reinterpret_cast<int (*)(int)>(sym);
#endif
  if (f(456) != (123 + 456)) {
    std::cerr << "Incorrect return value from plugin!" << std::endl;
    return 1;
  }
  kwsys::DynamicLoader::CloseLibrary(handle);
  return 0;
}
