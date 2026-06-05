find_package(Qt6 REQUIRED COMPONENTS Test Core Gui)

function(uplink_test TARGET)
    cmake_parse_arguments("" "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${TARGET} ${_SOURCES})
    target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${TARGET} PRIVATE Qt6::Test Qt6::Core ${_LIBS})
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endfunction()

uplink_test(tst_ircparser
    SOURCES
        tst_ircparser.cpp
        ${CMAKE_SOURCE_DIR}/src/irc/ircparser.cpp
)

uplink_test(tst_chatformat
    SOURCES
        tst_chatformat.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/chatrenderer.cpp
    LIBS Qt6::Gui
)
