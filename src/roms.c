#include "roms.h"
#include "config.h"

void scan_roms(App *app) {
    char dir[512];
    roms_dir(app, dir, sizeof(dir));
#ifdef _WIN32
    WIN32_FIND_DATA fd;
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s*.zip", dir);
    HANDLE h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        int len = strlen(fd.cFileName);
        if (len > 4 && strcmp(fd.cFileName + len - 4, ".zip") == 0) len -= 4;
        int cpy = len < 64 ? len : 63;
        memcpy(app->roms[app->rom_count], fd.cFileName, cpy);
        app->roms[app->rom_count][cpy] = 0;
        app->rom_count++;
    } while (FindNextFile(h, &fd) && app->rom_count < MAX_ROMS);
    FindClose(h);
#else
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && app->rom_count < MAX_ROMS) {
        int len = strlen(e->d_name);
        if (len < 5) continue;
        if (strcmp(e->d_name + len - 4, ".zip") != 0) continue;
        len -= 4;
        int cpy = len < 64 ? len : 63;
        memcpy(app->roms[app->rom_count], e->d_name, cpy);
        app->roms[app->rom_count][cpy] = 0;
        app->rom_count++;
    }
    closedir(d);
#endif
}
