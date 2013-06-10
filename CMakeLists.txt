PROJECT(visualization.waveform)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

IF(WIN32)
  SET(BINDING_FILE ${XBMC_BINDINGS}.zip)
  message (STATUS "downloading XBMC bindings: " ${BINDING_FILE})
  file(DOWNLOAD http://mirrors.xbmc.org/build-deps/win32/${BINDING_FILE} ${CMAKE_BINARY_DIR}/${BINDING_FILE} STATUS STATUSLIST SHOW_PROGRESS)
  LIST(GET STATUSLIST 0 VALUE)
  IF(${VALUE} STRGREATER "0")
    LIST(GET STATUSLIST 1 VALUE)
    message (STATUS "failed to download XBMC bindings: " ${VALUE})
  ENDIF(${VALUE} STRGREATER "0")
  execute_process(
   COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/${BINDING_FILE}
  )
  FIND_PATH(XBMC_INCLUDE_DIR xbmc/xbmc_addon_dll.h ${CMAKE_BINARY_DIR}/include)
  INCLUDE_DIRECTORIES(${XBMC_INCLUDE_DIR})
  ADD_DEFINITIONS(-DTARGET_WINDOWS)
ELSE(WIN32)
  FIND_PATH(XBMC_INCLUDE_DIR xbmc/xbmc_addon_dll.h)
  FIND_PACKAGE(OpenGL REQUIRED)
  FIND_PACKAGE(GLEW REQUIRED)
  ADD_DEFINITIONS(-DHAS_SDL_OPENGL)
  INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIR}
                      ${GLEW_INCLUDE_DIR}
                      ${XBMC_INCLUDE_DIR})
ENDIF(WIN32)

SET(SOURCES src/Main.cpp)

ADD_LIBRARY(waveform ${SOURCES})
IF(NOT WIN32)
  TARGET_LINK_LIBRARIES(waveform ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})
ENDIF(NOT WIN32)
SET_TARGET_PROPERTIES(waveform PROPERTIES VERSION 1.0.0
                                          SOVERSION 13.0
                                          PREFIX "")

IF(WIN32)
  # Pack files together to create an archive
  INSTALL(DIRECTORY visualization.waveform DESTINATION ./)
  INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/waveform.dll DESTINATION visualization.waveform)
  SET(CPACK_GENERATOR "ZIP")
  SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  #SET(CPACK_PACKAGE_FILE_NAME "waveform.test")
  INCLUDE(CPack) 
ELSE(WIN32)                                       
  INSTALL(TARGETS waveform DESTINATION lib/xbmc/addons/visualization.waveform)
  INSTALL(DIRECTORY visualization.waveform DESTINATION share/xbmc/addons)
ENDIF(WIN32)
