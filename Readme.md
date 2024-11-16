# Clipboard Access Restriction Software

## Objective
This software restricts users from copying and pasting files or folders from a specified "root" directory (`C:\Windows` by default) to any location outside this folder, including external drives or cloud locations.

---

## Key Features
- **Clipboard Monitoring**: Detects copy-paste attempts at the system level.
- **Root Folder Protection**: Blocks pasting files or folders that are outside the root directory.
- **const std::wstring ROOT_FOLDER = L"C:\\Windows"; // past the root folder to restrict**
- **Unauthorized Action Logging**: Logs all unauthorized paste attempts in a file (`ClipboardMonitorLog.txt`).
- **Console Logging**: Provides real-time logs for debugging and monitoring.
- **Hidden Window**: Uses a hidden window for clipboard monitoring events.

---

## Setup Instructions

### Prerequisites
- **Operating System**: Windows 11.
- **Programming Environment**: `g++` compiler (MinGW or MSYS2 recommended).
- **Administrator Privileges**: Run the software as an administrator to allow system-level clipboard access.

---

## Installation and Build Steps

### Step 1: Clone the Repository
1. Clone the repository:
   ```bash
   git clone <https://github.com/mithlesh-27/Clipboard_Monitor.git>
   cd <Clipboard_Monitor>

----

### Step 2: Compile the Code
Run the following command in the terminal:
g++ -municode -mconsole -o ClipboardMonitor main.cpp

### Step 3: Run the Executable
ClipboardMonitor.exe
Run as Administrator:
