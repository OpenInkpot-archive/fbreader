#ifndef ZLEWLCHOICEBOX_H
#define ZLEWLCHOICEBOX_H

#include <Ewl.h>

typedef void (*choice_handler)(int choice, Ewl_Widget *parent);
Ewl_Widget *init_choicebox(const char *choices[], int numchoices,choice_handler handler, Ewl_Widget *parent, bool master = false);
void fini_choicebox(Ewl_Widget *win);
char *get_theme_file();

#endif