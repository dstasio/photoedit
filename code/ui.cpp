#define FLAGS_MOUSE 0
#define FLAGS_MOUSE_INACTIVE 0
#define FLAGS_MOUSE_HOT      1
#define FLAGS_MOUSE_ACTIVE   2

struct Ui
{
    u32 hot;
    u32 active;

    Input *input;
};

b32
start_window(Ui *ui, u32 id, v2 pos, v2 size)
{
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
    }
    else {
        if (ui->hot == id)  ui->hot = 0;
    }

    u32 flag = FLAGS_MOUSE_INACTIVE;
    if (ui->active == id)
        flag = FLAGS_MOUSE_ACTIVE;
    else if (ui->hot == id)
        flag = FLAGS_MOUSE_HOT;
    draw_square(pos, size, &flag);

    return (ui->active == id);
}

