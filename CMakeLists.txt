cmake_minimum_required(VERSION 3.21)
project(DupFindQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Test)
qt_standard_project_setup()

# UI-free core. Only QtCore, so changing the scanner does not rebuild the widgets.
add_library(dupfind_core STATIC
    src/scan/ScanEngine.cpp
    src/scan/ScanEngine.h
)
target_link_libraries(dupfind_core PUBLIC Qt6::Core)
target_include_directories(dupfind_core PUBLIC src)

qt_add_executable(DupFindQt
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/DuplicatesModel.cpp
    src/ui/DuplicatesModel.h
    src/worker/ScanWorker.cpp
    src/worker/ScanWorker.h
)
target_link_libraries(DupFindQt PRIVATE dupfind_core Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
