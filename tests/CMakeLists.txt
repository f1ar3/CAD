find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

# --- tst_CommandHistory ---
add_executable(tst_CommandHistory
    tst_CommandHistory.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/CommandHistory.cpp
)
target_include_directories(tst_CommandHistory PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_CommandHistory PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    TKernel TKMath TKBRep
)
add_test(NAME tst_CommandHistory COMMAND tst_CommandHistory)

# --- tst_RebuildShape ---
add_executable(tst_RebuildShape
    tst_RebuildShape.cpp
    ${CMAKE_SOURCE_DIR}/src/model/Document.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/CommandHistory.cpp
    ${CMAKE_SOURCE_DIR}/src/io/FileIO.cpp
)
target_include_directories(tst_RebuildShape PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_RebuildShape PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    TKernel TKMath TKG3d TKBRep TKTopAlgo
    TKGeomBase TKGeomAlgo TKPrim TKBO
    TKFillet TKShHealing
    TKDESTEP TKDEIGES TKXSBase TKDE
    TKMesh TKV3d TKService TKOpenGl
)
add_test(NAME tst_RebuildShape COMMAND tst_RebuildShape)

# --- tst_FileIO ---
add_executable(tst_FileIO
    tst_FileIO.cpp
    ${CMAKE_SOURCE_DIR}/src/io/FileIO.cpp
)
target_include_directories(tst_FileIO PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_FileIO PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    TKernel TKMath TKG3d TKBRep TKTopAlgo
    TKGeomBase TKGeomAlgo TKPrim
    TKDESTEP TKDEIGES TKXSBase TKDE
)
add_test(NAME tst_FileIO COMMAND tst_FileIO)

# --- tst_Measure ---
add_executable(tst_Measure
    tst_Measure.cpp
)
target_include_directories(tst_Measure PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_Measure PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    TKernel TKMath TKG3d TKBRep TKTopAlgo
    TKGeomBase TKGeomAlgo TKPrim
)
add_test(NAME tst_Measure COMMAND tst_Measure)

# --- tst_Sketch ---
add_executable(tst_Sketch
    tst_Sketch.cpp
    ${CMAKE_SOURCE_DIR}/src/model/Document.cpp
    ${CMAKE_SOURCE_DIR}/src/commands/CommandHistory.cpp
    ${CMAKE_SOURCE_DIR}/src/io/FileIO.cpp
)
target_include_directories(tst_Sketch PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_Sketch PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    TKernel TKMath TKG3d TKBRep TKTopAlgo
    TKGeomBase TKGeomAlgo TKPrim TKBO
    TKFillet TKShHealing
    TKDESTEP TKDEIGES TKXSBase TKDE
    TKMesh TKV3d TKService TKOpenGl
)
add_test(NAME tst_Sketch COMMAND tst_Sketch)
