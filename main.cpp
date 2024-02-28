#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <fstream>
#pragma comment(lib, "kernel32.lib")

bool InjectDLL(const char* processName, const char* dllPath) {
    HANDLE hProcess = NULL;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(hSnapshot, &entry)) {
        do {
            if (strcmp(entry.szExeFile, processName) == 0) {
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                break;
            }
        } while (Process32Next(hSnapshot, &entry));
    }
    CloseHandle(hSnapshot);

    if (hProcess == NULL) {
        std::cerr << "Failed to find the process." << std::endl;
        return false;
    }

    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (loadLibraryAddr == NULL) {
        std::cerr << "Failed to get address of LoadLibraryA." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    LPVOID remoteStringAddr = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (remoteStringAddr == NULL) {
        std::cerr << "Failed to allocate memory in the remote process." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteStringAddr, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cerr << "Failed to write to remote process memory." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteStringAddr, 0, NULL);
    if (hThread == NULL) {
        std::cerr << "Failed to create remote thread." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hProcess);

    return true;
}

int main() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    setlocale(NULL, "RUS");

    std::ifstream acceptedFile("accepted.txt");
    if (acceptedFile.good()) {
        acceptedFile.close();
    }
    else {
        acceptedFile.close();
        MessageBox(NULL, "Продолжение использования нашего античита подразумевает ваше согласие с условиями настоящей политики конфиденциальности и обработкой ваших данных в соответствии с указанными принципами(https://vk.cc/cv0Nba).", "Соглашение", MB_OK);
        std::ofstream outFile("accepted.txt");
        if (outFile.is_open()) {
            outFile << "1";
            outFile.close();
        }
    }

    char currentPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentPath);
    std::string rustClientPath = currentPath;
    rustClientPath += "\\RustClient.exe";

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(NULL, (LPSTR)rustClientPath.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBox(NULL, "Failed to start RustClient.exe", "Error", MB_OK);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    Sleep(3000);

    std::string easyAntiCheatPath = currentPath;
    easyAntiCheatPath += "\\EasyAntiCheat.dll";

    if (!InjectDLL("RustClient.exe", easyAntiCheatPath.c_str())) {
        MessageBox(NULL, "Failed to inject", "Error", MB_OK);
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
