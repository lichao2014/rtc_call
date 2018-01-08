cmake_minimum_required(VERSION 2.8)

project(rtc_call)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(USE_RUNTIME_STATIC ON)

if (USE_RUNTIME_STATIC)
set(config_flags CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
	foreach(flag ${config_flags})
		if (${flag} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
		endif()
	endforeach()
endif()

add_definitions(-DWEBRTC_WIN -DNOMINMAX)

set(RESIP_ROOT E:/3rd/resip/resiprocate-1.10.2)
set(WEBRTC_ROOT E:/webrtc-checkout/src)
set(SDL2_ROOT "C:/Program Files/SDL2")

include_directories(
	src 
	${WEBRTC_ROOT}
	${WEBRTC_ROOT}/third_party/jsoncpp/source/include
	${SDL2_ROOT}/include)

link_directories(
	${RESIP_ROOT}/x64 
	${WEBRTC_ROOT}/out/Debug/obj
	${SDL2_ROOT}/lib)

link_libraries(
	SDL2d
	SDL2maind
	webrtc
	webrtc_common
	dum
	resiprocate
	rutil
	ares)

if (WIN32)
	link_libraries(
		wmcodecdspuuid 
		strmiids
		dmoguids
		msdmo
		secur32
		winmm
		ws2_32)
endif()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(samples)