# Try to find Wlroots.
# Once done, this will define:
#   WLROOTS_FOUND
#   WLROOTS_DEFINITIONS
#   WLROOTS_INCLUDE_DIRS
#   WLROOTS_LIBRARIES

find_package(PkgConfig)

pkg_check_modules(PKG_WLROOTS QUIET "wlroots")

set(WLROOTS_DEFINITIONS ${PKG_WLROOTS_CFLAGS_OTHER})

find_path(
	WLROOTS_INCLUDE_DIRS
	NAMES "wlr/version.h"
	HINTS ${PKG_WLROOTS_INCLUDE_DIRS}
)

find_library(
	WLROOTS_LIBRARIES
	NAMES "wlroots"
	HINTS ${PKG_WLROOTS_LIBRARY_DIRS}
)

find_package_handle_standard_args(
	Wlroots
	DEFAULT_MSG
	WLROOTS_LIBRARIES WLROOTS_INCLUDE_DIRS
)

mark_as_advanced(WLROOTS_LIBRARIES WLROOTS_INCLUDE_DIRS)
