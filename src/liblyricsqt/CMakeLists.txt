add_library(lyricsqt
  version.cpp
  settings/AppSettings.cpp
  lyrics/LyricsDocument.cpp
  lyrics/LrcParser.cpp
  lyrics/LyricsSession.cpp
  store/LyricsStore.cpp
  controller/LyricsController.cpp
  player/MprisPlayerBackend.cpp
  player/PlayerService.cpp
  include/lyricsqt/Version.h
  include/lyricsqt/AppSettings.h
  include/lyricsqt/LyricsDocument.h
  include/lyricsqt/LrcParser.h
  include/lyricsqt/LyricsSession.h
  include/lyricsqt/LyricsStore.h
  include/lyricsqt/LyricsController.h
  include/lyricsqt/TrackInfo.h
  include/lyricsqt/PlayerService.h
  player/MprisPlayerBackend.h
)
add_library(LyricsQt::lyricsqt ALIAS lyricsqt)

target_include_directories(lyricsqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lyricsqt
  PUBLIC
    Qt6::Core
    Qt6::DBus
    Qt6::Network
)
