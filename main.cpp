#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>

const std::wstring ROOT_FOLDER = L"C:\\Users\\Mithlesh\\Documents\\Codes\\C-prog\\assignment\\ClipboardMonitor\\root";
const std::wstring LOG_FILE = L"ClipboardMonitorLog.txt";

// Get current timestamp for logging
std::wstring GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    wchar_t buffer[30];
    wcsftime(buffer, 30, L"%Y-%m-%d %H:%M:%S", localtime(&time));
    return buffer;
}

// Check if a file or folder path is within a specific folder
bool IsWithinFolder(const std::wstring& path, const std::wstring& folder) {
    try {
        std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path);
        std::filesystem::path normalizedFolder = std::filesystem::weakly_canonical(folder);
        return normalizedPath.string().find(normalizedFolder.string()) == 0;
    } catch (const std::exception& e) {
        std::wcerr << L"Error checking path: " << e.what() << std::endl;
        return false;
    }
}

// Log operations
void LogOperation(const std::wstring& filePath, const std::wstring& message, bool isRestricted = false) {
    std::wofstream logFile(LOG_FILE.c_str(), std::ios::app);
    if (logFile) {
        logFile << GetTimestamp() << L" | " << (isRestricted ? L"[RESTRICTED] " : L"[ALLOWED] ") << filePath
                << L" | " << message << std::endl;
    } else {
        std::wcerr << L"Failed to open log file: " << LOG_FILE << std::endl;
    }
    logFile.close();
}

// Block unauthorized paste attempts
void BlockPaste() {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        CloseClipboard();
        std::wcout << L"Unauthorized paste operation blocked and clipboard cleared!" << std::endl;
    } else {
        std::wcerr << L"Failed to access clipboard to block operation. Error: " << GetLastError() << std::endl;
    }
}

// Handle clipboard files and enforce rules
void HandleClipboardFiles(HDROP hDropInfo) {
    UINT fileCount = DragQueryFileW(hDropInfo, 0xFFFFFFFF, nullptr, 0); // Get number of files
    for (UINT i = 0; i < fileCount; ++i) {
        wchar_t filePath[MAX_PATH];
        if (DragQueryFileW(hDropInfo, i, filePath, MAX_PATH)) {
            std::wcout << L"Clipboard file detected: " << filePath << std::endl;

            bool isSourceWithinRoot = IsWithinFolder(filePath, ROOT_FOLDER);

            if (isSourceWithinRoot) {
                // Disallow pasting files from root to outside
                LogOperation(filePath, L"Attempted to paste outside the root folder.", true);
                BlockPaste();
                break;
            } else {
                // Allow pasting into root folder or other operations
                LogOperation(filePath, L"Paste operation allowed.");
                std::wcout << L"Paste operation allowed for: " << filePath << std::endl;
            }
        }
    }
}

// Create a hidden window for clipboard monitoring
HWND CreateHiddenWindow() {
    const wchar_t CLASS_NAME[] = L"HiddenClipboardMonitor";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassW(&wc)) {
        std::cerr << "Failed to register window class. Error: " << GetLastError() << std::endl;
        return nullptr;
    }

    HWND hwnd = CreateWindowExW(
        0,                              // Optional styles
        CLASS_NAME,                     // Window class
        L"Clipboard Monitor",           // Window title
        0,                              // Hidden window
        0, 0, 0, 0,                     // Position and size
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!hwnd) {
        std::cerr << "Failed to create hidden window. Error: " << GetLastError() << std::endl;
    }

    return hwnd;
}

// Monitor clipboard changes
void ClipboardMonitor() {
    HWND hwnd = CreateHiddenWindow();
    if (!hwnd) return;

    if (!AddClipboardFormatListener(hwnd)) {
        std::cerr << "Failed to attach clipboard listener. Error: " << GetLastError() << std::endl;
        DestroyWindow(hwnd);
        return;
    }

    std::wcout << L"Clipboard listener attached successfully!" << std::endl;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_CLIPBOARDUPDATE) {
            std::wcout << L"Clipboard updated!" << std::endl;

            if (IsClipboardFormatAvailable(CF_HDROP)) {
                if (OpenClipboard(nullptr)) {
                    HANDLE hDrop = GetClipboardData(CF_HDROP);
                    if (hDrop) {
                        HandleClipboardFiles((HDROP)hDrop);
                    } else {
                        std::wcerr << L"Failed to get clipboard data. Error: " << GetLastError() << std::endl;
                    }
                    CloseClipboard();
                } else {
                    std::wcerr << L"Failed to open clipboard. Error: " << GetLastError() << std::endl;
                }
            } else {
                std::wcout << L"Clipboard does not contain files." << std::endl;
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveClipboardFormatListener(hwnd);
    DestroyWindow(hwnd);
}

// Entry point of the application
int main() {
    std::wcout << L"Starting clipboard and file operation monitor for path: " << ROOT_FOLDER << std::endl;

    // Start clipboard monitoring in a separate thread
    std::thread clipboardThread(ClipboardMonitor);
    clipboardThread.detach();

    // Message loop to keep the program running
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
