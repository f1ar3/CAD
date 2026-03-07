cmake_minimum_required(VERSION 3.16)

project(Qt_Project VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# OpenCASCADE
set(OpenCASCADE_DIR "/opt/homebrew/opt/opencascade/lib/cmake/opencascade")
find_package(OpenCASCADE REQUIRED)
include_directories(${OpenCASCADE_INCLUDE_DIR})

set(PROJECT_SOURCES
    main.cpp
    app/MainWindow.h
    app/MainWindow.cpp
    src/viewer/OccView.h
    src/viewer/OccView.cpp
    src/model/Document.h
    src/model/Document.cpp
    src/io/FileIO.h
    src/io/FileIO.cpp
    src/commands/CommandHistory.h
    src/commands/CommandHistory.cpp
    src/widgets/PrimitiveDialog.h
    src/widgets/PrimitiveDialog.cpp
    src/widgets/TransformDialog.h
    src/widgets/TransformDialog.cpp
    src/widgets/ModelTreeWidget.h
    src/widgets/ModelTreeWidget.cpp
    src/widgets/PropertyPanel.h
    src/widgets/PropertyPanel.cpp
    src/widgets/BooleanDialog.h
    src/widgets/BooleanDialog.cpp
    src/widgets/MeasureDialog.h
    src/widgets/MeasureDialog.cpp
    src/widgets/PatternDialog.h
    src/widgets/PatternDialog.cpp
    src/widgets/MirrorDialog.h
    src/widgets/MirrorDialog.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Qt_Project
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(Qt_Project
        ${PROJECT_SOURCES}
    )
endif()

# OccView.cpp includes Cocoa_Window.hxx which requires Objective-C++
set_source_files_properties(src/viewer/OccView.cpp PROPERTIES
    COMPILE_FLAGS "-x objective-c++"
)

target_link_libraries(Qt_Project PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    # OpenCASCADE libraries
    TKernel TKMath TKG3d TKBRep TKTopAlgo
    TKGeomBase TKGeomAlgo TKPrim TKBO
    TKFillet TKShHealing
    TKDESTEP TKDEIGES TKXSBase TKDE
    TKMesh TKV3d TKService TKOpenGl
    # macOS frameworks
    "-framework Cocoa"
)

set_target_properties(Qt_Project PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Qt_Project
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Qt_Project)
endif()

# Tests
enable_testing()
add_subdirectory(tests)
