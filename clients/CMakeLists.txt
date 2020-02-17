include_directories(
	${WAYLAND_CLIENT_INCLUDE_DIRS}
	${WAYLAND_EGL_INCLUDE_DIRS}
	${XDG_SHELL_PROTOCOL_INCLUDE_DIR}
	"."
)
set_source_files_properties(
	${XDG_SHELL_PROTOCOL_CLIENT_HEADER}
	${XDG_SHELL_PROTOCOL_SOURCE}
	PROPERTIES GENERATED 1
)

add_executable(
	"wakana-egl"
	"egl.c"
	${XDG_SHELL_PROTOCOL_CLIENT_HEADER}
	${XDG_SHELL_PROTOCOL_SOURCE}
)
add_dependencies("wakana-egl" "xdg-shell-protocol")
target_compile_definitions(
	"wakana-egl"
	PUBLIC
	${WAYLAND_CLIENT_DEFINITIONS}
	${WAYLAND_EGL_DEFINITIONS}
)
target_link_libraries(
	"wakana-egl"
	${WAYLAND_CLIENT_LIBRARIES}
	${WAYLAND_EGL_LIBRARIES}
	"EGL"
	"GLESv2"
	"loaders"
	"mvmath"
	"m"
)