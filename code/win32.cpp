/* ========================================================================
   PhotoEdit win32_d3d11
   ======================================================================== */
#include <windows.h>
#include <d3d11.h>

#if PHE_DEBUG
#define Assert(expr)  if(!(expr)) {*(int *)0 = 0;}
#define AssertF(expr) if(!(expr)) {*(int *)0 = 0;}
#else
#define Assert(expr)
#define AssertF(expr) (expr)
#endif

#include "datatypes.h"
#include "phe_math.h"
#include <stdio.h>

#if PHE_INTERNAL
#define output_string(s, ...)        {char Buffer[100];sprintf_s(Buffer, s, __VA_ARGS__);OutputDebugStringA(Buffer);}
#define throw_error_and_exit(e, ...) {output_string(" ------------------------------[ERROR] " ## e, __VA_ARGS__); getchar(); global_error = true;}
#define throw_error(e, ...)           output_string(" ------------------------------[ERROR] " ## e, __VA_ARGS__)
#define inform(i, ...)                output_string(" ------------------------------[INFO] " ## i, __VA_ARGS__)
#define warn(w, ...)                  output_string(" ------------------------------[WARNING] " ## w, __VA_ARGS__)
#else
#define output_string(s, ...)
#define throw_error_and_exit(e, ...)
#define throw_error(e, ...)
#define inform(i, ...)
#define warn(w, ...)
#endif
#define key_down(code, key)    {if(message.wParam == (code))  input.held.key = 1;}
#define key_up(code, key)      {if(message.wParam == (code)) {input.held.key = 0;input.pressed.key = 1;}}
#define file_time_to_u64(wt) ((wt).dwLowDateTime | ((u64)((wt).dwHighDateTime) << 32))

#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4a
#define VK_K 0x4b
#define VK_L 0x4c
#define VK_M 0x4d
#define VK_N 0x4e
#define VK_O 0x4f
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5a

global b32 global_running;
global b32 global_error;

global u32 global_width = 1024;
global u32 global_height = 720;

global ID3D11Buffer        *global_matrix_buff;
global ID3D11Buffer        *global_flags_buff;
global ID3D11Device        *device;
global ID3D11DeviceContext *context;
global ID3D11Buffer        *vbuffer = 0;
global ID3D11InputLayout   *input_layout = 0;


inline u64
win32_get_last_write_time(char *Path)
{
    WIN32_FILE_ATTRIBUTE_DATA FileAttribs = {};
    GetFileAttributesExA(Path, GetFileExInfoStandard, (void *)&FileAttribs);
    FILETIME last_write_time = FileAttribs.ftLastWriteTime;

    u64 result = file_time_to_u64(last_write_time);
    return result;
}

#define CURSOR_ARROW  0
#define CURSOR_RESIZE 1
inline void
win32_debug_set_cursor(u32 id = CURSOR_ARROW)
{
    HCURSOR arrow = 0;
    switch(id)
    {
        case CURSOR_ARROW:
        {
            arrow = LoadCursorA(0, IDC_ARROW);
        } break;

        case CURSOR_RESIZE:
        {
            arrow = LoadCursorA(0, IDC_SIZENWSE);
        } break;
    }
    Assert(arrow);
    SetCursor(arrow);
}

struct Input_File
{
    char *path;
    u8   *data;
    u64   size;
    u64   write_time;  // @todo: this should be useless in release build
};

#define PLATFORM_RELOAD_CHANGED_FILE(name) b32 name(Input_File *file)
typedef PLATFORM_RELOAD_CHANGED_FILE(Platform_Reload_Changed_File);
#define PLATFORM_READ_FILE(name) Input_File name(char *Path)
typedef PLATFORM_READ_FILE(Platform_Read_File);

