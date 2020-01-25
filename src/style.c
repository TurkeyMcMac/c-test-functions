#include "style.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

static bool styles_on = false;

#define STYLE(code) (styles_on ? "\033["code"m" : "")

void init_styles(enum styles_setting setting)
{
	switch (setting) {
	case STYLES_AUTO:
		if (getenv("NO_COLOR")) {
			styles_on = false;
		} else {
			int errnum = errno;
			styles_on = isatty(STDOUT_FILENO);
			errno = errnum;
		}
		break;
	case STYLES_ON:
		styles_on = true;
		break;
	case STYLES_OFF:
		styles_on = false;
		break;
	}
}

const char *style_bold(void)
{
	return STYLE("1");
}

const char *style_fg_red(void)
{
	return STYLE("31");
}

const char *style_fg_green(void)
{
	return STYLE("32");
}

const char *style_end_all(void)
{
	return STYLE("0");
}
