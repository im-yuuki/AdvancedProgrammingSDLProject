#include "Menu.h"
#include "Maze.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <SDL_image.h>
#include <filesystem> // Thư viện duyệt thư mục

// Định nghĩa biến tĩnh
std::string Menu::chosenSaveFile = "";

void Menu::loadSettings() {
    std::ifstream file("settings.txt");
    if (file.is_open()) {
        std::string key;
        int value;
        while (file >> key >> value) {
            if (key == "volume") {
                Mix_VolumeMusic(value * MIX_MAX_VOLUME / 100);
            }
        }
        file.close();
    }
}

Menu::Menu(SDL_Renderer* renderer) 
    : renderer(renderer), selectedOption(0), firstPlay(true), blinkTimer(0), blinkState(true) {
    font = TTF_OpenFont("resources/fonts/arial-unicode-ms.ttf", 28);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    // Khởi tạo SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! Error: " << Mix_GetError() << std::endl;
    }
    loadSettings(); // Load settings ngay khi game khởi động
    backgroundMusic = Mix_LoadMUS("resources/audio/menu_music.mp3");
    if (!backgroundMusic) {
        std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
    }

    textColor = {255, 255, 255, 255};
    options = {"Singleplayer", "2 Players", "Guide", "Settings", "Thoát"};

    SDL_Surface* bgSurface = IMG_Load("resources/images/menu_background.jpg");
    if (!bgSurface) {
        std::cerr << "Failed to load background image: " << IMG_GetError() << std::endl;
    } else {
        backgroundTexture = SDL_CreateTextureFromSurface(renderer, bgSurface);
        SDL_FreeSurface(bgSurface);
    }
}

int Menu::chooseNewOrLoad() {
    std::vector<std::string> choices = {"New Game", "Load Game"};
    int selection = 0;
    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return -1;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) selection = (selection - 1 + 2) % 2;
                if (e.key.keysym.sym == SDLK_DOWN) selection = (selection + 1) % 2;
                if (e.key.keysym.sym == SDLK_RETURN) return selection;
                if (e.key.keysym.sym == SDLK_ESCAPE) return -1;
            }
        }
        renderSubMenu(choices, selection);
    }
}

std::string Menu::chooseSaveFile() {
    std::vector<std::string> saveFiles;

	std::filesystem::path p{"Save"};
    if (!std::filesystem::exists(p)) return ""; // Nếu thư mục Save không tồn tại

	for (auto& entry : std::filesystem::directory_iterator(p)) {
		std::string filename = entry.path().filename().string();
		if (filename.find(".txt") != std::string::npos) {
			saveFiles.push_back(filename);
		}
	}
	
    if (saveFiles.empty()) return "";
    int selection = 0;
    SDL_Event e;
    while (true) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return "";
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) selection = (selection - 1 + saveFiles.size()) % saveFiles.size();
                if (e.key.keysym.sym == SDLK_DOWN) selection = (selection + 1) % saveFiles.size();
                if (e.key.keysym.sym == SDLK_RETURN) return saveFiles[selection];
                if (e.key.keysym.sym == SDLK_ESCAPE) return "";
            }
        }
        renderSubMenu(saveFiles, selection);
    }
}

int Menu::run() {
    bool running = true;
    SDL_Event e;
    playMusic();
    // Tạo một đối tượng Maze (nếu cần cho Load Game, không dùng trong New Game)
    Maze loadedMaze;  


    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return -1;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_UP) 
                    selectedOption = (selectedOption - 1 + options.size()) % options.size();
                if (e.key.keysym.sym == SDLK_DOWN) 
                    selectedOption = (selectedOption + 1) % options.size();
                if (e.key.keysym.sym == SDLK_RETURN) {
                    if (selectedOption == 0 || selectedOption == 1) {
                        int gameMode = selectedOption; 
                        int choice = chooseNewOrLoad();
                        if (choice == 0) {  // New Game
                            int ret = handleNewGame();
                            if (ret != -1)
                                return gameMode + ret;
                        }
                        if (choice == 1) {  // Load Game
                            int ret = handleLoadGame();
                            if (ret == 20) return 20;  // Vào game ngay lập tức
                        }                        
                    }
                     else if (selectedOption == 2) {
                        showGuide();
                    } else if (selectedOption == 3) {
                        SettingsMenu settings(renderer);
                        settings.run();
                        loadSettings();
                    } else if (selectedOption == 4) {
                        if (confirmExit()) return -1;
                    }
                }
            }
        }
        if (!Mix_PlayingMusic()) {
            playMusic();
        }
        renderMenu();
        SDL_Delay(16);
    }
    return -1;
}

