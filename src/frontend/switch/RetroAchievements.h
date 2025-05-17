#pragma once
#include <string>
#include <atomic>
#include <vector>

struct Achievement {
    std::string title;
    std::string description;
    std::string progress;
    int textureId = -1;
    int width = 0;
    int height = 0;
};

extern std::atomic<bool> g_loadingAchievements;
extern std::vector<Achievement> g_achievements;

int InitRetroAchievements(const char* username, const char* password);

void load_game_from_file(const char* path);

void rc_client_process_ra();

bool isConnected();