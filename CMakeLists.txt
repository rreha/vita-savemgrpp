cmake_minimum_required(VERSION 3.10)

## This includes the Vita toolchain, must go before project definition
# It is a convenience so you do not have to type
# -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake for cmake. It is
# highly recommended that you include this block for all projects.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(vita-savemgrpp C)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(TITLE_ID "SAVEMGRPP")
set(APP_NAME "Save Manager++")
set(VERSION "05.00")

add_definitions(-DSQLITE_OS_OTHER=1 -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -fcommon -Wno-int-conversion -Wno-incompatible-pointer-types")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=cortex-a9")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

add_subdirectory(libs/VitaShell/user)
add_subdirectory(libs/VitaShell/kernel)

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/libs/VitaShell/user
    ${CMAKE_CURRENT_BINARY_DIR}/libs/VitaShell/kernel
)

# From vita shell CMakeLists - credits TheFlow
FUNCTION(ADD_RESOURCES out_var)
    SET(result)
    FOREACH(in_f ${ARGN})
        SET(out_f "${CMAKE_CURRENT_BINARY_DIR}/${in_f}.o")
        GET_FILENAME_COMPONENT(out_dir ${out_f} DIRECTORY)
        ADD_CUSTOM_COMMAND(OUTPUT ${out_f}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
                COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
                DEPENDS ${in_f}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Building resource ${out_f}"
                VERBATIM
                )
        LIST(APPEND result ${out_f})
    ENDFOREACH()
    SET(${out_var} "${result}" PARENT_SCOPE)
ENDFUNCTION()

set(res_files res/noicon.png)
add_resources(svmgr_res ${res_files})

add_custom_target(build_ftpvita ALL
    COMMAND make
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/libs/libftpvita"
    BYPRODUCTS "${LOCAL_FTPVITA_DIR}/libftpvita.a"
    COMMENT "Building libftpvita..."
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/sqlite3/source
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/VitaShell/user
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/VitaShell/kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/libftpvita
)

add_executable(${PROJECT_NAME}
    ${svmgr_res}
    src/utils.c
    src/file.c
    src/font.c
    src/display.c
    src/input.c
    src/main.c
    src/pfs.c
    src/vita_sqlite.c
    src/ime_dialog.c
)

add_dependencies(${PROJECT_NAME} vitashell_user_stubs build_ftpvita)

target_link_libraries(${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/libs/sqlite3/sqlite3.c.obj
    vita2d
    png
    z
    m
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/libftpvita/libftpvita.a
    SceNet_stub
    SceNetCtl_stub
    SceIofilemgr_stub
    SceKernelThreadMgr_stub
    SceCommonDialog_stub
    SceDisplay_stub
    SceGxm_stub
    SceAppMgr_stub
    SceAppUtil_stub
    SceSysmodule_stub
    SceCtrl_stub
    SceTouch_stub
    ScePgf_stub
    ScePvf_stub
    SceShellSvc_stub
    SceRegistryMgr_stub
    taihen_stub
    VitaShellUser_stub_weak
    SceVshBridge_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)

vita_create_vpk(${PROJECT_NAME}.vpk ${TITLE_ID} ${PROJECT_NAME}.self
		VERSION ${VERSION}
		NAME ${APP_NAME}
        FILE sce_sys/icon0.png sce_sys/icon0.png
        FILE sce_sys/pic0.png sce_sys/pic0.png
		FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
		FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
		FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
