################################################################################
# libjpg external project
################################################################################

include(ExternalProject)

# Pre-vendored third-party sources live one level above the repo checkout
# (<root>/third_party next to <root>/re-plants-vs-zombies), referenced relatively so
# no machine-specific absolute path is baked in (see TRANSFER.md).
get_filename_component(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party" ABSOLUTE)
if(NOT EXISTS "${THIRD_PARTY_DIR}")
    message(FATAL_ERROR "third_party not found at ${THIRD_PARTY_DIR} - keep the third_party/ folder next to re-plants-vs-zombies/")
endif()

# MSVC has no bundled NASM; disable JPEG SIMD assembly
set(libjpeg_extra_args "")
if(MSVC)
    list(APPEND libjpeg_extra_args "-DWITH_SIMD=0")
endif()

# support for windows
if(MSVC)
    set(libjpeg_binary "jpeg-static.lib")
    set(zlib_binary "zlibstaticd.lib")
    set(libpng_binary "libpng16_staticd.lib")
else() # linux
    set(libjpeg_binary "libjpeg.a")
    set(zlib_binary "libzlibstatic.a")
    set(libpng_binary "libpng.a")
endif()

# @pvz-online: vendored sources are archives, not necessarily Git checkouts; clean builds must remain offline.
set(libjpeg_dir "${CMAKE_BINARY_DIR}/vendor-build/libjpeg")
ExternalProject_Add(libjpeg-src
    DOWNLOAD_COMMAND    ""
    UPDATE_COMMAND      ""
    SOURCE_DIR          "${THIRD_PARTY_DIR}/libjpeg-turbo"
    BINARY_DIR          "${libjpeg_dir}"
    CMAKE_ARGS          "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
                        "-DCMAKE_INSTALL_PREFIX=${libjpeg_dir}/install" # unused, have to specify
                        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
                        "-DCMAKE_C_FLAGS=${universal_cflags}"
                        "-DENABLE_SHARED=FALSE"
                        "-DWITH_TURBOJPEG=FALSE"
                        ${libjpeg_extra_args}
                        "-Wno-dev"

    BUILD_COMMAND       ${CMAKE_COMMAND} --build "${libjpeg_dir}" --target jpeg-static

    INSTALL_COMMAND     ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/jpeg/;
    COMMAND             ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_DIR}/libjpeg-turbo/jpeglib.h
                                                 ${THIRD_PARTY_DIR}/libjpeg-turbo/jerror.h
                                                 ${libjpeg_dir}/jconfig.h
                                                 ${THIRD_PARTY_DIR}/libjpeg-turbo/jmorecfg.h
                                                 ${THIRD_PARTY_DIR}/libjpeg-turbo/jpegint.h
                                            ${CMAKE_BINARY_DIR}/include/jpeg/
    BUILD_BYPRODUCTS    "${libjpeg_dir}/${libjpeg_binary}"
                        "${CMAKE_BINARY_DIR}/include/jpeg/jpeglib.h"
                        "${CMAKE_BINARY_DIR}/include/jpeg/jerror.h"
    USES_TERMINAL_DOWNLOAD TRUE
    USES_TERMINAL_UPDATE TRUE
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD TRUE
    UPDATE_DISCONNECTED TRUE
)

################################################################################
# zlib external project
################################################################################

set(zlib_dir "${CMAKE_BINARY_DIR}/vendor-build/zlib")
ExternalProject_Add(zlib-src
    DOWNLOAD_COMMAND        ""
    UPDATE_COMMAND          ""
    SOURCE_DIR              "${THIRD_PARTY_DIR}/zlib"
    CMAKE_ARGS              "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
                            "-DCMAKE_INSTALL_PREFIX=${zlib_dir}"
                            "-DCMAKE_C_FLAGS=${universal_cflags}"
                            "-Wno-dev"
    BUILD_COMMAND           "${CMAKE_COMMAND}"
                            "--build" "${zlib_dir}"
                            "--target" "zlibstatic"
    INSTALL_COMMAND         ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/zlib/;
    COMMAND                 ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_DIR}/zlib/zlib.h
                                                     ${zlib_dir}/zconf.h
                                                ${CMAKE_BINARY_DIR}/include/zlib/

    BINARY_DIR              ${zlib_dir}
    BUILD_BYPRODUCTS        "${zlib_dir}/${zlib_binary}"
                            "${CMAKE_BINARY_DIR}/include/zlib/zlib.h" # Use a custom dir to avoid system headers
    USES_TERMINAL_DOWNLOAD  TRUE
    USES_TERMINAL_UPDATE    TRUE
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD     TRUE
    USES_TERMINAL_INSTALL   TRUE
    UPDATE_DISCONNECTED     TRUE
)

