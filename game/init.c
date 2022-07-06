#include<stdint.h>

#include "main.h"
#include "math.h"

extern PLAYER g_MainPlayer;
extern ENEMY g_Enemies[20];
extern RECTANGLE g_PlayableArea;

PIXEL InitializePixel(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    PIXEL pixel = { 0 };

    pixel.color.red = red;
    pixel.color.green = green;
    pixel.color.blue = blue;
    pixel.alpha = alpha;

    return pixel;
}

void InitializeMainPlayer(void) {
    COLOR playerColor = { .red = 0xFF, .green = 0x1F, .blue = 0x1F };
    g_MainPlayer.color = playerColor;
    g_MainPlayer.rect.x = GAME_WIDTH / 2;
    g_MainPlayer.rect.y = 75;
    g_MainPlayer.speedX = 2;
    g_MainPlayer.speedY = 2;
    g_MainPlayer.rect.width = 16;
    g_MainPlayer.rect.height = 16;
}

void InitializeEnemies(void) {
    COLOR enemyColor = { .red = 0x13, .green = 0x16, .blue = 0xff };

    float speedScale = 0.0f;
    float spawnRegionWidth = (g_PlayableArea.width - g_PlayableArea.x) * 0.8f;
    float spawnRegionHeight = GAME_HEIGHT * 0.2f;
    float spawnRegionPositionX = (g_PlayableArea.x + GAME_WIDTH - spawnRegionWidth) / 2;
    float spawnRegionPositionY = GAME_HEIGHT - 110;

    for (uint32_t i = 0; i < ENEMY_COUNT; i++)
    {
        g_Enemies[i].color = enemyColor;
        g_Enemies[i].rect.x = (float)RandomUInt32InRange((uint32_t)spawnRegionPositionX, (uint32_t)(spawnRegionPositionX + spawnRegionWidth));
        g_Enemies[i].rect.y = (float)RandomUInt32InRange((uint32_t)spawnRegionPositionY, (uint32_t)(spawnRegionPositionY + spawnRegionHeight));
        g_Enemies[i].rect.width = 16;
        g_Enemies[i].rect.height = 16;
        g_Enemies[i].speedX = RandomUInt32InRange(2, 5) * speedScale * RandomSign();
        g_Enemies[i].speedY = RandomUInt32InRange(2, 5) * speedScale * RandomSign();
    }
}