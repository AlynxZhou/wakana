# Try to find Wayland.
# Once done, this will define:
#   WAYLAND_FOUND
#   WAYLAND_DEFINITIONS
#   WAYLAND_INCLUDE_DIRS
#   WAYLAND_LIBRARIES
#   WAYLAND_CLIENT_INCLUDE_DIRS
#   WAYLAND_CLIENT_LIBRARIES
#   WAYLAND_SERVER_INCLUDE_DIRS
#   WAYLAND_SERVER_LIBRARIES
#   WAYLAND_EGL_INCLUDE_DIRS
#   WAYLAND_EGL_LIBRARIES
#   WAYLAND_CURSOR_INCLUDE_DIRS
#   WAYLAND_CURSOR_LIBRARIES

find_package(PkgConfig)

pkg_check_modules(
	PKG_WAYLAND
	QUIET
	"wayland-client"
	"wayland-server"
	"wayland-egl"
	"wayland-cursor"
)

set(WAYLAND_DEFINITIONS ${PKG_WAYLAND_CFLAGS})

find_path(
	WAYLAND_CLIENT_INCLUDE_DIRS
	NAMES "wayland-client.h"
	HINTS ${PKG_WAYLAND_INCLUDE_DIRS}
)
find_path(
	WAYLAND_SERVER_INCLUDE_DIRS
	NAMES "wayland-server.h"
	HINTS ${PKG_WAYLAND_INCLUDE_DIRS}
)

find_path(
	WAYLAND_EGL_INCLUDE_DIRS
	NAMES "wayland-egl.h"
	HINTS ${PKG_WAYLAND_INCLUDE_DIRS}
)
find_path(
	WAYLAND_CURSOR_INCLUDE_DIRS
	NAMES "wayland-cursor.h"
	HINTS ${PKG_WAYLAND_INCLUDE_DIRS}
)

find_library(
	WAYLAND_CLIENT_LIBRARIES
	NAMES "wayland-client"
	HINTS ${PKG_WAYLAND_LIBRARY_DIRS}
)
find_library(
	WAYLAND_SERVER_LIBRARIES
	NAMES "wayland-server"
	HINTS ${PKG_WAYLAND_LIBRARY_DIRS}
)
find_library(
	WAYLAND_EGL_LIBRARIES
	NAMES "wayland-egl"
	HINTS ${PKG_WAYLAND_LIBRARY_DIRS}
)
find_library(
	WAYLAND_CURSOR_LIBRARIES
	NAMES "wayland-cursor"
	HINTS ${PKG_WAYLAND_LIBRARY_DIRS}
)

set(
	WAYLAND_INCLUDE_DIRS
	${WAYLAND_CLIENT_INCLUDE_DIRS}
	${WAYLAND_SERVER_INCLUDE_DIRS}
	${WAYLAND_EGL_INCLUDE_DIRS}
	${WAYLAND_CURSOR_INCLUDE_DIRS}
)

set(
	WAYLAND_LIBRARIES
	${WAYLAND_CLIENT_LIBRARIES}
	${WAYLAND_SERVER_LIBRARIES}
	${WAYLAND_EGL_LIBRARIES}
	${WAYLAND_CURSOR_LIBRARIES}
)

list(REMOVE_DUPLICATES WAYLAND_INCLUDE_DIRS)
list(REMOVE_DUPLICATES WAYLAND_LIBRARIES)

find_package_handle_standard_args(
	WAYLAND_CLIENT
	DEFAULT_MSG
	WAYLAND_CLIENT_LIBRARIES WAYLAND_CLIENT_INCLUDE_DIRS
)
find_package_handle_standard_args(
	WAYLAND_SERVER
	DEFAULT_MSG
	WAYLAND_SERVER_LIBRARIES WAYLAND_SERVER_INCLUDE_DIRS
)
find_package_handle_standard_args(
	WAYLAND_EGL
	DEFAULT_MSG
	WAYLAND_EGL_LIBRARIES WAYLAND_EGL_INCLUDE_DIRS
)
find_package_handle_standard_args(
	WAYLAND_CURSOR
	DEFAULT_MSG
	WAYLAND_CURSOR_LIBRARIES WAYLAND_CURSOR_INCLUDE_DIRS
)
find_package_handle_standard_args(
	WAYLAND
	DEFAULT_MSG
	WAYLAND_LIBRARIES WAYLAND_INCLUDE_DIRS
)

mark_as_advanced(
	WAYLAND_INCLUDE_DIRS
	WAYLAND_LIBRARIES
	WAYLAND_CLIENT_INCLUDE_DIRS
	WAYLAND_CLIENT_LIBRARIES
	WAYLAND_SERVER_INCLUDE_DIRS
	WAYLAND_SERVER_LIBRARIES
	WAYLAND_EGL_INCLUDE_DIRS
	WAYLAND_EGL_LIBRARIES
	WAYLAND_CURSOR_INCLUDE_DIRS
	WAYLAND_CURSOR_LIBRARIES
)