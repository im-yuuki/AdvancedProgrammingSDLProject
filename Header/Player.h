#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>
#include <map>
#include <string>   

class Player {
public:
    Player();
    Player(int x, int y);
    void handleInput(const Uint8* keys);
    void update();
    void render(SDL_Renderer* renderer);
    void resetPosition(int x, int y); // Thêm hàm reset
    void loadKeybinds();
    
private:
    SDL_Rect rect;
    int speed;
    float velocityY;  // Vận tốc theo trục Y
    bool isJumping;   // Kiểm tra có đang nhảy không
    const float gravity = 0.5f; // Trọng lực
    std::map<std::string, SDL_Keycode> keybinds;
};

#endif
