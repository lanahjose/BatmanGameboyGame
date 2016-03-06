#include <stdio.h>
#include <stdlib.h>
#include "myLib.h"
#include "text.h"

//State screens
#include "Splash.h"
#include "Lose.h"
#include "Win.h"
#include "Pause.h"
#include "Instructions.h"

//Music and Sounds
#include "BGMusic.h"
#include "GameMusic.h"
#include "Punch.h"
#include "Batarang.h"
#include "GruntSFX.h"
#include "BatmanGrunt.h"
#include "JokerLaugh.h"
#include "JokerLaugh2.h"
#include "GunShot.h"
#include "LaughGoodTimes.h"
#include "Cheat.h"
#include "Explosion.h"
#include "JokerOw.h"
#include "WinSong.h"

//game backgrounds and collision maps
#include "HUD.h"
#include "HUD2.h"
#include "BG0a.h"
#include "BG0aCM.h"
#include "BG0b.h"
#include "BG0bCM.h"
#include "BG2.h"
#include "BG2CM.h"

//Sprites
#include "Sprites0.h"
#include "Sprites1.h"



int hOff=0;
int vOff=0;

OBJ_ATTR shadowOAM[128];


#define ROWMASK 0xFF
#define COLMASK 0x1FF



#define MAXEXPLOSIONS 3

typedef struct explosion {
    int row;
    int col;
    int bigRow;
    int bigCol;
    int exploding;
} EXPLOSION;

typedef struct explosive {
    int row;
    int col;
    int width;
    int height;
    int currFrame;
    int active;
    int bigRow;
    int bigCol;
    int exploding;
    EXPLOSION explosions[MAXEXPLOSIONS];
} EXPLOSIVE;

typedef struct bullet
{
    int row;
    int col;
    int rdel;
    int cdel;
    int width;
    int height;
    int aniCounter;
    int aniState;
    int prevAniState;
    int currFrame;
    int speed;
    int direction;
    int bigRow;
    int bigCol;
    int active;
} BULLET;

typedef struct  
{
    int row;
    int col;
    int rdel;
    int cdel;
    int width;
    int height;
    int aniCounter;
    int aniState;
    int prevAniState;
    int currFrame;
    int speed;
    int direction;
    int health;
    int MAX_HEALTH;
    int bigRow;
    int bigCol;
    int activeBullets;
    int MAX_BULLETS;
    int range;
    int bulletCounter;
    int startPosR;
    int startPosC;
    BULLET bullets[3];
    BULLET batarang;
} MOVOBJ;

MOVOBJ batman;
int HealthWidth = 5;

int jokerTransition = 0;
int jokerPrevHealth = 3;
int MAXBULLETS = 2;
int NUMOFENEMIES = 3;
int ACTIVEENEMIES;
MOVOBJ enemies[10];

#define MAXEXPLOSIVES 3

EXPLOSIVE explosives[MAXEXPLOSIVES];
enum { STARTSCREEN, INSTRUCTIONSCREEN, GAMESCREEN, LOSESCREEN, WINSCREEN, PAUSESCREEN};
enum { BATIDLE, BATRUN, BATATTACK, BATARANG, BATSTUNNED};
enum { ACTIVE, HURT, STUNNED, DEAD, EXPLOSIONSTUNNED};
enum { SPRITERIGHT, SPRITELEFT};
enum {STAGE0, STAGE1, STAGE2};


unsigned int buttons;
unsigned int oldButtons;

int state = STARTSCREEN;
int stage = STAGE0;
#define MAXSTAGE 2
char buffer[41];

int cheat = 0;


void game();
void splash();
void instructions();
void pause();
void win();
void lose();
void animate();
void updateOAM();
void checkCollision();

typedef struct{
    const unsigned char* data;
    int length;
    int frequency;
    int isPlaying;
    int loops;
    int duration;
    int priority;
} SOUND;

SOUND soundA;
SOUND soundB;
int vbCountA;
int vbCountB;

void setupSounds();
void playSoundA( const unsigned char* sound, int length, int frequency);
void playSoundB( const unsigned char* sound, int length, int frequency);
void muteSound();
void unmuteSound();
void stopSound();

void setupInterrupts();
void interruptHandler();

void enemyShoot(int enemy);
void punchCollision();
void batarang();
void updateBatarang();
void batarangCollision(int enemy);
void updateEnemies();
void bulletCollision(int enemy, int bullet);
void drawBatman();
void drawEnemies();
void explosiveCollision();
void updateExplosives();
void explosionHit(int i);
void drawExplosives();

int main() {
    REG_DISPCTL = MODE3 | BG2_ENABLE;

    setupInterrupts();
    setupSounds();

    drawBackgroundImage3(SplashBitmap);

    playSoundA(BGMusic, BGMUSICLEN, BGMUSICFREQ);
    
    for(;;)
	{
		oldButtons = buttons;
		buttons = BUTTONS;

		switch(state)
		{
			case STARTSCREEN:
				splash();
				break;
            case INSTRUCTIONSCREEN:
                instructions();
                break;
			case GAMESCREEN:
				game();
				break;
			case PAUSESCREEN:
				pause();
				break;
			case WINSCREEN:
				win();
				break;
			case LOSESCREEN:
				lose();
				break;
		} 
	}
	return 0;
    
}

void setupSounds()
{
    REG_SOUNDCNT_X = SND_ENABLED;

    REG_SOUNDCNT_H = SND_OUTPUT_RATIO_100 | 
                        DSA_OUTPUT_RATIO_100 | 
                        DSA_OUTPUT_TO_BOTH | 
                        DSA_TIMER0 | 
                        DSA_FIFO_RESET |
                        DSB_OUTPUT_RATIO_100 | 
                        DSB_OUTPUT_TO_BOTH | 
                        DSB_TIMER1 | 
                        DSB_FIFO_RESET;

    REG_SOUNDCNT_L = 0;
}

void playSoundA( const unsigned char* sound, int length, int frequency) {
        dma[1].cnt = 0;
        vbCountA = 0;
    
        int interval = 16777216/frequency;
    
        DMANow(1, sound, REG_FIFO_A, DMA_DESTINATION_FIXED | DMA_AT_REFRESH | DMA_REPEAT | DMA_32);
    
        REG_TM0CNT = 0;
    
        REG_TM0D = -interval;
        REG_TM0CNT = TIMER_ON;


        int isLooping = 1;

        
        soundA.data = sound;
        soundA.length = length;
        soundA.frequency = frequency;
        soundA.isPlaying = 1;
        soundA.loops = isLooping;
        soundA.duration = ((60*length)/frequency) - ((length/frequency)*3)-1;
        soundA.priority = 1;
}


void playSoundB( const unsigned char* sound, int length, int frequency) {

        dma[2].cnt = 0;
        vbCountB = 0;

        int interval = 16777216/frequency;

        DMANow(2, sound, REG_FIFO_B, DMA_DESTINATION_FIXED | DMA_AT_REFRESH | DMA_REPEAT | DMA_32);

        REG_TM1CNT = 0;
    
        REG_TM1D = -interval;
        REG_TM1CNT = TIMER_ON;

        int isLooping = 0;

        soundB.data = sound;
        soundB.length = length;
        soundB.frequency = frequency;
        soundB.isPlaying = 1;
        soundB.loops = isLooping;
        soundB.duration = ((60*length)/frequency) - ((length/frequency)*3)-1;
        soundB.priority = 1;

}

