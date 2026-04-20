#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>

#ifdef _WIN32
    #include <windows.h>
    #include <commdlg.h>
#else
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/Xatom.h>
    #include <sys/time.h>
    #include <unistd.h>
    #include <libgen.h>
#endif

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

#ifdef _WIN32
    #define NK_GDI_IMPLEMENTATION
    #include "nuklear_gdi.h"
#else
    #define NK_XLIB_IMPLEMENTATION
    #include "nuklear_xlib.h"
#endif

#include "gui.h"
#include "server.h"
#include "qrcodegen.h"
#include "net_utils.h"
#include "i18n.h"

static void urldecode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10); else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10); else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' '; src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static void parse_uri_list(PhoneDropServer *server, char *data) {
    char *line = strtok(data, "\r\n");
    while (line && server->file_count < 32) {
        if (strncmp(line, "file://", 7) == 0) line += 7;
        if (*line != '\0') {
            char decoded_path[1024];
            urldecode(decoded_path, line);
            server->file_paths[server->file_count] = strdup(decoded_path);
            server->file_count++;
        }
        line = strtok(NULL, "\r\n");
    }
}

static void open_file_picker(PhoneDropServer *server) {
#ifdef _WIN32
    wchar_t file_name[MAX_PATH * 32] = {0};
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = file_name;
    ofn.nMaxFile = sizeof(file_name) / sizeof(wchar_t);
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) {
        wchar_t *p = file_name;
        wchar_t *dir = p;
        p += wcslen(p) + 1;
        if (*p == 0) {
            char path[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, dir, -1, path, MAX_PATH, NULL, NULL);
            server->file_paths[server->file_count++] = strdup(path);
        } else {
            while (*p && server->file_count < 32) {
                wchar_t full_path[MAX_PATH];
                swprintf(full_path, MAX_PATH, L"%ls\\%ls", dir, p);
                char path[MAX_PATH];
                WideCharToMultiByte(CP_UTF8, 0, full_path, -1, path, MAX_PATH, NULL, NULL);
                server->file_paths[server->file_count++] = strdup(path);
                p += wcslen(p) + 1;
            }
        }
    }
#else
    FILE *f = popen("zenity --file-selection --multiple --separator='|' --title='PhoneDrop'", "r");
    if (!f) return;
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), f)) {
        buffer[strcspn(buffer, "\n")] = 0;
        char *token = strtok(buffer, "|");
        while (token && server->file_count < 32) {
            server->file_paths[server->file_count] = strdup(token);
            server->file_count++;
            token = strtok(NULL, "|");
        }
    }
    pclose(f);
#endif
}

static void draw_qr(struct nk_context *ctx, const char *text) {
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t temp[qrcodegen_BUFFER_LEN_MAX];
    if (!qrcodegen_encodeText(text, temp, qrcode, qrcodegen_Ecc_LOW, 1, 40, -1, true)) return;
    int size = qrcodegen_getSize(qrcode);
    struct nk_rect bounds;
    nk_layout_row_dynamic(ctx, 230, 1);
    if (!nk_widget(&bounds, ctx)) return;
    nk_fill_rect(nk_window_get_canvas(ctx), bounds, 0, nk_rgb(255, 255, 255));
    float padding = 15.0f;
    float block_size = ((bounds.w < bounds.h ? bounds.w : bounds.h) - (padding * 2)) / size;
    float offset_x = (bounds.w - (size * block_size)) / 2.0f;
    float offset_y = (bounds.h - (size * block_size)) / 2.0f;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                struct nk_rect block = { bounds.x + offset_x + (x * block_size), bounds.y + offset_y + (y * block_size), block_size + 0.1f, block_size + 0.1f };
                nk_fill_rect(nk_window_get_canvas(ctx), block, 0, nk_rgb(0, 0, 0));
            }
        }
    }
}

#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
#endif

