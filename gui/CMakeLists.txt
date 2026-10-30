find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 QuickDialogs2 Concurrent Widgets)
qt_standard_project_setup(REQUIRES 6.5)
qt_add_executable(huffman_gui
    main.cpp controllers/FoundryController.h controllers/FoundryController.cpp
)
target_link_libraries(huffman_gui PRIVATE huffman_core Qt6::Core Qt6::Gui Qt6::Qml
    Qt6::Quick Qt6::QuickControls2 Qt6::QuickDialogs2 Qt6::Concurrent Qt6::Widgets)
target_include_directories(huffman_gui PRIVATE ${PROJECT_SOURCE_DIR}/include)
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS "Main.qml")
set_source_files_properties(qml/components/PressButton.qml PROPERTIES QT_RESOURCE_ALIAS "components/PressButton.qml")
set_source_files_properties(qml/components/RuleLabel.qml PROPERTIES QT_RESOURCE_ALIAS "components/RuleLabel.qml")
set_source_files_properties(qml/components/TypeField.qml PROPERTIES QT_RESOURCE_ALIAS "components/TypeField.qml")
set_source_files_properties(resources/grain.png PROPERTIES QT_RESOURCE_ALIAS "resources/grain.png")
set_source_files_properties(resources/type-sort.png PROPERTIES QT_RESOURCE_ALIAS "resources/type-sort.png")
qt_add_qml_module(huffman_gui URI TypeFoundry VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/PressButton.qml
        qml/components/RuleLabel.qml
        qml/components/TypeField.qml
    RESOURCES
        resources/type-sort.png resources/grain.png
)
set_target_properties(huffman_gui PROPERTIES
    MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Type Foundry"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.typefoundry.huffman")
if(WIN32)
    target_sources(huffman_gui PRIVATE packaging/app.rc)
    target_include_directories(huffman_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/packaging)
    target_link_libraries(huffman_gui PRIVATE ole32)
endif()
install(TARGETS huffman_gui BUNDLE DESTINATION . RUNTIME DESTINATION bin)
set(CPACK_PACKAGE_NAME "Huffman-Type-Foundry")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Type Foundry")
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "The Type Foundry")
    set(CPACK_NSIS_PACKAGE_NAME "The Type Foundry")
endif()
include(CPack)