void muteSound() {
    REG_SOUNDCNT_X = REG_SOUNDCNT_X & ~SND_ENABLED;

}

void unmuteSound() {
    REG_SOUNDCNT_X = REG_SOUNDCNT_X | SND_ENABLED;

}

void pauseSound(){
    soundA.isPlaying = 0;
    soundB.isPlaying = 0;
    REG_TM1CNT = REG_TM1CNT & ~TIMER_ON;
    REG_TM0CNT = REG_TM0CNT & ~TIMER_ON;

}

void unpauseSound() {
    soundA.isPlaying = 1;
    soundB.isPlaying = 1;
    REG_TM1CNT = REG_TM1CNT | TIMER_ON;
    REG_TM0CNT = REG_TM0CNT | TIMER_ON;

}

void stopSound() {
    soundA.isPlaying = 0;
    REG_TM0CNT = 0;
    soundB.isPlaying = 0;
    REG_TM1CNT = 0;
}

void setupInterrupts() {
    REG_IME = 0;
    REG_INTERRUPT = (unsigned int)interruptHandler;
    REG_IE |= INT_VBLANK;
    REG_DISPSTAT |= INT_VBLANK_ENABLE;
    REG_IME = 1;
}

void interruptHandler() {
    REG_IME = 0;
    if(REG_IF & INT_VBLANK) {
        vbCountA++;
        vbCountB++;

        if((vbCountA > soundA.duration) && soundA.isPlaying) {
            if(soundA.loops) {
                playSoundA(soundA.data, soundA.length, soundA.frequency);
            } else {
                soundA.isPlaying = 0;
                REG_TM0CNT = 0;
            }
        }
        if((vbCountB > soundB.duration) && soundB.isPlaying) {
            if(soundB.loops) {
                playSoundB(soundB.data, soundB.length, soundB.frequency);
            } else {
                soundB.isPlaying = 0;
                REG_TM1CNT = 0;
            }
        }
        REG_IF = INT_VBLANK; 
    }
    REG_IME = 1;
}

void win() {

    while(1){
        oldButtons = buttons;
		buttons = BUTTONS;

        if(BUTTON_PRESSED(BUTTON_START)){
            state = STARTSCREEN;
            stage = STAGE0;
            drawBackgroundImage3(SplashBitmap);
            stopSound();
            playSoundA(BGMusic, BGMUSICLEN, BGMUSICFREQ);
            return;
        }
    }
}

void lose() {

    while(1){
        oldButtons = buttons;
		buttons = BUTTONS;

        if(BUTTON_PRESSED(BUTTON_START)) {
            
            state = GAMESCREEN;
            cheat = 0;
            stopSound();
            playSoundA(GameMusic, GAMEMUSICLEN, GAMEMUSICFREQ);

            REG_DISPCTL = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE;
            
            REG_BG1CNT = CBB(1) | SBB(27) | BG_SIZE0 | COLOR256 | 1;
            REG_BG0CNT = CBB(0) | SBB(28) | BG_SIZE1 | COLOR256 | 2;

            DMANow(3, HUDTiles, &CHARBLOCKBASE[1], HUDTilesLen);
            DMANow(3, HUDMap, &SCREENBLOCKBASE[27], HUDMapLen);

            if(stage == STAGE0) {
                loadPalette(BG0aPal);  
                DMANow(3, BG0aTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0aTilesLen/2);
                DMANow(3, BG0aMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0aMapLen/2); 
                
                loadSpritePalette(Sprites0Pal);
                DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);
            }else if (stage == STAGE1) {
                loadPalette(BG0bPal);
                DMANow(3, BG0bTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0bTilesLen/2);
                DMANow(3, BG0bMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0bMapLen/2);

                loadSpritePalette(Sprites0Pal);
                DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);
                
            } else if(stage == STAGE2) {
                loadPalette(BG2Pal);
                DMANow(3, BG2Tiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG2TilesLen/2);
                DMANow(3, BG2Map, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG2MapLen/2);

                loadSpritePalette(Sprites1Pal);
                DMANow(3, Sprites1Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites1TilesLen/2); 
            }
            
            hideSprites();
            initialize();
            return;
        }

        if(BUTTON_PRESSED(BUTTON_SELECT)){

            state = STARTSCREEN;
            stage = STAGE0;
            drawBackgroundImage3(SplashBitmap);
            stopSound();
            playSoundA(BGMusic, BGMUSICLEN, BGMUSICFREQ);
            return;
        }
    }
}

void pause() {   

    while(1){
        oldButtons = buttons;
		buttons = BUTTONS;

        if(BUTTON_PRESSED(BUTTON_START)){
            state = GAMESCREEN;

            REG_DISPCTL = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE;
            
            REG_BG1CNT = CBB(1) | SBB(27) | BG_SIZE0 | COLOR256 | 1;
            REG_BG0CNT = CBB(0) | SBB(28) | BG_SIZE1 | COLOR256 | 2;

            if(cheat) {
                DMANow(3, HUD2Tiles, &CHARBLOCKBASE[1], HUD2TilesLen);
                DMANow(3, HUD2Map, &SCREENBLOCKBASE[27], HUD2MapLen);
            } else {
                DMANow(3, HUDTiles, &CHARBLOCKBASE[1], HUDTilesLen);
                DMANow(3, HUDMap, &SCREENBLOCKBASE[27], HUDMapLen);
            }

            if(stage == STAGE0) {
                loadPalette(BG0aPal);  

                DMANow(3, BG0aTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0aTilesLen/2);
                DMANow(3, BG0aMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0aMapLen/2); 
                
                loadSpritePalette(Sprites0Pal);
                DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);
            }else if (stage == STAGE1) {
                loadSpritePalette(Sprites0Pal);
                DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);
                loadPalette(BG0bPal);


                DMANow(3, BG0bTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0bTilesLen/2);
                DMANow(3, BG0bMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0bMapLen/2);
            } else if(stage == STAGE2) {
                loadSpritePalette(Sprites1Pal);
                DMANow(3, Sprites1Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites1TilesLen/2);
                loadPalette(BG2Pal);

                DMANow(3, BG2Tiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG2TilesLen/2);
                DMANow(3, BG2Map, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG2MapLen/2);
            }

            hideSprites();
            unpauseSound();
            return;
        }
        if(BUTTON_PRESSED(BUTTON_SELECT)){
            state = STARTSCREEN;
            stage = STAGE0;
            stopSound();
            drawBackgroundImage3(SplashBitmap);
            playSoundA(BGMusic, BGMUSICLEN, BGMUSICFREQ);
            return;
        }
    }
}

