#ifndef STYLE_H_
#define STYLE_H_

#include <stdbool.h>

/* Set styles on or off, overriding the default that depends on whether the
 * output is a tty. */
void set_styles(bool on);

/* Returns the bold style start if styles are on. */
const char *style_bold(void);

/* Returns the red foreground style start if styles are on. */
const char *style_fg_red(void);

/* Returns the green foreground style start if styles are on. */
const char *style_fg_green(void);

/* Returns the code to end all styles if styles are on. */
const char *style_end_all(void);

#endif /* STYLE_H_ */
