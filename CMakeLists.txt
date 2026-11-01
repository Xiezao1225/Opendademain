cmake_minimum_required(VERSION 3.20)
project(dademain LANGUAGES CXX)

enable_language(RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# 资源：res/payload/ 下的所有文件都打包成 RCDATA，并自动生成 TOOLS_MANIFEST。
# 往 res/payload/ 里加 .bat / .exe / .ps1，重新构建后首次运行就会释放到 dade\ 下。
#
# 注意：rc.exe 不知道这些数据文件的存在，必须显式列进 DEPENDS，
#       否则改完载荷之后 exe 里仍是旧内容。
# ---------------------------------------------------------------------------
set(RES_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/res")
set(RES_RC       "${CMAKE_CURRENT_BINARY_DIR}/dademain.rc")
set(RES_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/tools_manifest.txt")
set(RES_STATIC   "${RES_DIR}/readme.html" "${RES_DIR}/default.dade")

file(GLOB_RECURSE PAYLOAD_FILES CONFIGURE_DEPENDS "${RES_DIR}/payload/*")

add_custom_command(
  OUTPUT  "${RES_RC}" "${RES_MANIFEST}"
  COMMAND "${CMAKE_COMMAND}"
          "-DRES_DIR=${RES_DIR}"
          "-DOUT_RC=${RES_RC}"
          "-DOUT_MANIFEST=${RES_MANIFEST}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_res.cmake"
  DEPENDS ${RES_STATIC} ${PAYLOAD_FILES}
          "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_res.cmake"
  COMMENT "生成 TOOLS_MANIFEST 与 dademain.rc"
  VERBATIM
)

# ---------------------------------------------------------------------------
# 目标
# ---------------------------------------------------------------------------
add_executable(dademain
  src/main.cpp
  src/console.cpp
  src/util.cpp
  src/config.cpp
  src/payload.cpp
  src/elevate.cpp
  src/runner.cpp
  "${RES_RC}"
)

target_include_directories(dademain PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(dademain PRIVATE
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(dademain PRIVATE shell32 advapi32 user32)

if(MSVC)
  target_compile_options(dademain PRIVATE /utf-8 /W4 /permissive-)
else()
  target_compile_options(dademain PRIVATE -Wall -Wextra)
endif()

set_target_properties(dademain PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
