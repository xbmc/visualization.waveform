project(visualization.waveform)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(kodi REQUIRED)

if(WIN32)
  find_package(DirectX REQUIRED)
  set(INCLUDES ${DIRECTX_INCLUDE_DIR})
  set(DEPLIBS ${DIRECTX_D3DX9_LIBRARY})
else()
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
  add_definitions(-DHAS_SDL_OPENGL)
  set(INCLUDES ${OpenGL_INCLUDE_DIR}
               ${GLEW_INCLUDE_DIR})
  set(DEPLIBS ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
endif()

include_directories(${INCLUDES}
                    ${KODI_INCLUDE_DIR})

set(WAVEFORM_SOURCES src/Main.cpp)

build_addon(visualization.waveform WAVEFORM DEPLIBS)

include(CPack)
