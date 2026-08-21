add_executable(test_app_settings test_app_settings.cpp)
target_link_libraries(test_app_settings PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME AppSettings COMMAND test_app_settings)

add_executable(test_lrc_parser test_lrc_parser.cpp)
target_link_libraries(test_lrc_parser PRIVATE LyricsQt::lyricsqt Qt6::Test)
target_compile_definitions(test_lrc_parser PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME LrcParser COMMAND test_lrc_parser)
