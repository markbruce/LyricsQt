add_executable(test_app_settings test_app_settings.cpp)
target_link_libraries(test_app_settings PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME AppSettings COMMAND test_app_settings)

add_executable(test_lrc_parser test_lrc_parser.cpp)
target_link_libraries(test_lrc_parser PRIVATE LyricsQt::lyricsqt Qt6::Test)
target_compile_definitions(test_lrc_parser PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME LrcParser COMMAND test_lrc_parser)

add_executable(test_lyrics_session_sync test_lyrics_session_sync.cpp)
target_link_libraries(test_lyrics_session_sync PRIVATE LyricsQt::lyricsqt Qt6::Test)
target_compile_definitions(test_lyrics_session_sync PRIVATE SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME LyricsSessionSync COMMAND test_lyrics_session_sync)

add_executable(test_lyrics_store test_lyrics_store.cpp)
target_link_libraries(test_lyrics_store PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME LyricsStore COMMAND test_lyrics_store)

add_executable(test_quality_scorer test_quality_scorer.cpp)
target_link_libraries(test_quality_scorer PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME QualityScorer COMMAND test_quality_scorer)

add_executable(test_lyrics_filter test_lyrics_filter.cpp)
target_link_libraries(test_lyrics_filter PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME LyricsFilter COMMAND test_lyrics_filter)

add_executable(test_export_server test_export_server.cpp)
target_link_libraries(test_export_server PRIVATE LyricsQt::lyricsqt Qt6::Test)
add_test(NAME ExportServer COMMAND test_export_server)