################################################################################
# libpng external project
################################################################################

set(libpng_dir "${CMAKE_BINARY_DIR}/vendor-build/libpng")
ExternalProject_Add(libpng-src
    DOWNLOAD_COMMAND        ""
    UPDATE_COMMAND          ""
    DEPENDS                 zlib-src
    SOURCE_DIR              "${THIRD_PARTY_DIR}/libpng"
    CMAKE_ARGS              "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_TOOLCHAIN_FILE}"
                            "-DCMAKE_INSTALL_PREFIX=${libpng_dir}"
                            "-DCMAKE_C_FLAGS=${universal_cflags}"
                            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
                            "-DPNG_HARDWARE_OPTIMIZATIONS=yes"
                            "-DPNG_INTEL_SSE=on"
                            "-DPNG_SHARED=OFF"
                            "-DPNG_EXECUTABLES=OFF"
                            "-DPNG_TESTS=OFF"
                            "-DPNG_BUILD_ZLIB=${zlib_dir}"
                            "-DZLIB_INCLUDE_DIRS=${CMAKE_BINARY_DIR}/include/zlib"
                            "-DZLIB_LIBRARIES=${zlib_dir}/${zlib_binary}"
                            "-Wno-dev"

    BUILD_COMMAND           ${CMAKE_COMMAND} --build "${libpng_dir}" --target png_static

    INSTALL_COMMAND         ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/png/
    COMMAND                 ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_DIR}/libpng/png.h
                                                     ${THIRD_PARTY_DIR}/libpng/pngconf.h
                                                     ${libpng_dir}/pnglibconf.h
                                                     ${CMAKE_BINARY_DIR}/include/png/                                            
    BINARY_DIR              ${libpng_dir}
    BUILD_BYPRODUCTS        "${libpng_dir}/${libpng_binary}"
                            "${CMAKE_BINARY_DIR}/include/png/png.h" # Use a custom dir to avoid system headers
    USES_TERMINAL_DOWNLOAD  TRUE
    USES_TERMINAL_UPDATE    TRUE
    USES_TERMINAL_CONFIGURE TRUE
    USES_TERMINAL_BUILD     TRUE
    USES_TERMINAL_INSTALL   TRUE
    UPDATE_DISCONNECTED     TRUE
)

add_library(jpeg STATIC IMPORTED)
add_library(zlib STATIC IMPORTED)
add_library(png STATIC IMPORTED)

add_dependencies(jpeg libjpeg-src)
add_dependencies(zlib zlib-src)
add_dependencies(png libpng-src)

set_target_properties(jpeg
    PROPERTIES IMPORTED_LOCATION "${libjpeg_dir}/${libjpeg_binary}"
)

set_target_properties(zlib
    PROPERTIES IMPORTED_LOCATION "${zlib_dir}/${zlib_binary}"
)

set_target_properties(png
    PROPERTIES IMPORTED_LOCATION "${libpng_dir}/${libpng_binary}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE jpeg zlib png)
add_dependencies(${PROJECT_NAME} libjpeg-src zlib-src libpng-src) # Ensures headers exist

set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES
   "${libjpeg_dir}"
   "${zlib_dir}"
   "${libpng_dir}"
   "${CMAKE_BINARY_DIR}/include"
)

target_sources(${PROJECT_NAME} PRIVATE
    "ImageLib.cpp"
    "ImageLib.h"
)
