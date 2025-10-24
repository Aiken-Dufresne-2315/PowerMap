cmake_minimum_required(VERSION 3.10)
project(GurobiTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set GUROBI installation path
set(GUROBI_HOME "C:/gurobi1203/win64")

# Find GUROBI installation
find_path(GUROBI_INCLUDE_DIR 
    NAMES gurobi_c++.h
    PATHS ${GUROBI_HOME}/include
    NO_DEFAULT_PATH
)

# Find GUROBI C++ library
find_library(GUROBI_CXX_LIBRARY
    NAMES gurobi_c++md2017 gurobi_c++md gurobi_c++
    PATHS ${GUROBI_HOME}/lib
    NO_DEFAULT_PATH
)

# Find GUROBI main library
find_library(GUROBI_LIBRARY
    NAMES gurobi1203 gurobi120 gurobi
    PATHS ${GUROBI_HOME}/lib
    NO_DEFAULT_PATH
)

# Find Boost
find_package(Boost REQUIRED COMPONENTS graph)

# Display found libraries
message(STATUS "GUROBI_HOME: ${GUROBI_HOME}")
message(STATUS "GUROBI_INCLUDE_DIR: ${GUROBI_INCLUDE_DIR}")
message(STATUS "GUROBI_CXX_LIBRARY: ${GUROBI_CXX_LIBRARY}")
message(STATUS "GUROBI_LIBRARY: ${GUROBI_LIBRARY}")

# Include directories
include_directories(${GUROBI_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)

# Collect source files manually (excluding main.cpp, RelativeCoord.cpp)
set(SOURCES
    src/Test.cpp
    src/EdgeOrientation.cpp
    src/MapFileReader.cpp
    src/BaseVertexProperty.cpp
    src/BaseEdgeProperty.cpp
    src/BaseUGraphProperty.cpp
    src/BaseGraphProperty.cpp
    src/Coord2.cpp
    src/VisualizeSVG.cpp
    src/VertexAlignment.cpp
    src/DynamicGrid.cpp
    src/PowerMap.cpp
    src/DVPositioning.cpp
    src/CheckOverlap.cpp
    src/Commons.cpp
    src/AuxLineSpacing.cpp
    src/SpatialGrid.cpp
)

# Create executable file for edge orientation test
add_executable(test_3 ${SOURCES})

# Link GUROBI and Boost libraries
target_link_libraries(test_3 
    ${GUROBI_CXX_LIBRARY}
    ${GUROBI_LIBRARY}
    ${Boost_LIBRARIES}
)

# Windows specific settings
if(WIN32)
    # Add GUROBI DLL path to runtime path
    set_target_properties(test_3 PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=${GUROBI_HOME}/bin;$ENV{PATH}"
    )
endif()
