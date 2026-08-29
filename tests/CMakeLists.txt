qt_add_executable(FileNamingTest FileNamingTest.cpp)
target_link_libraries(FileNamingTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME FileNamingTest COMMAND FileNamingTest)

qt_add_executable(ImageDocumentTest ImageDocumentTest.cpp)
target_link_libraries(ImageDocumentTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME ImageDocumentTest COMMAND ImageDocumentTest)

qt_add_executable(CanvasWidgetTest CanvasWidgetTest.cpp)
target_link_libraries(CanvasWidgetTest PRIVATE FitToListUi FitToListCore Qt6::Test)
add_test(NAME CanvasWidgetTest COMMAND CanvasWidgetTest)
# Runs without a real display/window manager, so it works in headless CI too.
set_tests_properties(CanvasWidgetTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
