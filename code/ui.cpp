#define FLAGS_MOUSE 0
#define FLAGS_MOUSE_INACTIVE 0
#define FLAGS_MOUSE_HOT      1
#define FLAGS_MOUSE_ACTIVE   2
#define FLAGS_MOUSE_WINDOW_BACKGROUND 3

struct Ui_Window
{
    u32 item_index;

    v2 pos;
    v2 size;
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
    if (win->size.x == 0)
        win->size.x = 0.2f;
    if (win->size.y == 0)
        win->size.y = 0.2f;
    //win->size.x /= ((r32)WIDTH/(r32)HEIGHT);

    if ((Abs(ui->input->mouse.x - win->pos.x - win->size.x) < 0.05f) &&
        (Abs(ui->input->mouse.y - win->pos.y + win->size.y) < 0.05f))
    {
        win32_debug_set_cursor(CURSOR_RESIZE);
        ui->resize_win = win;
    }
    if ((ui->input->mouse.x > win->pos.x) &&
        (ui->input->mouse.x < win->pos.x + win->size.x) &&
        (ui->input->mouse.y > win->pos.y - win->size.y) &&
        (ui->input->mouse.y < win->pos.y) &&
        !ui->resize_win)
    {
        if (ui->input->lmouse_down)
            ui->drag_win = win;
    }
    if (ui->input->lmouse_up) {
        ui->drag_win = 0;
        ui->resize_win = 0;
    }

    if (ui->input->lmouse_down) {
        if (ui->drag_win == win) {
            win->pos += ui->input->drag_delta;
        }
        else if (ui->resize_win == win) {
            win->size.x += ui->input->drag_delta.x;
            win->size.y -= ui->input->drag_delta.y;
        }
    }

    u32 flag = FLAGS_MOUSE_WINDOW_BACKGROUND;
    draw_square(win->pos, win->size, &flag);

    ui->drawing_win = win;
    // @cleanup: find another way for ar correction
    //win->size.x *= ((r32)WIDTH/(r32)HEIGHT);
}

// @todo: maybe if user clicks, then leaves the button, but without
// releasing hovers the button again it should activate?
b32
button(Ui *ui, char *label)
{
    b32 result = 0;
    u32 hash = get_hash(label);
    v2 pos = ui->drawing_win->pos;
    r32 margin = 0.05f;
    pos += make_v2(margin, -margin);
    v2 size = {ui->drawing_win->size.x - 2.f*margin, 0.05f};
    inform("w(%f)  m(%f)  b(%f)\n", ui->drawing_win->size.x, margin, size.x);

    if ((ui->input->mouse.x > pos.x) &&
        (ui->input->mouse.x < pos.x + size.x) &&
        (ui->input->mouse.y > pos.y - size.y) &&
        (ui->input->mouse.y < pos.y))
    {
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

    u32 flag = FLAGS_MOUSE_INACTIVE;
    if (ui->active == hash)
        flag = FLAGS_MOUSE_ACTIVE;
    else if (ui->hot == hash)
        flag = FLAGS_MOUSE_HOT;
    draw_square(pos, size, &flag);

    return result;
}

