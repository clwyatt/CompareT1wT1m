cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

project(PROJNAME) 

# ITK 3.20 required
FIND_PACKAGE(ITK 3.20 REQUIRED) 
INCLUDE(${ITK_USE_FILE})

SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL 
"Single output directory for building all executables.")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Utilities/vul)

SUBDIRS(Utilities Code)
