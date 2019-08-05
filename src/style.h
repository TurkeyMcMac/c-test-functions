#ifndef STYLE_H_
#define STYLE_H_

#include <stdbool.h>

void set_styles(bool on);

const char *style_bold(void);

const char *style_fg_red(void);

const char *style_fg_green(void);

const char *style_end_all(void);

#endif /* STYLE_H_ */
