cmake_minimum_required(VERSION 2.6)
project(SmallFem)

## Add Source Macro ##
######################
macro(add_sources dir files)
  foreach(file ${files})
    list(APPEND list ${dir}/${file})
  endforeach(file)

  set(sources ${sources} ${list} PARENT_SCOPE)
endmacro(add_sources)

## Compatibility with gmsh (TO BE REMOVED !!!)
##############################################
macro(add_sources_in_gmsh dir files)
  foreach(file ${files})
    list(APPEND list ../../${dir}/${file})
  endforeach(file)

  set(sources ${sources} ${list} PARENT_SCOPE)
endmacro(add_sources_in_gmsh)

macro(append_gmsh_src dir files)
  foreach(file ${files})
    list(APPEND list ../../${dir}/${file})
  endforeach(file)

  set(sources ${sources} ${list} PARENT_SCOPE)
endmacro(append_gmsh_src)
##############################################

## Includes for CMake ##
########################
include(FindPackageHandleStandardArgs)

## PETSc ##
###########
## PETSc dir & arch
if(NOT DEFINED PETSC_DIR OR NOT DEFINED PETSC_ARCH)
  set(PETSC_DIR $ENV{PETSC_DIR})
  set(PETSC_ARCH $ENV{PETSC_ARCH})
endif(NOT DEFINED PETSC_DIR OR NOT DEFINED PETSC_ARCH)

## PETSc include
set(PETSC_INCLUDES "${PETSC_DIR}/include;${PETSC_DIR}/${PETSC_ARCH}/include")

## PETSc variables
file(STRINGS ${PETSC_DIR}/${PETSC_ARCH}/conf/petscvariables
  PETSC_VARIABLES NEWLINE_CONSUME)

## PETSc external packages
string(REGEX MATCH "PETSC_WITH_EXTERNAL_LIB = [^\n\r]*"
  PETSC_LIBS ${PETSC_VARIABLES})
string(REPLACE "PETSC_WITH_EXTERNAL_LIB = " "" PETSC_LIBS ${PETSC_LIBS})
string(STRIP ${PETSC_LIBS} PETSC_LIBS)

## PETSc BLAS and LAPACK
string(REGEX MATCH "BLASLAPACK_LIB = [^\n\r]*"
  PETSC_BLASLAPACK ${PETSC_VARIABLES})
string(REPLACE "BLASLAPACK_LIB = " "" PETSC_BLASLAPACK ${PETSC_BLASLAPACK})
string(STRIP ${PETSC_BLASLAPACK} PETSC_BLASLAPACK)

## SLEPc ##
###########
## SLEPc dir
if(NOT DEFINED SLEPC_DIR)
  set(SLEPC_DIR $ENV{SLEPC_DIR})
endif(NOT DEFINED SLEPC_DIR)

## SLEPc lib & include
set(SLEPC_LIBS     "-Wl,-rpath,${SLEPC_DIR}/${PETSC_ARCH}/lib")
set(SLEPC_LIBS     "${SLEPC_LIBS} -L${SLEPC_DIR}/${PETSC_ARCH}/lib  -lslepc")
set(SLEPC_INCLUDES "${SLEPC_DIR}/include;${SLEPC_DIR}/${PETSC_ARCH}/include")

## OpenMP ##
############
find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif(OPENMP_FOUND)

## User Options ##
##################
set(PETSC_DIR  "${PETSC_DIR}"  CACHE PATH   "PETSc directory")
set(PETSC_ARCH "${PETSC_ARCH}" CACHE STRING "PETSc architecture")
set(SLEPC_DIR  "${SLEPC_DIR}"  CACHE PATH   "SLEPc directory")

## Include Gmsh ##
##################
set(BLAS_LAPACK_LIBRARIES "${PETSC_BLASLAPACK}")
add_subdirectory(../.. "${CMAKE_CURRENT_BINARY_DIR}/gmsh")

## Status ##
############
message(STATUS "PETSc: ${PETSC_DIR}/${PETSC_ARCH}")
message(STATUS "SLEPc: ${SLEPC_DIR}/${PETSC_ARCH}")

## Add Sources ##
#################
add_subdirectory(assembler)
add_subdirectory(common)
add_subdirectory(formulation)
add_subdirectory(geometry)
add_subdirectory(postprocessing)
add_subdirectory(solver)
add_subdirectory(context)

## TO BE REMOVED !!!
####################
add_subdirectory(../../FunctionSpace
  "${CMAKE_CURRENT_BINARY_DIR}/FunctionSpace")
####################

## Include Path ##
##################
include_directories(
  ## SmallFEM
  assembler
  common
  formulation
  geometry
  postprocessing
  solver
  context
  simulation

  ## Gmsh
  ${CMAKE_CURRENT_BINARY_DIR}/gmsh/Common
  ../../Common
  ../../FunctionSpace
  ../../Numeric
  ../../Geo
  ../../Mesh
  ../../Post

  ## PETSc & SLEPc
  ${PETSC_INCLUDES}
  ${SLEPC_INCLUDES}
)

## Compiler Flags ##
####################
add_definitions(-pedantic -Wall -Wno-long-long) # -Wno-long-long for PETSc 3.3
add_definitions(-O3 -march=native)
add_definitions(-g)

## Build SmallFEM Library ##
############################
## Build
add_library(sf STATIC ${sources})
set_target_properties(sf PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(sf lib ${SLEPC_LIBS} ${PETSC_LIBS} -lrt)

## Make sure of dependencies
add_dependencies(sf lib gmsh)

## Build Simulations ##
#######################
add_executable(poisson simulation/Poisson.cpp)
add_executable(swave   simulation/SteadyWave.cpp)
add_executable(slow    simulation/Slow.cpp)
add_executable(free    simulation/FreeSpace.cpp)
add_executable(lagr    simulation/Lagrange.cpp)
add_executable(emode   simulation/EigenMode.cpp)

add_executable(pml   simulation/Pml.cpp)
add_executable(scat  simulation/Scattering.cpp simulation/ScatteringHelper.cpp)
add_executable(har   simulation/Haroche.cpp simulation/HarocheHelper.cpp)
add_executable(har2d simulation/Haroche2D.cpp simulation/HarocheHelper2D.cpp)

add_executable(ddm   simulation/Ddm.cpp)
add_executable(cir   simulation/DdmCircle.cpp)
add_executable(disc  simulation/DiscScattering.cpp)
add_executable(waveg simulation/Waveguide.cpp)

add_executable(proj simulation/Projection.cpp)
add_executable(mesh simulation/Mesh.cpp)
add_executable(fs   simulation/ShowFunctionSpace.cpp)
add_executable(ref  simulation/RefSpace.cpp)
add_executable(test simulation/Test.cpp)

## Link ##
##########
target_link_libraries(poisson sf)
target_link_libraries(swave sf)
target_link_libraries(slow sf)
target_link_libraries(free sf)
target_link_libraries(lagr sf)
target_link_libraries(emode sf)

target_link_libraries(pml   sf)
target_link_libraries(scat  sf)
target_link_libraries(har   sf)
target_link_libraries(har2d sf)

target_link_libraries(ddm   sf)
target_link_libraries(cir   sf)
target_link_libraries(disc  sf)
target_link_libraries(waveg sf)

target_link_libraries(proj sf)
target_link_libraries(mesh sf)
target_link_libraries(fs   sf)
target_link_libraries(ref  sf)
target_link_libraries(test sf)
