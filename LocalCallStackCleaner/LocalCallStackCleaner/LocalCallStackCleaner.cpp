#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include "CallStackCleaner.h"

using namespace std;

DWORD WINAPI DummyThreadFunction(LPVOID lpParam) {
    OpenProcess(0, FALSE, GetCurrentProcessId());

    while (true) {
        Sleep(1000);
    }
    return 0;
}

void showHelp() {
    cout << "=====================================\n";
    cout << "   Local Call Stack Cleaner - Help\n";
    cout << "=====================================\n\n";
    cout << "Usage:\n";
    cout << "  stackcleaner [OPTION]...\n\n";
    cout << "Options:\n";
    cout << "  -all            Clean all stack entries in the selected thread\n";
    cout << "  -ma <address>   Clean entries with the given memory address\n";
    cout << "  -fn <module> <function>  Clean entries related to the given function\n\n";
    cout << "Examples:\n";
    cout << "  stackcleaner -all\n";
    cout << "  stackcleaner -ma 0x12345678\n";
    cout << "  stackcleaner -fn kernel32.dll OpenProcess\n\n";
    cout << "=====================================\n";
}

vector<DWORD> getRunningThreads() {
    vector<DWORD> threadIDs;
    DWORD pid = GetCurrentProcessId();
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) return threadIDs;
    if (Thread32First(hThreadSnap, &te32)) {
        do {
            if (te32.th32OwnerProcessID == pid) {
                threadIDs.push_back(te32.th32ThreadID);
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }
    CloseHandle(hThreadSnap);
    return threadIDs;
}

DWORD selectThreadDynamically() {
    vector<DWORD> threadIDs = getRunningThreads();
    if (threadIDs.empty()) {
        cout << "No running threads found. Exiting...\n";
        exit(1);
    }
    cout << "Available threads:\n";
    for (size_t i = 0; i < threadIDs.size(); i++) {
        cout << "  [" << i + 1 << "] Thread ID: " << threadIDs[i] << (i == threadIDs.size() - 1 ? " (Dummy Thread)" : "") << endl;
    }
    int choice;
    cout << "Select a thread (1-" << threadIDs.size() << "): ";
    cin >> choice;
    if (choice < 1 || choice > threadIDs.size()) {
        cout << "Invalid selection. Exiting...\n";
        exit(1);
    }
    return threadIDs[choice - 1];
}

DWORD64 GetFunctionAddress(const char* moduleName, const char* functionName) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) return 0;
    FARPROC funcAddr = GetProcAddress(hModule, functionName);
    return reinterpret_cast<DWORD64>(funcAddr);
}

int main(int argc, char* argv[]) {
    cout << "\nLocal Call Stack Cleaner\n";
    cout << "=========================\n";

    HANDLE hDummyThread = CreateThread(NULL, 0, DummyThreadFunction, NULL, 0, NULL);
    if (!hDummyThread) {
        cout << "Error: Failed to create dummy thread.\n";
        return 1;
    }

    if (argc < 2) {
        cout << "Error: No arguments provided.\n";
        cout << "Use '-help' to see available options.\n";
        return 1;
    }

    string option = argv[1];
    DWORD threadID = selectThreadDynamically();
    cout << "Selected Thread ID: " << threadID << endl;

    if (option == "-all") {
        cout << "Cleaning all stack entries for Thread ID: " << threadID << endl;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadID);
        if (hThread == NULL) {
            cout << "Error: Failed to open thread.\n";
            return 1;
        }
        CallStackCleaner::CleanAllStack(hThread);
        cout << endl << "Press any key to exit...\n";
		getchar();
        CloseHandle(hThread);
    }
    else if (option == "-ma") {
        if (argc < 3) {
            cout << "Error: Missing memory address.\n";
            return 1;
        }
        DWORD address = (DWORD)strtoul(argv[2], nullptr, 16);
        cout << "Cleaning entries with memory address: " << argv[2] << " in Thread ID: " << threadID << endl;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadID);
        if (hThread == NULL) {
            cout << "Error: Failed to open thread.\n";
            return 1;
        }
        CallStackCleaner::CleanSpecificAddressStack(hThread, address);
		cout << endl << "Press any key to exit...\n";
        getchar();
        CloseHandle(hThread);
    }
    else if (option == "-fn") {
        if (argc < 4) {
            cout << "Error: Missing module name or function name.\n";
            return 1;
        }
        const char* moduleName = argv[2];
        const char* functionName = argv[3];
        cout << "Cleaning entries related to function: " << functionName << " in module: " << moduleName << " for Thread ID: " << threadID << endl;
        DWORD64 functionAddr = GetFunctionAddress(moduleName, functionName);
        if (!functionAddr) {
            cout << "Error: Could not resolve function address.\n";
            return 1;
        }
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadID);
        if (hThread == NULL) {
            cout << "Error: Failed to open thread.\n";
            return 1;
        }
        CallStackCleaner::CleanSpecificAddressStack(hThread, functionAddr);
        cout << endl << "Press any key to exit...\n";
		getchar();
        CloseHandle(hThread);
    }
    else if (option == "-help") {
		showHelp();
	}
    else {
        cout << "Error: Unknown option '" << option << "'.\n";
        cout << "Use '-help' to see available options.\n";
        return 1;
    }
    return 0;
}
