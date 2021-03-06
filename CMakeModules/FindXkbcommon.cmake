# Try to find Xkbcommon.
# Once done, this will define:
#   XKBCOMMON_FOUND
#   XKBCOMMON_DEFINITIONS
#   XKBCOMMON_INCLUDE_DIRS
#   XKBCOMMON_LIBRARIES

find_package(PkgConfig)

pkg_check_modules(PKG_XKBCOMMON QUIET "xkbcommon")

set(XKBCOMMON_DEFINITIONS ${PKG_XKBCOMMON_CFLAGS_OTHER})

find_path(
	XKBCOMMON_INCLUDE_DIRS
	NAMES "xkbcommon/xkbcommon.h"
	HINTS ${PKG_XKBCOMMON_INCLUDE_DIRS}
)

find_library(
	XKBCOMMON_LIBRARIES
	NAMES "xkbcommon"
	HINTS ${PKG_XKBCOMMON_LIBRARY_DIRS}
)

find_package_handle_standard_args(
	Xkbcommon
	DEFAULT_MSG
	XKBCOMMON_LIBRARIES XKBCOMMON_INCLUDE_DIRS
)

mark_as_advanced(XKBCOMMON_LIBRARIES XKBCOMMON_INCLUDE_DIRS)
