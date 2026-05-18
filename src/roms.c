#include "roms.h"
#include "config.h"

void scan_roms(App *app)
{
    char dir[512];
    roms_dir(app, dir, sizeof(dir));
#ifdef _WIN32
    WIN32_FIND_DATA fd;
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s*.zip", dir);
    TraceLog(LOG_INFO, "%s", pattern);
    HANDLE h = FindFirstFileA(pattern, &fd);

    if (h == INVALID_HANDLE_VALUE)
    {
        app->rom_count = 0; // garante estado limpo
        TraceLog(LOG_INFO, "FindFirstFile failed (Error: %lu)\n", GetLastError());
        return;
    }
    do
    {
        int len = strlen(fd.cFileName);
        if (len > 4)
        {
            char *ext = fd.cFileName + len - 4;
            // case-insensitive no Windows
            if ((ext[0] == '.' || ext[0] == '.') &&
                (ext[1] == 'z' || ext[1] == 'Z') &&
                (ext[2] == 'i' || ext[2] == 'I') &&
                (ext[3] == 'p' || ext[3] == 'P'))
                len -= 4;
        }
        int cpy = (len < 64) ? len : 63;
        memcpy(app->roms[app->rom_count], fd.cFileName, cpy);
        app->roms[app->rom_count][cpy] = 0;
        app->rom_count++;
    } while (FindNextFile(h, &fd) && app->rom_count < MAX_ROMS);

    FindClose(h);
#else
    DIR *d = opendir(dir);
    if (!d)
        return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && app->rom_count < MAX_ROMS)
    {
        int len = strlen(e->d_name);
        if (len < 5)
            continue;
        if (strcmp(e->d_name + len - 4, ".zip") != 0)
            continue;
        len -= 4;
        int cpy = len < 64 ? len : 63;
        memcpy(app->roms[app->rom_count], e->d_name, cpy);
        app->roms[app->rom_count][cpy] = 0;
        app->rom_count++;
    }
    closedir(d);
#endif
    int off = 0;
for (int i = 0; i < app->rom_count && off < (int)sizeof(app->rom_list) - 2; i++) {
    int n = snprintf(app->rom_list + off, sizeof(app->rom_list) - off,
                     "%s%s", app->roms[i], (i < app->rom_count - 1) ? ";" : "");
    if (n > 0) off += n;
}
}
