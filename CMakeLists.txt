cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

project(PlantsVsZombies C CXX)

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "List of supported configurations." FORCE
)

set(CMAKE_CXX_STANDARD 23)

# @Patoke todo: probably add MSVC counterparts?
if(MSVC)
    # Some MSVC installations ignore VSLANG when the English language pack is absent.
    # Probe the actual UTF-8 output instead of silently losing header dependencies.
    if(CMAKE_GENERATOR MATCHES "Ninja")
        set(include_probe "${CMAKE_BINARY_DIR}/pvz_include_probe")
        file(WRITE "${include_probe}.h" "#pragma once\n")
        file(WRITE "${include_probe}.cpp" "#include \"pvz_include_probe.h\"\n")
        execute_process(COMMAND "${CMAKE_COMMAND}" -E env VSLANG=1033
            "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /c "${include_probe}.cpp" "/Fo${include_probe}.obj"
            OUTPUT_VARIABLE include_output ERROR_VARIABLE include_error
            RESULT_VARIABLE include_result ENCODING UTF-8)
        string(REGEX MATCH "([^\r\n]+: +)[A-Za-z]:[/\\\\][^\r\n]*pvz_include_probe.h"
            include_match "${include_output}${include_error}")
        if(NOT include_result EQUAL 0 OR NOT include_match)
            message(FATAL_ERROR "Cannot detect MSVC /showIncludes prefix: ${include_output}${include_error}")
        endif()
        set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_MATCH_1}")
    endif()
    set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;VSLANG=1033")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;VSLANG=1033")
    # sources are UTF-8 (no BOM); MSVC defaults to system codepage and misparses CJK comments
    set(universal_compiler_flags "/utf-8")
else()
    #set(universal_compiler_flags "-fsanitize=undefined -fsanitize-trap -pthread -fuse-ld=lld -fno-omit-frame-pointer -fstack-protector-all -fms-extensions -fdiagnostics-color=always -gdwarf-2 -O0")
    set(universal_compiler_flags "-pthread -fuse-ld=lld -fms-extensions -march=native -fdiagnostics-color=always -O3")
endif()
set(universal_cxxflags "${CMAKE_CXX_FLAGS} ${universal_compiler_flags}")
set(universal_cflags   "${CMAKE_C_FLAGS} ${universal_compiler_flags}")

if(MSVC)
    set(warn_flags "")
else()
    set(warn_flags "-Wall -Wextra -Werror")
endif()

set(CMAKE_CXX_FLAGS "${universal_cxxflags} ${warn_flags}")
set(CMAKE_C_FLAGS   "${universal_cflags} ${warn_flags}")
# @pvz-online: 默认仍是 Debug（日常开发），但允许显式覆盖——发布版走
# build-msvc-release.bat 传 -DCMAKE_BUILD_TYPE=Release（Debug 构建链接调试运行库，
# 没装 VS 的玩家机器起不来）。
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
# @pvz-online: Release 也带符号（崩溃取证用）。/Zi + /DEBUG + /MAP 只影响产物侧
# 车文件（.pdb/.map），不改变代码生成；map 随玩家包放 exe 旁边供现场解符号，pdb 留本机。
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /Zi")
    string(APPEND CMAKE_C_FLAGS_RELEASE   " /Zi")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /DEBUG /MAP")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT MSVC)
    set(CMAKE_RC_COMPILER_INIT windres)
endif()
ENABLE_LANGUAGE(RC)

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################
add_subdirectory(SexyAppFramework)
