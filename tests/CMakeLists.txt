cmake_minimum_required(VERSION 3.24)
project(MaxsuPoisePerformanceChecks LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
find_package(nlohmann_json CONFIG REQUIRED)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/PoiseHealthHandler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/Hooks/PoiseRegenHandler.cpp")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../src/PoiseHealthHandler.cpp" health_source)
string(REPLACE "#include \"SettingsHandler.h\"" "" health_source "${health_source}")
string(REPLACE "#include \"Utils.h\"" "" health_source "${health_source}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PoiseHealthHandler.cpp"
    CONTENT "#include \"EngineStubs.h\"\n${health_source}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../src/Hooks/PoiseRegenHandler.cpp" regen_source)
string(REPLACE "#include \"PoiseRegenHandler.h\"" "#include \"RegenStubs.h\"" regen_source "${regen_source}")
string(REPLACE "#include \"Utils.h\"" "" regen_source "${regen_source}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/PoiseRegenHandler.cpp" CONTENT "${regen_source}")
add_executable(performance_checks performance_checks.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/PoiseHealthHandler.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/PoiseRegenHandler.cpp" ../src/AnnoInfo.cpp)
target_include_directories(performance_checks PRIVATE . ../src)
target_link_libraries(performance_checks PRIVATE nlohmann_json::nlohmann_json)
enable_testing()
add_test(NAME performance_checks COMMAND performance_checks)
