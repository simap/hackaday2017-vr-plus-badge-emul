cmake_minimum_required(VERSION 3.8)

#
# Set the name of the app that you are emulating here
#
set(EMULATION_APP "breakout")


project(hackaday2017)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES_NONEMU
        "MDD_File_System/FS Phys Interface Template.c"
        MDD_File_System/FSconfig.h
        MDD_File_System/FSDefs.h
        MDD_File_System/FSIO.c
        MDD_File_System/FSIO.h
        MDD_File_System/GenericTypeDefs.h
        MDD_File_System/HardwareProfile.h
        MDD_File_System/SD-SPI.c
        MDD_File_System/SD-SPI.h
        MDD_File_System/TEMPLATEFILE.h
        appmap.h
        apptemplate.c
        box_game.c
        breakout.c
        browser.c
        cambadge.c
        cambadge.h
        camera.c
        camvals_9650.h
        codescan.c
        display.c
        fileformats.c
        globals.c
        globals.h
        hardware.c
        imagefx.c
        interrupts.c
        monorgb.h
        particle.c
        pindefs.h
        scope.c
        serial.c
        settings.c
        tetrapuzz.c
        utils.c)

add_executable(hackaday2017 ${SOURCE_FILES_NONEMU})

project(SFMLDemo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SFML_DIR "${CMAKE_CURRENT_LIST_DIR}/SFML-2.4.2-osx-clang")


include_directories("${SFML_DIR}/include")


set(SOURCE_FILES main.cpp "${EMULATION_APP}.c")
add_executable(SFMLDemo MACOSX_BUNDLE ${SOURCE_FILES})

set_target_properties(SFMLDemo PROPERTIES COMPILE_DEFINITIONS "CAMBADGE_EMUL=1 APP_FUNC=${EMULATION_APP}")
target_compile_definitions(SFMLDemo PRIVATE APP_FUNC=${EMULATION_APP})


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/SFML-2.4.2-osx-clang/cmake/Modules")
find_package(SFML REQUIRED system window graphics network audio)
if (SFML_FOUND)
    include_directories(${SFML_INCLUDE_DIR})
    target_link_libraries(SFMLDemo ${SFML_LIBRARIES})
endif()
