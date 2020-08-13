#define FLAGS_MOUSE 0
#define FLAGS_DEPTH 1
#define FLAGS_MOUSE_INACTIVE 0
#define FLAGS_MOUSE_HOT      1
#define FLAGS_MOUSE_ACTIVE   2
#define FLAGS_MOUSE_WINDOW_BACKGROUND 3
#define FLAGS_DEPTH_0 0
#define FLAGS_DEPTH_1 1
#define FLAGS_DEPTH_2 2
#define FLAGS_DEPTH_3 3
#define FLAGS_DEPTH_4 4

#define CLEAR_COLOR 0x3F3F3FFF  // 0xRRGGBBAA
#define hex_to_rgba(hex) {(r32)(((hex) & 0xFF000000) >> 24) / 255.f, (r32)(((hex) & 0x00FF0000) >> 16) / 255.f, (r32)(((hex) & 0x0000FF00) >> 8) / 255.f, (r32)((hex) & 0x000000FF) / 255.f};

#define WINDOW_MARGIN 0.005f

struct Ui_Window
{
    u32 item_index;

    Ui_Window *left;
    Ui_Window *right;
    Ui_Window *top;
    Ui_Window *bottom;
};

struct Ui
{
    u32 hot;
    u32 active;

    Ui_Window *drag_win;
    Ui_Window *resize_win;
    Ui_Window *drawing_win;
    Input *input;
};

// compares strings and returns 1 if equal, 0 if different
b32 same_string(char *s1, char *s2)
{
    b32 result = 1;

    if (!s1 || !s2)
        result = 0;
    else {
        while (!((*s1 == '\0') && (*s2 == '\0')) && (result))
        {
            result = (*(s1++) == *(s2++));
        }
    }

    return result;
}

u32 get_hash(char *s)
{
    u32 hash = 0;
    u32 primes[] = {
          2,   3,   5,   7,  11,  13,  17,  19,  23,  29,
         31,  37,  41,  43,  47,  53,  59,  61,  67,  71,
         73,  79,  83,  89,  97, 101, 103, 107, 109, 113,
        127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
        179, 181, 191, 193, 197, 199, 211, 223, 227, 229
    };
    u32 prime_index = 0;
    for (char *c = s; (*c != '\0') && (prime_index < 60);)
    {
        u32 sum = *(c++);
        if (*c != '\0')
            sum += *(c++);
        hash += sum * primes[prime_index++];
    }
    if (prime_index >= 60)
        warn("UI label '%s' is longer than 100 characters", s);
    Assert(hash);
    return hash;
}

// @todo: fix corners when resizing
void start_window(Ui *ui, Ui_Window *win)
{
    v2 top_left  = {};
    v2 bot_right = {};

    if (win->left) { }
    else {
        top_left.x = WINDOW_MARGIN;
    }

    if (win->top) { }
    else {
        top_left.y = 0.04f;
    }

    if (win->bottom) { }
    else {
        bot_right.y = 1.f - WINDOW_MARGIN;
    }

    if (win->right) { }
    else {
        bot_right.x = ((r32)global_width/(r32)global_height) - WINDOW_MARGIN;
    }

    //if ((Abs(ui->input->mouse.x - win->pos.x - win->size.x) < 0.01f) &&
    //    (Abs(ui->input->mouse.y - win->pos.y - win->size.y) < 0.01f))
    //{
    //    win32_debug_set_cursor(CURSOR_RESIZE);
    //    ui->resize_win = win;
    //}
    //else if ((ui->resize_win == win) && !ui->input->lmouse_down)
    //    ui->resize_win = 0;
    //if ((ui->input->mouse.x > win->pos.x) &&
    //    (ui->input->mouse.x < win->pos.x + win->size.x) &&
    //    (ui->input->mouse.y > win->pos.y) &&
    //    (ui->input->mouse.y < win->pos.y + win->size.y) &&
    //    !ui->resize_win)
    //{
    //    if (ui->input->lmouse_down)
    //        ui->drag_win = win;
    //}
    //if (ui->input->lmouse_up) {
    //    ui->drag_win = 0;
    //    ui->resize_win = 0;
    //}

    u32 flags[] = {FLAGS_MOUSE_WINDOW_BACKGROUND, FLAGS_DEPTH_0};
    draw_square(top_left, bot_right - top_left, flags);
    ui->drawing_win = win;
}

//void end_window(Ui *ui)
//{
//    Ui_Window *win = ui->drawing_win;
//    ui->drawing_win = 0;
//    
//    if (ui->input->lmouse_down) {
//        if (ui->drag_win == win) {
//            win->pos += ui->input->drag_delta;
//        }
//        else if (ui->resize_win == win) {
//            win->size += ui->input->drag_delta;
//        }
//    }
//
//    u32 flags[] = {FLAGS_MOUSE_WINDOW_BACKGROUND, FLAGS_DEPTH_0};
//    draw_square(win->pos, win->size, flags);
//}

// @todo: maybe if user clicks, then leaves the button, but without
// releasing hovers the button again it should activate?
#if 0
b32
button(Ui *ui, char *label)
{
    b32 result = 0;
    u32 hash = get_hash(label);
    v2 pos = ui->drawing_win->pos;
    r32 margin = 0.05f;
    pos += make_v2(margin);
    v2 size = {ui->drawing_win->size.x - 2.f*margin, 0.05f};

    if ((ui->input->mouse.x > pos.x) &&
        (ui->input->mouse.x < pos.x + size.x) &&
        (ui->input->mouse.y > pos.y) &&
        (ui->input->mouse.y < pos.y + size.y))
    {
        ui->drag_win = 0;
        ui->resize_win = 0;
        if (ui->hot == hash) {
            if (ui->input->lmouse_down)  ui->active = hash;
        }
        else {
            if (!ui->input->lmouse_down)  ui->hot = hash;
        }

        if ((ui->active == hash) && (ui->input->lmouse_up)) {
            result = 1;
            ui->active = 0;
        }
    }
    else {
        if (ui->hot == hash)  ui->hot = 0;
        if (ui->active == hash)  ui->active = 0;
    }

    u32 flags[] = {FLAGS_MOUSE_INACTIVE, FLAGS_DEPTH_1};
    if (ui->active == hash)
        flags[0] = FLAGS_MOUSE_ACTIVE;
    else if (ui->hot == hash)
        flags[0] = FLAGS_MOUSE_HOT;
    draw_square(pos, size, flags);

    return result;
}
#endif
