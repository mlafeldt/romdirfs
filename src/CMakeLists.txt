SET(CMAKE_C_FLAGS "-Wall -pedantic -O2 -s")

SET(SRCS romdirfs.c)

ADD_EXECUTABLE(romdirfs ${SRCS})

INSTALL(TARGETS romdirfs DESTINATION bin)
