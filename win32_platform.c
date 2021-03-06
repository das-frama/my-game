#include "utils.c"
#include "math.c"

#include "platform_common.c"

#include <windows.h>

struct {
    // Platform non-specific part.
    int width, height;
    u32 *pixels;
    // Platform specific part.
    BITMAPINFO bitmap_info;
} typedef Render_Buffer;

// Globals.
global_variable Render_Buffer render_buffer;
global_variable b32 running = true;

#include "software_rendering.c"
#include "game.c"

internal LRESULT 
window_callback (HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    LRESULT result = 0;
    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY: {
            running = false;
        } break;
        case WM_SIZE: {
            RECT rect;
            GetWindowRect(window, &rect);
            render_buffer.width = rect.right - rect.left;
            render_buffer.height = rect.bottom - rect.top;
            if (render_buffer.pixels) {
                VirtualFree(render_buffer.pixels, 0, MEM_RELEASE);
            }
            render_buffer.pixels = VirtualAlloc(0, sizeof(u32)*render_buffer.width*render_buffer.height, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            // Fill the bitmap_info.
            render_buffer.bitmap_info.bmiHeader.biSize = sizeof(render_buffer.bitmap_info.bmiHeader);
            render_buffer.bitmap_info.bmiHeader.biWidth = render_buffer.width;
            render_buffer.bitmap_info.bmiHeader.biHeight = render_buffer.height;
            render_buffer.bitmap_info.bmiHeader.biPlanes = 1;
            render_buffer.bitmap_info.bmiHeader.biBitCount = 32;
            render_buffer.bitmap_info.bmiHeader.biCompression = BI_RGB;
        } break;
        
        default: {
            result = DefWindowProcA(window, message, w_param, l_param);
        }
    }
    
    return result;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Create Class.
    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = window_callback;
    window_class.lpszClassName = "Hello Game";
    
    // Register the class.
    RegisterClassA(&window_class);
    HWND window = CreateWindowExA(0, window_class.lpszClassName, "Hello!", WS_VISIBLE|WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, 0, 0);
    HDC hdc = GetDC(window);
    
    Input input = {0};
    
    // Main loop.
    while (running) {
        // Input.
        for (int i = 0; i < BUTTON_COUNT; i++) {
            input.buttons[i].changed = false;
        }
        
        MSG message;
        while (PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
            switch(message.message) {
                case WM_SYSKEYDOWN: 
                case WM_SYSKEYUP: 
                case WM_KEYDOWN: 
                case WM_KEYUP: {
                    u32 vk_code  = (u32)message.wParam;
                    u32 was_down = ((message.lParam & (1 << 30)) != 0);
                    u32 is_down  = ((message.lParam & (1 << 30)) == 0);
                    
#define process_button(vk, b) \
if (vk_code == vk) {\
input.buttons[b].changed = is_down != input.buttons[b].is_down;\
input.buttons[b].is_down = is_down;\
}
                    process_button(VK_LEFT, BUTTON_LEFT);
                    process_button(VK_RIGHT, BUTTON_RIGHT);
                    process_button(VK_UP, BUTTON_UP);
                    process_button(VK_DOWN, BUTTON_DOWN);
                } break;
                
                default: {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
            }
        }
        // Simulation.
        simulate_game(&input);
        
        // Render.
        int width, height;
        void *memory;
        BITMAPINFO bitmap_info;
        
        StretchDIBits(hdc, 0, 0, render_buffer.width, render_buffer.height, 0, 0, render_buffer.width, render_buffer.height, render_buffer.pixels, &render_buffer.bitmap_info, DIB_RGB_COLORS, SRCCOPY );
    }
}