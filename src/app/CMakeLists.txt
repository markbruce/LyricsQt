add_executable(lyricsqt-app
  main.cpp
  ui/DesktopLyricsWindow.cpp
  ui/DesktopLyricsWindow.h
  ui/TrayController.cpp
  ui/TrayController.h
  ${CMAKE_SOURCE_DIR}/resources/lyricsqt.qrc
)
set_target_properties(lyricsqt-app PROPERTIES OUTPUT_NAME lyricsqt)
target_include_directories(lyricsqt-app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lyricsqt-app PRIVATE LyricsQt::lyricsqt Qt6::Widgets)
