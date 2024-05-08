

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
#include <fstream>
#include <stdlib.h>

#define RED "\x1b[31;1m"
#define GREEN "\x1b[32;1m"
#define CYAN "\x1b[36;1m"
#define WHITE "\x1b[37;1m"

#define RESET "\x1b[0m"

void initConsole() {
	
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

void exitWithCode(int code = 0, std::string msg = std::string("Press enter to exit")) {
	std::cout << (code ? RED : WHITE);
	std::cout << msg << RESET << "\n";
	
	if(msg != "Press enter to exit") {
		std::cout << WHITE << "Press enter to exit\n" << RESET;
	}
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
		std::cerr << RED << "Unable to find " << processName << ", is it running?\n" << RESET;
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

	std::cerr << GREEN << "Found " << processName << " at " << processPath << "\n" <<RESET;
		
	
	return result;
}

std::string getPathOnly(const std::string& s) {
	
	char buffer[256];
	
	memset(buffer, '\0', 256);
	
	const char* temp = s.c_str();
	
	memcpy(buffer, s.c_str(), std::strrchr(temp, '\\') + 1 - temp);
	
	return std::string(buffer);
}

bool fileExists(const std::string& s) {
	std::ifstream f(s.c_str());
    return f.good();
}

bool inject(unsigned long procID, const char* dllPath) {
	
	unsigned long pid = procID;
	const char* path = dllPath;
	
		
	HANDLE procHandle = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD, false, pid);
	if (procHandle == NULL)
		return false;

	printf("[INJECTION] Got a handle to %lu\n", pid);

	PVOID dllNameAdr = VirtualAllocEx(procHandle, NULL, strlen(path) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (dllNameAdr == NULL)
		return false;


	if (WriteProcessMemory(procHandle, dllNameAdr, path, strlen(path) + 1, NULL) == 0)
		return false;

	HANDLE tHandle = CreateRemoteThread(procHandle, 0, 0, (LPTHREAD_START_ROUTINE)(void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"), dllNameAdr, 0, 0);
	if (tHandle == NULL)
		return false;

	printf("[INJECTION] Started LoadLibraryA into %lu\n", pid);

	return true;
}

int main() {
	
	initConsole();
	
	const std::string programName = "MBAA.exe";
	
	std::pair<int, std::string> result = getPID(programName);
	
	int pid = result.first;
	std::string fullPath = result.second;
	std::string path = getPathOnly(fullPath);

	char buffer[256];
	GetModuleFileNameA(NULL, buffer, 256);
	*(std::strrchr(buffer, '\\') + 1) = '\0';
	
	std::string source = std::string(buffer) + "ReduceAttackInfoDisplay.dll";
	std::string dest = path + "ReduceAttackInfoDisplay.dll";

	if(!fileExists(dest)) {
		exitWithCode(1, "unable to find ReduceAttackInfoDisplay.dll in melty path. copy it in");
	}
	
	bool res = inject(pid, dest.c_str());
	
	if(!res) {
		printf("%ssomethings wrong with the injection, msg me%s\n", RED, RESET);
		getchar();
		return 1;
	}
	
	printf("%sinjection ran successfully%s\n", GREEN, RESET);
	std::cout << WHITE << "Press RControl and arrow keys to move display\n" << RESET;
	std::cout << WHITE << "Press enter to exit\n" << RESET;
	getchar();
	return 0;
}
