

# D:/Mingw/gdb32/bin/gdb32.exe ./MBAA.exe

CC = D:/Mingw/bin/g++  
CFLAGS =  -g -Wall -Wextra -lpsapi -static -std=c++17 -lstdc++fs -mtune=generic -march=x86-64

# cd "D:/Games/melty blood/Community_Edition_3-1-004BACK/MBAACC - Community Edition/MBAACC/" && MBAA.exe
# "D:/Games/melty blood/Community_Edition_3-1-004BACK/MBAACC - Community Edition/MBAACC/MBAA.exe"

# if i add the melty to my path, would this magically work? 
# D:/Games/melty blood/Community_Edition_3-1-004BACK/MBAACC - Community Edition/MBAACC

# Source files
EXE_SOURCE = injector.cpp
DLL_SOURCE = maindll.cpp

# Output files
EXE_NAME = injector.exe
DLL_NAME = hideAttackDisplay.dll

# Targets
all: $(EXE_NAME) $(DLL_NAME)

$(EXE_NAME): $(EXE_SOURCE)
	$(CC) -o $@ $^ $(CFLAGS)

$(DLL_NAME): $(DLL_SOURCE)
	$(CC) -shared -o $@ $^ $(CFLAGS)
	# this is a bad idea, but if my memory is correct, the dll should be in there before boot
	cp $(DLL_NAME) "D:/Games/melty blood/Community_Edition_3-1-004BACK/MBAACC - Community Edition/MBAACC"

clean:
		rm $(DLL_NAME) $(EXE_NAME)
