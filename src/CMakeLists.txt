qt_add_executable(virtmonitors
    main.cpp
    monitormanager.cpp
    monitormanager.h
    sunshineintegration.cpp
    sunshineintegration.h
)

qt_add_resources(virtmonitors sunshine_helper
    PREFIX "/sunshine"
    FILES helpers/sunshine-display.py
)

qt_add_qml_module(virtmonitors
    URI VirtMonitors
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        qml/Main.qml
)

# Use the QtQuick module from Qt 6.
target_link_libraries(virtmonitors PRIVATE Qt6::Quick)

# Install the executable
install(TARGETS virtmonitors DESTINATION bin)

if(BUILD_TESTING)
    qt_add_executable(sunshineintegration_test
        ../tests/sunshineintegration_test.cpp
        sunshineintegration.cpp
    )
    target_include_directories(sunshineintegration_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(sunshineintegration_test PRIVATE Qt6::Core)
    qt_add_resources(sunshineintegration_test sunshine_test_helper
        PREFIX "/sunshine"
        FILES helpers/sunshine-display.py
    )
    add_test(NAME sunshineintegration COMMAND sunshineintegration_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME sunshine_display_helper
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_display_helper.py)
endif()
