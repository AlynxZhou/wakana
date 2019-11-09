# Try to find WaylandScanner.
# Once done, this will define:
#   WAYLAND_SCANNER_FOUND
#   WAYLAND_SCANNER_EXECUTABLE

find_program(WAYLAND_SCANNER_EXECUTABLE NAMES "wayland-scanner")

find_package_handle_standard_args(
	WAYLAND_SCANNER
	DEFAULT_MSG
	WAYLAND_SCANNER_EXECUTABLE
)

mark_as_advanced(WAYLAND_SCANNER_EXECUTABLE)