void splash(){
    while(1){
        oldButtons = buttons;
		buttons = BUTTONS;

        if(BUTTON_PRESSED(BUTTON_START)){
            
            state = GAMESCREEN;
            stage = STAGE0;
            cheat = 0;
            
            stopSound();
            playSoundA(GameMusic, GAMEMUSICLEN, GAMEMUSICFREQ);

            REG_DISPCTL = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE;
            
            REG_BG1CNT = CBB(1) | SBB(27) | BG_SIZE0 | COLOR256 | 1;
            REG_BG0CNT = CBB(0) | SBB(28) | BG_SIZE1 | COLOR256 | 2;

            loadPalette(BG0aPal);  

            DMANow(3, HUDTiles, &CHARBLOCKBASE[1], HUDTilesLen);
            DMANow(3, HUDMap, &SCREENBLOCKBASE[27], HUDMapLen);

            DMANow(3, BG0aTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0aTilesLen/2);
            DMANow(3, BG0aMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0aMapLen/2); 
  
            loadSpritePalette(Sprites0Pal);
            DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);

            hideSprites();
            initialize();

            return;
        }

        if(BUTTON_PRESSED(BUTTON_SELECT)){
            state = INSTRUCTIONSCREEN;
            drawBackgroundImage3(InstructionsBitmap);

            return;
        }
    }
}

void instructions(){
    while(1){
        oldButtons = buttons;
        buttons = BUTTONS;

        if(BUTTON_PRESSED(BUTTON_START)){
            
            state = GAMESCREEN;
            cheat = 0;

            stopSound();
            playSoundA(GameMusic, GAMEMUSICLEN, GAMEMUSICFREQ);

            REG_DISPCTL = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE;
            
            REG_BG1CNT = CBB(1) | SBB(27) | BG_SIZE0 | COLOR256 | 1;
            REG_BG0CNT = CBB(0) | SBB(28) | BG_SIZE1 | COLOR256 | 2;

            loadPalette(BG0aPal);  
            
            DMANow(3, HUDTiles, &CHARBLOCKBASE[1], HUDTilesLen);
            DMANow(3, HUDMap, &SCREENBLOCKBASE[27], HUDMapLen);

            DMANow(3, BG0aTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0aTilesLen/2);
            DMANow(3, BG0aMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0aMapLen/2);  

            loadSpritePalette(Sprites0Pal);
            DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);

            hideSprites();
            initialize();

            return;
        }

        if(BUTTON_PRESSED(BUTTON_SELECT)){
            state = STARTSCREEN;
            //stopSound();
            drawBackgroundImage3(SplashBitmap);
            //playSoundA(BGMusic, BGMUSICLEN, BGMUSICFREQ);
            return;
        }

    }
}
void game(){
    while(1){
        oldButtons = buttons;
        buttons = BUTTONS;

        animate();
        updateOAM();

        REG_BG0HOFS = hOff;
        REG_BG0VOFS = vOff;

        DMANow(3, shadowOAM , OAM, 512);
        waitForVblank();

        if(BUTTON_PRESSED(BUTTON_START)){
            state = PAUSESCREEN;
            pauseSound();
            REG_DISPCTL = MODE3 | BG2_ENABLE;
            drawBackgroundImage3(PauseBitmap);
            return;    
        }

        if(BUTTON_PRESSED(BUTTON_SELECT)){
            cheat = cheat ^ 1;

            if(cheat) {
                DMANow(3, HUD2Tiles, &CHARBLOCKBASE[1], HUD2TilesLen);
                DMANow(3, HUD2Map, &SCREENBLOCKBASE[27], HUD2MapLen);
                playSoundB(Cheat, CHEATLEN, CHEATFREQ);
            } else {
                DMANow(3, HUDTiles, &CHARBLOCKBASE[1], HUDTilesLen);
                DMANow(3, HUDMap, &SCREENBLOCKBASE[27], HUDMapLen);
            }

            if(stage == STAGE0) {
                loadPalette(BG0aPal);  
                DMANow(3, BG0aTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0aTilesLen/2);
                DMANow(3, BG0aMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0aMapLen/2); 

            } else if (stage == STAGE1) {
                loadPalette(BG0bPal);
                DMANow(3, BG0bTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0bTilesLen/2);
                DMANow(3, BG0bMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0bMapLen/2);
    
            } else if(stage == STAGE2) {
                loadPalette(BG2Pal);
                DMANow(3, BG2Tiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG2TilesLen/2);
                DMANow(3, BG2Map, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG2MapLen/2);
            }
        }

        if(ACTIVEENEMIES == 0 && batman.bigCol >= 512){
            if(stage == MAXSTAGE) {
                state = WINSCREEN;
                stopSound();
                REG_DISPCTL = MODE3 | BG2_ENABLE;
                playSoundB(WinSong, WINSONGLEN, WINSONGFREQ);
                drawBackgroundImage3(WinBitmap);
                return;
            } else {
                stage++;
                if (stage == STAGE1) {
                    loadSpritePalette(Sprites0Pal);
                    DMANow(3, Sprites0Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites0TilesLen/2);
                    loadPalette(BG0bPal); 
                    DMANow(3, BG0bTiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG0bTilesLen/2);
                    DMANow(3, BG0bMap, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG0bMapLen/2);
                } else if(stage == STAGE2) {
                    loadSpritePalette(Sprites1Pal);
                    DMANow(3, Sprites1Tiles , &CHARBLOCKBASE[4], DMA_SOURCE_INCREMENT | Sprites1TilesLen/2);
                    loadPalette(BG2Pal);  
                    DMANow(3, BG2Tiles , &CHARBLOCKBASE[0], DMA_SOURCE_INCREMENT | BG2TilesLen/2);
                    DMANow(3, BG2Map, &SCREENBLOCKBASE[28], DMA_SOURCE_INCREMENT | BG2MapLen/2);
                }
                hideSprites();
                initialize();
            }
        }

        if(batman.health == 0) {
            state = LOSESCREEN;
            stopSound();
            REG_DISPCTL = MODE3 | BG2_ENABLE;
            playSoundB(LaughGoodTimes, LAUGHGOODTIMESLEN, LAUGHGOODTIMESFREQ);
            drawBackgroundImage3(LoseBitmap);
            return;
        }
    }    
}

