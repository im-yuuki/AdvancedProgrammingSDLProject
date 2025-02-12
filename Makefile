CC = g++
CFLAGS = -I src/include -I src/include/SDL2 -I Header
LDFLAGS = -L src/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_mixer

OBJ = main.o Game/Player.o Game/Menu.o Game/PauseMenu.o Game/SettingsMenu.o

all: game

game: $(OBJ)
	$(CC) -o game $(OBJ) $(LDFLAGS)

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp -o main.o

Game/Menu.o: Game/Menu.cpp
	$(CC) $(CFLAGS) -c Game/Menu.cpp -o Game/Menu.o

Game/PauseMenu.o: Game/PauseMenu.cpp
	$(CC) $(CFLAGS) -c Game/PauseMenu.cpp -o Game/PauseMenu.o

Game/Player.o: Game/Player.cpp
	$(CC) $(CFLAGS) -c Game/Player.cpp -o Game/Player.o

Game/Player.o: Game/SettingsMenu.cpp
	$(CC) $(CFLAGS) -c Game/SettingsMenu.cpp -o Game/SettingsMenu.o
clean:
	rm -f game $(OBJ)