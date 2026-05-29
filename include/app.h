#ifndef APP_H
#define APP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI    // elimina Rectangle() do wingdi.h
#define NOMINMAX // elimina min/max macros

// Renomeia declarações do Windows que conflitam com raylib
#define CloseWindow __win32_CloseWindow
#define ShowCursor __win32_ShowCursor

#include <windows.h>
#include <shlobj.h>

// Desfaz macros do Windows que atrapalham raylib
#ifdef DrawText
#undef DrawText // DrawText → DrawTextA
#endif
#ifdef DrawTextEx
#undef DrawTextEx // DrawText → DrawTextA
#endif
#ifdef LoadImage
#undef LoadImage // LoadImage → LoadImageA
#endif

// Restaura nomes originais (não usamos CloseWindow/ShowCursor diretamente)
#undef CloseWindow
#undef ShowCursor

typedef HANDLE ProcHandle;
#define INVALID_PROC NULL
#define SEP "\\"
#define LAUNCHER ""
#else
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
typedef pid_t ProcHandle;
#define INVALID_PROC 0
#define SEP "/"
#define LAUNCHER "wine"
#endif

#include "raylib.h"
#include "discovery.h"

#define WIDTH 660
#define HEIGHT 580
#define MAX_FIELD 128
#define MAX_OUTPUT 4096
#define MAX_ROMS 64
#define MAX_LINES 128

#define COL_BG CLITERAL(Color){22, 22, 32, 255}
#define COL_PANEL CLITERAL(Color){32, 32, 44, 255}
#define COL_BORDER CLITERAL(Color){50, 50, 70, 255}
#define COL_ACCENT CLITERAL(Color){0, 180, 255, 255}
#define COL_ACCENT2 CLITERAL(Color){0, 230, 118, 255}
#define COL_TEXT CLITERAL(Color){220, 220, 230, 255}
#define COL_DIM CLITERAL(Color){110, 110, 130, 255}

typedef enum
{
    FIELD_DIR,
    FIELD_ROM,
    FIELD_IP,
    FIELD_PLAYER,
    FIELD_WINDOWED,
    FIELD_COUNT
} FieldId;

typedef struct
{
    char buf[MAX_FIELD];
    FieldId id;
    Rectangle rect;
    float prop;
    int line;
    int columns;
    int pos;
    const char *label;
} Field;

typedef struct
{
    Field fields[FIELD_COUNT];
    int player;
    int windowed;
    int selected_rom;
    int editing_field;
    char output[MAX_OUTPUT];
    int output_len;
    int output_scroll;
    ProcHandle launch_handle;
    char status[128];
    char roms[MAX_ROMS][64];
    char rom_list[4096];
    int rom_count;
    int dirty;
    double save_timer;
    DiscoveryState discovery;
    int show_peers;
    int selected_peer;
} App;

#endif
