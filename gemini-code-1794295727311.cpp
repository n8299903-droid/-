#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>

HWND hwndOverlay = NULL;
int screenWidth = 0;
int screenHeight = 0;

struct Vector3 {
    float x, y, z;
};

struct Vector2 {
    float x, y;
};

template <typename T>
T ReadMemory(HANDLE hProcess, uintptr_t address) {
    T value{};
    ReadProcessMemory(hProcess, (LPCVOID)address, &value, sizeof(T), NULL);
    return value;
}

bool WorldToScreen(Vector3 pos, Vector2& screen, float matrix[16], int width, int height) {
    float w = matrix[12] * pos.x + matrix[13] * pos.y + matrix[14] * pos.z + matrix[15];
    if (w < 0.01f) return false;

    float invW = 1.0f / w;
    float x = matrix[0] * pos.x + matrix[1] * pos.y + matrix[2] * pos.z + matrix[3];
    float y = matrix[4] * pos.x + matrix[5] * pos.y + matrix[6] * pos.z + matrix[7];

    screen.x = (width / 2.0f) + (1.0f * x * invW * (width / 2.0f));
    screen.y = (height / 2.0f) - (1.0f * y * invW * (height / 2.0f));
    return true;
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch (Message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, Message, wParam, lParam);
    }
    return 0;
}

bool CreateOverlay() {
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WinProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)RGB(0, 0, 0);
    wc.lpszClassName = L"CS2OverlayClass";

    if (!RegisterClassEx(&wc)) return false;

    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hwndOverlay = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        L"CS2OverlayClass",
        L"Overlay",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, wc.hInstance, NULL
    );

    if (!hwndOverlay) return false;

    SetLayeredWindowAttributes(hwndOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwndOverlay, SW_SHOW);
    return true;
}

DWORD GetProcessId(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (!_wcsicmp(entry.szExeFile, processName)) {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName) {
    uintptr_t modBaseAddr = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Module32FirstW(snapshot, &entry)) {
            do {
                if (!_wcsicmp(entry.szModule, moduleName)) {
                    modBaseAddr = (uintptr_t)entry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return modBaseAddr;
}

int main() {
    std::cout << "[!] Waiting for CS2 process...\n";

    DWORD PID = 0;
    while (!PID) {
        PID = GetProcessId(L"cs2.exe");
        Sleep(1000);
    }

    std::cout << "[+] CS2 found! PID: " << PID << std::endl;

    uintptr_t clientBase = 0;
    while (!clientBase) {
        clientBase = GetModuleBaseAddress(PID, L"client.dll");
        Sleep(1000);
    }

    std::cout << "[+] client.dll base address: 0x" << std::hex << clientBase << std::dec << std::endl;

    if (!CreateOverlay()) {
        std::cout << "[-] Failed to create overlay window!\n";
        system("pause");
        return 1;
    }

    std::cout << "[+] Overlay created successfully! ESP loop running...\n";

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, PID);
    if (!hProcess) {
        std::cout << "[-] Failed to open process memory!\n";
        system("pause");
        return 1;
    }

    HDC hdc = GetDC(hwndOverlay);
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                             OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                             VARIABLE_PITCH, TEXT("Arial"));

    int debugTimer = 0;

    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RECT rect = { 0, 0, screenWidth, screenHeight };
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
        SelectObject(hdcMem, hBitmap);
        
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdcMem, &rect, blackBrush);
        DeleteObject(blackBrush);

        SetTextColor(hdcMem, RGB(0, 255, 0));
        SetBkMode(hdcMem, TRANSPARENT);
        SelectObject(hdcMem, hFont);

        float viewMatrix[16] = { 0 };
        ReadProcessMemory(hProcess, (LPCVOID)(clientBase + 0x2570250), &viewMatrix, sizeof(viewMatrix), NULL);

        uintptr_t entityList = ReadMemory<uintptr_t>(hProcess, clientBase + 0x271FBB8);
        
        int foundPlayers = 0;

        if (entityList) {
            for (int i = 1; i < 64; i++) {
                uintptr_t listEntry = ReadMemory<uintptr_t>(hProcess, entityList + (8 * (i >> 9)) + 16);
                if (!listEntry) continue;

                uintptr_t playerController = ReadMemory<uintptr_t>(hProcess, listEntry + 120 * (i & 0x1FF));
                if (!playerController) continue;

                int health = ReadMemory<int>(hProcess, playerController + 0x34C);
                
                if (health > 0 && health <= 100) {
                    // Используем свежий оффсет m_hPlayerPawn (0x92C) из твоего дампа
                    unsigned int pawnHandle = ReadMemory<unsigned int>(hProcess, playerController + 0x92C);
                    if (!pawnHandle) continue;

                    uintptr_t listEntry2 = ReadMemory<uintptr_t>(hProcess, entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16);
                    if (!listEntry2) continue;

                    uintptr_t playerPawn = ReadMemory<uintptr_t>(hProcess, listEntry2 + 120 * (pawnHandle & 0x1FF));
                    if (!playerPawn) continue;

                    // Используем свежий оффсет m_vOldOrigin (0x14A4) из твоего дампа
                    Vector3 origin = ReadMemory<Vector3>(hProcess, playerPawn + 0x14A4);

                    Vector2 screenPos;
                    if (WorldToScreen(origin, screenPos, viewMatrix, screenWidth, screenHeight)) {
                        foundPlayers++;
                        std::string hpText = "HP: " + std::to_string(health);
                        TextOutA(hdcMem, (int)screenPos.x, (int)screenPos.y, hpText.c_str(), (int)hpText.length());
                    }
                }
            }
        }

        if (debugTimer++ > 200) {
            std::cout << "[DEBUG] Active players found with valid HP: " << foundPlayers << std::endl;
            debugTimer = 0;
        }

        BitBlt(hdc, 0, 0, screenWidth, screenHeight, hdcMem, 0, 0, SRCCOPY);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);

        Sleep(5);
    }

    DeleteObject(hFont);
    CloseHandle(hProcess);
    return 0;
}