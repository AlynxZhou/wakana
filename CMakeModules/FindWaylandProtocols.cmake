# Try to find WaylandProtocols.
# Once done, this will define:
#   WAYLAND_PROTOCOLS_FOUND
#   WAYLAND_PROTOCOLS_PKGDATADIR

find_package(PkgConfig)

pkg_check_modules(
	PKG_WAYLAND_PROTOCOLS
	QUIET
	"wayland-protocols>=${WaylandProtocols_FIND_VERSION}"
)

execute_process(
	COMMAND ${PKG_CONFIG_EXECUTABLE} "--variable=pkgdatadir" "wayland-protocols"
	OUTPUT_VARIABLE WAYLAND_PROTOCOLS_PKGDATADIR
	RESULT_VARIABLE COMMAND_FAILED
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(COMMAND_FAILED)
	message(FATAL_ERROR "Missing wayland-protocols pkgdatadir")
endif()

find_package_handle_standard_args(
	WaylandProtocols
	DEFAULT_MSG
	WAYLAND_PROTOCOLS_PKGDATADIR
)

mark_as_advanced(WAYLAND_PROTOCOLS_PKGDATADIR)
