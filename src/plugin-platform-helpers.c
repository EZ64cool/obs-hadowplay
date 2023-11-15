#include "plugin-platform-helpers.h"

bool dstr_get_filename(struct dstr *filepath, struct dstr *filename)
{
	const char *filename_start = strrchr(filepath->array, '\\') + 1;

	const char *filename_end = strrchr(filename_start, '.');

	if (filename_start != NULL && filename_end != NULL) {
		size_t start = (filename_start - filepath->array);
		size_t count = (filename_end - filename_start);

		dstr_mid(filename, filepath, start, count);
	} else {
		return false;
	}

	return true;
}