void checkCollision(const short unsigned int* map) {
    if(BUTTON_HELD(BUTTON_UP)) {
        if(map[OFFSET(batman.bigRow-batman.speed, batman.bigCol + 1, 512)]
                && map[OFFSET(batman.bigRow-batman.speed, batman.bigCol + batman.width - 1, 512)]
                && map[OFFSET(batman.bigRow-batman.speed, batman.bigCol + batman.width/2, 512)]) {
            if ((batman.row > 0 && vOff == 0) || batman.row > 50) {
                batman.row -= batman.speed;
            } else if (vOff> 0){
                vOff -= batman.speed;
            }
        }
    }

    if(BUTTON_HELD(BUTTON_DOWN)) {
        if(map[OFFSET(batman.bigRow + batman.height + batman.speed, batman.bigCol + 1, 512)]
                && map[OFFSET(batman.bigRow + batman.height + batman.speed -8, batman.bigCol + batman.width - 1 , 512)]
                && map[OFFSET(batman.bigRow + batman.height + batman.speed, batman.bigCol + batman.width/2 , 512)]) {
            if ((batman.row < (160-batman.height) && vOff == 96) || batman.row < 50) {
                batman.row += batman.speed;
            } else if(vOff < 96) {
                vOff += batman.speed;
            }
        } 
    }

    if(BUTTON_HELD(BUTTON_LEFT)) {
        if(map[OFFSET(batman.bigRow + 1, batman.bigCol - batman.speed, 512)]
                && map[OFFSET(batman.bigRow + batman.height - 8, batman.bigCol - batman.speed, 512)]
                && map[OFFSET(batman.bigRow + batman.height/2, batman.bigCol - batman.speed, 512)]) {
            if ((batman.col > (0 + batman.speed) && hOff == 0) || batman.col > 60) {
                batman.col -= batman.speed;
            } else if(hOff > 0){
                hOff -= batman.speed;
            }
        }
    }

    if(BUTTON_HELD(BUTTON_RIGHT)) {
        if(map[OFFSET(batman.bigRow + 1, batman.bigCol + batman.width + batman.speed, 512)]
                && map[OFFSET(batman.bigRow + batman.height -8 , batman.bigCol + batman.width + batman.speed, 512)]
                && map[OFFSET(batman.bigRow + batman.height/2 , batman.bigCol + batman.width + batman.speed, 512)]) {
            if ((ACTIVEENEMIES == 0 && (batman.col < (240) && hOff == 272)) || batman.col < 60) {
                batman.col += batman.speed;
            } else if ((batman.col < (240 - batman.width + 8) && hOff == 272) || batman.col < 60) {
                batman.col += batman.speed;
            } else if(hOff < 272) {
                hOff += batman.speed;
            }
        }  
    }
}

void animate(){

        batman.bigRow = batman.row + vOff;
        batman.bigCol = batman.col + hOff;

        for(int i = 0; i < NUMOFENEMIES; i++) {
            enemies[i].row = enemies[i].bigRow - vOff;
            enemies[i].col = enemies[i].bigCol - hOff;
            for(int j = 0; j < enemies[i].MAX_BULLETS; j++) {
                if(enemies[i].bullets[j].active) {
                    enemies[i].bullets[j].row = enemies[i].bullets[j].bigRow - vOff;
                    enemies[i].bullets[j].col = enemies[i].bullets[j].bigCol - hOff;
                }
            }
        }

        if(batman.batarang.active) {
            batman.batarang.row = batman.batarang.bigRow - vOff;
            batman.batarang.col = batman.batarang.bigCol - hOff;
        }

        if(stage == STAGE2) {
            for(int i = 0; i < MAXEXPLOSIVES; i++) {
                if(explosives[i].active) {
                    explosives[i].row = explosives[i].bigRow - vOff;
                    explosives[i].col = explosives[i].bigCol - hOff;
                } else if(explosives[i].exploding) {
                    for(int j = 0; j < MAXEXPLOSIONS; j++) {
                        if(explosives[i].explosions[j].exploding) {
                            explosives[i].explosions[j].row = explosives[i].explosions[j].bigRow - vOff;
                            explosives[i].explosions[j].col = explosives[i].explosions[j].bigCol - hOff;
                        }
                        explosionHit(i);
                    }
                }
            }
        }

        if (batman.aniState != BATATTACK
            && batman.aniState != BATARANG
            && batman.aniState != BATSTUNNED) {
            batman.aniState = BATIDLE;
        }
   
        if(batman.aniCounter%10==0) 
        {
            if (batman.currFrame == 3) {
                batman.currFrame = 0;
                if (batman.aniState == BATATTACK || batman.aniState == BATARANG) {
                    batman.aniState = BATIDLE;
                }
            } else if(batman.currFrame == 1) {
                if(batman.aniState == BATATTACK) {
                    punchCollision();
                } else if(batman.aniState == BATARANG) {
                    batarang();
                } else if(batman.aniState == BATSTUNNED) {
                    batman.aniState = BATIDLE;
                }
                batman.currFrame++;
            } else {
                batman.currFrame++;
            }

            for(int i = 0; i < NUMOFENEMIES; i++) {
                if(enemies[i].currFrame == 3 && enemies[i].aniState != STUNNED && enemies[i].aniState != EXPLOSIONSTUNNED) {   
                    enemies[i].currFrame = 0;
                } else if(enemies[i].currFrame == 10 && enemies[i].aniState == STUNNED && stage != STAGE2) {
                    enemies[i].aniState = ACTIVE;
                    enemies[i].currFrame = 0;
                } else if(enemies[i].currFrame == 1 && ((enemies[i].aniState == HURT) || (stage == STAGE2 && enemies[i].aniState == STUNNED))) {
                    enemies[i].aniState = ACTIVE;
                    enemies[i].currFrame = 0;
                    if(enemies[i].health == 0) {
                        enemies[i].aniState = DEAD;
                        ACTIVEENEMIES--;
                    }
                } else if(enemies[i].currFrame == 25 && stage == STAGE2 && enemies[i].aniState == EXPLOSIONSTUNNED) {
                    enemies[i].aniState = ACTIVE;
                    enemies[i].currFrame = 0;
                } else {
                    enemies[i].currFrame++;
                }      
            }

            if(stage == STAGE2) {
                for(int i = 0; i < MAXEXPLOSIVES; i++) {
                    if(explosives[i].exploding) {
                        if(explosives[i].currFrame == 6) {
                            explosives[i].exploding = 0;
                            for(int j = 0; j < MAXEXPLOSIONS; j++) {
                                explosives[i].explosions[j].exploding = 0;
                            }
                            explosives[i].currFrame = 0;
                            explosives[i].bigRow = rand()%100 + 108 ;
                            explosives[i].bigCol = rand()%100 + 120*i + 100;
                            explosives[i].row = explosives[i].bigRow - vOff;
                            explosives[i].col = explosives[i].bigCol - hOff;
                        } else {
                            int curr = (explosives[i].currFrame-(explosives[i].currFrame & 1))/2;
                            explosives[i].explosions[curr].exploding = 1;
                            explosives[i].explosions[curr].bigRow = explosives[i].bigRow - 48 + rand()%50 ;
                            explosives[i].explosions[curr].bigCol = explosives[i].bigCol - 48 + curr*16;
                            explosives[i].explosions[curr].row = explosives[i].explosions[curr].bigRow - vOff;
                            explosives[i].explosions[curr].col = explosives[i].explosions[curr].bigCol - hOff;
                            explosives[i].currFrame++;
                        }
                    } else if(!explosives[i].active && explosives[i].currFrame == 30) {
                        if(enemies[0].health == jokerPrevHealth) {
                            explosives[i].active = 1;
                        } else {
                            jokerPrevHealth = enemies[0].health;
                            explosives[i].currFrame++;
                        }
                    } else if(!explosives[i].active) {
                        explosives[i].currFrame++;
                    }
                }
            }
        }

        if(BUTTON_PRESSED(BUTTON_A) && (batman.aniState != BATATTACK)){
            batman.aniState = BATATTACK;
            playSoundB(Punch, PUNCHLEN, PUNCHFREQ);
            batman.currFrame = 0;   
        } else if(BUTTON_PRESSED(BUTTON_B) && (batman.aniState != BATARANG)) {
            if(!batman.batarang.active) {
                batman.aniState = BATARANG;
                batman.currFrame = 0;
                playSoundB(Batarang, BATARANGLEN, BATARANGFREQ);
            }
        } else if (batman.aniState != BATATTACK && batman.aniState != BATARANG && batman.aniState != BATSTUNNED) {
            if(BUTTON_HELD(BUTTON_UP)) {
                batman.aniState = BATRUN; 
            } if(BUTTON_HELD(BUTTON_DOWN)) {
                batman.aniState = BATRUN;
            } if(BUTTON_HELD(BUTTON_LEFT)) {
                batman.aniState = BATRUN;
                batman.direction = SPRITELEFT; 
            } if(BUTTON_HELD(BUTTON_RIGHT)) {
                batman.aniState = BATRUN;
                batman.direction = SPRITERIGHT; 
            }
            if(stage == STAGE0) {
                checkCollision(BG0aCMBitmap);
            } else if(stage == STAGE1) {
                checkCollision(BG0bCMBitmap);
            } else if(stage == STAGE2) {
                checkCollision(BG2CMBitmap);
            }
        }
   
        batman.aniCounter++;
        updateEnemies();    
        updateBatarang();

        if(stage == STAGE2) {
            updateExplosives();
        }
            
}

