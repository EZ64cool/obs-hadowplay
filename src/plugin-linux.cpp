#if defined(__linux__) || defined(__linux) || defined(linux) || \
	defined(__gnu_linux__)

#include <obs-module.h>
#include <obs-source.h>
#include <util/platform.h>
#include <util/dstr.h>

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "plugin-platform-helpers.hpp"

// Forward declaration for the function to get process name from PID
static std::string get_process_name_by_pid(pid_t pid)
{
	char path[64] = {0};
	snprintf(path, sizeof(path), "/proc/%d/comm", pid);

	FILE *fp = fopen(path, "r");
	if (!fp) {
		return "";
	}

	char name[1024] = {0};
	if (fgets(name, sizeof(name) - 1, fp) == NULL) {
		fclose(fp);
		return "";
	}
	fclose(fp);

	// Remove trailing newline
	name[strcspn(name, "\n")] = 0;

	return std::string(name);
}

// Function to get the PID of the active X11 window
static bool get_active_window_process_name(std::string &process_name)
{
	Display *display = XOpenDisplay(NULL);
	if (!display) {
		return false;
	}

	Window root = DefaultRootWindow(display);
	Atom netActiveWindow = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
	Atom netWmPid = XInternAtom(display, "_NET_WM_PID", False);

	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;

	if (XGetWindowProperty(display, root, netActiveWindow, 0, 1, False,
			       XA_WINDOW, &actual_type, &actual_format,
			       &nitems, &bytes_after, &prop) != Success ||
	    nitems == 0) {
		if (prop)
			XFree(prop);
		XCloseDisplay(display);
		return false;
	}

	Window active_window = *(Window *)prop;
	XFree(prop);

	if (XGetWindowProperty(display, active_window, netWmPid, 0, 1, False,
			       XA_CARDINAL, &actual_type, &actual_format,
			       &nitems, &bytes_after, &prop) != Success ||
	    nitems == 0) {
		if (prop)
			XFree(prop);
		XCloseDisplay(display);
		return false;
	}

	pid_t pid = *(pid_t *)prop;
	XFree(prop);
	XCloseDisplay(display);

	process_name = get_process_name_by_pid(pid);
	return !process_name.empty();
}

void obs_hadowplay_play_sound(const char *filepath)
{
	pid_t pid = fork();
	if (pid == 0) {
		execlp("paplay", "paplay", filepath, (char *)NULL);
	}
}

std::string obs_hadowplay_cleanup_path_string(const std::string &path)
{
	std::string new_path = path;
	for (size_t i = 0; i < new_path.length(); ++i) {
		if (new_path[i] == '/') {
			new_path[i] = '_';
		}
	}
	return new_path;
}

bool obs_hadowplay_get_product_name_from_source(obs_source_t *source,
						std::string &product_name)
{
	UNUSED_PARAMETER(source);
	return get_active_window_process_name(product_name);
}

#endif