cmake_minimum_required(VERSION 3.28.0)
cmake_policy(SET CMP0077 NEW)

project(cxmf VERSION 1.0.0 LANGUAGES CXX)

option(CXMF_BUILD_ZLIB "Build zlib" ON)
option(CXMF_INCLUDE_IMPORTER "Include glTF 2.0 importer (includes assimp, glm, meshoptimizer)" ON)



set(CXMF_IS_STANDALONE_BUILD OFF)
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	set(CXMF_IS_STANDALONE_BUILD ON)
endif()



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(LIBRARIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

include(cmake/dependencies.cmake)

set(SOURCE_FILES
	${SOURCE_DIR}/CXMF.cpp
)

if(CXMF_IS_STANDALONE_BUILD)
	list(APPEND SOURCE_FILES ${SOURCE_DIR}/main.cpp)
	if(MSVC)
		list(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc)
	endif()
	add_executable(cxmf ${SOURCE_FILES})
else()
	add_library(cxmf STATIC ${SOURCE_FILES})
endif()

target_compile_definitions(cxmf PUBLIC
	CXMF_MAX_MESHLET_VERTICES=64
	CXMF_MAX_MESHLET_TRIANGLES=96
)

target_include_directories(cxmf PUBLIC ${INCLUDE_DIR})
target_include_directories(cxmf PRIVATE ${SOURCE_DIR})
target_link_libraries(cxmf PRIVATE ${ZLIB_LIBRARIES})

if(CXMF_INCLUDE_IMPORTER)
	target_compile_definitions(cxmf PRIVATE CXMF_INCLUDE_IMPORTER)
	target_link_libraries(cxmf PRIVATE assimp glm::glm meshoptimizer)
endif()

target_compile_definitions(cxmf PRIVATE
	CXMF_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	CXMF_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	CXMF_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

if(MSVC)
	target_compile_definitions(cxmf PRIVATE
		_CRT_SECURE_NO_WARNINGS=1
		_CRT_NONSTDC_NO_WARNINGS=1
	)
endif()
