qt_add_executable(FileNamingTest FileNamingTest.cpp)
target_link_libraries(FileNamingTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME FileNamingTest COMMAND FileNamingTest)

qt_add_executable(ImageDocumentTest ImageDocumentTest.cpp)
target_link_libraries(ImageDocumentTest PRIVATE FitToListCore Qt6::Test)
add_test(NAME ImageDocumentTest COMMAND ImageDocumentTest)