internal
PLATFORM_READ_FILE(win32_read_file)
{
    Input_File Result = {};
    HANDLE FileHandle = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        // TODO(dave): Currently only supports up to 4GB files
        u32 FileSize = GetFileSize(FileHandle, 0);
        DWORD BytesRead;

        // TODO(dave): Remove this allocation
        u8 *Buffer = (u8 *)VirtualAlloc(0, FileSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        if(ReadFile(FileHandle, Buffer, FileSize, &BytesRead, 0))
        {
            Result.data = Buffer;
            Result.size = (u32)BytesRead;
            Result.path = Path;
            Result.write_time = win32_get_last_write_time(Path);
        }
        else
        {
            throw_error("Unable to read file: %s\n", Path);
        }

        CloseHandle(FileHandle);
    }
    else
    {
        throw_error("Unable to open file: %s\n", Path);
        DWORD error = GetLastError();
        error = 0;
    }

    return(Result);
}

internal 
PLATFORM_RELOAD_CHANGED_FILE(win32_reload_file_if_changed)
{
    b32 has_changed = false;
    u64 current_write_time = win32_get_last_write_time(file->path);

    Input_File new_file = {};
    if(current_write_time != file->write_time)
    {
        u32 trial_count = 0;
        while(!new_file.size)
        {
            VirtualFree(new_file.data, 0, MEM_RELEASE);
            new_file = win32_read_file(file->path);
            new_file.write_time = current_write_time;
            has_changed = true;

            if (trial_count++ >= 100) // Try to read until it succeeds, or until trial count reaches max.
            {
                has_changed = false;
                throw_error_and_exit("Couldn't read changed file '%s'!", file->path);
                break;
            }
        }

        if (has_changed) {
            VirtualFree(file->data, 0, MEM_RELEASE);
            *file = new_file;
        }
    }

    return(has_changed);
}

LRESULT CALLBACK window_proc(
    HWND   window,
    UINT   message,
    WPARAM w,
    LPARAM l
)
{
    LRESULT result = 0;
    switch(message)
    {
        case WM_CLOSE:
        {
            DestroyWindow(window);
        } break;
            
        case WM_DESTROY:
        {
            global_running = false;
        } break;

        case WM_ACTIVATE:
        case WM_MOUSEHOVER:
        {
            if (((w & 0xF) == WA_ACTIVE) || ((w & 0xF) == WA_CLICKACTIVE))
            {
                win32_debug_set_cursor();
            }
        } break;

        default:
        {
            result = DefWindowProcA(window, message, w, l);
        } break;
    }
    return(result);
}

struct Input_Keyboard
{
    u32 up;
    u32 down;
    u32 left;
    u32 right;

    u32 w;
    u32 a;
    u32 s;
    u32 d;

    u32 f;

    u32 space;
    u32 shift;
    u32 ctrl;
    u32 alt;
    u32 esc;
};

struct Input
{
    Input_Keyboard pressed;
    Input_Keyboard held;

    v2  mouse;
    u32 lmouse_down;
    u32 lmouse_up;
    v2  drag_delta;  // this delta is frame-by-frame
    v2  drag_start;  // drag_start gets set every frame

    i16 dwheel;
};

struct Matrix_Map
{
    m4 model;
    m4 ortho;
};

