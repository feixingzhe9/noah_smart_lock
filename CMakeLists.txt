cmake_minimum_required(VERSION 2.8.3)
project(smart_lock)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  std_msgs
)

catkin_package(
  CATKIN_DEPENDS roscpp std_msgs
#  LIBRARIES ${PROJECT_NAME}
)

###########
## Build ##
###########
set (CMAKE_CXX_FLAGS "-std=c++11")
include_directories(
  ${catkin_INCLUDE_DIRS}
  include/smart_lock
)

add_executable(smart_lock_node 
    src/main.cpp
	src/uart.cpp
	src/smart_lock.cpp
	src/data_proc.cpp
)

#set(EXTRALIB_BIN ${PROJECT_SOURCE_DIR}/lib/libsqlite.so.0)


LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/) 
#LINK_DIRECTORIES(/usr/lib/) 


target_link_libraries(smart_lock_node
  ${catkin_LIBRARIES} sqlite3 
)

#############
## Install ##
#############


#############
## Testing ##
#############


install(TARGETS smart_lock_node 
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

## Install project namespaced headers
install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        FILES_MATCHING PATTERN "*.h"
        PATTERN ".svn" EXCLUDE)

install(DIRECTORY launch

    DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
    USE_SOURCE_PERMISSIONS
    )








