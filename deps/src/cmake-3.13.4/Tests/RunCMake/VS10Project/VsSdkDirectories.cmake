enable_language(CXX)

set(CMAKE_VS_SDK_EXECUTABLE_DIRECTORIES "$(VC_ExecutablePath_x86);C:\\Program Files\\Custom-SDK\\;")
set(CMAKE_VS_SDK_INCLUDE_DIRECTORIES "$(VC_IncludePath);C:\\Program Files\\Custom-SDK\\;")
set(CMAKE_VS_SDK_REFERENCE_DIRECTORIES "$(VC_ReferencesPath_x86);C:\\Program Files\\Custom-SDK\\;")
set(CMAKE_VS_SDK_LIBRARY_DIRECTORIES "$(VC_LibraryPath_x86);C:\\Program Files\\Custom-SDK\\;")
set(CMAKE_VS_SDK_LIBRARY_WINRT_DIRECTORIES "$(WindowsSDK_MetadataPath);C:\\Program Files\\Custom-SDK\\;")
set(CMAKE_VS_SDK_SOURCE_DIRECTORIES "$(VC_SourcePath);C:\\Program Files\\Custom-SDK\\;")
set(CMAKE_VS_SDK_EXCLUDE_DIRECTORIES "$(VC_IncludePath);C:\\Program Files\\Custom-SDK\\;")

add_library(foo foo.cpp)
