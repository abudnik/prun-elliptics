cmake_minimum_required(VERSION 2.6)
set(TARGET prun-elliptics)
set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -Wno-unused-parameter -Wno-long-long -pedantic -pthread -g -O3")

set(REQUIRED_LIBRARIES
  elliptics_cpp
  elliptics_client
)

set(SOURCE
  ${PROJECT_SOURCE_DIR}/dbelliptics.cpp
)

add_library(${TARGET} SHARED ${SOURCE})
target_link_libraries( ${TARGET} ${REQUIRED_LIBRARIES} -lrt -rdynamic )

install(TARGETS ${TARGET}
  LIBRARY DESTINATION lib${LIB_SUFFIX}
  BUNDLE DESTINATION library)