void draw_square(v2 pos, v2 size, u32 *flags)
{
    r32 ar = (r32)global_width / (r32)global_height;
    // sending transform matrix to gpu
    {
        D3D11_MAPPED_SUBRESOURCE cbuffer_map = {};
        context->Map(global_matrix_buff, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbuffer_map);

        Matrix_Map *matrix_map = (Matrix_Map *)cbuffer_map.pData;
        matrix_map->model = Translation_m4(pos.x, pos.y, 0)*Scale_m4(size);
        matrix_map->ortho = Ortho_m4(0, ar, 0.f, 1.f);
        context->Unmap(global_matrix_buff, 0);
    }

    // sending flags to gpu
    {
        D3D11_MAPPED_SUBRESOURCE cbuffer_map = {};
        context->Map(global_flags_buff, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbuffer_map);
        u32 *flags_map = (u32 *)cbuffer_map.pData;

        flags_map[0] = flags[0];
        flags_map[1] = flags[1];

        context->Unmap(global_flags_buff, 0);
    }

    // setting square vertex buffer
    u32 offsets = 0;
    u32 vert_stride = 5*sizeof(r32);
    context->IASetVertexBuffers(0, 1, &vbuffer, &vert_stride, &offsets);
    context->IASetInputLayout(input_layout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // rendering square
    context->Draw(18, 0);
}

#include "ui.cpp"

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPSTR command_line,
    int show_flag
)
{
    // @todo add support for low-resolution performance counters
    // @todo maybe add support for 32-bit
    i64 performance_counter_frequency = 0;
    Assert(QueryPerformanceFrequency((LARGE_INTEGER *)&performance_counter_frequency));
    // @todo make this hardware-dependant
    r32 target_ms_per_frame = 1.f/60.f;

    WNDCLASSA window_class = {};
    window_class.style = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    window_class.lpfnWndProc = window_proc;
    //window_class.lpfnWndProc = ImGui_ImplWin32_WndProcHandler;
    window_class.hInstance = instance;
    //window_class.hIcon;
    //window_class.hCursor;
    //window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    //window_class.lpszMenuName;
    window_class.lpszClassName = "phe_class";

    RegisterClassA(&window_class);

    RECT window_rect = {0, 0, (i32)global_width, (i32)global_height};
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    window_rect.right  -= window_rect.left;
    window_rect.bottom -= window_rect.top;

    HWND main_window = CreateWindowA("phe_class", "PhotoEdit",
                                     WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     window_rect.right,
                                     window_rect.bottom,
                                     0, 0, instance, 0);

    if(main_window)
    {
        // @note: kinda useless at this point because the window will probably be resized
        // @todo: remove
        GetWindowRect(main_window, &window_rect);
        u32 window_width  = window_rect.right - window_rect.left;
        u32 window_height = window_rect.bottom - window_rect.top;

#if PHE_INTERNAL
        LPVOID base_address = (LPVOID)Terabytes(2);
#else
        LPVOID base_address = 0;
#endif
        global_running = true;
        //
        //
        //        // ===========================================================
        //        //  Initializing Direct2D
        //        // ===========================================================
        //
        //#if PHE_DEBUG
        //        D2D1_FACTORY_OPTIONS factory_options = {D2D1_DEBUG_LEVEL_INFORMATION};
        //#else
        //        D2D1_FACTORY_OPTIONS factory_options = {D2D1_DEBUG_LEVEL_NONE};
        //#endif
        //        ID2D1Factory1 *d2d_factory = 0;
        //        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &factory_options, &d2d_factory);

        IDXGISwapChain *swap_chain;
        DXGI_MODE_DESC display_mode_desc = {};
        //display_mode_desc.Width = global_width;
        //display_mode_desc.Height = global_height;
        display_mode_desc.RefreshRate = {60, 1};
        display_mode_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        display_mode_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
        display_mode_desc.Scaling = DXGI_MODE_SCALING_CENTERED;

        DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
        swap_chain_desc.BufferDesc = display_mode_desc;
        swap_chain_desc.SampleDesc = {1, 0};
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = 2;
        swap_chain_desc.OutputWindow = main_window;
        swap_chain_desc.Windowed = true;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        //swap_chain_desc.Flags;

        D3D11CreateDeviceAndSwapChain(
            0,
            D3D_DRIVER_TYPE_HARDWARE,
            0,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT|D3D11_CREATE_DEVICE_DEBUG,
            0,
            0,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &swap_chain,
            &device,
            0,
            &context
        );

        // ===========================================================
        // Viewport set-up
        // ===========================================================
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width  = (r32)global_width;
        viewport.Height = (r32)global_height;
        viewport.MinDepth = 0.f;
        viewport.MaxDepth = 1.f;
        context->RSSetViewports(1, &viewport);

        // ===========================================================
        // Depth and rgb buffers
        // ===========================================================
        // @todo: maybe 32 bits are too much
        ID3D11Resource  *backbuffer = 0;
        ID3D11Texture2D *depth_texture = 0;
        ID3D11RenderTargetView *render_target_rgb   = 0;
        ID3D11DepthStencilView *render_target_depth = 0;

        D3D11_DEPTH_STENCIL_VIEW_DESC depth_view_desc = {DXGI_FORMAT_D32_FLOAT, D3D11_DSV_DIMENSION_TEXTURE2D};

        D3D11_TEXTURE2D_DESC depth_buffer_desc = {};
        depth_buffer_desc.Width = global_width;
        depth_buffer_desc.Height = global_height;
        depth_buffer_desc.MipLevels = 1;
        depth_buffer_desc.ArraySize = 1;
        depth_buffer_desc.Format = DXGI_FORMAT_D32_FLOAT;
        depth_buffer_desc.SampleDesc.Count = 1;
        depth_buffer_desc.SampleDesc.Quality = 0;
        depth_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depth_buffer_desc.MiscFlags = 0;
        device->CreateTexture2D(&depth_buffer_desc, 0, &depth_texture);

        swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
        device->CreateRenderTargetView(backbuffer, 0, &render_target_rgb);

        device->CreateDepthStencilView(depth_texture, &depth_view_desc, &render_target_depth);
        //context->OMSetRenderTargets(1, &render_target_rgb, render_target_depth);

        // ===========================================================
        // Depth states
        // ===========================================================
        ID3D11DepthStencilState *depth_nostencil_state = 0;
        D3D11_DEPTH_STENCIL_DESC depth_stencil_settings;
        depth_stencil_settings.DepthEnable    = 1;
        //depth_stencil_settings.DepthEnable    = 0;
        depth_stencil_settings.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth_stencil_settings.DepthFunc      = D3D11_COMPARISON_GREATER_EQUAL;
        //depth_stencil_settings.DepthFunc      = D3D11_COMPARISON_ALWAYS;
        depth_stencil_settings.StencilEnable  = 0;
        depth_stencil_settings.StencilReadMask;
        depth_stencil_settings.StencilWriteMask;
        depth_stencil_settings.FrontFace      = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};
        depth_stencil_settings.BackFace       = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};

        device->CreateDepthStencilState(&depth_stencil_settings, &depth_nostencil_state);
        //context->OMSetDepthStencilState(depth_nostencil_state, 1);

        // ===========================================================
        // Texture sampler set-up
        // ===========================================================
        ID3D11SamplerState *sampler;
        D3D11_SAMPLER_DESC sampler_desc = {};
        sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;//D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
        sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
        sampler_desc.MipLODBias = -1;
        sampler_desc.MaxAnisotropy = 16;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        sampler_desc.MinLOD = 0;
        sampler_desc.MaxLOD = 100;
        device->CreateSamplerState(&sampler_desc, &sampler);
        context->PSSetSamplers(0, 1, &sampler);

        // ===========================================================
        // rasterizer set-up
        // ===========================================================
        D3D11_RASTERIZER_DESC raster_settings = {};
        raster_settings.FillMode = D3D11_FILL_SOLID;
        raster_settings.CullMode = D3D11_CULL_BACK;
        raster_settings.FrontCounterClockwise = 1;
        raster_settings.DepthBias = 0;
        raster_settings.DepthBiasClamp = 0;
        raster_settings.SlopeScaledDepthBias = 0;
        raster_settings.DepthClipEnable = 1;
        raster_settings.ScissorEnable = 0;
        raster_settings.MultisampleEnable = 0;
        raster_settings.AntialiasedLineEnable = 0;

        ID3D11RasterizerState *raster_state = 0;
        device->CreateRasterizerState(&raster_settings, &raster_state);
        context->RSSetState(raster_state);

        // ===========================================================
        // alpha blending set-up
        // ===========================================================
        CD3D11_BLEND_DESC blend_state_desc = {};
        blend_state_desc.RenderTarget[0].BlendEnable = 1;
        //blend_state_desc.RenderTarget[0].LogicOpEnable = 0;
        blend_state_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blend_state_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_state_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_state_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_state_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blend_state_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        //blend_state_desc.RenderTarget[0].LogicOp;
        blend_state_desc.RenderTarget[0].RenderTargetWriteMask = 0x0F;

        ID3D11BlendState *blend_state = 0;
        device->CreateBlendState(&blend_state_desc, &blend_state);
        context->OMSetBlendState(blend_state, 0, 0xFFFFFFFF);

        // ===========================================================
        // Ui shaders
        // ===========================================================
        Input_File vsh_file = {};
        vsh_file.path = "ui.vsh";
        Input_File psh_file = {};
        psh_file.path = "ui.psh";

        ID3D11VertexShader *vshader = 0;
        ID3D11PixelShader  *pshader = 0;
        if(win32_reload_file_if_changed(&vsh_file))
            device->CreateVertexShader(vsh_file.data, vsh_file.size, 0, &vshader);

        if(win32_reload_file_if_changed(&psh_file))
            device->CreatePixelShader(psh_file.data, psh_file.size, 0, &pshader);

        // ===========================================================
        // Square mesh buffer
        // ===========================================================
        // @todo: fix texture coordinates
        r32 eps = 0.02f;
        r32 rounded_square[] = {
            1-eps,     0.f,  1.f, 0.f, 0.f,
            0.f,     eps,  1.f, 0.f, 0.f,
            eps,     1.f,  1.f, 0.f, 0.f,

            eps,     1.f,  0.f, 1.f, 0.f,
            1.f, 1.f-eps,  0.f, 1.f, 0.f,
            1-eps,     0.f,  0.f, 1.f, 0.f, 

            1-eps,     0.f,  0.f, 0.f, 1.f, 
            eps,     0.f,  0.f, 0.f, 1.f, 
            0.f,     eps,  0.f, 0.f, 1.f,

            0.f,     eps,  1.f, 1.f, 0.f,
            0.f, 1.f-eps,  1.f, 1.f, 0.f,
            eps,     1.f,  1.f, 1.f, 0.f,

            eps,     1.f,  0.f, 1.f, 1.f,
            1-eps,     1.f,  0.f, 1.f, 1.f,
            1.f, 1.f-eps,  0.f, 1.f, 1.f,

            1.f, 1.f-eps,  1.f, 0.f, 1.f,
            1.f,     eps,  1.f, 0.f, 1.f,
            1-eps,     0.f,  1.f, 0.f, 1.f
        };

        u32 vertices_size = sizeof(rounded_square);
        u32 vert_stride  = 5*sizeof(r32);

        D3D11_SUBRESOURCE_DATA raw_vert_data = {rounded_square};
        D3D11_BUFFER_DESC vert_buff_desc     = {};
        vert_buff_desc.ByteWidth = vertices_size;
        vert_buff_desc.Usage = D3D11_USAGE_IMMUTABLE;
        vert_buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vert_buff_desc.StructureByteStride = vert_stride;
        device->CreateBuffer(&vert_buff_desc, &raw_vert_data, (ID3D11Buffer **)&vbuffer);

        // ===========================================================
        // Input Layout
        // ===========================================================
        D3D11_INPUT_ELEMENT_DESC in_desc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        device->CreateInputLayout(in_desc, 2, vsh_file.data, vsh_file.size, &input_layout); 


        // ===========================================================
        // constant buffer setup
        // ===========================================================
        D3D11_BUFFER_DESC cbuffer_desc = {};
        cbuffer_desc.ByteWidth = sizeof(Matrix_Map);
        cbuffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        cbuffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbuffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbuffer_desc.MiscFlags = 0;
        cbuffer_desc.StructureByteStride = sizeof(m4);
        device->CreateBuffer(&cbuffer_desc, 0, &global_matrix_buff);
        context->VSSetConstantBuffers(0, 1, &global_matrix_buff);

        cbuffer_desc.ByteWidth = max(2*sizeof(u32), 16);
        cbuffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        cbuffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbuffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        cbuffer_desc.MiscFlags = 0;
        cbuffer_desc.StructureByteStride = sizeof(u32);
        device->CreateBuffer(&cbuffer_desc, 0, &global_flags_buff);
        context->PSSetConstantBuffers(0, 1, &global_flags_buff);
        context->VSSetConstantBuffers(1, 1, &global_flags_buff);  // @todo: make separate buffers for each shader

        MSG message = {};
        Input input = {};

        Ui ui = {};
        ui.input = &input;

        i64 last_performance_counter = 0;
        i64 current_performance_counter = 0;
        AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&last_performance_counter));
        while(global_running && !global_error)
        {
            AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&current_performance_counter));
            r32 dtime = (r32)(current_performance_counter - last_performance_counter) / (r32)performance_counter_frequency;

            input.pressed = {};
            input.lmouse_up = 0;
            input.dwheel = 0;
            while(dtime <= target_ms_per_frame)
            {
                while(PeekMessageA(&message, main_window, 0, 0, PM_REMOVE))
                {
                    switch(message.message)
                    {
                        case WM_KEYDOWN:
                        {
                            key_down(VK_UP,      up);
                            key_down(VK_DOWN,    down);
                            key_down(VK_LEFT,    left);
                            key_down(VK_RIGHT,   right);
                            key_down(VK_W,       w);
                            key_down(VK_A,       a);
                            key_down(VK_S,       s);
                            key_down(VK_D,       d);
                            key_down(VK_F,       f);
                            key_down(VK_SPACE,   space);
                            key_down(VK_SHIFT,   shift);
                            key_down(VK_CONTROL, ctrl);
                            key_down(VK_ESCAPE,  esc);
                            key_down(VK_MENU,    alt);
                        } break;

                        case WM_KEYUP:
                        {
                            key_up(VK_UP,      up);
                            key_up(VK_DOWN,    down);
                            key_up(VK_LEFT,    left);
                            key_up(VK_RIGHT,   right);
                            key_up(VK_W,       w);
                            key_up(VK_A,       a);
                            key_up(VK_S,       s);
                            key_up(VK_D,       d);
                            key_up(VK_F,       f);
                            key_up(VK_SPACE,   space);
                            key_up(VK_SHIFT,   shift);
                            key_up(VK_CONTROL, ctrl);
                            key_up(VK_ESCAPE,  esc);
                            key_up(VK_MENU,    alt);
                        } break;

                        case WM_LBUTTONDOWN:
                        {
                            input.lmouse_down = 1;
                            input.drag_start = input.mouse;
                            input.drag_delta = {};
                        } break;

                        case WM_LBUTTONUP:
                        {
                            input.lmouse_down = 0;
                            input.lmouse_up = 1;
                        } break;

                        case WM_MOUSEMOVE:
                        {
                            i16 x = ((i16*)&message.lParam)[0];
                            i16 y = ((i16*)&message.lParam)[1];
                            input.mouse = {(r32)x / (r32)global_height, (r32)y / (r32)global_height};
                        } break;

                        default:
                        {
                            TranslateMessage(&message);
                            DispatchMessage(&message);
                        } break;
                    }
                }
                AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&current_performance_counter));
                dtime = (r32)(current_performance_counter - last_performance_counter) / (r32)performance_counter_frequency;
            }

            if (input.lmouse_down) {
                input.drag_delta = input.mouse - input.drag_start;
                input.drag_start = input.mouse;
            }
            // clearing frame
            //r32 clear_color[] = {0.247f, 0.247f, 0.247f, 1.f};
            r32 clear_color[] = hex_to_rgba(CLEAR_COLOR);
            context->ClearRenderTargetView(render_target_rgb, clear_color);
            context->ClearDepthStencilView(render_target_depth, D3D11_CLEAR_DEPTH, 0, 0);

            context->OMSetRenderTargets(1, &render_target_rgb, render_target_depth);
            context->OMSetDepthStencilState(depth_nostencil_state, 1);

            // shaders
            context->VSSetShader(vshader, 0, 0);
            context->PSSetShader(pshader, 0, 0);

            local_persist Ui_Window win[2] = {
                {0, { 0.f,  0.f}, {0.3f, 0.3f}},
                {0, {0.5f, 0.4f}, {0.4f, 0.4f}}
            };

            start_window(&ui, &win[0]);
            if (button(&ui, "button"))
                inform("Button pressed\n");
            end_window(&ui);
            start_window(&ui, &win[1]);
            end_window(&ui);

            // @todo, @cleanup: move this elsewhere?
            if (!ui.resize_win)
                win32_debug_set_cursor(CURSOR_ARROW);

            swap_chain->Present(1, 0);

            // FPS counter
            AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&current_performance_counter));
            dtime = (r32)(current_performance_counter - last_performance_counter) / (r32)performance_counter_frequency;
            last_performance_counter = current_performance_counter;
            //inform("Frametime: %f     FPS:%d\n", dtime, (u32)(1/dtime));
        }
        }
        else
        {
            // TODO: Logging
            return 1;
        }

        if (global_error)
        {
            return 1;
        }
        else
        {
            return 0;
        }
}
