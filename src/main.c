#include "app.h"
#include "config.h"
#include "process.h"
#include "roms.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define LX 120
#define LY 48
#define LS 32
#define IH 28

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
    snprintf(f->buf, MAX_FIELD, def);
}

int main(void)
{
    setlocale(LC_ALL,"");
    App app = {0};

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIDTH, HEIGHT, "FBNeo LAN Launcher");
    SetTargetFPS(60);
    ini_load(&app);
    discovery_start(&app.discovery);
    GuiLoadStyleDefault();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL, ColorToInt(COL_BG));
    scan_roms(&app);

    field_init(&app.fields[FIELD_DIR], FIELD_DIR, "Fightcade Dir", app.fields[FIELD_DIR].buf,
               0, 0, 1);
    field_init(&app.fields[FIELD_ROM], FIELD_ROM, "ROM", app.fields[FIELD_ROM].buf,
               0, 2, 1);
    field_init(&app.fields[FIELD_IP], FIELD_IP, "Peer IP", app.fields[FIELD_IP].buf,
               0, 3, 1);
    field_init(&app.fields[FIELD_PLAYER], FIELD_PLAYER, "Player", "",
               0, 4, 2);

    app.editing_field = -1;
    app.selected_rom = -1;

    for (int i = 0; i < app.rom_count; i++)
        if (strcmp(app.fields[FIELD_ROM].buf, app.roms[i]) == 0)
            app.selected_rom = i;

    while (!WindowShouldClose())
    {
        check_child(&app);
        bool rom_edit = (app.editing_field == FIELD_ROM);

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
        if (IsKeyPressed(KEY_ESCAPE))
            app.show_peers = 0;

        int w = GetScreenWidth();
        int h = GetScreenHeight();

        float fw = w - 160; // largura dos campos

        BeginDrawing();
        ClearBackground(COL_BG);

        DrawText("FBNeo LAN Launcher", 14, 14, 22, COL_ACCENT);
        DrawLine(0, 44, w, 44, COL_BORDER);

        if (rom_edit)
            GuiLock();

        for (int i = 0; i < FIELD_COUNT; i++)
        {
            app.fields[i].rect = (Rectangle){
                LX + ((fw) / app.fields[i].columns) * app.fields[i].pos,
                LY + LS * app.fields[i].line,
                ((fw) / app.fields[i].columns),
                IH};

            switch (i)
            {
            case FIELD_PLAYER:
                GuiToggleGroup(app.fields[i].rect, "P1;P2", &app.player);
                break;
            case FIELD_ROM:
                break;
            case FIELD_IP:
                app.fields[i].rect.width -= 74;
                Rectangle sbtn = {app.fields[i].rect.x + app.fields[i].rect.width + 4,
                                  app.fields[i].rect.y, 70, app.fields[i].rect.height};
                if (GuiButton(sbtn, "Scan"))
                {
                    app.show_peers = !app.show_peers;
                    app.selected_peer = -1;
                }
            default:
                if (GuiTextBox(app.fields[i].rect, app.fields[i].buf, MAX_FIELD, app.editing_field == i))
                {
                    app.editing_field = (app.editing_field == i) ? -1 : i;
                    app.dirty = 1;
                }
            }
        }

        if (GuiButton((Rectangle){LX, LY + LS * 1, fw, IH}, "Browse"))
        {
            char path[512];
            app.editing_field = -1;
            if (browse_folder(path, sizeof(path)))
            {
                snprintf(app.fields[FIELD_DIR].buf, MAX_FIELD, path);
                app.dirty = 1;
                app.rom_count = 0;
                scan_roms(&app);
            }
        }

        int by = (LY + LS * 4) + 40;
        Color btn_col = app.launch_handle != INVALID_PROC ? COL_DIM : COL_ACCENT2;
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, ColorToInt(btn_col));

        for (int i = 0; i < FIELD_COUNT; i++)
        {
            DrawText(app.fields[i].label,
                     app.fields[i].rect.x - MeasureText(app.fields[i].label, 14) - 8,
                     app.fields[i].rect.y + 6, 14, COL_DIM);
        }

        if (GuiButton((Rectangle){w / 2 - 100, by, 200, 34}, "LAUNCH GAME") && app.launch_handle == INVALID_PROC)
        {
            launch_emulator(&app);
        }

        int oy = by + 48;
        Rectangle out_box = {10, oy, w - 20, h - oy - 30};
        if (!app.show_peers)
        {
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
        }

        if (app.show_peers)
        {
            GuiGroupBox(out_box, "Peers on LAN");
            if (app.discovery.count == 0)
            {
                const char *msg = app.discovery.running ? "Scanning..." : "No peers found";
                DrawText(msg, out_box.x + 6, out_box.y + 6, 14, COL_DIM);
            }
            else
            {
                int py = out_box.y + 4;
                for (int i = 0; i < app.discovery.count; i++)
                {
                    char label[96];
                    if (app.discovery.peers[i].hostname[0])
                        snprintf(label, sizeof(label), "%s (%s)",
                                 app.discovery.peers[i].hostname,
                                 app.discovery.peers[i].ip);
                    else
                        snprintf(label, sizeof(label), "%s",
                                 app.discovery.peers[i].ip);

                    Rectangle item = {out_box.x + 4, py, out_box.width - 8, 22};
                    Color ic = (i == app.selected_peer) ? COL_ACCENT : COL_PANEL;
                    DrawRectangleRec(item, ic);
                    DrawRectangleLinesEx(item, 1, COL_BORDER);
                    DrawText(label, item.x + 4, item.y + 3, 14, COL_TEXT);

                    Vector2 mp = GetMousePosition();
                    if (CheckCollisionPointRec(mp, item) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    {
                        strncpy(app.fields[FIELD_IP].buf,
                                app.discovery.peers[i].ip, MAX_FIELD - 1);
                        app.dirty = 1;
                        app.show_peers = 0;
                    }
                    py += 24;
                    if (py > out_box.y + out_box.height - 8)
                        break;
                }
            }
            app.discovery.updated = 0;
        }
        else
        {
            GuiGroupBox(out_box, NULL);
            BeginScissorMode(out_box.x, out_box.y, out_box.width, out_box.height);
            int ty = out_box.y + 4 - app.output_scroll;
            int lpos = 0;
            int line_h = 16;
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
        }

        DrawText(app.status, 14, h - 22, 14, COL_DIM);

        if (rom_edit)
            GuiUnlock();

        if (GuiDropdownBox(app.fields[FIELD_ROM].rect, app.rom_list,
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

        EndDrawing();
    }

    ini_save(&app);
    discovery_stop(&app.discovery);
    CloseWindow();
    return 0;
}
