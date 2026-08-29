qt_add_executable(FileNamingTest FileNamingTest.cpp)
target_link_libraries(FileNamingTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME FileNamingTest COMMAND FileNamingTest)

qt_add_executable(ImageDocumentTest ImageDocumentTest.cpp)
target_link_libraries(ImageDocumentTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME ImageDocumentTest COMMAND ImageDocumentTest)

qt_add_executable(ConfigTest ConfigTest.cpp)
target_link_libraries(ConfigTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME ConfigTest COMMAND ConfigTest)

qt_add_executable(CanvasWidgetTest CanvasWidgetTest.cpp)
target_link_libraries(CanvasWidgetTest PRIVATE FitToListUi FitToListCore Qt6::Test)
add_test(NAME CanvasWidgetTest COMMAND CanvasWidgetTest)
# Runs without a real display/window manager, so it works in headless CI too.
set_tests_properties(CanvasWidgetTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(ImageTreeWidgetTest ImageTreeWidgetTest.cpp)
target_link_libraries(ImageTreeWidgetTest PRIVATE FitToListUi FitToListCore Qt6::Test)
add_test(NAME ImageTreeWidgetTest COMMAND ImageTreeWidgetTest)
set_tests_properties(ImageTreeWidgetTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(MaxSizeDialogTest MaxSizeDialogTest.cpp)
target_link_libraries(MaxSizeDialogTest PRIVATE FitToListUi FitToListCore Qt6::Test)
add_test(NAME MaxSizeDialogTest COMMAND MaxSizeDialogTest)
set_tests_properties(MaxSizeDialogTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(FolderSelectionDialogTest FolderSelectionDialogTest.cpp)
target_link_libraries(FolderSelectionDialogTest PRIVATE FitToListUi FitToListCore Qt6::Test)
add_test(NAME FolderSelectionDialogTest COMMAND FolderSelectionDialogTest)
set_tests_properties(FolderSelectionDialogTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# MainWindow.cpp isn't in a library (it's only ever built once, into the app),
# so it's compiled directly into this test target too.
qt_add_executable(MainWindowTest MainWindowTest.cpp ${CMAKE_SOURCE_DIR}/src/MainWindow.cpp)
target_link_libraries(MainWindowTest PRIVATE FitToListUi FitToListCore Qt6::Test)
add_test(NAME MainWindowTest COMMAND MainWindowTest)
set_tests_properties(MainWindowTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
