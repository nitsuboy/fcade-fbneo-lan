#ifndef APP_H
#define APP_H

#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <shlobj.h>
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

#define WIDTH       660
#define HEIGHT      580
#define MAX_FIELD   128
#define MAX_OUTPUT  4096
#define MAX_ROMS    64
#define MAX_LINES   128

#define COL_BG      CLITERAL(Color){ 22, 22, 32, 255 }
#define COL_PANEL   CLITERAL(Color){ 32, 32, 44, 255 }
#define COL_BORDER  CLITERAL(Color){ 50, 50, 70, 255 }
#define COL_ACCENT  CLITERAL(Color){ 0, 180, 255, 255 }
#define COL_ACCENT2 CLITERAL(Color){ 0, 230, 118, 255 }
#define COL_TEXT    CLITERAL(Color){ 220, 220, 230, 255 }
#define COL_DIM     CLITERAL(Color){ 110, 110, 130, 255 }

typedef enum {
    FIELD_DIR,
    FIELD_ROM,
    FIELD_PORT,
    FIELD_IP,
    FIELD_PEER_PORT,
    FIELD_COUNT
} FieldId;

typedef struct {
    char        buf[MAX_FIELD];
    FieldId     id;
    Rectangle   rect;
    float       prop;
    int         line;
    int         columns;
    int         pos;
    const char *label;
} Field;

typedef struct {
    Field      fields[FIELD_COUNT];
    int        player;
    int        windowed;
    int        selected_rom;
    int        editing_field;
    char       output[MAX_OUTPUT];
    int        output_len;
    int        output_scroll;
    ProcHandle launch_handle;
    char       status[128];
    char       roms[MAX_ROMS][64];
    int        rom_count;
    int        dirty;
    double     save_timer;
} App;

#endif
