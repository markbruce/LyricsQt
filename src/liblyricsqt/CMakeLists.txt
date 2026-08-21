add_library(lyricsqt
  version.cpp
  include/lyricsqt/Version.h
)
add_library(LyricsQt::lyricsqt ALIAS lyricsqt)

target_include_directories(lyricsqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(lyricsqt
  PUBLIC
    Qt6::Core
    Qt6::DBus
    Qt6::Network
)
