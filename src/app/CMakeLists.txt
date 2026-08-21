add_executable(lyricsqt-app
  main.cpp
  AutostartHelper.cpp
  AutostartHelper.h
  ui/DesktopLyricsWindow.cpp
  ui/DesktopLyricsWindow.h
  ui/KaraokeLyricLabel.cpp
  ui/KaraokeLyricLabel.h
  ui/LyricsHudWindow.cpp
  ui/LyricsHudWindow.h
  ui/SearchLyricsDialog.cpp
  ui/SearchLyricsDialog.h
  ui/PreferencesDialog.cpp
  ui/PreferencesDialog.h
  ui/TrayController.cpp
  ui/TrayController.h
  ${CMAKE_SOURCE_DIR}/resources/lyricsqt.qrc
)
set_target_properties(lyricsqt-app PROPERTIES OUTPUT_NAME lyricsqt)
target_include_directories(lyricsqt-app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lyricsqt-app PRIVATE LyricsQt::lyricsqt Qt6::Widgets)

# Desktop entry for install + optional XDG autostart (runtime copy uses AutostartHelper).
set(LYRICSQT_EXEC "lyricsqt")
configure_file(
  ${CMAKE_SOURCE_DIR}/resources/lyricsqt.desktop.in
  ${CMAKE_BINARY_DIR}/lyricsqt.desktop
  @ONLY
)
install(FILES ${CMAKE_BINARY_DIR}/lyricsqt.desktop
        DESTINATION share/applications)
install(TARGETS lyricsqt-app RUNTIME DESTINATION bin)
