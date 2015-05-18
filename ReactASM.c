/*
 * ReactASM: A shitty game by Melody Kelly for GRUE
 * MIT Licence look it up.
*/

#include <stdbool.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <uzebox.h>
#include "data/ReactASM.inc"

// Time allowed to press key
#define TIME    10
#define LEFT    0
#define RIGHT   1

// Screen Defines
#define HZ          60
#define SCRN_HGT    (SCREEN_TILES_H * TILE_WIDTH)
#define SCRN_WID    (SCREEN_TILES_V * TILE_HEIGHT)
#define TILE_WID2   (TILE_WIDTH / 2)
#define TILE_HGT2   (TILE_HEIGHT / 2)

// Macros
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))
#define ABS(x) (((x) > 0) ? (x) : -(x))

// GUI Elements
#define PRESS_START_LOC_X   9
#define PRESS_START_LOC_Y   13

// Tiles
#define CLEAR_TILE                      0
#define MAP_TOP_LEFT_CORNER_BUTTON      1
#define MAP_TOP_MIDDLE_BUTTON           2
#define MAP_TOP_RIGHT_CORNER_BUTTON     3
#define MAP_BOTTOM_LEFT_CORNER_BUTTON   4
#define MAP_BOTTOM_MIDDLE_BUTTON        5
#define MAP_BOTTOM_RIGHT_CORNER_BUTTON  6
#define MAP_LEFT_SIDE_BUTTON            7
#define MAP_RIGHT_SIDE_BUTTON           8
#define MAP_MIDDLE_BUTTON               9
#define MAP_PRESS_START                 10

void FillRegion(u8 x, u8 y, u8 width, u8 height, u8 tile);
void DrawButton(u8 x, u8 y, u8 width, u8 height);
void FlashPressStart(void);

// Each significant game entity has a FSM to simplify and centralise game logic
typedef enum {
    title, playing, dead, paused, gameOver
} GameState;

u8 prng; // Pseudo-random number generator
GameState gameState;
u32 hiScore;

/**
 *
 * Main Loop
 *
*/

int main() {
    SetTileTable(RASMTiles);
    ClearVram();
    gameState = title;

    u16 btnPrev = 0;        // Previous buttons that were held
    u16 btnHeld = 0;        // Buttons that are held right now
    u16 btnPressed = 0;     // Buttons that were pressed this frame
    u16 btnReleased = 0;    // Buttons that were released this frame
    u8 round = 0,
       flashStartTimer = 0,
       playerDeadTimer = 0,
       gameOverTimer = 0;

    // Init
    gameState = title;
    hiScore = 0;
    flashStartTimer = 1;

    while(1) {

        // This forces the game to run at 60FPS
        if (GetVsyncFlag()) {
            ClearVsyncFlag();
            btnHeld = ReadJoypad(0);
            btnPressed = btnHeld&(btnHeld^btnPrev);
            btnReleased = btnPrev&(btnHeld^btnPrev);
            btnPrev = btnHeld;

            switch (gameState) {
            case title:
                // Start the game if START is pressed
                if (btnPressed&BTN_START) {
                    //InitRound(round = 0);
                    ClearVram();
                    gameState = playing;
                } else {
                    if (--flashStartTimer == 0) {
                        FlashPressStart();
                        flashStartTimer = HZ>>1;
                    }
                }
                // Increment the pseudo RNG
                ++prng;
                prng = MAX(prng, 1);
                break;
            case playing:
                // Game Logic Goes Here
                DrawButton(1, 1, 28, 26);
                SetTile(0, 0, MAP_0);
                break;
            case dead:
                // Ignore pause game while dead
                // Dead Logic Goes Here
                break;
            case paused:
                if (btnPressed&BTN_START) {
                    gameState = playing;
                    break;
                }
                break;
            case gameOver:
                // Game Over Logic Goes Here
                break;
            }
        }
    }
}

/**
 *
 * Animates the "Press Start" message.
 *
*/
void FlashPressStart(void) {
    static u8 flashState = 1;

    if (flashState) {
        DrawMap2(PRESS_START_LOC_X, PRESS_START_LOC_Y, map_press_start);
    } else {
        FillRegion(PRESS_START_LOC_X, PRESS_START_LOC_Y, 11, 1, CLEAR_TILE);
    }
    flashState = !flashState;
}


/**
 *
 * Fills a region with the tile specified (tile-based coordinates).
 *
 * @param x the x-axis position to begin the fill.
 * @param y the y-axis position to begin the fill.
 * @param width how far along the x-axis from x to fill.
 * @param height how far along the y-axis from y to fill.
 * @param the tile to be used for the fill.
 *
*/
void FillRegion(u8 x, u8 y, u8 width, u8 height, u8 tile) {
    for (u8 i = y; i < (y + height); i++) {
        for (u8 j = x; j < (x + width); j++) {
            SetTile(j, i, tile);
        }
    }
}

/**
 *
 * Draws a button on the screen.
 *
 * @param x the x-axis position to begin the button.
 * @param y the y-axis position to begin the button.
 * @param width sub one of how far along the x-axis from x to fill.
 * @param height sub one of how far along the y-axis from y to fill.
 * @param the tile to be used for the fill.
 *
*/
void DrawButton(u8 x, u8 y, u8 width, u8 height) {
    // Fills Box
    FillRegion(x + 1, y + 1, width - 2, height - 2, MAP_MIDDLE_BUTTON);
    SetTile(x, y,                          MAP_TOP_LEFT_CORNER_BUTTON);
    SetTile(x + width - 1, y,              MAP_TOP_RIGHT_CORNER_BUTTON);
    SetTile(x + width - 1, y + height - 1, MAP_BOTTOM_RIGHT_CORNER_BUTTON);
    SetTile(x, y + height - 1,             MAP_BOTTOM_LEFT_CORNER_BUTTON);
    for (u8 i = x + 1; i < x + width - 1; i++) {
        SetTile(i, y, MAP_TOP_MIDDLE_BUTTON);
    }
    for (u8 i = x + 1; i < x + width - 1; i++) {
        SetTile(i, y + height - 1, MAP_BOTTOM_MIDDLE_BUTTON);
    }

    for (u8 i = y + 1; i < y + height - 1; i++) {
        SetTile(x, i, MAP_LEFT_SIDE_BUTTON);
    }
    for (u8 i = y + 1; i < y + height - 1; i++) {
        SetTile(x + width - 1, i, MAP_RIGHT_SIDE_BUTTON);
    }
}