bool Menu::selectGameMode() {
    SDL_Event e;
    int selected = 0; // 0 = New Game, 1 = Load Game
    bool choosing = true;

    while (choosing) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return false;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                        selected = 1 - selected;
                        break;
                    case SDLK_RETURN:
                        return selected == 0;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Texture* newGameText = renderText(selected == 0 ? "> New Game <" : "New Game", {255, 255, 255, 255});
        SDL_Texture* loadGameText = renderText(selected == 1 ? "> Load Game <" : "Load Game", {255, 255, 255, 255});

        SDL_Rect newGameRect = {300, 200, 200, 40};
        SDL_Rect loadGameRect = {300, 300, 200, 40};

        SDL_RenderCopy(renderer, newGameText, NULL, &newGameRect);
        SDL_RenderCopy(renderer, loadGameText, NULL, &loadGameRect);

        SDL_DestroyTexture(newGameText);
        SDL_DestroyTexture(loadGameText);

        SDL_RenderPresent(renderer);
    }
    return false;
}

int Menu::selectSaveSlot() {
    SDL_Event e;
    int selectedSlot = 1;
    bool choosing = true;

    while (choosing) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 1;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        if (selectedSlot > 1) selectedSlot--;
                        break;
                    case SDLK_RIGHT:
                        if (selectedSlot < 3) selectedSlot++; // Giả sử có tối đa 3 slot
                        break;
                    case SDLK_RETURN:
                        return selectedSlot;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        std::string slotText = "Save Slot: " + std::to_string(selectedSlot);
        SDL_Texture* slotTexture = renderText(slotText, {255, 255, 255, 255});
        SDL_Rect slotRect = {300, 250, 200, 40};

        SDL_RenderCopy(renderer, slotTexture, NULL, &slotRect);
        SDL_DestroyTexture(slotTexture);

        SDL_RenderPresent(renderer);
    }
    return 1;
}

void Menu::renderSubMenu(const std::vector<std::string>& options, int selected) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (size_t i = 0; i < options.size(); i++) {
        SDL_Texture* texture = renderText(i == selected ? "> " + options[i] + " <" : options[i], {255, 255, 255, 255});
        SDL_Rect rect = {300, 200 + (int)i * 50, 200, 40};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }

    SDL_RenderPresent(renderer);
}

Menu::~Menu() {
    if (backgroundTexture) {
        SDL_DestroyTexture(backgroundTexture);
    }
    Mix_FreeMusic(backgroundMusic);
    Mix_CloseAudio();
    TTF_CloseFont(font);
}

void Menu::playMusic() {
    if (!Mix_PlayingMusic()) {
        Mix_PlayMusic(backgroundMusic, -1);
    }
}

void Menu::showGuide() {
    std::ifstream file("resources/data/guide.txt");
    if (!file) {
        std::cerr << "Không thể mở file hướng dẫn!" << std::endl;
        return;
    }

    std::string line;
    std::vector<std::string> guideText;
    while (std::getline(file, line)) {
        guideText.push_back(line);
    }
    file.close();
    SDL_Event e;
    bool viewingGuide = true;
    while (viewingGuide) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                viewingGuide = false;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                viewingGuide = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int yOffset = 100;
        for (const std::string& line : guideText) {
            SDL_Texture* textTexture = renderText(line, {255, 255, 255, 255});
            if (textTexture) {
                SDL_Rect textRect = {100, yOffset, 600, 30};
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
                yOffset += 40;
            }
        }
        SDL_RenderPresent(renderer);
    }
}

void Menu::stopMusic() {
    if (Mix_PlayingMusic()) {
        Mix_HaltMusic();
    }
}

bool Menu::confirmExit() {
    SDL_Event e;
    bool choosing = true;
    int selected = 0; // 0 = Không, 1 = Đúng

    while (choosing) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return true;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                        selected = 1 - selected;
                        break;
                    case SDLK_RETURN:
                        return selected == 1;
                    case SDLK_ESCAPE:
                        return false;
                }
            }
        }
    
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    
        SDL_Texture* message = renderText("Bạn thực sự muốn thoát game?", {255, 255, 255, 255});
        SDL_Rect messageRect = {200, 200, 400, 50};
        SDL_RenderCopy(renderer, message, NULL, &messageRect);
        SDL_DestroyTexture(message);
    
        SDL_Texture* yesText = renderText(selected == 1 ? "> Đúng <" : "Đúng", {255, 255, 255, 255});
        SDL_Texture* noText = renderText(selected == 0 ? "> Không <" : "Không", {255, 255, 255, 255});
        SDL_Rect yesRect = {250, 300, 100, 40};
        SDL_Rect noRect = {450, 300, 100, 40};
        SDL_RenderCopy(renderer, yesText, NULL, &yesRect);
        SDL_RenderCopy(renderer, noText, NULL, &noRect);
        SDL_DestroyTexture(yesText);
        SDL_DestroyTexture(noText);
    
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return false;
}