void updateOAM() {
    drawBatman();
    drawEnemies();
    drawExplosives();

    if(ACTIVEENEMIES == 0) {
        shadowOAM[100].attr0 = 140| ATTR0_WIDE;
        shadowOAM[100].attr1 = 200 | ATTR1_SIZE32;
        shadowOAM[100].attr2 = SPRITEOFFSET16(21, 0);
    } else {
        shadowOAM[100].attr0 = 0;
        shadowOAM[100].attr1 = 0;
        shadowOAM[100].attr2 = 0;
    }

}

void punchCollision() {
    for(int i = 0; i < NUMOFENEMIES; i++) {
        if(batman.direction == SPRITERIGHT) {
            if((batman.row + 14 > enemies[i].row) && (batman.row + 14 < enemies[i].row + enemies[i].height)
                    && (batman.col + batman.width > enemies[i].col) && (batman.col + batman.width < enemies[i].col + enemies[i].width)
                    && (enemies[i].health > 0) ) {
                enemies[i].health--;
                enemies[i].currFrame = 0;
                enemies[i].aniState = HURT;
                if(stage == STAGE2) {
                    playSoundB(JokerOw, JOKEROWLEN, JOKEROWFREQ);
                } else {
                    playSoundB(GruntSFX, GRUNTSFXLEN, GRUNTSFXFREQ);
                }
                if(stage == STAGE2) {
                    jokerTransition = 1;
                }
            }
        } else {
            if((batman.row + 14 > enemies[i].row) && (batman.row + 14 < enemies[i].row + enemies[i].height)
                    && (batman.col > enemies[i].col) && (batman.col < enemies[i].col + enemies[i].width)
                    && (enemies[i].health > 0) ) {
                enemies[i].health--;
                enemies[i].currFrame = 0;
                enemies[i].aniState = HURT;
                if(stage == STAGE2) {
                    playSoundB(JokerOw, JOKEROWLEN, JOKEROWFREQ);
                } else {
                    playSoundB(GruntSFX, GRUNTSFXLEN, GRUNTSFXFREQ);
                }
                if(stage == STAGE2) {
                    jokerTransition = 1;
                }
            }
        }
    }
}

void batarang() {
    batman.batarang.active = 1;
    batman.batarang.direction = batman.direction;
    if(batman.batarang.direction == SPRITELEFT) {

        batman.batarang.bigCol = batman.bigCol  + 24;
    } else {
        batman.batarang.bigCol = batman.bigCol  + batman.width - 24;
    }
    batman.batarang.bigRow = batman.bigRow + 16;
    
    batman.batarang.aniCounter = 0;
    batman.batarang.speed = 3;
    if(batman.direction) {
        batman.batarang.cdel = -1;
    } else {
        batman.batarang.cdel = 1;
    }
    batman.batarang.row = batman.batarang.bigRow - vOff;
    batman.batarang.col = batman.batarang.bigCol - hOff;
}

void updateBatarang() {
    if(batman.batarang.active) {
        if(batman.batarang.aniCounter >= batman.range) {
            batman.batarang.active = 0;
        } else {
            batman.batarang.bigCol += (batman.batarang.cdel * batman.batarang.speed);
            batman.batarang.aniCounter++;
        }
    }
}

void batarangCollision(int i) {
    if(batman.batarang.active) {
        if((batman.batarang.row + 4 > enemies[i].row) && (batman.batarang.row + 4 < enemies[i].row + enemies[i].height)
                && (batman.batarang.col + 8 > enemies[i].col) && (batman.batarang.col + 8 < enemies[i].col + enemies[i].width)
                && enemies[i].aniState != DEAD
                && enemies[i].col > 0 && enemies[i].col < 240) {
            enemies[i].currFrame = 0;
            enemies[i].aniState = STUNNED;
            if(stage == STAGE2) {
                playSoundB(JokerOw, JOKEROWLEN, JOKEROWFREQ);
            } else {
                playSoundB(GruntSFX, GRUNTSFXLEN, GRUNTSFXFREQ);
            }
            batman.batarang.active = 0;
        }    
    }
}

void explosiveCollision(int i) {
    if((batman.batarang.row + 4 > explosives[i].row) && (batman.batarang.row + 4 < explosives[i].row + explosives[i].height)
            && (batman.batarang.col + 8 > explosives[i].col) && (batman.batarang.col + 8 < explosives[i].col + explosives[i].width)
            && explosives[i].active
            && explosives[i].col > 0 && explosives[i].col < 240) {
        explosives[i].active = 0;
        playSoundB(Explosion, EXPLOSIONLEN, EXPLOSIONFREQ);
        batman.batarang.active = 0;
        explosives[i].exploding = 1;
        explosives[i].currFrame = 0;
    }
}

void updateExplosives() {
    for(int i = 0; i < MAXEXPLOSIVES; i++) {
        if(batman.batarang.active) {
            explosiveCollision(i);
        }
    }
}

void explosionHit(int i) {
    for (int j = 0; j < NUMOFENEMIES; j++) {
        if((enemies[j].bigRow + enemies[j].height > explosives[i].bigRow - 16) && (enemies[j].bigRow < explosives[i].bigRow + explosives[i].height + 16) 
                && (enemies[j].bigCol + enemies[j].width > explosives[i].bigCol - 16) && (enemies[j].bigCol < explosives[i].bigCol + explosives[i].width + 16)
                && (enemies[j].aniState != DEAD) && enemies[j].col > 0 && enemies[j].col < 240 ) {
            enemies[j].currFrame = 0;
                if(enemies[j].aniState != HURT) {
                    enemies[j].aniState = EXPLOSIONSTUNNED;
                }
                //playSoundB(GruntSFX, GRUNTSFXLEN, GRUNTSFXFREQ);
        }
    }
}

