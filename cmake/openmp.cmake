FIND_PACKAGE(OpenMP QUIET)
if(OPENMP_FOUND)
	message(STATUS "OPENMP FOUND")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_CXX_FLAGS}")
	message(STATUS "CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}")
endif()

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#       message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
