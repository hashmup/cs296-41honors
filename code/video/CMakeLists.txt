cmake_minimum_required(VERSION 2.8)
project( video )

find_package( OpenCV 3 REQUIRED )

include_directories(${PROJECT_SOURCE_DIR}/src)
add_library(VIDEO src/video.h src/video.cpp)
set(MAIN ${VIDEO} src/main.cpp src/audio.h)
add_executable( video ${MAIN} )
target_link_libraries( video ${OpenCV_LIBS} portaudio asound)