void bulletCollision(int enemy, int bullet) {
    if(enemies[enemy].bullets[bullet].active) {
        if((enemies[enemy].bullets[bullet].row + 4 > batman.row + 4) && (enemies[enemy].bullets[bullet].row + 4 < batman.row + batman.height - 16)
                && (enemies[enemy].bullets[bullet].col + 4 > batman.col + 16) && (enemies[enemy].bullets[bullet].col + 4< batman.col + batman.width - 16)) {
            batman.currFrame = 0;
            batman.health--;
            batman.aniState = BATSTUNNED;
            playSoundB(BatmanGrunt, BATMANGRUNTLEN, BATMANGRUNTFREQ);
            enemies[enemy].bullets[bullet].active = 0;
        }     
    }
}

void enemyShoot(int enemy){

    for(int i = 0; i < enemies[enemy].MAX_BULLETS; i++){
        if(!enemies[enemy].bullets[i].active){
            enemies[enemy].bullets[i].active = 1;
            enemies[enemy].bullets[i].aniCounter = 0;
            enemies[enemy].bullets[i].direction = enemies[enemy].direction;
            enemies[enemy].bullets[i].speed = 2;
            if(enemies[enemy].direction == SPRITELEFT) {
                enemies[enemy].bullets[i].cdel = -1;
                enemies[enemy].bullets[i].bigCol = enemies[enemy].bigCol;
            } else {
                enemies[enemy].bullets[i].cdel = 1;
                enemies[enemy].bullets[i].bigCol = enemies[enemy].bigCol + enemies[enemy].width;
            }
            enemies[enemy].bullets[i].bigRow = enemies[enemy].bigRow + 16;
            enemies[enemy].bullets[i].row = enemies[enemy].bullets[i].bigRow - vOff;
            enemies[enemy].bullets[i].col = enemies[enemy].bullets[i].bigCol - hOff;
            playSoundB(GunShot, GUNSHOTLEN, GUNSHOTFREQ);
            break;
        }
    }  
}

void updateEnemies() {
    for(int i = 0; i < NUMOFENEMIES; i++) {
        if(enemies[i].aniState == ACTIVE) {
            if(batman.col + batman.width/2 > enemies[i].col + enemies[i].width/2) {
                enemies[i].direction = SPRITERIGHT;
            } else {
                enemies[i].direction = SPRITELEFT;
            }

            if(stage == STAGE0 && enemies[i].aniCounter%2) {
                if((enemies[i].bigRow <= 8*13) || (enemies[i].bigRow >= 256-64)) {
                    enemies[i].rdel *= -1;
                }
                enemies[i].bigRow += (enemies[i].rdel * enemies[i].speed);
                
            } else if (stage == STAGE1 && enemies[i].aniCounter%2) {
                if((enemies[i].bigRow <= 8*13) || (enemies[i].bigRow >= 256-64)) {
                    enemies[i].rdel *= -1;
                } else if(rand()%55 == 7) {
                    enemies[i].rdel *= -1;
                }

                if((enemies[i].bigCol <= enemies[i].startPosC - 50) || (enemies[i].bigCol >= enemies[i].startPosC + 50) || (enemies[i].bigCol >= 480)) {
                    enemies[i].cdel *= -1;
                } else if(rand()%55 == 7) {
                    enemies[i].cdel *= -1;
                }
                enemies[i].bigCol += (enemies[i].cdel * enemies[i].speed);
                enemies[i].bigRow += (enemies[i].rdel * enemies[i].speed);
            } else if (stage == STAGE2) {

                if((enemies[i].bigRow <= 8*13) || (enemies[i].bigRow >= 256-64)) {
                    enemies[i].rdel *= -1;
                } else if(rand()%55 == 7) {
                    enemies[i].rdel *= -1;
                }

                if(enemies[i].health == 3) {
                    if(enemies[i].bigCol <= 100) {
                        enemies[i].cdel = 1;
                    } else if(enemies[i].bigCol + enemies[i].width >= 220) {
                        enemies[i].cdel = -1;
                    } else if (jokerTransition && enemies[i].bigCol <= 220) {
                        enemies[i].cdel = 1;
                    } else if(rand()%55 == 7 && enemies[i].bigCol > 100 && enemies[i].bigCol + enemies[i].width < 220) {
                        enemies[i].cdel *= -1;
                    }

                } else if (enemies[i].health == 2) {
                    if(enemies[i].bigCol <= 220 && !jokerTransition) {
                        enemies[i].cdel = 1;
                    } else if(enemies[i].bigCol + enemies[i].width >= 340 && !jokerTransition) {
                        enemies[i].cdel = -1;
                    } else if (jokerTransition && enemies[i].bigCol <= 220) {
                        enemies[i].cdel = 1;
                    } else if(enemies[i].bigCol > 220 && jokerTransition) {
                        jokerTransition = 0;
                    } else if(rand()%55 == 7 && enemies[i].bigCol > 220 && enemies[i].bigCol + enemies[i].width < 340) {
                        enemies[i].cdel *= -1;
                        
                    }

                } else if (enemies[i].health == 1) {
                    if(enemies[i].bigCol <= 340 && !jokerTransition) {
                        enemies[i].cdel = 1;
                    } else if(enemies[i].bigCol + enemies[i].width >= 500 && !jokerTransition) {
                        enemies[i].cdel = -1;
                    } else if (jokerTransition && enemies[i].bigCol <= 340) {
                        enemies[i].cdel = 1;
                    } else if(enemies[i].bigCol > 340 && jokerTransition) {
                        jokerTransition = 0;
                    } else if(rand()%55 == 7 && enemies[i].bigCol > 340 && enemies[i].bigCol + enemies[i].width < 500) {
                        enemies[i].cdel *= -1;
                        
                    }
                }

                enemies[i].bigCol += (enemies[i].cdel * enemies[i].speed);
                enemies[i].bigRow += (enemies[i].rdel * enemies[i].speed);

            }
            enemies[i].aniCounter++;

            if(enemies[i].col > 0 && enemies[i].col < 240) {
                if(rand()%30 == 5 && !cheat){
                    enemyShoot(i);
                } else if(rand()%60 == 5 && stage == STAGE2) {
                    playSoundB(JokerLaugh2, JOKERLAUGH2LEN, JOKERLAUGH2FREQ);
                }
            } 
        }

        for(int j = 0; j < enemies[i].MAX_BULLETS; j++) {
            if(enemies[i].bullets[j].active) {
                bulletCollision(i, j);
                if(enemies[i].bullets[j].aniCounter >= enemies[i].range) {
                    enemies[i].bullets[j].active = 0;
                } else {
                    enemies[i].bullets[j].bigCol += (enemies[i].bullets[j].cdel * enemies[i].bullets[j].speed);
                    enemies[i].bullets[j].aniCounter++;
                }
            }
        }
        batarangCollision(i);
    }
}

