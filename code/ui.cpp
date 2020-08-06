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

// @todo: maybe if user clicks, then leaves the button, but without
// releasing hovers the button again it should activate?

v2
start_window(Ui *ui, u32 id, v2 pos, v2 size)
{
    size.x /= ((r32)WIDTH/(r32)HEIGHT);
    if ((ui->input->mouse.x > pos.x) &&
        (ui->input->mouse.x < pos.x + size.x) &&
        (ui->input->mouse.y > pos.y - size.y) &&
        (ui->input->mouse.y < pos.y))
    {
        if (ui->hot == id) {
            if (ui->input->lmouse_down)  ui->active = id;
        }
        else {
            ui->hot = id;
        }

    }
    else {
        if (ui->hot == id)  ui->hot = 0;
    }
    if ((ui->active == id) && (ui->input->lmouse_up)) {
        ui->active = 0;
    }

    if (ui->active == id) {
        pos += ui->input->drag_delta;
    }

    u32 flag = FLAGS_MOUSE_INACTIVE;
    draw_square(pos, size, &flag);
    return pos;
}

//b32
//button(Ui *ui, u32 id, v2 pos, v2 size)
//{
//    b32 result = 0;
//
//    if ((ui->input->mouse.x > pos.x) &&
//        (ui->input->mouse.x < pos.x + size.x) &&
//        (ui->input->mouse.y > pos.y - size.y) &&
//        (ui->input->mouse.y < pos.y))
//    {
//        if (ui->hot == id) {
//            if (ui->input->lmouse_down)  ui->active = id;
//        }
//        else {
//            if (!ui->input->lmouse_down)  ui->hot = id;
//        }
//
//        if ((ui->active == id) && (ui->input->lmouse_up)) {
//            result = 1;
//            ui->active = 0;
//        }
//    }
//    else {
//        if (ui->hot == id)  ui->hot = 0;
//        if (ui->active == id)  ui->active = 0;
//    }
//
//    u32 flag = FLAGS_MOUSE_INACTIVE;
//    if (ui->active == id)
//        flag = FLAGS_MOUSE_ACTIVE;
//    else if (ui->hot == id)
//        flag = FLAGS_MOUSE_HOT;
//    draw_square(pos, size, &flag);
//
//    return result;
//}

