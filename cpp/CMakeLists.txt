cmake_minimum_required(VERSION 3.16)

project(Forcivate)

set(CMAKE_CXX_STANDARD 17)

find_package(Python COMPONENTS Interpreter Development REQUIRED)
get_filename_component(PYTHON_DIR "${Python_EXECUTABLE}" DIRECTORY)
find_package(pybind11 CONFIG REQUIRED HINTS "${PYTHON_DIR}/Lib/site-packages/pybind11/share/cmake")

add_executable(
    forcivate

    main.cpp
    services/DraftEngine.cpp
    services/SafetyGate.cpp
    services/ReviewQueue.cpp
    config/VoiceConfig.cpp
)

target_include_directories(forcivate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(forcivate PRIVATE pybind11::embed)