SDL_Texture* Menu::renderText(const std::string& text, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void Menu::renderMenu() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Vẽ ảnh nền nếu có
    if (backgroundTexture) {
        SDL_Rect destRect = {0, 0, 800, 600};
        SDL_RenderCopy(renderer, backgroundTexture, NULL, &destRect);
    }

    // Cập nhật hiệu ứng nhấp nháy
    blinkTimer++;
    if (blinkTimer >= 30) {
        blinkTimer = 0;
        blinkState = !blinkState;
    }
    
    SDL_Rect highlightRect;
    for (size_t i = 0; i < options.size(); i++) {
        SDL_Color color = (i == selectedOption) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
        SDL_Texture* texture = renderText(options[i], {255, 255, 255, 255});
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect rect = { (800 - w) / 2, 200 + (int)i * 60, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
        if (i == selectedOption) {
            highlightRect = { rect.x - 10, rect.y - 5, rect.w + 20, rect.h + 10 };
        }
    }
    if (blinkState) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderDrawRect(renderer, &highlightRect);
    }
    SDL_RenderPresent(renderer);
}

std::string Menu::getChosenSaveFile() {
    return chosenSaveFile;
}

void Menu::showConfirmationScreen(const std::string& message) {
    SDL_Event e;
    bool done = false;
    while (!done) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Texture* msgTexture = renderText(message, {255, 255, 255, 255});
        SDL_Rect msgRect = {50, 250, 700, 50};
        SDL_RenderCopy(renderer, msgTexture, NULL, &msgRect);
        SDL_DestroyTexture(msgTexture);

        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_KEYDOWN || e.type == SDL_QUIT) {
                done = true;
                break;
            }
        }
        SDL_Delay(16);
    }
}

int Menu::handleNewGame() {
    // 1) Gọi promptForSaveName
    std::string saveFileName = promptForSaveName(renderer, font);
    if (!saveFileName.empty()) {
        // 2) Hỏi xác nhận
        bool confirm = confirmSaveFile(saveFileName);
        if (!confirm) {
            // Người chơi chọn Không => về lại menu
            return -1; 
        }
        // Nếu confirm = true => tạo file
        std::string fullPath = "Save/" + saveFileName + ".txt";
        Maze newMaze(true);
        newMaze.saveMaze(fullPath);
        chosenSaveFile = fullPath;
        showConfirmationScreen("File save \"" + saveFileName + ".txt\" da duoc tao thanh cong!");
        return 10; 
    }
    return -1;
}

bool Menu::confirmSaveFile(const std::string& fileName) {
    SDL_Event e;
    bool choosing = true;
    int selected = 0; // 0 = Không, 1 = Có

    while (choosing) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                // Người chơi tắt game
                return false; 
            }
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                    case SDLK_RIGHT:
                    case SDLK_UP:
                    case SDLK_DOWN:
                        selected = 1 - selected; // Chuyển giữa Có (1) và Không (0)
                        break;
                    case SDLK_RETURN:
                        // Nhấn Enter => xác nhận lựa chọn
                        return (selected == 1); // Nếu =1 => Có => true, ngược lại false
                    case SDLK_ESCAPE:
                        return false; // Thoát => coi như Không
                }
            }
        }

        // Vẽ hộp thoại xác nhận
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Ví dụ: hiển thị nội dung: "Bạn đã xác nhận FileSave của bạn tên: <fileName>?"
        std::string message = "FileSave: " + fileName + "\nCo xac nhan khong?";
        // Hoặc tách ra 2 dòng, tuỳ ý

        SDL_Texture* msgTexture = renderText(message, {255, 255, 255, 255});
        // Lấy kích thước text
        int w, h;
        SDL_QueryTexture(msgTexture, NULL, NULL, &w, &h);
        SDL_Rect msgRect = { (800 - w)/2, 200, w, h };
        SDL_RenderCopy(renderer, msgTexture, NULL, &msgRect);
        SDL_DestroyTexture(msgTexture);

        // Hiển thị 2 lựa chọn: Có / Không
        std::string yesOption = (selected == 1) ? "> Co <" : "Co";
        std::string noOption  = (selected == 0) ? "> Khong <" : "Khong";

        // Vẽ "Có"
        SDL_Texture* yesTexture = renderText(yesOption, {255, 255, 255, 255});
        SDL_QueryTexture(yesTexture, NULL, NULL, &w, &h);
        SDL_Rect yesRect = {200, 300, w, h}; 
        SDL_RenderCopy(renderer, yesTexture, NULL, &yesRect);
        SDL_DestroyTexture(yesTexture);

        // Vẽ "Không"
        SDL_Texture* noTexture = renderText(noOption, {255, 255, 255, 255});
        SDL_QueryTexture(noTexture, NULL, NULL, &w, &h);
        SDL_Rect noRect = {400, 300, w, h}; 
        SDL_RenderCopy(renderer, noTexture, NULL, &noRect);
        SDL_DestroyTexture(noTexture);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return false;
}

int Menu::handleLoadGame() {
    std::string saveFileName = chooseSaveFile();
    if (!saveFileName.empty()) {
        chosenSaveFile = "Save/" + saveFileName;
        showConfirmationScreen("Load thanh cong! Vao game ngay lap tuc!");
        return 20;  // Trả về giá trị để vào game ngay
    }
    return -1;
}
