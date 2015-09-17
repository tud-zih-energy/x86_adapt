cmake_minimum_required(VERSION 2.8)

project(x86_adapt)

set(CMAKE_C_FLAGS "-fPIC -ggdb -Wall")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wstrict-prototypes -Wall -Wundef -Wno-long-long -Wsign-compare -Wcomment -pedantic -finline-functions -fno-strict-aliasing -g")



# kernel module names
set(DRIVER_SRC x86_adapt_driver.c)
set(DRIVER_SRC_TEMPLATE x86_adapt_driver_template.c)
set(DRIVER_MODULE x86_adapt_driver.ko)

# run python script to generate c file from template
add_custom_command(OUTPUT ${DRIVER_SRC}
                    COMMAND ./prepare.py ${CMAKE_BINARY_DIR}
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    DEPENDS ${DRIVER_SRC_TEMPLATE}
                  )

# build module
add_custom_command(OUTPUT ${DRIVER_MODULE}
                   COMMAND echo "obj-m += x86_adapt_driver.o" > Kbuild
                   COMMAND make W=1 -C  /lib/modules/`uname -r`/build M=`pwd`
                   DEPENDS ${DRIVER_SRC}
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  )

# add target to build module
add_custom_target(x86_adapt_driver
                  DEPENDS ${DRIVER_MODULE}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                 )

# build library
add_library(${PROJECT_NAME} SHARED library/src/x86_adapt.c)
add_library(${PROJECT_NAME}_static STATIC library/src/x86_adapt.c)
include_directories(${PROJECT_NAME} library/include)
include_directories(${PROJECT_NAME}_static library/include)
add_dependencies(${PROJECT_NAME} x86_adapt_driver)

# use make install to install files under CMAKE_INSTALL_PREFIX
install(FILES library/include/x86_adapt.h DESTINATION include)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(TARGETS ${PROJECT_NAME}_static ARCHIVE DESTINATION lib)
