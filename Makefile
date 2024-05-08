

CC = g++ 
CFLAGS =  -g -Wall -Wextra -lpsapi -static -std=c++17 -lstdc++fs -mtune=generic -march=x86-64

# Source files
EXE_SOURCE = injector.cpp
DLL_SOURCE = maindll.cpp

# Output files
EXE_NAME = ReduceAttackInfoDisplay.exe
DLL_NAME = ReduceAttackInfoDisplay.dll

# Targets
all: $(EXE_NAME) $(DLL_NAME)

$(EXE_NAME): $(EXE_SOURCE)
	$(CC) -o $@ $^ $(CFLAGS)

$(DLL_NAME): $(DLL_SOURCE)
	$(CC) -shared -o $@ $^ $(CFLAGS)
	
clean:
		rm $(DLL_NAME) $(EXE_NAME)
