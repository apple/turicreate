unset(VERSION)

# Ignore all default paths for this test to avoid finding system Lua
set(CMAKE_INCLUDE_PATH )
set(CMAKE_PREFIX_PATH )
set(CMAKE_FRAMEWORK_PATH )

set(ENV{CMAKE_INCLUDE_PATH} )
set(ENV{CMAKE_PREFIX_PATH} )
set(ENV{CMAKE_FRAMEWORK_PATH} )

set(ENV{PATH} )
set(ENV{INCLUDE} )

set(CMAKE_SYSTEM_INCLUDE_PATH )
set(CMAKE_SYSTEM_PREFIX_PATH )
set(CMAKE_SYSTEM_FRAMEWORK_PATH )

function(require_found path version)
    find_package(Lua ${VERSION} QUIET)
    if(NOT "${LUA_INCLUDE_DIR}" STREQUAL "${path}")
        message(FATAL_ERROR "LUA_INCLUDE_PATH != path: '${LUA_INCLUDE_DIR}' != '${path}'")
    endif()
    if(NOT LUA_VERSION_STRING MATCHES "^${version}\.[0-9]$")
        message(FATAL_ERROR "Wrong versionfound in '${LUA_INCLUDE_DIR}': ${LUA_VERSION_STRING} != ${version}")
    endif()
endfunction()

# Use functions for scoping and better error messages
function(require_find path version)
    unset(LUA_INCLUDE_DIR CACHE)
    require_found(${lua_path} ${version})
endfunction()

function(test_prefix_path path lua_path version)
    set(CMAKE_PREFIX_PATH ${path})
    require_find(lua_path ${version})
endfunction()

function(test_include_path path lua_path version)
    set(CMAKE_INCLUDE_PATH ${path})
    require_find(lua_path  ${version})
endfunction()

function(test_env_path path lua_path version)
    set(ENV{LUA_DIR} ${path})
    require_find(lua_path  ${version})
    unset(ENV{LUA_DIR})
endfunction()

function(test_path prefix_path lua_path version)
    # Shortcut: Make paths relative to current list dir
    set(prefix_path ${CMAKE_CURRENT_LIST_DIR}/${prefix_path})
    set(lua_path ${CMAKE_CURRENT_LIST_DIR}/${lua_path})

    test_prefix_path(${prefix_path} ${lua_path} ${version})
    test_include_path(${prefix_path}/include ${lua_path}  ${version})
    test_env_path(${prefix_path} ${lua_path}  ${version})
endfunction()

# Simple test
test_path(prefix1 prefix1/include 5.3)
# Find highest version
test_path(prefix2 prefix2/include/lua5.3 5.3)
foreach(ver 5.3 5.2 5.1)
    # At least X or X.0 -> Highest
    set(VERSION "${ver}")
    test_path(prefix2 prefix2/include/lua5.3 5.3)
    set(VERSION "${ver}.0")
    test_path(prefix2 prefix2/include/lua5.3 5.3)
    # Exactly X/X.0
    set(VERSION "${ver}" EXACT)
    test_path(prefix2 prefix2/include/lua${ver} ${ver})
    set(VERSION "${ver}.0" EXACT)
    test_path(prefix2 prefix2/include/lua${ver} ${ver})
endforeach()

# Find unknown version
set(VERSION "5.9")
test_path(prefix2 prefix2/include/lua5.9 5.9)
set(VERSION "5.9" EXACT)
test_path(prefix2 prefix2/include/lua5.9 5.9)

# Set LUA_INCLUDE_DIR (non-cache) to unsuitable version
set(LUA_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/prefix2/include/lua5.2)
set(VERSION "5.1" EXACT)
test_path(prefix2 prefix2/include/lua5.1 5.1)
