#ifndef PROCESS_H
#define PROCESS_H

#include "app.h"

void build_cmd(App *app, int side, char *out, int out_sz);
void launch_emulator(App *app);
void check_child(App *app);

#endif
