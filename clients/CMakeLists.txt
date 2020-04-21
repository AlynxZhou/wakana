include_directories(
	${WAYLAND_CLIENT_INCLUDE_DIRS}
	${WAYLAND_EGL_INCLUDE_DIRS}
	${XDG_SHELL_PROTOCOL_INCLUDE_DIR}
	${WLR_LAYER_SHELL_PROTOCOL_INCLUDE_DIR}
	${XDG_OUTPUT_PROTOCOL_INCLUDE_DIR}
	"."
)
set_source_files_properties(
	${XDG_SHELL_PROTOCOL_CLIENT_HEADER}
	${XDG_SHELL_PROTOCOL_SOURCE}
	${WLR_LAYER_SHELL_CLIENT_PROTOCOL_HEADER}
	${WLR_LAYER_SHELL_PROTOCOL_SOURCE}
	${XDG_OUTPUT_CLIENT_PROTOCOL_HEADER}
	${XDG_OUTPUT_PROTOCOL_SOURCE}
	PROPERTIES GENERATED 1
)

add_executable(
	"wakana-shm"
	"shm.c"
	${XDG_SHELL_PROTOCOL_CLIENT_HEADER}
	${XDG_SHELL_PROTOCOL_SOURCE}
)
add_dependencies("wakana-shm" "xdg-shell-protocol")
target_compile_definitions(
	"wakana-shm"
	PUBLIC
	${WAYLAND_CLIENT_DEFINITIONS}
	${WAYLAND_EGL_DEFINITIONS}
)
target_link_libraries(
	"wakana-shm"
	${WAYLAND_CLIENT_LIBRARIES}
	${WAYLAND_EGL_LIBRARIES}
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

add_executable(
	"wakana-background"
	"background.c"
	${XDG_SHELL_PROTOCOL_CLIENT_HEADER}
	${XDG_SHELL_PROTOCOL_SOURCE}
	${WLR_LAYER_SHELL_CLIENT_PROTOCOL_HEADER}
	${WLR_LAYER_SHELL_PROTOCOL_SOURCE}
	${XDG_OUTPUT_CLIENT_PROTOCOL_HEADER}
	${XDG_OUTPUT_PROTOCOL_SOURCE}
)
add_dependencies("wakana-background" "xdg-shell-protocol")
add_dependencies("wakana-background" "wlr-layer-shell-protocol")
add_dependencies("wakana-background" "xdg-output-protocol")
target_compile_definitions(
	"wakana-background"
	PUBLIC
	${WAYLAND_CLIENT_DEFINITIONS}
	${WAYLAND_EGL_DEFINITIONS}
)
target_link_libraries(
	"wakana-background"
	${WAYLAND_CLIENT_LIBRARIES}
	${WAYLAND_EGL_LIBRARIES}
	"EGL"
	"GLESv2"
	"loaders"
	"mvmath"
	"stb"
	"m"
)