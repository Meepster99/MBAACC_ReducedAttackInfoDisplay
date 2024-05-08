

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

float randFloat(float Min, float Max) {
    return ((float(rand()) / float(RAND_MAX)) * (Max - Min)) + Min;
}

typedef void (*OriginalFunctionType)(void);

OriginalFunctionType originalFunction = nullptr;

void doNothing() {
	
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
	
	jumpCode[1] = (funcOffset & 0xFF) >> 0;
	jumpCode[2] = (funcOffset & 0xFF00) >> 8;
	jumpCode[3] = (funcOffset & 0xFF0000) >> 16;
	jumpCode[4] = (funcOffset & 0xFF000000) >> 24;
	
	doPatch(patchAddr, jumpCode, sizeof(jumpCode));	
}

void patchByte(DWORD addr, const u8 code) {
	
	u8 temp[] = {code};

	doPatch(addr, temp, 1);
}

// -----

class MessagePush { // 0x68 if 4, 0x6a if 1
public:
	
	MessagePush(std::initializer_list<unsigned> addresses_, std::initializer_list<unsigned> lengths_) : addresses(addresses_), lengths(lengths_) { }
	
	void setPosition(std::vector<unsigned> pos) {
		
		u8 pushBytes[5] = {0x68, 0x00, 0x00, 0x00, 0x00};
		
		for(size_t i=0; i<addresses.size(); i++) {
			
			pushBytes[0] = lengths[i] == 4 ? 0x68 : 0x6a;
			
			pushBytes[1] = (pos[i] & 0xFF) >> 0;
			pushBytes[2] = (pos[i] & 0xFF00) >> 8;
			pushBytes[3] = (pos[i] & 0xFF0000) >> 16;
			pushBytes[4] = (pos[i] & 0xFF000000) >> 24;

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

struct Pair {
	float x = 0;
	float y = 0;
};

void newTrainingDisplay() {

	/*
	
	hit someone
	random vector, magnitude related to hit strength 
	just like throw it in that direction?
	
	combo bar has gravity
	
	
	00478cf7 8b 44 24 48     MOV        EAX,dword ptr [ESP + local_14c]
	00478cfb 8b 50 0c        MOV        EDX,dword ptr [EAX + 0xc]
	00478cfe 8b 40 08        MOV        EAX,dword ptr [EAX + 0x8]

	SP -> 0019FA2C
	0x00557e40
		
	*/
	
	// go backwards from call!
	DisplayLine combo(
	MessagePush({0x00478c61, 0x00478c5c, 0x00478c55, 0x00478c49, 0x00478c44}, {4, 4, 4, 1, 4}),
	MessagePush({0x00478cbe, 0x00478cb9, 0x00478cb0, 0x00478cae, 0x00478ca9}, {4, 4, 4, 1, 4})
	);
	
	DisplayLine damage(
	MessagePush({0x00478ce6, 0x00478ce1, 0x00478cdc, 0x00478cda, 0x00478cd5}, {4, 4, 4, 1, 4}),
	MessagePush({0x00478d37, 0x00478d32, 0x00478d29, 0x00478d27, 0x00478d22}, {4, 4, 4, 1, 4})
	);
	
	static Pair pos;
	static Pair vel;
	
	static bool wait = true;
	static std::vector<unsigned> what = {0x88, 0xff, 0xff, 0x0e, 0xfe};
	static int val = 0x01a4;

	/*
	if(GetKeyState(VK_RIGHT) & 0x8000) {
		val++;
	}
	
	if(GetKeyState(VK_LEFT) & 0x8000) {
		val--;
	}
	
	if(GetKeyState(VK_DOWN) & 0x8000) {
		what[0]++;
	}
	
	if(GetKeyState(VK_UP) & 0x8000) {
		what[0]--;
	}
	
	what[0] &= 0x1ff;
	val &= 0x1ff;
	*/

	if(!wait) {
			
		val = pos.x + 240;
		what[0] = 400 - pos.y;

		u8 xPosCode[] = {0x81, 0xc1, 0xa4, 0x01, 0x00, 0x00};
		
		xPosCode[2] = (val & 0xFF) >> 0;
		xPosCode[3] = (val & 0xFF00) >> 8;
		xPosCode[4] = (val & 0xFF0000) >> 16;
		xPosCode[5] = (val & 0xFF000000) >> 24;
		doPatch(0x00478c11, xPosCode, sizeof(xPosCode));

		
		combo.setPosition(what);
		std::vector<unsigned> tempWhat = what;
		tempWhat[0] += 0xE;
		damage.setPosition(tempWhat);
	}
	
	u8 originalFuncCode[] = {0x55, 0x8b, 0xec, 0x83, 0xe4};
	doPatch(0x00478bc0, originalFuncCode, sizeof(originalFuncCode));
	asm("CALL 0x00478bc0");
	patchFunction(0x00478bc0, (DWORD)newTrainingDisplay);

	unsigned damageComboVal = *reinterpret_cast<unsigned*>(0x00557e40 + 8);
	unsigned comboVal = *reinterpret_cast<unsigned*>(0x00557e40);
	
	if(wait && damageComboVal) {
		wait = false;
	}
	
	unsigned damageVal = *reinterpret_cast<unsigned*>(0x5551EC) + *reinterpret_cast<unsigned*>(0x5551EC + 0xAFC);

	unsigned redDamageVal = *reinterpret_cast<unsigned*>(0x5551F0) + *reinterpret_cast<unsigned*>(0x5551F0 + 0xAFC);

	static unsigned prevDamageVal = -1;
	
	if(prevDamageVal != damageVal && damageVal < prevDamageVal) {

		unsigned damageDif = prevDamageVal - damageVal;

		Pair newVel;
		
		const float scale = 0.05;
		
		newVel.x = damageDif * scale * randFloat(0.3, 1.0);
		
		if(rand() % 2) {
			newVel.x = -newVel.x;
		}
		
		if(vel.x > 4) {
			newVel.x = fabs(newVel.x);
		} else if(newVel.x < -4) {
			newVel.x = -fabs(newVel.x);
		}
		
		newVel.y = damageDif * scale * 0.7;

		newVel.x = CLAMP(newVel.x, -20, 20);
		newVel.y = CLAMP(newVel.y, 1, 20);
		
		if(vel.y < -19) {
			vel.y = 0;
		}
		
		vel.x += newVel.x;
		vel.y += fabs(newVel.y);
	}
	
	prevDamageVal = damageVal;
	
	if(pos.y > 4 && pos.y + vel.y < -4 && vel.y < 0) {
		vel.y = -vel.y * 0.40;
	}
	
	pos.x += vel.x;
	pos.y += vel.y;
	
	if(GetKeyState(VK_RCONTROL) & 0x8000) {
		FILE* f = fopen("bruh.txt", "a");
		fprintf(f, "%6.2f %6.2f %6.2f %6.2f %d %d %d\n", pos.x, pos.y, vel.x, vel.y, damageVal, redDamageVal, comboVal);
		fclose(f);
	}
		
	const float accel = -1;
	
	float air = 0.1;
	
	if(pos.y < 0) {
		air *= 5;
	}
	
	if(vel.x > 2 * air) {
		vel.x -= air;
	} else if(vel.x < -2 * air) {
		vel.x += air;
	} else {
		vel.x = 0;
	}

	if(pos.x < -230 || pos.x > 220) {
		vel.x = -vel.x * .60;
	}
	
	if(pos.y > 420) {
		vel.y = -fabs(vel.y) * 1.5;
	}
	
	vel.x = CLAMP(vel.x, -100, 100);
	vel.y = MAX(vel.y + accel, -20);
	
	pos.x = CLAMP(pos.x, -240, 220);
	pos.y = CLAMP(pos.y, 0, 420);
	
}

void threadFunc() {
	
	//system("echo start > bruh.txt");
	
	srand(time(NULL));
	

	patchFunction(0x00478bc0, (DWORD)newTrainingDisplay);

	// turn on training disp
	patchByte(0x005595b8, 1);
	
	// byte 1 of total meter gain? from dll?? how??
	//patchByte(0x665f17d5, 1);
	

	//ListLoadedDLLs();
	
	
	HMODULE hModule = GetModuleHandle("hook.dll");
	
	if(hModule == NULL) {
		system("echo hook dll addr was null, what? >> bruh.txt");
	}
	
	DWORD hookOffset = (DWORD)hModule;
	
	char buffer[256];
	snprintf(buffer, 256, "echo hook dll addr was %08lX >> bruh.txt", (DWORD)hModule);
	//	system(buffer);
	
	// 0x66380000 is the base addr in ghidra??
	
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