void drawBatman() {
    int flip = 0;
    int SpriteSize = 0;
    int SpritePos = 0;
    int SpriteShape = 0;
    if(batman.direction == SPRITELEFT) {
        flip = ATTR1_HFLIP;   
    } 
    if(batman.aniState == BATIDLE || batman.aniState == BATARANG) {

        if(batman.aniState == BATIDLE) {
            if(flip) {
                SpriteSize = ATTR1_SIZE32;
                SpritePos = 11;
                SpriteShape = ATTR0_SQUARE;
            } else {
                SpriteSize = ATTR1_SIZE32;
                SpritePos = 11;
                SpriteShape = ATTR0_SQUARE;
            }
        } else {
            if(flip) {
                SpriteSize = ATTR1_SIZE64;
                SpritePos = -10;
                SpriteShape = ATTR0_WIDE;
            } else {
                SpriteSize = ATTR1_SIZE64;
                SpritePos = 0;
                SpriteShape = ATTR0_WIDE;
            }
        }
        shadowOAM[0].attr0 = SpriteShape | (batman.row & ROWMASK);
        shadowOAM[0].attr1 = flip | SpriteSize | ((batman.col + SpritePos) & COLMASK);
        shadowOAM[0].attr2 = SPRITEOFFSET16(batman.currFrame*4, batman.aniState*8);

        shadowOAM[1].attr0 = ATTR0_SQUARE | (batman.row + 32);
        shadowOAM[1].attr1 = flip | ATTR1_SIZE32 | (batman.col + 11);
        shadowOAM[1].attr2 = SPRITEOFFSET16(4*4, 0);
    } else if(batman.aniState == BATSTUNNED) {
        if (flip) {
            SpritePos = -11;
        }
        shadowOAM[0].attr0 = ATTR0_SQUARE | (batman.row & ROWMASK);
        shadowOAM[0].attr1 = flip | ATTR1_SIZE64 | ((batman.col + SpritePos) & COLMASK);
        shadowOAM[0].attr2 = SPRITEOFFSET16(0, 16);

        shadowOAM[1].attr0 = 0;
        shadowOAM[1].attr1 = 0;
        shadowOAM[1].attr2 = 0;

    } else {
        if (flip && batman.aniState == BATATTACK) {
            SpritePos = -11;
        } else {
            SpritePos = 0;
        }
        shadowOAM[0].attr0 = ATTR0_SQUARE | (batman.row & ROWMASK);
        shadowOAM[0].attr1 = flip | ATTR1_SIZE64 | ((batman.col + SpritePos) & COLMASK);
        shadowOAM[0].attr2 = SPRITEOFFSET16(batman.currFrame*8, batman.aniState*8);

        shadowOAM[1].attr0 = 0;
        shadowOAM[1].attr1 = 0;
        shadowOAM[1].attr2 = 0;
    }

    flip = 0;
    if(batman.batarang.direction == SPRITELEFT) {
        flip = ATTR1_HFLIP;
    }
    if(batman.batarang.active) {
        shadowOAM[2].attr0 = ATTR0_SQUARE | (ROWMASK & batman.batarang.row);
        shadowOAM[2].attr1 = flip | ATTR1_SIZE8 | (COLMASK & batman.batarang.col);
        shadowOAM[2].attr2 = SPRITEOFFSET16(20, 1);
    } else {
        shadowOAM[2].attr0 = 0;
        shadowOAM[2].attr1 = 0;
        shadowOAM[2].attr2 = 0;
    }

    for(int i = 0; i < batman.MAX_HEALTH; i++) {
        if (i < batman.health) {
            shadowOAM[i + 3].attr0 = ATTR0_SQUARE | 12;
            shadowOAM[i + 3].attr1 = ATTR1_SIZE8 | (27 + (i*HealthWidth));
            if(batman.health < 3) {
               shadowOAM[i + 3].attr2 = SPRITEOFFSET16(23, 0); 
            } else {
                shadowOAM[i + 3].attr2 = SPRITEOFFSET16(20, 0);
            }
        } else {
            shadowOAM[i + 3].attr0 = 0;
            shadowOAM[i + 3].attr1 = 0;
            shadowOAM[i + 3].attr2 = 0;
        }
    }

}

void drawEnemies() {
    for(int i = 0; i < NUMOFENEMIES; i++) {
        int flip = 0;
        if(enemies[i].direction == SPRITELEFT) {
            flip = ATTR1_HFLIP;
        }
        if(enemies[i].aniState == ACTIVE) {
            shadowOAM[40 + i].attr0 = (ROWMASK & enemies[i].row)| ATTR0_TALL;
            shadowOAM[40 + i].attr1 = flip | (COLMASK & enemies[i].col) | ATTR1_SIZE64;
            shadowOAM[40 + i].attr2 = SPRITEOFFSET16(enemies[i].currFrame*8, 4);
        } else if(enemies[i].aniState == HURT || enemies[i].aniState == STUNNED || enemies[i].aniState == EXPLOSIONSTUNNED) {
            shadowOAM[40 + i].attr0 = (ROWMASK & enemies[i].row)| ATTR0_TALL;
            shadowOAM[40 + i].attr1 = (COLMASK & enemies[i].col) | ATTR1_SIZE64;
            shadowOAM[40 + i].attr2 = SPRITEOFFSET16(16,24);
        }else if(enemies[i].aniState == DEAD) {
            shadowOAM[40 + i].attr0 = (ROWMASK & (enemies[i].row + 32))| ATTR0_WIDE;
            shadowOAM[40 + i].attr1 = (COLMASK & enemies[i].col) | ATTR1_SIZE32;
            shadowOAM[40 + i].attr2 = SPRITEOFFSET16(16,28);
        }

        for(int j = 0; j < enemies[i].MAX_HEALTH; j++) {
            if(j < enemies[i].health) {
                shadowOAM[80 + j + i*enemies[i].MAX_HEALTH].attr0 = (ROWMASK & (enemies[i].row - 8))| ATTR0_SQUARE;
                shadowOAM[80 + j + i*enemies[i].MAX_HEALTH].attr1 = (COLMASK & (enemies[i].col + j*8)) | ATTR1_SIZE8;
                shadowOAM[80 + j + i*enemies[i].MAX_HEALTH].attr2 = SPRITEOFFSET16(20,3);
            } else {
                shadowOAM[80 + j + i*enemies[i].MAX_HEALTH].attr0 = 0;
                shadowOAM[80 + j + i*enemies[i].MAX_HEALTH].attr1 = 0;
                shadowOAM[80 + j + i*enemies[i].MAX_HEALTH].attr2 = 0;

            }
        }

        for(int j = 0; j < enemies[i].MAX_BULLETS; j++) {
            if(enemies[i].bullets[j].active) {
                shadowOAM[10 + j + i*enemies[i].MAX_BULLETS].attr0 = ATTR0_SQUARE | (ROWMASK & enemies[i].bullets[j].row);
                shadowOAM[10 + j + i*enemies[i].MAX_BULLETS].attr1 = flip | ATTR1_SIZE8 | (COLMASK & enemies[i].bullets[j].col);
                shadowOAM[10 + j + i*enemies[i].MAX_BULLETS].attr2 = SPRITEOFFSET16(20, 2);
            } else {
                shadowOAM[10 + j + i*enemies[i].MAX_BULLETS].attr0 = 0;
                shadowOAM[10 + j + i*enemies[i].MAX_BULLETS].attr1 = 0;
                shadowOAM[10 + j + i*enemies[i].MAX_BULLETS].attr2 = 0;

            } 

        }
    }
}

