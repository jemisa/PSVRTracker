cmake_minimum_required(VERSION 3.0)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(PSVR_MATH_INCL_DIRS)

list(APPEND PSVR_MATH_INCL_DIRS
    ${EIGEN3_INCLUDE_DIR}
    ${ROOT_DIR}/thirdparty/glm/)

# Source files that are needed for the static library
file(GLOB PSVR_MATH_LIBRARY_SRC
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

# Static library
add_library(PSVRMath STATIC ${PSVR_MATH_LIBRARY_SRC})

target_include_directories(PSVRMath PUBLIC ${PSVR_MATH_INCL_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(PSVRMath PROPERTIES
    COMPILE_FLAGS "-DBUILDING_STATIC_LIBRARY -fPIC")

#MacOS OpenCV must be self-built, this links against older std, which is hidden
#Therefore the PSVRConfigTool must be hidden
#Therefore this must be hidden.
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET_TARGET_PROPERTIES(PSMoveMath
        PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
ENDIF()
