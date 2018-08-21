cmake_minimum_required(VERSION 2.8)

project(x86_adapt)
# set -DNO_KERNEL_DRIVER=On to disable building the driver
option(NO_KERNEL_DRIVER "Disable build/install of kernel driver")


set(CMAKE_C_FLAGS "-fPIC -ggdb -Wall")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wstrict-prototypes -Wall -Wundef -Wno-long-long -Wsign-compare -Wcomment -pedantic -finline-functions -fno-strict-aliasing -g")



# kernel module names
set(SUPPORT_DRIVER_SRC ${CMAKE_BINARY_DIR}/kernel_module/definition_driver/x86_adapt_defs.c)
set(SUPPORT_DRIVER_SRC_TEMPLATE definition_driver/x86_adapt_defs_template.c)
set(SUPPORT_DRIVER_MODULE kernel_module/definition_driver/Kbuild)

set(DRIVER_SRC x86_adapt_driver.c)
set(DRIVER_MODULE kernel_module/driver/x86_adapt_driver.ko)

set(LIBRARY_INC library/include)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/kernel_module)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/kernel_module/definition_driver)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/kernel_module/driver)

# run python script to generate c file from template
add_custom_command(OUTPUT ${SUPPORT_DRIVER_SRC}
                    COMMAND ./prepare.py ${CMAKE_BINARY_DIR}/kernel_module/definition_driver
                    COMMAND cp ${CMAKE_SOURCE_DIR}/definition_driver/*.h  ${CMAKE_BINARY_DIR}/kernel_module/definition_driver
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/definition_driver
                    DEPENDS ${SUPPORT_DRIVER_SRC_TEMPLATE}
                  )

# build support module
add_custom_command(OUTPUT ${SUPPORT_DRIVER_MODULE}
                   COMMAND echo "obj-m += x86_adapt_defs.o" > kernel_module/definition_driver/Kbuild
                   DEPENDS ${SUPPORT_DRIVER_SRC}
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  )


# build module
add_custom_command(OUTPUT ${DRIVER_MODULE}
                   COMMAND cp ${CMAKE_SOURCE_DIR}/driver/x86_adapt_driver.c ${CMAKE_BINARY_DIR}/kernel_module/driver
                   COMMAND echo "obj-m += x86_adapt_driver.o" > kernel_module/driver/Kbuild
                   COMMAND echo "obj-y += driver/ definition_driver/" > kernel_module/Kbuild
                   COMMAND make W=1 -C  /lib/modules/`uname -r`/build M=`pwd`/kernel_module
                   DEPENDS ${SUPPORT_DRIVER_MODULE}
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                  )

# add target to build support module
add_custom_target(x86_adapt_driver
                  DEPENDS ${DRIVER_MODULE}
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                 )

execute_process(COMMAND uname -r OUTPUT_VARIABLE KERNEL_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)


# build library
add_library(${PROJECT_NAME} SHARED library/src/x86_adapt.c)
add_library(${PROJECT_NAME}_static STATIC library/src/x86_adapt.c)
include_directories(${PROJECT_NAME} ${LIBRARY_INC})
include_directories(${PROJECT_NAME}_static ${LIBRARY_INC})

if(NOT ${NO_KERNEL_DRIVER})
	add_dependencies(${PROJECT_NAME} x86_adapt_driver)
endif(NOT ${NO_KERNEL_DRIVER})

add_executable(x86a_read library/examples/x86a_read.c)
add_executable(x86a_write library/examples/x86a_write.c)
include_directories(x86a_read ${LIBRARY_INC})
include_directories(x86a_write ${LIBRARY_INC})
target_link_libraries(x86a_read ${PROJECT_NAME}_static)
target_link_libraries(x86a_write ${PROJECT_NAME}_static)

# use make install to install files under CMAKE_INSTALL_PREFIX
install(FILES library/include/x86_adapt.h DESTINATION include)
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static x86a_read x86a_write
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
install(DIRECTORY library/doc/man DESTINATION share)
if(NOT ${NO_KERNEL_DRIVER})
	install(CODE "execute_process(COMMAND make W=1 -C /lib/modules/${KERNEL_VERSION}/build M=${CMAKE_BINARY_DIR}/kernel_module modules_install)")
	install(CODE "execute_process(COMMAND depmod)")
endif(NOT ${NO_KERNEL_DRIVER})


