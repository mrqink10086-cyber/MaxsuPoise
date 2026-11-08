cmake_minimum_required(VERSION 3.24)

project(MaxsuPoise VERSION 0.3.6 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(SKSEPluginAuthor "Maxsu" CACHE STRING "Plugin author")
set(COMMONLIBSSE_NG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../CommonLibSSE-NG-ng" CACHE PATH "CommonLibSSE-NG source directory")
set(DKUTIL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/DKUtil" CACHE PATH "DKUtil source directory")

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.")
endif()

option(ENABLE_SKYRIM_SE "Build for Skyrim 1.5.97" ON)
option(ENABLE_SKYRIM_AE "Build for Skyrim 1.6.1170" ON)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "VR hook offsets have not been verified" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build CommonLibSSE tests")
set(SKSE_SUPPORT_XBYAK ON CACHE BOOL "Enable Xbyak trampoline support")
add_subdirectory("${COMMONLIBSSE_NG_PATH}" CommonLibSSE EXCLUDE_FROM_ALL)

if(EXISTS "${DKUTIL_PATH}/CMakeLists.txt")
    add_subdirectory("${DKUTIL_PATH}" DKUtil EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(DKUtil
        URL https://codeload.github.com/gottyduke/DKUtil/zip/19fcb2d2ee00a9b678bf08b24c53208ab7618fc6)
    FetchContent_MakeAvailable(DKUtil)
endif()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(xbyak CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS SimpleIni.h REQUIRED)

configure_file(cmake/Version.h.in include/Version.h @ONLY)
configure_file(cmake/version.rc.in version.rc @ONLY)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h include/*.h)
add_library(${PROJECT_NAME} SHARED ${SOURCES} "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include" include src ${SIMPLEINI_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE DKUtil::DKUtil
    nlohmann_json::nlohmann_json spdlog::spdlog xbyak::xbyak)
target_compile_options(${PROJECT_NAME} PRIVATE /W3 /permissive- /Zc:__cplusplus /Zc:preprocessor)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION SKSE/Plugins COMPONENT Plugin)
install(FILES MaxsuPoise.ini DESTINATION SKSE/Plugins COMPONENT Plugin)
install(DIRECTORY BehaviorDataInjector dmenu DESTINATION . COMPONENT Plugin)
install(FILES README-NG.md DESTINATION . COMPONENT Plugin)
install(DIRECTORY docs DESTINATION . COMPONENT Plugin)
