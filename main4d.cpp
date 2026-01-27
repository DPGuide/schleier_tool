#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>

typedef short (__stdcall *inpfuncPtr)(short portaddr);
typedef void (__stdcall *oupfuncPtr)(short portaddr, short datum);

#define LPT_PORT 0x378
#define BIT_MOON 0x01
#define BIT_SUN  0x02

#define IDC_EDIT_MOON   101
#define IDC_EDIT_SUN    102
#define IDC_BTN_MOON    103
#define IDC_BTN_SUN     104
#define IDC_COMBO_TAKT  105
#define IDC_CHK_TAKT    106
#define IDC_EDIT_BPM    107
#define IDC_BTN_FULLSCR 108

HINSTANCE hInst;
HWND hWndMain, hEditMoon, hEditSun, hBtnMoon, hBtnSun, hComboTakt, hChkTakt, hEditBpm, hBtnFull;
HBITMAP hBgBitmap = nullptr;
std::atomic<bool> g_moonRunning{false};
std::atomic<bool> g_sunRunning{false};
std::atomic<double> g_moonFreq{7.83};
std::atomic<double> g_sunFreq{1000.0};
std::atomic<int> g_taktType{0};
std::atomic<int> g_bpm{120};
std::atomic<bool> g_useTakt{false};
int g_windowWidth = 800, g_windowHeight = 600;

// GLOBAL: Treiber-Funktionen
inpfuncPtr Inp32 = nullptr;
oupfuncPtr Out32 = nullptr;
bool g_driverReady = false;

void preciseSleep(double microseconds) {
    static double freq = 0;
    if(freq == 0) {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        freq = (double)f.QuadPart;
    }
    LARGE_INTEGER start, now;
    QueryPerformanceCounter(&start);
    double counter_end = (microseconds / 1000000.0) * freq;
    do {
        QueryPerformanceCounter(&now);
    } while((double)(now.QuadPart - start.QuadPart) < counter_end);
}

bool initLPT() {
    // Versuche 64-bit zuerst, dann 32-bit
    HMODULE hLib = LoadLibrary("inpout64.dll");
    if(!hLib) hLib = LoadLibrary("inpout32.dll");
    if(!hLib) return false;
    
    Inp32 = (inpfuncPtr)GetProcAddress(hLib, "Inp32");
    Out32 = (oupfuncPtr)GetProcAddress(hLib, "Out32");
    
    if(Out32 && Inp32) {
        Out32(LPT_PORT, 0); // Reset
        return true;
    }
    return false;
}

