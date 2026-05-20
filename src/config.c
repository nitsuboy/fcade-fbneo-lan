#include "config.h"

static const char *ini_key(const char *line)
{
    while (*line == ' ' || *line == '\t')
        line++;
    if (*line == '#' || *line == '\n' || *line == 0)
        return NULL;
    return line;
}

static char *ini_val(const char *key, char *out, int sz)
{
    const char *eq = strchr(key, '=');
    if (!eq)
        return NULL;
    const char *v = eq + 1;
    while (*v == ' ')
        v++;
    int len = strlen(v);
    while (len > 0 && (v[len - 1] == '\n' || v[len - 1] == '\r' || v[len - 1] == ' '))
        len--;
    int cpy = len < sz - 1 ? len : sz - 1;
    memcpy(out, v, cpy);
    out[cpy] = 0;
    return out;
}

static int ini_match(const char *line, const char *key)
{
    // pula espaços iniciais
    while (*line == ' ' || *line == '\t')
        line++;

    int klen = strlen(key);
    if (strncmp(line, key, klen) != 0)
        return 0;

    line += klen;
    // pula espaços depois da chave
    while (*line == ' ' || *line == '\t')
        line++;

    return *line == '=';
}

void ini_path(char *buf, int sz)
{
    buf[0] = 0;
#ifdef _WIN32
    wchar_t wide[512];
    if (GetModuleFileNameW(NULL, wide, 512))
    {
        char utf8[512];
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), NULL, NULL);
        char *bs = strrchr(utf8, '\\');
        if (bs)
            *bs = 0;
        snprintf(buf, sz, "%s\\fclauncher.ini", utf8);
    }
#else
    char link[512] = {0};
    ssize_t r = readlink("/proc/self/exe", link, sizeof(link) - 1);
    if (r > 0)
    {
        link[r] = 0;
        char *slash = strrchr(link, '/');
        if (slash)
            *slash = 0;
        snprintf(buf, sz, "%s/fclauncher.ini", link);
    }
#endif
    if (buf[0] == 0)
        snprintf(buf, sz, "fclauncher.ini");
}

void ini_load(App *app)
{
    char path[512];
    ini_path(path, sizeof(path));
    TraceLog(LOG_DEBUG, path);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        TraceLog(LOG_DEBUG, line);
        if (!ini_key(line))
            continue;
        char v[1024];
        if (ini_match(line, "fightcade_dir"))
        {
            if (ini_val(line, v, sizeof(v)))
                snprintf(app->fields[FIELD_DIR].buf,MAX_FIELD,v);
        }
        else if (ini_match(line, "rom"))
        {
            if (ini_val(line, v, sizeof(v)))
                snprintf(app->fields[FIELD_ROM].buf,MAX_FIELD,v);
        }
        else if (ini_match(line, "peer_ip"))
        {
            if (ini_val(line, v, sizeof(v)))
                snprintf(app->fields[FIELD_IP].buf,MAX_FIELD,v);
        }
        else if (ini_match(line, "player"))
        {
            if (ini_val(line, v, sizeof(v)))
                app->player = atoi(v);
        }
        else if (ini_match(line, "windowed"))
        {
            if (ini_val(line, v, sizeof(v)))
                app->windowed = atoi(v);
        }
    }
    fclose(f);
}

void ini_save(App *app)
{
    char path[512];
    ini_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "# FBNeo LAN Launcher config\n\n");
    fprintf(f, "fightcade_dir = %s\n", app->fields[FIELD_DIR].buf);
    fprintf(f, "rom = %s\n", app->fields[FIELD_ROM].buf);
    fprintf(f, "peer_ip = %s\n", app->fields[FIELD_IP].buf);
    fprintf(f, "player = %d\n", app->player);
    fprintf(f, "windowed = %d\n", app->windowed);
    fclose(f);
}

const char *fc_dir(App *app)
{
    if (app->fields[FIELD_DIR].buf[0])
        return app->fields[FIELD_DIR].buf;
    const char *env = getenv("FIGHTCADE_DIR");
    return env ? env : "";
}

void roms_dir(App *app, char *buf, int sz)
{
    #ifdef _WIN32
        snprintf(buf, sz, "%s" SEP "emulator" SEP "fbneo" SEP "ROMs" SEP, fc_dir(app));
    #else
        snprintf(buf, sz, "%s" SEP "ROMs" SEP "FBNeo ROMs" SEP, fc_dir(app));
    #endif
}

void emu_path(App *app, char *buf, int sz)
{
    snprintf(buf, sz, "%s" SEP "emulator" SEP "fbneo" SEP "fcadefbneo.exe", fc_dir(app));
}
