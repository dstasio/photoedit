#define FLAGS_MOUSE 0
#define FLAGS_MOUSE_INACTIVE 0
#define FLAGS_MOUSE_HOT      1
#define FLAGS_MOUSE_ACTIVE   2

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

    Ui_Window *active_window;
    Ui_Window *hot_window;
    Ui_Window *current_win;
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

// @todo: check if title address persists (if not, copy individual
// characters instead of address) 
void start_window(Ui *ui, Ui_Window *win)
{
    if (win->size.x == 0)
        win->size.x = 0.2f;
    if (win->size.y == 0)
        win->size.y = 0.2f;
    win->size.x /= ((r32)WIDTH/(r32)HEIGHT);

    if ((ui->input->mouse.x > win->pos.x) &&
        (ui->input->mouse.x < win->pos.x + win->size.x) &&
        (ui->input->mouse.y > win->pos.y - win->size.y) &&
        (ui->input->mouse.y < win->pos.y))
    {
        if (ui->hot_window == win) {
            if (ui->input->lmouse_down)  ui->active_window = win;
        }
        else {
            ui->hot_window = win;
        }

    }
    else {
        if (ui->hot_window == win)  ui->hot_window = 0;
    }
    if ((ui->active_window == win) && (ui->input->lmouse_up)) {
        ui->active_window = 0;
    }

    if (ui->active_window == win) {
        win->pos += ui->input->drag_delta;
    }

    u32 flag = FLAGS_MOUSE_INACTIVE;
    draw_square(win->pos, win->size, &flag);

    // @cleanup: find another way for ar correction
    win->size.x *= ((r32)WIDTH/(r32)HEIGHT);
}

// @todo: maybe if user clicks, then leaves the button, but without
// releasing hovers the button again it should activate?
b32
button(Ui *ui, u32 id, v2 pos, v2 size)
{
    b32 result = 0;

    if ((ui->input->mouse.x > pos.x) &&
        (ui->input->mouse.x < pos.x + size.x) &&
        (ui->input->mouse.y > pos.y - size.y) &&
        (ui->input->mouse.y < pos.y))
    {
        if (ui->hot == id) {
            if (ui->input->lmouse_down)  ui->active = id;
        }
        else {
            if (!ui->input->lmouse_down)  ui->hot = id;
        }

        if ((ui->active == id) && (ui->input->lmouse_up)) {
            result = 1;
            ui->active = 0;
        }
    }
    else {
        if (ui->hot == id)  ui->hot = 0;
        if (ui->active == id)  ui->active = 0;
    }

    u32 flag = FLAGS_MOUSE_INACTIVE;
    if (ui->active == id)
        flag = FLAGS_MOUSE_ACTIVE;
    else if (ui->hot == id)
        flag = FLAGS_MOUSE_HOT;
    draw_square(pos, size, &flag);

    return result;
}

