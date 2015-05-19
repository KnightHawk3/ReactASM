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

#define TIME    250 // Time to press the key in miliseconds

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
#define PRESS_START_LOC_X               9
#define PRESS_START_LOC_Y               13

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
#define MAP_UP_ARROW                    10
#define MAP_RIGHT_ARROW                 12
#define MAP_DOWN_ARROW                  13
#define MAP_LEFT_ARROW                  14

void FillRegion(u8 x, u8 y, u8 width, u8 height, u8 tile);
void DrawButton(u8 x, u8 y, u8 width, u8 height);
void FlashPressStart(void);
uint16_t prng(void);
void getNewKey(void);

// Each significant game entity has a FSM to simplify and centralise game logic
typedef enum {
    title, countIn, playing, dead, paused, gameOver
} GameState;

// The keys are actually ints!
typedef enum {
    up, right, down, left
} KeyCode;

uint16_t lfsr = 0xBEEF;  /* Any number goes here */
GameState gameState;
KeyCode keyCode;
u16 hiScore;
u16 timer = 0;

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
    u8 flashStartTimer = 0; // "Do we flash the button"

    // Init
    gameState = title;
    hiScore = 0;
    flashStartTimer = 1;

    // Main game loop, not supposed to ever end.
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
                    gameState = countIn;
                } else {
                    if (--flashStartTimer == 0) {
                        FlashPressStart();
                        flashStartTimer = HZ>>1;
                    }
                }
                keyCode = prng() % 4;
                break;

            case countIn:
                if (timer % 33 == 0) {
                    ClearVram();
                } else {
                    DrawButton(28/2 - 5, 26/2 - 5, 10, 10);
                    if (timer <= 60) {
                        DrawMap2(28/2 - 1, 26/2 - 1, map_three);
                    } else if (timer <= 120) {
                        DrawMap2(28/2 - 1, 26/2 - 1, map_two);
                    } else if (timer < 180) {
                        DrawMap2(28/2 - 1, 26/2 - 1, map_one);
                    } else {
                        gameState = playing;
                        timer = 0;
                    }
                }
                timer++;
                break;

            case playing:
                // Game Logic Goes Here
                DrawButton(1, 1, 28, 26);
                if (btnPressed&BTN_START) {
                    gameState = paused;
                }
                if (timer++ > 50) {
                    gameState = gameOver;
                }
                // Draw the right key and see if its pressed.
                switch(keyCode) {
                    case up:
                        DrawMap2(28/2 - 2, 26/2, map_up_arrow);
                        if (btnPressed&BTN_UP) {
                            getNewKey();
                            timer = 0;
                        }
                        break;
                    case down:
                        DrawMap2(28/2 - 1, 26/2, map_down_arrow);
                        if (btnPressed&BTN_DOWN) {
                            getNewKey();
                            timer = 0;
                        }
                        break;
                    case left:
                        DrawMap2(28/2, 26/2, map_left_arrow);
                        if (btnPressed&BTN_LEFT) {
                            getNewKey();
                            timer = 0;
                        }
                        break;
                    case right:
                        DrawMap2(28/2 + 1, 26/2, map_right_arrow);
                        if (btnPressed&BTN_RIGHT) {
                            getNewKey();
                            timer = 0;
                        }
                        break;
                }
                break;

            case dead:
                // Ignore pause game while dead
                // Dead Logic Goes Here
                ClearVram();
                // YOUR DEAD SUCKER
                break;

            case paused:
                if (btnPressed&BTN_START) {
                    gameState = playing;
                    ClearVram();
                    break;
                }
                // TODO: PAUSED
                //DrawMap2();
                break;

            case gameOver:
                // Game Over Logic Goes Here
                ClearVram();
                //DrawMap2(SCREEN_TILES_H/2 - (9/2), SCREEN_TILES_V/2, map_game_over);
                if (btnPressed&BTN_START) {
                    //InitRound(round = 0);
                    ClearVram();
                    gameState = countIn;
                } else {
                    if (--flashStartTimer == 0) {
                        FlashPressStart();
                        flashStartTimer = HZ>>1;
                    }
                }
                keyCode = prng() % 4;
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
 * @param width of the button in tiles.
 * @param height of the button in tiles
 *
*/
void DrawButton(u8 x, u8 y, u8 width, u8 height) {
    // Fills Box
    FillRegion(x + 1, y + 1, width - 2, height - 2, MAP_MIDDLE_BUTTON);
    SetTile(x, y, MAP_TOP_LEFT_CORNER_BUTTON);
    SetTile(x + width - 1, y, MAP_TOP_RIGHT_CORNER_BUTTON);
    SetTile(x + width - 1, y + height - 1, MAP_BOTTOM_RIGHT_CORNER_BUTTON);
    SetTile(x, y + height - 1, MAP_BOTTOM_LEFT_CORNER_BUTTON);
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

/*
 *
 * Returns a random unsigned 16 bit int.
 * I really have no idea what kind of number it returns. It's probably u16.
 *
*/
uint16_t prng(void) {
    /* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    return lfsr;
}

/*
 *
 * Sets keyCode to a new random key
 *
*/
void getNewKey(void) {
    u8 oldKey = keyCode;
    // This possibly isn't entirely spread evenly so we do it until it seems so.
    keyCode = prng() % 4;
    while (keyCode == oldKey){
        keyCode = prng() % 4; // Not sure if this actually creates a good distro
    }
}
