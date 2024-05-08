

#include <windows.h>
#include <cstring> 
#include <cstdio>
#include <psapi.h>
#include <cstdint>
#include <vector> 
#include <cmath>
#include <time.h>
#include <stdlib.h>

typedef unsigned char u8;

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(value, min_val, max_val) MAX(MIN((value), (max_val)), (min_val))

#define ATTACKDISPLAYADDR 0x00478bc0
#define TRAININGDISPLAYTOGGLE 0x005595b8

float randFloat(float Min, float Max) {
    return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
}

void doPatch(unsigned addr, const u8* code, unsigned length) {
	
	LPVOID tempAddr = reinterpret_cast<LPVOID>(addr); 
	
	DWORD oldProtect;
	VirtualProtect(tempAddr, length, PAGE_EXECUTE_READWRITE, &oldProtect);
	
	memcpy(tempAddr, code, length);
	
	VirtualProtect(tempAddr, length, oldProtect, NULL);
}

void patchFunction(DWORD patchAddr, DWORD newAddr) {
	
	u8 jumpCode[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
	
	DWORD funcOffset = (DWORD)newAddr - patchAddr - 5;
	
	*(unsigned*)(&jumpCode[1]) = funcOffset;

	doPatch(patchAddr, jumpCode, sizeof(jumpCode));	
}

void patchByte(DWORD addr, const u8 code) {
	
	u8 temp[] = {code};

	doPatch(addr, temp, 1);
}

// -----

class MessagePush {
public:
	
	MessagePush(std::initializer_list<unsigned> addresses_, std::initializer_list<unsigned> lengths_) : addresses(addresses_), lengths(lengths_) { }
	
	void setPosition(std::vector<unsigned> pos) {
		
		u8 pushBytes[5];
		
		for(size_t i=0; i<addresses.size(); i++) {
			
			// push is 0x68 if 4 bytes, 0x6a if 1 byte
			pushBytes[0] = lengths[i] == 4 ? 0x68 : 0x6a;
			
			*(unsigned*)(&pushBytes[1]) = pos[i];

			doPatch(addresses[i], pushBytes, lengths[i] + 1);
		}
	}

private:
	/*
	addresses are:
	y pos
	brightness
	transparency 
	stretch 
	UNKNOWN
	*/
	std::vector<unsigned> addresses;
	std::vector<unsigned> lengths;
};

class DisplayLine {
public:

	DisplayLine(MessagePush word_, MessagePush num_) : word(word_), num(num_) {
	}
	
	void setPosition(std::vector<unsigned> pos) {
		word.setPosition(pos);
		num.setPosition(pos);	
	}

	void setPosition(std::initializer_list<unsigned> pos) {
		setPosition(pos);
	}
	
private:	
	MessagePush word;
	MessagePush num;

};

// -----

unsigned xPos = 200;
unsigned yPos = 200;

void readCoords() {

	FILE* f = fopen("ReduceAttackInfoDisplay.ini", "r");
	
	if(f != NULL) {		
		fscanf(f, "%d %d", &xPos, &yPos);
		fclose(f);
	}
}

void saveCoords() {
	FILE* f = fopen("ReduceAttackInfoDisplay.ini", "w");
	fprintf(f, "%d %d", xPos, yPos);
	fclose(f);
}

void newTrainingDisplay() {

	DisplayLine combo(
		MessagePush({0x00478c61, 0x00478c5c, 0x00478c55, 0x00478c49, 0x00478c44}, {4, 4, 4, 1, 4}),
		MessagePush({0x00478cbe, 0x00478cb9, 0x00478cb0, 0x00478cae, 0x00478ca9}, {4, 4, 4, 1, 4})
	);
	
	DisplayLine damage(
		MessagePush({0x00478ce6, 0x00478ce1, 0x00478cdc, 0x00478cda, 0x00478cd5}, {4, 4, 4, 1, 4}),
		MessagePush({0x00478d37, 0x00478d32, 0x00478d29, 0x00478d27, 0x00478d22}, {4, 4, 4, 1, 4})
	);
	
	static bool rControlState = false;
	
	if(rControlState && !(GetKeyState(VK_RCONTROL) & 0x8000)) {
		saveCoords();
	}
	
	rControlState = GetKeyState(VK_RCONTROL) & 0x8000;
	
	if(rControlState) {
		if(GetKeyState(VK_RIGHT) & 0x8000) {
			xPos = (xPos + 1) % 480;
		}
		
		if(GetKeyState(VK_LEFT) & 0x8000) {
			xPos = (xPos - 1) % 480;
		}
		
		if(GetKeyState(VK_DOWN) & 0x8000) {
			yPos = (yPos + 1) % 480;
		}
		
		if(GetKeyState(VK_UP) & 0x8000) {
			yPos = (yPos - 1) % 480;
		}
	}
	
	// patch display xPos
	u8 xPosCode[] = {0x81, 0xc1, 0x00, 0x00, 0x00, 0x00};
	*(unsigned*)(&xPosCode[2]) = xPos;
	doPatch(0x00478c11, xPosCode, sizeof(xPosCode));
	
	// patch display yPos
	std::vector<unsigned> yPosCode = {yPos, 0xff, 0xff, 0x0e, 0xfe};
	combo.setPosition(yPosCode);
	yPosCode[0] += 0xE;
	damage.setPosition(yPosCode);
	
	// patch the original function code back in, after our modifications have been made
	u8 originalFuncCode[] = {0x55, 0x8b, 0xec, 0x83, 0xe4};
	doPatch(ATTACKDISPLAYADDR, originalFuncCode, sizeof(originalFuncCode));
	
	// https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
	asm("CALL *%0" : : "r" (ATTACKDISPLAYADDR) );
	
	// reinsert hook to this function
	patchFunction(ATTACKDISPLAYADDR, (DWORD)newTrainingDisplay);
	
}

void threadFunc() {
	
	srand(time(NULL));
	
	readCoords();
	
	// patch the attack display to our funciton
	patchFunction(ATTACKDISPLAYADDR, (DWORD)newTrainingDisplay);

	// turn on training disp
	patchByte(TRAININGDISPLAYTOGGLE, 1);

	HMODULE hModule = GetModuleHandle("hook.dll");
	
	if(hModule == NULL) {
		system("echo hook dll addr was null, what? >> ReduceAttackInfoDisplay.log");
	}
	
	DWORD hookOffset = (DWORD)hModule;
	
	// 0x665f17d5 is total meter gain 
	patchByte(hookOffset + 0x665f17d5 - 0x66380000, '\0');
	
	// 0x665f17c1 is meter gain 
	patchByte(hookOffset + 0x665f17c1 - 0x66380000, '\0');
	
	// nullterm printf statements for both 
	patchByte(hookOffset + 0x665f17cd - 0x66380000, '\0');
	
	// not exactly a func, but just adding in a jmp to skip everything after the two needed lines
	patchFunction(0x00478d47, (DWORD)0x00478fcc);
	
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	(void)hinstDLL;
	(void)lpReserved;
	
	if(fdwReason == DLL_PROCESS_ATTACH) {
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)threadFunc, 0, 0, 0);
	}
	
	return TRUE;
}

