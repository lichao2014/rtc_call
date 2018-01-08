add_subdirectory(session)
add_subdirectory(render)

aux_source_directory(. SRC)
file(GLOB INC *.h)

add_library(rtc_call STATIC ${INC} ${SRC})