void moonThread() {
    while(true) {
        if(g_moonRunning && g_driverReady) {
            double freq = g_moonFreq.load();
            if(freq < 0.0001) freq = 0.0001;
            double period_us = 1000000.0 / freq;
            double half_us = period_us / 2.0;
            
            if(g_useTakt.load() && g_taktType.load() > 0) {
                double beatDuration = 60.0 / g_bpm.load();
                int beatsPerBar = (g_taktType.load() == 1) ? 4 : 
                                 (g_taktType.load() == 2) ? 3 :
                                 (g_taktType.load() == 3) ? 6 : 2;
                
                for(int i=0; i<beatsPerBar && g_moonRunning; i++) {
                    auto beatStart = std::chrono::high_resolution_clock::now();
                    while(g_moonRunning && g_useTakt.load()) {
                        auto now = std::chrono::high_resolution_clock::now();
                        double elapsed = std::chrono::duration<double>(now - beatStart).count();
                        if(elapsed > beatDuration) break;
                        
                        if(elapsed < beatDuration * 0.5) {
                            short current = Inp32(LPT_PORT);
                            Out32(LPT_PORT, (current & ~BIT_MOON) | BIT_MOON);
                            preciseSleep(half_us);
                            Out32(LPT_PORT, Inp32(LPT_PORT) & ~BIT_MOON);
                            preciseSleep(half_us);
                        } else {
                            std::this_thread::sleep_for(std::chrono::microseconds(100));
                        }
                    }
                }
            } else {
                short current = Inp32(LPT_PORT);
                Out32(LPT_PORT, current | BIT_MOON);
                preciseSleep(half_us);
                Out32(LPT_PORT, Inp32(LPT_PORT) & ~BIT_MOON);
                preciseSleep(half_us);
            }
        } else {
            // Demo-Modus oder Aus: Einfach warten
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void sunThread() {
    while(true) {
        if(g_sunRunning && g_driverReady) {
            double freq = g_sunFreq.load();
            if(freq < 100) freq = 100;
            if(freq > 50000) freq = 50000;
            
            double period_us = 1000000.0 / freq;
            double half_us = period_us / 2.0;
            
            if(g_useTakt.load() && g_taktType.load() > 0) {
                double beatDuration = 60.0 / g_bpm.load();
                int beatsPerBar = (g_taktType.load() == 1) ? 4 : 
                                 (g_taktType.load() == 2) ? 3 :
                                 (g_taktType.load() == 3) ? 6 : 2;
                
                for(int i=0; i<beatsPerBar && g_sunRunning; i++) {
                    auto beatStart = std::chrono::high_resolution_clock::now();
                    while(g_sunRunning && g_useTakt.load()) {
                        auto now = std::chrono::high_resolution_clock::now();
                        double elapsed = std::chrono::duration<double>(now - beatStart).count();
                        if(elapsed > beatDuration) break;
                        
                        if(elapsed < beatDuration * 0.5) {
                            short current = Inp32(LPT_PORT);
                            Out32(LPT_PORT, (current & ~BIT_SUN) | BIT_SUN);
                            preciseSleep(half_us);
                            Out32(LPT_PORT, Inp32(LPT_PORT) & ~BIT_SUN);
                            preciseSleep(half_us);
                        } else {
                            std::this_thread::sleep_for(std::chrono::microseconds(100));
                        }
                    }
                }
            } else {
                short current = Inp32(LPT_PORT);
                Out32(LPT_PORT, current | BIT_SUN);
                preciseSleep(half_us);
                Out32(LPT_PORT, Inp32(LPT_PORT) & ~BIT_SUN);
                preciseSleep(half_us);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void resizeControls(int width, int height) {
    int margin = 20;
    int ctrlHeight = 25;
    int btnWidth = 100;
    int editWidth = 120;
    
    SetWindowPos(hEditMoon, NULL, margin, 50, editWidth, ctrlHeight, SWP_NOZORDER);
    SetWindowPos(hBtnMoon, NULL, margin, 80, btnWidth, ctrlHeight, SWP_NOZORDER);
    
    SetWindowPos(hEditSun, NULL, width - margin - editWidth, 50, editWidth, ctrlHeight, SWP_NOZORDER);
    SetWindowPos(hBtnSun, NULL, width - margin - btnWidth, 80, btnWidth, ctrlHeight, SWP_NOZORDER);
    
    int centerX = width / 2 - 100;
    SetWindowPos(hComboTakt, NULL, centerX, 120, 200, 200, SWP_NOZORDER);
    SetWindowPos(hChkTakt, NULL, centerX, 150, 200, ctrlHeight, SWP_NOZORDER);
    SetWindowPos(hEditBpm, NULL, centerX, 180, 80, ctrlHeight, SWP_NOZORDER);
    
    SetWindowPos(hBtnFull, NULL, width - 120, height - 50, 100, ctrlHeight, SWP_NOZORDER);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            hEditMoon = CreateWindow("EDIT", "7.83", WS_VISIBLE | WS_CHILD | WS_BORDER, 
                20, 50, 120, 25, hwnd, (HMENU)IDC_EDIT_MOON, hInst, NULL);
            hBtnMoon = CreateWindow("BUTTON", "MOND AN", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                20, 80, 100, 25, hwnd, (HMENU)IDC_BTN_MOON, hInst, NULL);
            
            hEditSun = CreateWindow("EDIT", "1000", WS_VISIBLE | WS_CHILD | WS_BORDER,
                0, 0, 0, 0, hwnd, (HMENU)IDC_EDIT_SUN, hInst, NULL);
            hBtnSun = CreateWindow("BUTTON", "SONNE AN", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                0, 0, 0, 0, hwnd, (HMENU)IDC_BTN_SUN, hInst, NULL);
            
            hComboTakt = CreateWindow("COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)IDC_COMBO_TAKT, hInst, NULL);
            SendMessage(hComboTakt, CB_ADDSTRING, 0, (LPARAM)"Frei (kein Takt)");
            SendMessage(hComboTakt, CB_ADDSTRING, 0, (LPARAM)"4/4-Takt");
            SendMessage(hComboTakt, CB_ADDSTRING, 0, (LPARAM)"3/4-Takt");
            SendMessage(hComboTakt, CB_ADDSTRING, 0, (LPARAM)"6/8-Takt");
            SendMessage(hComboTakt, CB_ADDSTRING, 0, (LPARAM)"2/4-Takt");
            SendMessage(hComboTakt, CB_SETCURSEL, 0, 0);
            
            hChkTakt = CreateWindow("BUTTON", "Takt aktivieren", WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                0, 0, 0, 0, hwnd, (HMENU)IDC_CHK_TAKT, hInst, NULL);
            
            hEditBpm = CreateWindow("EDIT", "120", WS_VISIBLE | WS_CHILD | WS_BORDER,
                0, 0, 0, 0, hwnd, (HMENU)IDC_EDIT_BPM, hInst, NULL);
            
            hBtnFull = CreateWindow("BUTTON", "VOLLBILD", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                0, 0, 0, 0, hwnd, (HMENU)IDC_BTN_FULLSCR, hInst, NULL);
            
            // Versuche BMP zu laden (JPG funktioniert nicht mit LoadImage!)
            hBgBitmap = (HBITMAP)LoadImage(NULL, "sun_mon.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            if(!hBgBitmap) {
                // Versuch mit JPG wenn GDI+ verfügbar wäre - aber wir zeigen Fehler
                OutputDebugString("Hinweis: sun_mon.bmp nicht gefunden (nur BMP wird unterstuetzt!)");
            }
            
            // Threads starten
            std::thread(moonThread).detach();
            std::thread(sunThread).detach();
            
            resizeControls(g_windowWidth, g_windowHeight);
            break;
        }
        
        case WM_SIZE: {
            g_windowWidth = LOWORD(lParam);
            g_windowHeight = HIWORD(lParam);
            resizeControls(g_windowWidth, g_windowHeight);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if(hBgBitmap) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP oldBitmap = (HBITMAP)SelectObject(hdcMem, hBgBitmap);
                BITMAP bm;
                GetObject(hBgBitmap, sizeof(bm), &bm);
                StretchBlt(hdc, 0, 0, g_windowWidth, g_windowHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                SelectObject(hdcMem, oldBitmap);
                DeleteDC(hdcMem);
            } else {
                // Fallback: Schwarzer Hintergrund mit Info-Text
                RECT rc;
                GetClientRect(hwnd, &rc);
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
                
                SetTextColor(hdc, RGB(255, 255, 0));
                SetBkMode(hdc, TRANSPARENT);
                TextOut(hdc, 20, g_windowHeight/2, "Hintergrund: sun_mon.bmp fehlt (nur BMP Format!)", 45);
            }
            
            SetTextColor(hdc, RGB(0, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 20, 20, "LUNARIS (Mond)", 15);
            TextOut(hdc, g_windowWidth - 150, 20, "SOLARIS (Sonne)", 15);
            
            if(!g_driverReady) {
                SetTextColor(hdc, RGB(255, 0, 0));
                TextOut(hdc, 20, g_windowHeight - 30, "DEMO-MODUS (Treiber fehlt!) - Kein LPT-Zugriff", 44);
            }
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            
            if(id == IDC_BTN_MOON) {
                if(!g_driverReady) {
                    MessageBox(hwnd, "Treiber (inpout64.dll oder inpout32.dll) nicht gefunden!\nProgramm läuft im Demo-Modus.", "Fehler", MB_OK);
                    return 0;
                }
                if(g_moonRunning) {
                    g_moonRunning = false;
                    SetWindowText(hBtnMoon, "MOND AN");
                } else {
                    char buf[256];
                    GetWindowText(hEditMoon, buf, 256);
                    g_moonFreq = atof(buf);
                    if(g_moonFreq <= 0) g_moonFreq = 7.83;
                    g_moonRunning = true;
                    SetWindowText(hBtnMoon, "MOND AUS");
                }
            }
            
            if(id == IDC_BTN_SUN) {
                if(!g_driverReady) {
                    MessageBox(hwnd, "Treiber nicht gefunden!", "Fehler", MB_OK);
                    return 0;
                }
                if(g_sunRunning) {
                    g_sunRunning = false;
                    SetWindowText(hBtnSun, "SONNE AN");
                } else {
                    char buf[256];
                    GetWindowText(hEditSun, buf, 256);
                    g_sunFreq = atof(buf);
                    if(g_sunFreq <= 0) g_sunFreq = 1000;
                    g_sunRunning = true;
                    SetWindowText(hBtnSun, "SONNE AUS");
                }
            }
            
            if(id == IDC_CHK_TAKT) {
                BOOL checked = IsDlgButtonChecked(hwnd, IDC_CHK_TAKT);
                CheckDlgButton(hwnd, IDC_CHK_TAKT, checked ? BST_UNCHECKED : BST_CHECKED);
                g_useTakt = !checked;
            }
            
            if(id == IDC_COMBO_TAKT && HIWORD(wParam) == CBN_SELCHANGE) {
                g_taktType = SendMessage(hComboTakt, CB_GETCURSEL, 0, 0);
            }
            
            if(id == IDC_EDIT_BPM && HIWORD(wParam) == EN_CHANGE) {
                char buf[32];
                GetWindowText(hEditBpm, buf, 32);
                g_bpm = atoi(buf);
                if(g_bpm < 1) g_bpm = 1;
            }
            
            if(id == IDC_BTN_FULLSCR) {
                static bool fullscreen = false;
                fullscreen = !fullscreen;
                if(fullscreen) {
                    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                    ShowWindow(hwnd, SW_MAXIMIZE);
                } else {
                    SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                    ShowWindow(hwnd, SW_NORMAL);
                }
            }
            break;
        }
        
        case WM_DESTROY: {
            g_moonRunning = false;
            g_sunRunning = false;
            if(Out32) Out32(LPT_PORT, 0);
            PostQuitMessage(0);
            break;
        }
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;
    
    g_driverReady = initLPT();
    
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "MAIN_ICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "VeilToolClass";
    RegisterClassEx(&wc);
    
    hWndMain = CreateWindowEx(0, "VeilToolClass", "SCHLEIER-TOOL v2.0",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}