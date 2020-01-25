#ifndef STYLE_H_
#define STYLE_H_

/* See init_styles for details. */
enum styles_setting {
	STYLES_AUTO,
	STYLES_ON,
	STYLES_OFF,
};

/* Initialize the styles system. This must be called before using other
 * functions. STYLES_ON forces styles on, while STYLES_OFF forces them off.
 * STYLES_AUTO decides based on predicted client preference. */
void init_styles(enum styles_setting setting);

/* Returns the bold style start if styles are on. */
const char *style_bold(void);

/* Returns the red foreground style start if styles are on. */
const char *style_fg_red(void);

/* Returns the green foreground style start if styles are on. */
const char *style_fg_green(void);

/* Returns the code to end all styles if styles are on. */
const char *style_end_all(void);

#endif /* STYLE_H_ */