#ifndef _WIN32
void run_gui(PhoneDropServer *server, const char *url) {
    setlocale(LC_ALL, "");
    i18n_bundle *bundle = get_local_bundle();
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return;
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    char **missing_list; int missing_count; char *def_string;
    XFontSet font_set = XCreateFontSet(dpy, "fixed", &missing_list, &missing_count, &def_string);
    if (!font_set) return;
    XFontSetExtents *extent = XExtentsOfFontSet(font_set);
    static struct XFont font_wrapper;
    font_wrapper.set = font_set;
    font_wrapper.handle.height = (float)extent->max_logical_extent.height;
    font_wrapper.handle.userdata = nk_handle_ptr(&font_wrapper);
    font_wrapper.handle.width = nk_xfont_get_text_width;
    Window win = XCreateSimpleWindow(dpy, root, 100, 100, 320, 450, 1, WhitePixel(dpy, screen), BlackPixel(dpy, screen));
    Atom xdnd_aware = XInternAtom(dpy, "XdndAware", False), xdnd_enter = XInternAtom(dpy, "XdndEnter", False),
         xdnd_position = XInternAtom(dpy, "XdndPosition", False), xdnd_status = XInternAtom(dpy, "XdndStatus", False),
         xdnd_drop = XInternAtom(dpy, "XdndDrop", False), xdnd_finished = XInternAtom(dpy, "XdndFinished", False),
         xdnd_selection = XInternAtom(dpy, "XdndSelection", False), xdnd_action_copy = XInternAtom(dpy, "XdndActionCopy", False),
         uri_list_type = XInternAtom(dpy, "text/uri-list", False);
    Atom version = 5;
    XChangeProperty(dpy, win, xdnd_aware, XA_ATOM, 32, PropModeReplace, (unsigned char *)&version, 1);
    Atom wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wm_delete_window, 1);
    XStoreName(dpy, win, "PhoneDrop");
    XClassHint *class_hint = XAllocClassHint();
    if (class_hint) { class_hint->res_name = "phonedrop"; class_hint->res_class = "phonedrop"; XSetClassHint(dpy, win, class_hint); XFree(class_hint); }
    XSelectInput(dpy, win, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask | StructureNotifyMask);
    XMapWindow(dpy, win);
    struct nk_context *ctx = nk_xlib_init(&font_wrapper, dpy, screen, root, 320, 450);
    ctx->style.window.background = nk_rgb(28, 28, 30);
    ctx->style.window.fixed_background = nk_style_item_color(nk_rgb(28, 28, 30));
    ctx->style.button.normal = nk_style_item_color(nk_rgb(45, 45, 48));
    ctx->style.button.hover = nk_style_item_color(nk_rgb(60, 60, 65));
    ctx->style.text.color = nk_rgb(230, 230, 230);
    char current_url[256]; strncpy(current_url, url, 256);
    Window source_win = 0;
    while (server->keep_running) {
        XEvent evt; nk_input_begin(ctx);
        while (XPending(dpy)) {
            XNextEvent(dpy, &evt);
            if (evt.type == ClientMessage) {
                if ((Atom)evt.xclient.data.l[0] == wm_delete_window) server->keep_running = 0;
                else if (evt.xclient.message_type == xdnd_enter) source_win = evt.xclient.data.l[0];
                else if (evt.xclient.message_type == xdnd_position) {
                    XClientMessageEvent reply; memset(&reply, 0, sizeof(reply));
                    reply.type = ClientMessage; reply.window = evt.xclient.data.l[0]; reply.message_type = xdnd_status;
                    reply.format = 32; reply.data.l[0] = win; reply.data.l[1] = 1; reply.data.l[4] = xdnd_action_copy;
                    XSendEvent(dpy, evt.xclient.data.l[0], False, NoEventMask, (XEvent *)&reply);
                } else if (evt.xclient.message_type == xdnd_drop) XConvertSelection(dpy, xdnd_selection, uri_list_type, xdnd_selection, win, evt.xclient.data.l[2]);
            } else if (evt.type == SelectionNotify && evt.xselection.selection == xdnd_selection) {
                Atom type; int fmt; unsigned long n, b; unsigned char *data = NULL;
                XGetWindowProperty(dpy, win, xdnd_selection, 0, 1024, False, uri_list_type, &type, &fmt, &n, &b, &data);
                if (data) {
                    parse_uri_list(server, (char *)data); XFree(data);
                    XClientMessageEvent reply; memset(&reply, 0, sizeof(reply));
                    reply.type = ClientMessage; reply.window = source_win; reply.message_type = xdnd_finished;
                    reply.format = 32; reply.data.l[0] = win; reply.data.l[1] = 1; reply.data.l[2] = xdnd_action_copy;
                    XSendEvent(dpy, source_win, False, NoEventMask, (XEvent *)&reply);
                }
            }
            nk_xlib_handle_event(dpy, screen, win, &evt);
        }
        nk_input_end(ctx);
        if (!server->keep_running) break;
        if (nk_begin(ctx, "PhoneDrop", nk_rect(0, 0, 320, 450), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND)) {
            if (server->state == STATE_HOME) {
                nk_layout_row_dynamic(ctx, 40, 1); nk_label(ctx, "PHONEDROP", NK_TEXT_CENTERED);
                nk_layout_row_dynamic(ctx, 45, 1);
                if (nk_button_label(ctx, bundle->gui_add_btn)) open_file_picker(server);
                if (server->file_count > 0) {
                    if (nk_group_begin(ctx, "FileList", NK_WINDOW_BORDER)) {
                        for (int i = 0; i < server->file_count; i++) {
                            nk_layout_row_begin(ctx, NK_STATIC, 30, 2); nk_layout_row_push(ctx, 210);
                            nk_label(ctx, basename(server->file_paths[i]), NK_TEXT_LEFT);
                            nk_layout_row_push(ctx, 35);
                            if (nk_button_label(ctx, "X")) {
                                free(server->file_paths[i]);
                                for (int j = i; j < server->file_count - 1; j++) server->file_paths[j] = server->file_paths[j + 1];
                                server->file_count--; i--;
                            }
                            nk_layout_row_end(ctx);
                        }
                        nk_group_end(ctx);
                    }
                    char count_text[64]; snprintf(count_text, 64, bundle->gui_files_ready, server->file_count);
                    nk_layout_row_dynamic(ctx, 30, 1); nk_label(ctx, count_text, NK_TEXT_CENTERED);
                    nk_layout_row_dynamic(ctx, 45, 1);
                    if (nk_button_label(ctx, bundle->gui_gen_qr_btn)) {
                        char ip[16]; get_local_ip(ip, sizeof(ip));
                        if (start_server(server, 9877) == 0) {
                            start_server_thread(server); sprintf(current_url, "http://%s:%d", ip, server->port);
                            server->state = STATE_SERVER_RUNNING;
                        }
                    }
                } else { nk_layout_row_dynamic(ctx, 150, 1); nk_label(ctx, bundle->gui_drag_label, NK_TEXT_CENTERED); }
            } else {
                nk_layout_row_dynamic(ctx, 40, 1); nk_label(ctx, bundle->title, NK_TEXT_CENTERED);
                draw_qr(ctx, current_url);
                nk_layout_row_dynamic(ctx, 40, 1); nk_spacing(ctx, 1);
                nk_layout_row_dynamic(ctx, 45, 1);
                if (nk_button_label(ctx, bundle->exit_btn)) server->keep_running = 0;
            }
        }
        nk_end(ctx);
        XClearWindow(dpy, win); nk_xlib_render(win, nk_rgb(28, 28, 30)); XFlush(dpy); usleep(20000);
    }
    nk_xlib_shutdown(); XFreeFontSet(dpy, font_set); XUnmapWindow(dpy, win); XDestroyWindow(dpy, win); XCloseDisplay(dpy);
}
#else
void run_gui(PhoneDropServer *server, const char *url) {
    i18n_bundle *bundle = get_local_bundle();
    WNDCLASSW wc; RECT rect = { 0, 0, 320, 450 };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    HWND wnd; HDC dc; struct nk_context *ctx; GdiFont *font;
    memset(&wc, 0, sizeof(wc)); wc.style = CS_DBLCLKS; wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(0); wc.hIcon = LoadIconW(NULL, (LPWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, (LPWSTR)IDC_ARROW); wc.lpszClassName = L"PhoneDrop";
    RegisterClassW(&wc); AdjustWindowRectEx(&rect, style, FALSE, 0);
    wnd = CreateWindowExW(0, wc.lpszClassName, L"PhoneDrop", style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, wc.hInstance, NULL);
    dc = GetDC(wnd); font = nk_gdifont_create("Arial", 14); ctx = nk_gdi_init(font, dc, 320, 450);
    char current_url[256]; strncpy(current_url, url, 256);
    while (server->keep_running) {
        MSG msg; nk_input_begin(ctx);
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { server->keep_running = 0; break; }
            TranslateMessage(&msg); DispatchMessageW(&msg);
            nk_gdi_handle_event(wnd, msg.message, msg.wParam, msg.lParam);
        }
        nk_input_end(ctx);
        if (!server->keep_running) break;
        if (nk_begin(ctx, "PhoneDrop", nk_rect(0, 0, 320, 450), NK_WINDOW_NO_SCROLLBAR)) {
            if (server->state == STATE_HOME) {
                nk_layout_row_dynamic(ctx, 40, 1); nk_label(ctx, "PHONEDROP", NK_TEXT_CENTERED);
                nk_layout_row_dynamic(ctx, 45, 1);
                if (nk_button_label(ctx, bundle->gui_add_btn)) open_file_picker(server);
                if (server->file_count > 0) {
                    if (nk_group_begin(ctx, "FileList", NK_WINDOW_BORDER)) {
                        for (int i = 0; i < server->file_count; i++) {
                            nk_layout_row_begin(ctx, NK_STATIC, 30, 2); nk_layout_row_push(ctx, 210);
                            nk_label(ctx, server->file_paths[i], NK_TEXT_LEFT);
                            nk_layout_row_push(ctx, 35);
                            if (nk_button_label(ctx, "X")) {
                                free(server->file_paths[i]);
                                for (int j = i; j < server->file_count - 1; j++) server->file_paths[j] = server->file_paths[j + 1];
                                server->file_count--; i--;
                            }
                            nk_layout_row_end(ctx);
                        }
                        nk_group_end(ctx);
                    }
                    nk_layout_row_dynamic(ctx, 45, 1);
                    if (nk_button_label(ctx, bundle->gui_gen_qr_btn)) {
                        char ip[16]; get_local_ip(ip, sizeof(ip));
                        if (start_server(server, 9877) == 0) {
                            start_server_thread(server); sprintf(current_url, "http://%s:%d", ip, server->port);
                            server->state = STATE_SERVER_RUNNING;
                        }
                    }
                } else { nk_layout_row_dynamic(ctx, 150, 1); nk_label(ctx, bundle->gui_drag_label, NK_TEXT_CENTERED); }
            } else {
                nk_layout_row_dynamic(ctx, 30, 1); nk_label(ctx, bundle->title, NK_TEXT_CENTERED);
                draw_qr(ctx, current_url);
                nk_layout_row_dynamic(ctx, 40, 1); nk_spacing(ctx, 1);
                nk_layout_row_dynamic(ctx, 45, 1);
                if (nk_button_label(ctx, bundle->exit_btn)) server->keep_running = 0;
            }
        }
        nk_end(ctx); nk_gdi_render(nk_rgb(28, 28, 30)); Sleep(20);
    }
    nk_gdifont_del(font); ReleaseDC(wnd, dc); UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
#endif