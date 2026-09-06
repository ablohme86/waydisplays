qt_add_executable(virtmonitors
    main.cpp
    monitormanager.cpp
    monitormanager.h
)

qt_add_qml_module(virtmonitors
    URI VirtMonitors
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

# Use the QtQuick module from Qt 6.
target_link_libraries(virtmonitors PRIVATE Qt6::Quick)

# Install the executable
install(TARGETS virtmonitors DESTINATION bin)
