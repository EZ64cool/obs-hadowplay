#include <util/dstr.h>

const char *dstr_find_last(struct dstr *src, char c)
{
	int i = 0;
	const char *pos = NULL;
	while (i < src->len) {
		if (src->array[i] == c) {
			pos = &src->array[i];
		}

		i++;
	}

	return pos;
}