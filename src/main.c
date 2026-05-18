#include "app.h"
#include "config.h"
#include "process.h"
#include "roms.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define LX 120
#define LY 48
#define LS 32

#ifdef _WIN32
static int browse_folder(char *out, int sz)
{
    BROWSEINFOW bi = {0};
    wchar_t path[512];
    bi.lpszTitle = L"Select Fightcade folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    out[0] = 0;
    if (pidl && SHGetPathFromIDListW(pidl, path))
        WideCharToMultiByte(CP_UTF8, 0, path, -1, out, sz, NULL, NULL);
    CoTaskMemFree(pidl);
    return out[0] != 0;
}
#else
static int browse_folder(char *out, int sz)
{
    out[0] = 0;
    FILE *fp = popen("zenity --file-selection --directory 2>/dev/null", "r");
    if (!fp)
        fp = popen("kdialog --getexistingdirectory . 2>/dev/null", "r");
    if (!fp)
        return 0;
    if (fgets(out, sz, fp))
    {
        int len = strlen(out);
        while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
            out[--len] = 0;
    }
    pclose(fp);
    return out[0] != 0;
}
#endif

static void field_init(
    Field *f, FieldId id, const char *label, const char *def, int pos, int line, int col)
{
    f->id = id;
    f->label = label;
    f->pos = pos;
    f->line = line;
    f->columns = col;
    strncpy(f->buf, def, MAX_FIELD - 1);
}

