# TODO get info from: pkg-config fuse --cflags --libs

SET(CMAKE_C_FLAGS "-Wall -O2 -s")

ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64)

INCLUDE_DIRECTORIES(/usr/include/fuse)

ADD_EXECUTABLE(romdirfs romdir.c romdirfs.c)
TARGET_LINK_LIBRARIES(romdirfs pthread fuse rt dl)
INSTALL(TARGETS romdirfs DESTINATION bin)

ADD_EXECUTABLE(romdir romdir.c)
SET_TARGET_PROPERTIES(romdir PROPERTIES COMPILE_FLAGS -D_ROMDIR_STANDALONE)
INSTALL(TARGETS romdir DESTINATION bin)
