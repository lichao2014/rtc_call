include_directories(${RESIP_ROOT})

aux_source_directory(. SRC)
file(GLOB INC *.h)

add_library(rtc_session STATIC ${INC} ${SRC})
