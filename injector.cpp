

#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <string>
#include <psapi.h>
#include <vector>
#include <random>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <stdlib.h> // I HATE STD::FILESYSTEM

#define RED "\x1b[31;1m"
#define GREEN "\x1b[32;1m"
#define CYAN "\x1b[36;1m"
#define WHITE "\x1b[37;1m"

#define RESET "\x1b[0m"


void initConsole() {
	
	// https://solarianprogrammer.com/2019/04/08/c-programming-ansi-escape-codes-windows-macos-linux-terminals/
	
	#ifdef _WIN32
	
	#include <windows.h>

	#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
	#define ENABLE_VIRTUAL_TERMINAL_PROCESSING  0x0004
	#endif
	
	DWORD outMode = 0;
	HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if(stdoutHandle == INVALID_HANDLE_VALUE) {
		exit(GetLastError());
	}

	if(!GetConsoleMode(stdoutHandle, &outMode)) {
		exit(GetLastError());
	}

	outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	if(!SetConsoleMode(stdoutHandle, outMode)) {
		exit(GetLastError());
	}
	
	#endif
	
}

void exitWithCode(int code) {
	std::cout << WHITE << "Press enter to exit\n" << RESET;
	getchar();
	exit(code);
}

std::pair<int, std::string> getPID(const std::string& processName) {
	
	std::vector<int> res;
	
	HANDLE hndl = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPMODULE, 0);
	if(hndl) {

		PROCESSENTRY32 process;
		process.dwSize = sizeof(PROCESSENTRY32);
		
		Process32First(hndl, &process);
		
		do {
			if(process.szExeFile == processName) {
				res.push_back(process.th32ProcessID);
			}
		} while(Process32Next(hndl, &process));

		CloseHandle(hndl);
	}
	
	if(res.size() == 0) {
		std::cerr << "FUCK FUCK FUCK I CANT FIND " << processName << "\n";
		exitWithCode(1);
	}
	
	std::string processPath;
	
	for(auto it = res.begin(); it != res.end();) {
		
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, *it);
		
		char filename[256];
		
		if(!GetModuleFileNameExA(processHandle, NULL, filename, 256)) {
			std::cerr << "Error getting module filename for process\n";
			exitWithCode(1);
		}
		
		processPath = std::string(filename);
		break;
	}
	
	std::pair<int, std::string> result(res[0], processPath);

	return result;
}

std::string getPathOnly(const std::string& s) {
		
	// ik this is shit but im tired ok 
	// ive spent to much time with c strings to want to use std::string for anything other than ease 

	char buffer[256];
	
	memset(buffer, '\0', 256);
	
	const char* temp = s.c_str();
	
	memcpy(buffer, s.c_str(), std::strrchr(temp, '\\') + 1 - temp);
	
	return std::string(buffer);
}

bool inject(unsigned long procID, const char* dllPath) {
	
	/*BOOL WPM = 0;

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procID);
	if (hProc == INVALID_HANDLE_VALUE) {
		return -1;
	}
	void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	WPM = WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, 0);
	if (!WPM) {
		CloseHandle(hProc);
		return -1;
	}
	printf("DLL Injected Succesfully 0x%lX\n", WPM);
	HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
	if (!hThread) {
		VirtualFree(loc, strlen(dllPath) + 1, MEM_RELEASE);
		CloseHandle(hProc);
		return -1;
	}
	printf("Thread Created Succesfully 0x%lX\n", hThread);
	CloseHandle(hProc);
	VirtualFree(loc, strlen(dllPath) + 1, MEM_RELEASE);
	CloseHandle(hThread);
	return 0;
	*/
	
	unsigned long pid = procID;
	const char* path = dllPath;
	
	HANDLE procHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD, false, pid);
	if (procHandle == NULL)
		return false;

	printf("[INJECTION] Got a handle to %lu\n", pid);

	PVOID dllNameAdr = VirtualAllocEx(procHandle, NULL, strlen(path) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (dllNameAdr == NULL)
		return false;

	//printf("[INJECTION] Got %u bytes into %lu\n", strlen(path) + 1, pid);

	if (WriteProcessMemory(procHandle, dllNameAdr, path, strlen(path) + 1, NULL) == 0)
		return false;

	//printf("[INJECTION] Wrote %u bytes into %lu\n", strlen(path) + 1, pid);

	HANDLE tHandle = CreateRemoteThread(procHandle, 0, 0, (LPTHREAD_START_ROUTINE)(void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"), dllNameAdr, 0, 0);
	if (tHandle == NULL)
		return false;

	printf("[INJECTION] Started LoadLibraryA into %lu\n", pid);

	return true;
}

int main() {
	
	initConsole();
	
	printf("um hi?\n");
	
	const std::string programName = "MBAA.exe";
	
	std::pair<int, std::string> result = getPID(programName);
	
	int pid = result.first;
	std::string fullPath = result.second;
	std::string path = getPathOnly(fullPath);
	
	printf("%d at %s,, path: %s\n", pid, &(fullPath[0]), &(path[0]));
	
	
	char buffer[256];
	GetModuleFileNameA(NULL, buffer, 256);
	*(std::strrchr(buffer, '\\') + 1) = '\0';
	
	std::string source = std::string(buffer) + "hideAttackDisplay.dll";
	std::string dest = path + "hideAttackDisplay.dll";
	
	std::cout << source << "\n";
	std::cout << dest << "\n\n\n";
	
	//fs::copy_file(source, dest, fs::copy_options::overwrite_existing | fs::copy_options::update_existing);
	
	//std::string cmd = "copy /y /b \"" + source + "\" \"" + dest + "\"";
	
	//system(cmd.c_str());
	
	//printf("%s\n", buffer);
	
	//printf("%scopied dll into melty path%s\nI think?? i honestly dont know\n", GREEN, RESET);
	
	bool res = inject(pid, dest.c_str());
	
	if(!res) {
		printf("%ssomethings wrong%s\n", RED, RESET);
		return 1;
	}
	
	printf("%sthings ran successfully? i think?%s\n", GREEN, RESET);
	return 0;
}

