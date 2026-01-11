cmake_minimum_required(VERSION 3.7)

project(vsgts
	    VERSION 0.0.0
	    DESCRIPTION "VSG Train Simulator"
	    LANGUAGES CXX
)

# build all examples into the bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if (VULKAN_SDK)
	set(ENV{VULKAN_SDK} ${VULKAN_SDK})
endif()
find_package(vsg 1.0.0 REQUIRED)
find_package(vsgXchange 1.0.0 REQUIRED)
find_package(vsgImGui 0.7.0 REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_custom_target(clobber
	COMMAND git clean -d -f -x
)

set(SOURCES
	mstsfile.cc
	mstsbfile.cc
	mstsace.cc
	mstsshape.cc
	mstsroute.cc
	tsection.cc
	trackdb.cc
	track.cc
	mststerrain.cc
	mstsworld.cc
	trackshape.cc
	camerac.cc
	train.cc
	railcar.cc
	airbrake.cc
	airtank.cc
	brakevalve.cc
	signal.cc
	listener.cc
	morse.cc
	ttosim.cc
	timetable.cc
	dispatcher.cc
	switcher.cc
	trackpath.cc
	mstswag.cc
	locoeng.cc
	activity.cc
	service.cc
	consist.cc
	trainc.cc
)

add_executable(tsviewer tsviewer.cc ${SOURCES})
target_link_libraries(tsviewer vsg::vsg z plibul plibsl openal)
target_compile_definitions(tsviewer PRIVATE vsgXchange_FOUND)
target_link_libraries(tsviewer vsgXchange::vsgXchange)

add_executable(vsgts vsgts.cc tsgui.cc ${SOURCES})
target_link_libraries(vsgts vsgImGui::vsgImGui vsg::vsg z plibul plibsl openal)
target_compile_definitions(vsgts PRIVATE vsgXchange_FOUND)
target_link_libraries(vsgts vsgXchange::vsgXchange)

install(TARGETS tsviewer vsgts
	RUNTIME DESTINATION bin
)