void drawExplosives() {
    if(stage == STAGE2) {
        for(int i = 0; i < MAXEXPLOSIVES; i++) {
            if(explosives[i].active) {
                shadowOAM[110 + i].attr0 = ((explosives[i].row) & ROWMASK)| ATTR0_SQUARE;
                shadowOAM[110 + i].attr1 = (explosives[i].col & COLMASK) | ATTR1_SIZE32;
                shadowOAM[110 + i].attr2 = SPRITEOFFSET16(24, 0);
            } else {
                shadowOAM[110 + i].attr0 = 0;
                shadowOAM[110 + i].attr1 = 0;
                shadowOAM[110 + i].attr2 = 0;
            }

            if (explosives[i].exploding) {
                for(int j = 0; j < MAXEXPLOSIONS; j++) {
                    if(explosives[i].explosions[j].exploding) {
                        shadowOAM[115 + j + i*MAXEXPLOSIONS].attr0 = ((explosives[i].explosions[j].row) & ROWMASK)| ATTR0_SQUARE;
                        shadowOAM[115 + j + i*MAXEXPLOSIONS].attr1 = (explosives[i].explosions[j].col & COLMASK) | ATTR1_SIZE64;
                        shadowOAM[115 + j + i*MAXEXPLOSIONS].attr2 = SPRITEOFFSET16(24,24);
                    } 
                }
            } else {
                for(int j = 0; j < MAXEXPLOSIONS; j++) {
                    shadowOAM[115 + j + i*MAXEXPLOSIONS].attr0 = 0;
                    shadowOAM[115 + j + i*MAXEXPLOSIONS].attr1 = 0;
                    shadowOAM[115 + j + i*MAXEXPLOSIONS].attr2 = 0;
                }
            }
        }
    }
}

void initialize(){
    batman.width = 64;
    batman.height = 64;
    batman.rdel = 1;
    batman.cdel = 1;
    batman.row = 75;
    batman.col = 45;
    batman.bigRow = batman.row + vOff;
    batman.bigCol = batman.col + hOff;
    batman.speed = 2;
    batman.direction = SPRITERIGHT;
    batman.health = 7;
    batman.MAX_HEALTH = 7;
    batman.range = 50;
    batman.batarang.active = 0;

    batman.aniCounter = 0;
    batman.currFrame = 0;

    batman.aniState = BATIDLE;

    buttons = BUTTONS;
        
    hOff = 0;
    vOff = 80;

    if(stage == STAGE0) {

        NUMOFENEMIES = 3;
        ACTIVEENEMIES = NUMOFENEMIES;
        MAXBULLETS = 2;

        for(int i = 0; i < NUMOFENEMIES; i++) {
            enemies[i].width = 32;
            enemies[i].height = 64;
            if(i & 1) {
                enemies[i].rdel = 1;
            } else {
                enemies[i].rdel = -1;
            }
            enemies[i].cdel = 1;
            enemies[i].speed = 1;
            enemies[i].direction = SPRITELEFT;
            enemies[i].health = 2;
            enemies[i].MAX_HEALTH = 2;
            enemies[i].MAX_BULLETS = MAXBULLETS;
            enemies[i].bigRow = 130;
            enemies[i].bigCol = 200 + (i*125);
            enemies[i].aniState = ACTIVE;
            enemies[i].range = 75;
            enemies[i].aniCounter = 0;
            enemies[i].currFrame = i%4;
        }
    } else if(stage == STAGE1) {
        NUMOFENEMIES = 3;
        ACTIVEENEMIES = NUMOFENEMIES;
        MAXBULLETS = 1;
        for(int i = 0; i < NUMOFENEMIES; i++) {
            enemies[i].width = 32;
            enemies[i].height = 64;
            if(i & 1) {
                enemies[i].rdel = 1;
            } else {
                enemies[i].rdel = -1;
            }
            if(rand()&2) {
                enemies[i].cdel = 1;
            } else {
                enemies[i].cdel = -1;
            }
            enemies[i].speed = 1;
            enemies[i].direction = SPRITELEFT;
            enemies[i].health = 2;
            enemies[i].MAX_HEALTH = 2;
            enemies[i].MAX_BULLETS = MAXBULLETS;
            enemies[i].bigRow = 130;
            enemies[i].bigCol = 175 + (i*80);
            enemies[i].startPosC = enemies[i].bigCol;
            enemies[i].aniState = ACTIVE;
            enemies[i].range = 75;

            enemies[i].aniCounter = 0;
            enemies[i].currFrame = i%4;
        }
    } else if(stage == STAGE2) {
        NUMOFENEMIES = 1;
        ACTIVEENEMIES = NUMOFENEMIES;
        MAXBULLETS = 3;
        jokerTransition = 0;
        jokerPrevHealth = 3;
        for(int i = 0; i < NUMOFENEMIES; i++) {
            enemies[i].width = 32;
            enemies[i].height = 64;
            if(i & 1) {
                enemies[i].rdel = 1;
            } else {
                enemies[i].rdel = -1;
            }

            if(rand()&2) {
                enemies[i].cdel = 1;
            } else {
                enemies[i].cdel = -1;
            }
            enemies[i].speed = 1;
            enemies[i].direction = SPRITELEFT;
            enemies[i].health = 3;
            enemies[i].MAX_HEALTH = 3;
            enemies[i].MAX_BULLETS = MAXBULLETS;
            enemies[i].bigRow = 130;
            enemies[i].bigCol = 150;
            enemies[i].startPosC = enemies[i].bigCol;
            enemies[i].aniState = ACTIVE;
            enemies[i].range = 75;

            enemies[i].aniCounter = 0;
            enemies[i].currFrame = i%4;
        }

        for(int i = 0; i < MAXEXPLOSIVES; i++) {
            explosives[i].bigRow = rand()%100 + 108 ;
            explosives[i].bigCol = rand()%100 + 120*i + 100;
            explosives[i].active = 1;
            explosives[i].height = 32;
            explosives[i].width = 12;
            explosives[i].currFrame = 0;
            explosives[i].exploding = 0;

            for(int j = 0; j < MAXEXPLOSIONS; j++) {
                explosives[i].explosions[j].exploding = 0;
            }
        }
    }

    for(int i = 0; i < NUMOFENEMIES; i++) {
        for(int j = 0; j < enemies[i].MAX_BULLETS; j++) {
            enemies[i].bullets[j].active = 0;
        }
    }  
    
}

void hideSprites() {
    unsigned short hide = ATTR0_HIDE;
    DMANow(3, &hide, shadowOAM, 512 | DMA_SOURCE_FIXED);
}