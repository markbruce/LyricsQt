add_executable(lyricsqt-app main.cpp)
set_target_properties(lyricsqt-app PROPERTIES OUTPUT_NAME lyricsqt)
target_link_libraries(lyricsqt-app PRIVATE LyricsQt::lyricsqt Qt6::Widgets)
