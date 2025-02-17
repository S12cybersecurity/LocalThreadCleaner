#pragma once
#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>

using namespace std;

class CallStackCleaner
{
public:
    void static CleanAllStack(HANDLE hThread) {
        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_FULL;
        // Suspend the thread before modifying its context
        SuspendThread(hThread);
        // Get the thread's context
        if (!GetThreadContext(hThread, &ctx)) {
            cerr << "Error retrieving thread context" << endl;
            ResumeThread(hThread);
            return;
        }
        DWORD64 originalRsp = ctx.Rsp;
        cout << "Original RSP: " << hex << originalRsp << endl;
        constexpr int STACK_CLEAN_SIZE = 16;
        DWORD64 stackValues[STACK_CLEAN_SIZE];
        // Read the target thread's stack
        SIZE_T bytesRead;
        if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)originalRsp, stackValues, sizeof(stackValues), &bytesRead)) {
            cerr << "Error reading stack memory" << endl;
            ResumeThread(hThread);
            return;
        }
        // Clear the stack
        ZeroMemory(stackValues, sizeof(stackValues));
        // Write the cleaned values back to the thread's stack
        SIZE_T bytesWritten;
        if (!WriteProcessMemory(GetCurrentProcess(), (LPVOID)originalRsp, stackValues, sizeof(stackValues), &bytesWritten)) {
            cerr << "Error writing stack memory" << endl;
        }
        ctx.Rsp = originalRsp;  // Restore RSP
        // Apply the changes to the thread context
        if (!SetThreadContext(hThread, &ctx)) {
            cerr << "Error setting thread context" << endl;
        }
        // Resume the thread
        ResumeThread(hThread);

        cout << "Cleaned stack values: " << endl;
		for (int i = 0; i < STACK_CLEAN_SIZE; i++) {
			cout << hex << stackValues[i] << ", ";
		}
        getchar();
    }

    static void CleanSpecificAddressStack(HANDLE hThread, DWORD64 targetAddress) {
        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_FULL;
        SuspendThread(hThread);
        if (!GetThreadContext(hThread, &ctx)) {
            cerr << "Error retrieving thread context" << endl;
            ResumeThread(hThread);
            return;
        }
        DWORD64 originalRsp = ctx.Rsp;
        constexpr int STACK_CLEAN_SIZE = 16;
        DWORD64 stackValues[STACK_CLEAN_SIZE];
        SIZE_T bytesRead;
        if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)originalRsp, stackValues, sizeof(stackValues), &bytesRead)) {
            cerr << "Error reading stack memory" << endl;
            ResumeThread(hThread);
            return;
        }
        for (int i = 0; i < STACK_CLEAN_SIZE; i++) {
            if (stackValues[i] == targetAddress) {
                stackValues[i] = 0x0;
            }
        }
        SIZE_T bytesWritten;
        if (!WriteProcessMemory(GetCurrentProcess(), (LPVOID)originalRsp, stackValues, sizeof(stackValues), &bytesWritten)) {
            cerr << "Error writing stack memory" << endl;
        }
        ctx.Rsp = originalRsp;
        if (!SetThreadContext(hThread, &ctx)) {
            cerr << "Error setting thread context" << endl;
        }
        ResumeThread(hThread);

		cout << "Cleaned stack values: " << endl;
		for (int i = 0; i < STACK_CLEAN_SIZE; i++) {
			cout << hex << stackValues[i] << ", ";
		}
		getchar();
    }
};