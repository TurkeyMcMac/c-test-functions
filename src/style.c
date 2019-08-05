#include "style.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

static bool styles_set = false;
static bool styles_on = false;

static bool doing_styles(void)
{
	if (styles_set) {
		return styles_on;
	} else if (getenv("NO_COLOR")) {
		return false;
	} else {
		int errnum = errno;
		bool on = isatty(STDOUT_FILENO);
		errno = errnum;
		return on;
	}
}

#define STYLE(code) "\033["code"m"

void set_styles(bool on)
{
	styles_set = true;
	styles_on = on;
}

const char *style_bold(void)
{
	return doing_styles() ? STYLE("1") : "";
}

const char *style_fg_red(void)
{
	return doing_styles() ? STYLE("31") : "";
}

const char *style_fg_green(void)
{
	return doing_styles() ? STYLE("32") : "";
}

const char *style_end_all(void)
{
	return doing_styles() ? STYLE("0") : "";
}
