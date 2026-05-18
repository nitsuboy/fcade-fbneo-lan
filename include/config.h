#ifndef CONFIG_H
#define CONFIG_H

#include "app.h"

void ini_path(char *buf, int sz);
void ini_load(App *app);
void ini_save(App *app);
const char *fc_dir(App *app);
void roms_dir(App *app, char *buf, int sz);
void emu_path(App *app, char *buf, int sz);

#endif
