add_executable(resip_thread_test resip_thread_test.cc)

file(GLOB JSONCPP_OBJS ${WEBRTC_ROOT}/out/Debug/obj/third_party/jsoncpp/jsoncpp/*.obj)
message(STATUS "jsoncpp objs:" ${JSONCPP_OBJS})

add_executable(rtc_session_test rtc_session_test.cc ${JSONCPP_OBJS})
target_link_libraries(rtc_session_test PRIVATE rtc_session)

add_executable(video_render_test video_render_test.cc)
target_link_libraries(video_render_test PRIVATE rtc_call video_render)

add_executable(rtc_call_test rtc_call_test.cc ${JSONCPP_OBJS})
target_link_libraries(rtc_call_test PRIVATE rtc_call rtc_session video_render)