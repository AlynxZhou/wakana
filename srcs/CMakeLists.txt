find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -std=gnu11 -Wall -g -D__DEBUG__")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -std=gnu11")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

find_package(Wayland REQUIRED Server)
find_program(WaylandScanner NAMES wayland-scanner)
set(Pixman_INCLUDE_DIRS "/usr/include/pixman-1/")
set(
	XDG_SHELL_PROTOCOL_XML
	"/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
)
set(
	XDG_SHELL_PROTOCOL_HEADER
	"${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.h"
)
set(
	XDG_SHELL_PROTOCOL_SOURCE
	"${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
)
set_source_files_properties(${XDG_SHELL_PROTOCOL_HEADER} GENERATED)
set_source_files_properties(${XDG_SHELL_PROTOCOL_SOURCE} GENERATED)
add_custom_command(
	OUTPUT ${XDG_SHELL_PROTOCOL_HEADER}
	COMMAND ${WaylandScanner} server-header ${XDG_SHELL_PROTOCOL_XML} ${XDG_SHELL_PROTOCOL_HEADER}
	VERBATIM
)

add_custom_command(
	OUTPUT ${XDG_SHELL_PROTOCOL_SOURCE}
	COMMAND ${WaylandScanner} private-code ${XDG_SHELL_PROTOCOL_XML} ${XDG_SHELL_PROTOCOL_SOURCE}
	VERBATIM
)
aux_source_directory("." WAKANA_SOURCES)
add_executable(
	"wakana"
	${WAKANA_SOURCES}
	${XDG_SHELL_PROTOCOL_HEADER}
	${XDG_SHELL_PROTOCOL_SOURCE}
)
include_directories(
	${Wayland_INCLUDE_DIRS}
	${Pixman_INCLUDE_DIRS}
	${CMAKE_CURRENT_BINARY_DIR}
	"."
)
target_compile_definitions("wakana" PUBLIC ${Wayland_DEFINITIONS})
target_link_libraries("wakana" "wlroots" ${Wayland_LIBRARIES})