int main(void)
{
    App app = {0};
    char rom_list[4096] = {0};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH, HEIGHT, "FBNeo LAN Launcher");
    SetTargetFPS(60);
    ini_load(&app);
    GuiLoadStyleDefault();
    scan_roms(&app);

    TraceLog(LOG_INFO, "%s", app.fields[FIELD_DIR].buf);
    field_init(&app.fields[FIELD_DIR], FIELD_DIR, "Fightcade Dir", app.fields[FIELD_DIR].buf,
               0, 0, 1);
    field_init(&app.fields[FIELD_ROM], FIELD_ROM, "ROM", "",
               0, 2, 1);
    field_init(&app.fields[FIELD_PORT], FIELD_PORT, "My Port", app.fields[FIELD_PORT].buf,
               0, 3, 1);
    field_init(&app.fields[FIELD_PEER_PORT], FIELD_PEER_PORT, "Peer Port", app.fields[FIELD_PEER_PORT].buf,
               0, 4, 1);
    field_init(&app.fields[FIELD_IP], FIELD_IP, "Peer IP", app.fields[FIELD_IP].buf,
               0, 5, 1);

    app.editing_field = -1;
    app.selected_rom = -1;

    int off = 0;
    for (int i = 0; i < app.rom_count && off < (int)sizeof(rom_list) - 2; i++)
    {
        int n = snprintf(rom_list + off, sizeof(rom_list) - off,
                         "%s;", app.roms[i]);
        if (n > 0)
            off += n;
    }

    for (int i = 0; i < app.rom_count; i++)
        if (strcmp(app.fields[FIELD_ROM].buf, app.roms[i]) == 0)
            app.selected_rom = i;

    while (!WindowShouldClose())
    {
        if (app.editing_field == FIELD_ROM)
            GuiLock();

        int w = GetScreenWidth();
        int h = GetScreenHeight();

        check_child(&app);

        /* --- text fields (raygui) --- */
        float fw = w - 160; // largura dos campos
        for (int i = 0; i < FIELD_COUNT; i++)
        {
            app.fields[i].rect = (Rectangle){
                LX + ((fw) / app.fields[i].columns) * app.fields[i].pos,
                LY + LS * app.fields[i].line,
                ((fw) / app.fields[i].columns),
                28};
            if (i == FIELD_ROM)
                continue;

            if (GuiTextBox(app.fields[i].rect, app.fields[i].buf, MAX_FIELD, app.editing_field == i))
            {
                app.editing_field = (app.editing_field == i) ? -1 : i;
                app.dirty = 1;
            }
        }

        /* tab / enter navigation */
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_DOWN))
        {
            if (app.editing_field >= 0)
                app.editing_field = (app.editing_field + 1) % FIELD_COUNT;
            else
                app.editing_field = 0;
        }
        if (IsKeyPressed(KEY_UP) && app.editing_field >= 0)
        {
            app.editing_field = (app.editing_field + FIELD_COUNT - 1) % FIELD_COUNT;
        }
        if (IsKeyPressed(KEY_ENTER) && app.editing_field >= 0)
        {
            app.editing_field = -1;
            if (app.launch_handle == INVALID_PROC)
                launch_emulator(&app);
        }

        /* browse button */
        Rectangle browse_btn = {
            LX,
            LY + LS * 1,
            fw,
            28};
        if (GuiButton(browse_btn, "Browse"))
        {
            char path[512];
            app.editing_field = -1;
            if (browse_folder(path, sizeof(path)))
            {
                strncpy(app.fields[FIELD_DIR].buf, path, MAX_FIELD - 1);
                app.dirty = 1;
                app.rom_count = 0;
                scan_roms(&app);
            }
        }

        /* radios */
        /* radio labels */
        int rx = LX, ry = LY + LS * 6 + 4;
        GuiToggleGroup((Rectangle){rx, ry, w / 2 - 80, 22}, "P1;P2", &app.player);
        DrawText("Player", rx - MeasureText("Player", 14) - 8, ry, 14, COL_DIM);
        ry += 28;
        GuiToggleGroup((Rectangle){rx, ry, w / 2 - 80, 22}, "Fullscreen;Windowed", &app.windowed);
        DrawText("Window", rx - MeasureText("Window", 14) - 8, ry, 14, COL_DIM);

        /* launch button */
        int by = ry + 40;
        Color btn_col = app.launch_handle != INVALID_PROC ? COL_DIM : COL_ACCENT2;
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(btn_col));
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(COL_BG));
        Rectangle btn_rec = {w / 2 - 100, by, 200, 34};
        if (GuiButton(btn_rec, "LAUNCH GAME") && app.launch_handle == INVALID_PROC)
        {
            launch_emulator(&app);
        }
        GuiLoadStyleDefault();

        /* output box */
        int oy = by + 48;
        Rectangle out_box = {10, oy, w - 20, h - oy - 30};
        int wheel = GetMouseWheelMove();
        if (wheel)
        {
            app.output_scroll -= wheel * 16;
            if (app.output_scroll < 0)
                app.output_scroll = 0;
        }
        int line_h = 16;
        int lines = 0;
        for (int i = 0; i < app.output_len; i++)
            if (app.output[i] == '\n')
                lines++;
        int max_scroll = lines * line_h - (out_box.height - 8);
        if (max_scroll < 0)
            max_scroll = 0;
        if (app.output_scroll > max_scroll)
            app.output_scroll = max_scroll;

        /* auto-save */
        if (app.dirty)
        {
            app.save_timer += GetFrameTime();
            if (app.save_timer > 1.0)
            {
                ini_save(&app);
                app.dirty = 0;
                app.save_timer = 0;
            }
        }

        /* ── draw ── */
        BeginDrawing();
        ClearBackground(COL_BG);

        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(COL_BG));
        DrawText("FBNeo LAN Launcher", 14, 14, 22, COL_ACCENT);
        DrawLine(0, 44, w, 44, COL_BORDER);

        /* field labels */
        for (int i = 0; i < FIELD_COUNT; i++)
        {
            DrawText(app.fields[i].label,
                     app.fields[i].rect.x - MeasureText(app.fields[i].label, 14) - 8,
                     app.fields[i].rect.y + 6, 14, COL_DIM);
        }

        /* browse button text — drawn by GuiButton */

        /* output */
        GuiGroupBox(out_box, NULL);
        BeginScissorMode(out_box.x, out_box.y, out_box.width, out_box.height);
        int ty = out_box.y + 4 - app.output_scroll;
        int lpos = 0;
        for (int i = 0; i < MAX_LINES && lpos < app.output_len; i++)
        {
            int end = lpos;
            while (end < app.output_len && app.output[end] != '\n')
                end++;
            char saved = app.output[end];
            app.output[end] = 0;
            Color tc = (app.output[lpos] == '$') ? COL_ACCENT2 : COL_DIM;
            DrawText(app.output + lpos, out_box.x + 6, ty, 14, tc);
            app.output[end] = saved;
            lpos = end + 1;
            ty += line_h;
        }
        EndScissorMode();

        DrawText(app.status, 14, h - 22, 14, COL_DIM);

        bool rom_edit = (app.editing_field == FIELD_ROM);

        if (app.editing_field == FIELD_ROM)
            GuiUnlock();

        if (GuiDropdownBox(app.fields[FIELD_ROM].rect, rom_list,
                           &app.selected_rom, rom_edit))
        {
            if (rom_edit && app.selected_rom >= 0)
            {
                strncpy(app.fields[FIELD_ROM].buf,
                        app.roms[app.selected_rom], MAX_FIELD - 1);
                app.dirty = 1;
            }
            app.editing_field = rom_edit ? -1 : FIELD_ROM;
        }

        EndDrawing();
    }

    ini_save(&app);
    CloseWindow();
    return 0;
}
