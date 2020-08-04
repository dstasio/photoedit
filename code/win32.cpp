/* ========================================================================
   PhotoEdit win32_d3d11
   ======================================================================== */
#include <windows.h>
#include <d3d11.h>

#define WIDTH 1024
#define HEIGHT 720

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
#define key_down(code, key)    {if(Message.wParam == (code))  input.held.key = 1;}
#define key_up(code, key)      {if(Message.wParam == (code)) {input.held.key = 0;input.pressed.key = 1;}}
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

inline u64
win32_get_last_write_time(char *Path)
{
    WIN32_FILE_ATTRIBUTE_DATA FileAttribs = {};
    GetFileAttributesExA(Path, GetFileExInfoStandard, (void *)&FileAttribs);
    FILETIME last_write_time = FileAttribs.ftLastWriteTime;

    u64 result = file_time_to_u64(last_write_time);
    return result;
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

        default:
        {
            result = DefWindowProcA(window, message, w, l);
        } break;
    }
    return(result);
}


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

    RECT window_rect = {0, 0, WIDTH, HEIGHT};
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

        //
        // raw input set-up
        //
        //RAWINPUTDEVICE raw_in_devices[] = {
        //    {0x01, 0x02, 0, main_window}
        //};
        //RegisterRawInputDevices(raw_in_devices, 1, sizeof(raw_in_devices[0]));

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
        ID3D11Device *device;
        ID3D11DeviceContext *context;
        DXGI_MODE_DESC display_mode_desc = {};
        //display_mode_desc.Width = WIDTH;
        //display_mode_desc.Height = HEIGHT;
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
        swap_chain_desc.Flags;

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

        ID3D11Resource *backbuffer = 0;
        ID3D11RenderTargetView *render_target_rgb = 0;
        swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
        device->CreateRenderTargetView(backbuffer, 0, &render_target_rgb);
        context->OMSetRenderTargets(1, &render_target_rgb, 0);

        // ===========================================================
        // Viewport set-up
        // ===========================================================
        D3D11_VIEWPORT viewport = {};
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = WIDTH;
        viewport.Height = HEIGHT;
        viewport.MinDepth = 0.f;
        viewport.MaxDepth = 1.f;
        context->RSSetViewports(1, &viewport);

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
        // Depth states
        // ===========================================================
        //D3D11_DEPTH_STENCIL_VIEW_DESC depth_view_desc = {DXGI_FORMAT_D32_FLOAT, D3D11_DSV_DIMENSION_TEXTURE2D};
        //d11->device->CreateRenderTargetView(d11->backbuffer, 0, &d11->render_target_rgb);
        //d11->device->CreateDepthStencilView(depth_texture, &depth_view_desc, &d11->render_target_depth);

        ID3D11DepthStencilState *nodepth_nostencil_state = 0;
        D3D11_DEPTH_STENCIL_DESC depth_stencil_settings;
        //depth_stencil_settings.DepthEnable    = 1;
        depth_stencil_settings.DepthEnable    = 0;
        depth_stencil_settings.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        //depth_stencil_settings.DepthFunc      = D3D11_COMPARISON_LESS;
        depth_stencil_settings.DepthFunc      = D3D11_COMPARISON_ALWAYS;
        depth_stencil_settings.StencilEnable  = 0;
        depth_stencil_settings.StencilReadMask;
        depth_stencil_settings.StencilWriteMask;
        depth_stencil_settings.FrontFace      = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};
        depth_stencil_settings.BackFace       = {D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};

        //device->CreateDepthStencilState(&depth_stencil_settings, &d11->depth_nostencil_state);
        device->CreateDepthStencilState(&depth_stencil_settings, &nodepth_nostencil_state);

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
        ID3D11Buffer *vbuffer = 0;
        r32 square[] = {
            0.f, -1.f,  0.f, 1.f,
            1.f, -1.f,  1.f, 1.f,
            1.f,  0.f,  1.f, 0.f,

            1.f,  0.f,  1.f, 0.f,
            0.f,  0.f,  0.f, 0.f,
            0.f, -1.f,  0.f, 1.f
        };

        u32 vertices_size = sizeof(square);
        u32 vert_stride  = 4*sizeof(r32);

        D3D11_SUBRESOURCE_DATA raw_vert_data = {square};
        D3D11_BUFFER_DESC vert_buff_desc     = {};
        vert_buff_desc.ByteWidth = sizeof(square);
        vert_buff_desc.Usage = D3D11_USAGE_IMMUTABLE;
        vert_buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vert_buff_desc.StructureByteStride = vert_stride;
        device->CreateBuffer(&vert_buff_desc, &raw_vert_data, (ID3D11Buffer **)&vbuffer);

        // ===========================================================
        // Input Layout
        // ===========================================================
        ID3D11InputLayout *input_layout = 0;
        D3D11_INPUT_ELEMENT_DESC in_desc[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        device->CreateInputLayout(in_desc, 2, vsh_file.data, vsh_file.size, &input_layout); 


        // ===========================================================
        // constant buffer setup
        // ===========================================================
        ID3D11Buffer *matrix_buff = 0;
        D3D11_BUFFER_DESC matrix_buff_desc = {};
        matrix_buff_desc.ByteWidth = 3*sizeof(m4);
        matrix_buff_desc.Usage = D3D11_USAGE_DYNAMIC;
        matrix_buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        matrix_buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        matrix_buff_desc.MiscFlags = 0;
        matrix_buff_desc.StructureByteStride = sizeof(m4);
        device->CreateBuffer(&matrix_buff_desc, 0, &matrix_buff);
        context->VSSetConstantBuffers(0, 1, &matrix_buff);

        MSG message = {};

        i64 last_performance_counter = 0;
        i64 current_performance_counter = 0;
        AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&last_performance_counter));
        while(global_running && !global_error)
        {
            AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&current_performance_counter));
            r32 dtime = (r32)(current_performance_counter - last_performance_counter) / (r32)performance_counter_frequency;
            while(dtime <= target_ms_per_frame)
            {
                while(PeekMessageA(&message, main_window, 0, 0, PM_REMOVE))
                {
                    switch(message.message)
                    {
                        case WM_KEYDOWN:
                        {
                        } break;

                        case WM_KEYUP:
                        {
                        } break;

                        case WM_INPUT:
                        {
#if 0
                            RAWINPUT raw_input = {};
                            u32 raw_size = sizeof(raw_input);
                            GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, &raw_input, &raw_size, sizeof(RAWINPUTHEADER));
                            RAWMOUSE raw_mouse = raw_input.data.mouse;
                            if (raw_mouse.usFlags == MOUSE_MOVE_RELATIVE)
                            {
                                input.dmouse.x = raw_mouse.lLastX;
                                input.dmouse.y = raw_mouse.lLastY;
                            }

                            if (raw_mouse.usButtonFlags & RI_MOUSE_WHEEL)
                            {
                                input.dwheel = raw_mouse.usButtonData;
                            }
                            SetCursorPos((window_rect.right - window_rect.left)/2, (window_rect.bottom - window_rect.top)/2);
#endif
                        } break;

                        //case WM_MOUSEMOVE:
                        //{
                        //    i16 m_x = ((i16*)&Message.lParam)[0];
                        //    i16 m_y = ((i16*)&Message.lParam)[1];
                        //    NewInput->dm_x = NewInput->m_x - m_x;
                        //    NewInput->dm_y = NewInput->m_y - m_y;
                        //    NewInput->m_x = m_x;
                        //    NewInput->m_y = m_y;

                        //    //SetCursorPos((window_rect.right - window_rect.left)/2, (window_rect.bottom - window_rect.top)/2);
                        //} break;

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
            r32 clear_color[] = {0.06f, 0.5f, 0.8f, 1.f};
            context->ClearRenderTargetView(render_target_rgb, clear_color);

            context->OMSetRenderTargets(1, &render_target_rgb, 0);
            context->OMSetDepthStencilState(nodepth_nostencil_state, 1);

            context->VSSetShader(vshader, 0, 0);
            context->PSSetShader(pshader, 0, 0);

            D3D11_MAPPED_SUBRESOURCE matrices_map = {};
            context->Map(matrix_buff, 0, D3D11_MAP_WRITE_DISCARD, 0, &matrices_map);

            m4 *matrix_array = (m4 *)matrices_map.pData;

            r32 ar = (r32)WIDTH/(r32)HEIGHT;
            v2 size = {0.3f, 0.3f};
            size.x /= ar;
            v2 pos = {0.f, 0.f};

            matrix_array[0] = Translation_m4(pos.x, pos.y, 0)*Scale_m4(size);
            context->Unmap(matrix_buff, 0);

            u32 offsets = 0;
            context->IASetVertexBuffers(0, 1, &vbuffer, &vert_stride, &offsets);
            context->IASetInputLayout(input_layout);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            context->Draw(6, 0);

            swap_chain->Present(1, 0);
            AssertF(QueryPerformanceCounter((LARGE_INTEGER *)&current_performance_counter));
            dtime = (r32)(current_performance_counter - last_performance_counter) / (r32)performance_counter_frequency;
            last_performance_counter = current_performance_counter;
            inform("Frametime: %f     FPS:%d\n", dtime, (u32)(1/dtime));
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
