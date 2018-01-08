aux_source_directory(. SRC)
file(GLOB INC *.h)

add_library(video_render STATIC ${INC} ${SRC})