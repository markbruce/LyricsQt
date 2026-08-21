add_executable(test_app_settings test_app_settings.cpp)
target_link_libraries(test_app_settings PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME AppSettings COMMAND test_app_settings)
