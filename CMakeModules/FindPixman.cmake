# Try to find Pixman.
# Once done, this will define:
#   PIXMAN_FOUND
#   PIXMAN_DEFINITIONS
#   PIXMAN_INCLUDE_DIRS
#   PIXMAN_LIBRARIES

find_package(PkgConfig)

pkg_check_modules(PKG_PIXMAN QUIET "pixman-1")

set(PIXMAN_DEFINITIONS ${PKG_PIXMAN_CFLAGS_OTHER})

find_path(
	PIXMAN_INCLUDE_DIRS
	NAMES "pixman.h"
	HINTS ${PKG_PIXMAN_INCLUDE_DIRS}
)

find_library(
	PIXMAN_LIBRARIES
	NAMES "pixman-1"
	HINTS ${PKG_PIXMAN_LIBRARY_DIRS}
)

find_package_handle_standard_args(
	Pixman
	DEFAULT_MSG
	PIXMAN_LIBRARIES PIXMAN_INCLUDE_DIRS
)

mark_as_advanced(PIXMAN_LIBRARIES PIXMAN_INCLUDE_DIRS)
