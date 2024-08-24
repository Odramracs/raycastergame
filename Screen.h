#pragma once

/*
    to compile, just go against gdi32.dll like this:
    gcc example.c -o main -lgdi32
*/

#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>

typedef struct state {
    HWND hwnd;
    HINSTANCE hInstance;
    HDC windowHDC;
    int windowActive;
    int windowWidth;
    int windowHeight;
}state_t;

// adapted from https://croakingkero.com/tutorials/drawing_pixels_win32_gdi/
struct {
    int width;
    int height;
    uint32_t *pixels;
} frame = {0};

#define toRGB(r, g, b) (r << (8*2) | g << (8*1) | b)
#define fromRGB(color, r, g, b) r = (color & 0xff0000) >> 16; \
                                g = (color & 0x00ff00) >> 8; \
                                b = (color & 0x0000ff)

static BITMAPINFO frame_bitmap_info;
static HBITMAP frame_bitmap = 0;
static HDC frame_device_context = 0;

state_t state;

wchar_t* CharToWchar(const char* charString);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/**
 * 
 * Creates the applications window context. Only one may exist per application for now.
 * 
 * The window will handled in the background, its state will be available through other functions that modify state global to the library. This only initiates that state so that it may be properly utilized throughout the application.
 * 
 * \param title title of the window
 * \param width  width in pixels of the window (not client width, window title bar and such included)
 * \param height  height in pixels of the window (not client width, window title bar and such included)
 * \param xPos horizontal position of the window on spawn in screen coordinates
 * \param yPos vertical positon of the window on spawn in screen coordinates
 * 
 * \returns 1 if successful in initialization of the window, and 0 otherwise
*/
int InitWindow(const char* title, int width, int height, int xPos, int yPos) {
    // WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
    
    int nCmdShow = SW_SHOWNORMAL;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {0};
    
    const wchar_t CLASS_NAME[] = L"Base Window";

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_OWNDC;

    RegisterClass(&wc);    

    wchar_t* wideTitle = CharToWchar(title);

    state.hwnd = CreateWindowEx(
        0, // additional params
        CLASS_NAME, // class name
        wideTitle, // window name
        WS_OVERLAPPEDWINDOW, // attributes
        xPos, yPos, width, height, // x, y, width, height
        NULL, // Parent handle
        NULL,  // Menu handle
        hInstance, // hInstance
        NULL // additional pointer to data
    );

    state.hInstance = hInstance;
    state.windowHDC = GetDC(state.hwnd);
    state.windowActive = 1; // returns false in conditional
    state.windowWidth = width;
    state.windowHeight = height;

    frame_bitmap_info.bmiHeader.biSize = sizeof(frame_bitmap_info.bmiHeader);
    frame_bitmap_info.bmiHeader.biPlanes = 1;
    frame_bitmap_info.bmiHeader.biBitCount = 32;
    frame_bitmap_info.bmiHeader.biCompression = BI_RGB;
    frame_device_context = CreateCompatibleDC(0);

    free(wideTitle);
    ShowWindow(state.hwnd, nCmdShow);
    
    return 0;
}

/**
 * 
 * Updates the state of the application window, dispatches messages and such. 
 * 
*/
void Update(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            if(frame_bitmap) DeleteObject(frame_bitmap);
            state.windowActive = 0;
        }

    }
}

/**
 * 
 * For use in an event loop in the main application code. 
 * 
 * \returns 1 as long as the initialized window hasn't received and message to terminate the window.
*/
int WindowActive(void) {
    // funny function
    return state.windowActive;
}


/**
 * 
 * The managed window process.
 * 
*/
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

        case WM_SIZE:
            state.windowWidth = LOWORD(lParam);
            state.windowHeight = HIWORD(lParam);
            break;

        case WM_PAINT:

            static PAINTSTRUCT paint;
            static HDC device_context;
            device_context = BeginPaint(hwnd, &paint);
            StretchBlt(device_context,
                   paint.rcPaint.left, paint.rcPaint.top,
                   paint.rcPaint.right - paint.rcPaint.left, paint.rcPaint.bottom - paint.rcPaint.top,
                   frame_device_context,
                   paint.rcPaint.left, paint.rcPaint.top,
                   frame.width, frame.height,
                   SRCCOPY);
            EndPaint(hwnd, &paint);
            break;

        case WM_QUIT:
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


/// @brief Checks if the current key is being pressed
/// @param key character to check from keyboard input
/// @return 1 if the key is currently being pressed, 0 if not
int IsKeyPressed(const char *key) {
    int vkKeyCode = VkKeyScan(key[0]) & 0x00ff;
    return (GetKeyState(vkKeyCode) & 0x8000) ? 1 : 0;
}


// Convert const char* to wchar_t*
wchar_t* CharToWchar(const char* charString) {
    setlocale(LC_ALL, ""); // Set the locale to the user's default locale
    size_t len = mbstowcs(NULL, charString, 0);
    if (len == (size_t)-1) {
        perror("mbstowcs failed");
        exit(EXIT_FAILURE);
    }
    wchar_t* wideString = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (wideString == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    mbstowcs(wideString, charString, len + 1);
    return wideString;
}

/**
 * 
 * Initializes a bitmap to serve as the rendering context for the window.
 * 
 * \param width width of the bitmap in pixels
 * \param height height of the bitmap in pixels
 * \returns pointer to a uint32_t array of length width*height, where every integer represents a 32 bit encoding of an RGB value that corresponds to the pixel color [0, 255]. 
 * Bits
 * 32 - 24 unused |
 * 23 - 16 red |
 * 15 - 8 green |
 * 7 - 0 blue
*/
uint32_t* InitBitmap(int width, int height) {
    frame_bitmap_info.bmiHeader.biWidth  = width;
    frame_bitmap_info.bmiHeader.biHeight = -height;

    if(frame_bitmap) DeleteObject(frame_bitmap);
    frame_bitmap = CreateDIBSection(NULL, &frame_bitmap_info, DIB_RGB_COLORS, (void**)&frame.pixels, 0, 0);
    SelectObject(frame_device_context, frame_bitmap);

    frame.width =  width;
    frame.height = height;
    return frame.pixels;
}

// Calls for the window to redraw the screen with the currently initialized bitmap.
void RenderBitmap(void) {
    InvalidateRect(state.hwnd, NULL, FALSE);
    UpdateWindow(state.hwnd);
}
