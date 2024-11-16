#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <filesystem>
#include <vector>
#include <fstream> // For logging unauthorized operations

const std::wstring ROOT_FOLDER = L"C:\\Windows"; // past the root folder to restrict
const std::wstring LOG_FILE = L"ClipboardMonitorLog.txt"; // Log file for unauthorized actions

// Check if a file or folder path is within the restricted root folder
bool IsWithinRootFolder(const std::wstring& path) {
    try {
        std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(path);
        std::filesystem::path normalizedRoot = std::filesystem::weakly_canonical(ROOT_FOLDER);

        // Ensure the path is equivalent to or a subpath of the root folder
        return std::filesystem::equivalent(normalizedPath, normalizedRoot) ||
               normalizedPath.string().find(normalizedRoot.string() + "\\") == 0;
    } catch (const std::exception& e) {
        std::wcerr << L"Error checking path: " << e.what() << std::endl;
        return false;
    }
}

// Log unauthorized copy-paste attempts
void LogUnauthorizedAttempt(const std::wstring& filePath) {
    std::wofstream logFile(LOG_FILE.c_str(), std::ios::app); // Open the log file in append mode
    if (logFile) {
        logFile << L"Unauthorized copy-paste attempt detected: " << filePath << std::endl;
    } else {
        std::wcerr << L"Failed to open log file: " << LOG_FILE << std::endl;
    }
    logFile.close();
}

// Block unauthorized paste attempts
void BlockPaste() {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard(); // Clear the clipboard to block the paste operation
        CloseClipboard();
        std::wcout << L"Unauthorized paste operation blocked and clipboard cleared!" << std::endl;
    } else {
        std::wcerr << L"Failed to access clipboard to block operation. Error: " << GetLastError() << std::endl;
    }
}

// Handle files from the clipboard and check against the restricted root folder
void HandleClipboardFiles(HDROP hDropInfo) {
    UINT fileCount = DragQueryFileW(hDropInfo, 0xFFFFFFFF, nullptr, 0); // Get the number of files
    for (UINT i = 0; i < fileCount; ++i) {
        wchar_t filePath[MAX_PATH];
        if (DragQueryFileW(hDropInfo, i, filePath, MAX_PATH)) { // Retrieve each file path
            std::wcout << L"Detected file: " << filePath << std::endl;

            // If the file is within the restricted root folder, block the paste operation
            if (IsWithinRootFolder(filePath)) {
                std::wcout << L"Unauthorized file detected: " << filePath << std::endl;
                LogUnauthorizedAttempt(filePath);
                BlockPaste();
                break; // Stop further processing after blocking
            } else {
                std::wcout << L"File allowed: " << filePath << std::endl;
            }
        }
    }
}

// Create a hidden window for clipboard monitoring
HWND CreateHiddenWindow() {
    const wchar_t CLASS_NAME[] = L"HiddenClipboardMonitor";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW; // Unicode window procedure
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

    // Add clipboard format listener
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

            if (IsClipboardFormatAvailable(CF_HDROP)) { // Check for file drop format
                std::wcout << L"CF_HDROP format detected in clipboard!" << std::endl;
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
