#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int BLOCK_SIZE = 40;
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

const int BOARD_PIXEL_WIDTH = BOARD_WIDTH * BLOCK_SIZE;
const int BOARD_PIXEL_HEIGHT = BOARD_HEIGHT * BLOCK_SIZE;
const int BOARD_X = (SCREEN_WIDTH - BOARD_PIXEL_WIDTH) * 2;
const int BOARD_Y = 0;
const int TEXT_X = BOARD_X + BOARD_PIXEL_WIDTH + 20;
const int NEXT_BOX_SIZE = 4 * BLOCK_SIZE;
const int NEXT_BOX_X = TEXT_X + 5;
const int NEXT_BOX_Y = SCREEN_HEIGHT - 170;

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point blocks[4];
    int x, y;
    int rotation;
} Tetromino;

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_GameController* gGameController = NULL;
SDL_Texture* gBackgroundTexture = NULL;
TTF_Font* gFont = NULL;
int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
Tetromino current;
Tetromino next;
int score = 0;
int bestScore = 0;
int linesCleared = 0;
int bestLines = 0;
Uint32 currentTime = 0;
Uint32 bestTime = 0;
Uint32 startTime = 0;

const Tetromino tetrominoes[7] = {
    {{ {0,1}, {1,1}, {2,1}, {3,1} }, 0}, // I
    {{ {1,0}, {1,1}, {2,0}, {2,1} }, 0}, // O
    {{ {0,1}, {1,0}, {1,1}, {2,1} }, 0}, // T
    {{ {1,0}, {2,0}, {0,1}, {1,1} }, 0}, // S
    {{ {0,0}, {1,0}, {1,1}, {2,1} }, 0}, // Z
    {{ {0,0}, {0,1}, {1,1}, {2,1} }, 0}, // J
    {{ {0,1}, {1,1}, {2,0}, {2,1} }, 0}  // L
};

bool init();
void close();
void render();
void move_down();
bool check_collision(int dx, int dy);
void merge_tetromino();
void clear_lines();
void spawn_tetromino();
void game_loop();
void handle_input(SDL_Event* e);
void rotate_tetromino();
void render_score();
void render_best_score();
void render_time();
void render_best_time();
void render_lines();
void render_best_lines();
void render_next_tetromino();

bool init_audio() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
        return false;
    }

    return true;
}

Mix_Chunk* load_audio(const char* audioPath) {
    Mix_Chunk* backgroundSound = Mix_LoadWAV(audioPath);
    if (backgroundSound == NULL) {
        printf("Failed to load audio! SDL_mixer Error: %s\n", Mix_GetError());
    }

    Mix_PlayChannel(-1, backgroundSound, -1);

    return backgroundSound;
}

void close_audio(Mix_Chunk* audio) {
    Mix_FreeChunk(audio);
    Mix_CloseAudio();
}

bool init_font(const char* fontPath) {
    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    gFont = TTF_OpenFont(fontPath, 28);
    if (gFont == NULL) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    return true;
}

void close_font() {
    TTF_CloseFont(gFont);
    gFont = NULL;
    TTF_Quit();
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
    } else {
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        if (newTexture == NULL) {
            printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
        }
        SDL_FreeSurface(loadedSurface);
    }
    return newTexture;
}

void print_usage() {
    printf("Usage: tetris [OPTIONS]\n");
    printf("Options:\n");
    printf("  -m <music_file>   Path to the background music file (OGG/WAV format)\n");
    printf("  -f <font_file>    Path to the TTF font file for displaying text\n");
    printf("  -b <background_file> Path to the background image file (PNG/JPG format)\n");
    printf("  --help            Display this help message\n");
}

int main(int argc, char* args[]) {
    srand((unsigned int)time(NULL));

    const char* audioPath = NULL;
    const char* fontPath = NULL;
    const char* backgroundPath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(args[i], "-m") == 0 && i + 1 < argc) {
            audioPath = args[++i];
        } else if (strcmp(args[i], "-f") == 0 && i + 1 < argc) {
            fontPath = args[++i];
        } else if (strcmp(args[i], "-b") == 0 && i + 1 < argc) {
            backgroundPath = args[++i];
        } else if (strcmp(args[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    if (!init()) {
        printf("Failed to initialize!\n");
        return 1;
    }

    if (backgroundPath != NULL) {
        gBackgroundTexture = loadTexture(backgroundPath);
        if (gBackgroundTexture == NULL) {
            printf("Failed to load background image!\n");
            return 1;
        }
    }

    Mix_Chunk* backgroundSound = NULL;
    if (audioPath != NULL) {
        backgroundSound = load_audio(audioPath);
        if (backgroundSound == NULL) {
            printf("Failed to load audio!\n");
            return 1;
        }
    }

    if (fontPath != NULL && !init_font(fontPath)) {
        printf("Failed to load font, starting without score display!\n");
    }

    printf("Tetris PS5 - Running with SDL %d.%d.%d (compiled with %d.%d.%d)\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL, SDL_COMPILEDVERSION >> 24, (SDL_COMPILEDVERSION >> 16) & 0xFF, SDL_COMPILEDVERSION & 0xFFFF);

    startTime = SDL_GetTicks();
    game_loop();

    if (backgroundSound != NULL) {
        close_audio(backgroundSound);
    }

    close();
    return 0;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gWindow = SDL_CreateWindow("Joystick Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_SOFTWARE);
    if (gRenderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_NumJoysticks() < 1) {
        printf("Warning: No joysticks connected!\n");
    } else {
        gGameController = SDL_GameControllerOpen(0);
        if (gGameController == NULL) {
            printf("Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
        }
    }

    if (!init_audio()) {
        printf("Failed to initialize audio!\n");
        return false;
    }

    int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }

    return true;
}

void close() {
    if (gFont != NULL) {
        close_font();
    }
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_DestroyTexture(gBackgroundTexture);
    if (gGameController != NULL) {
        SDL_GameControllerClose(gGameController);
    }
    gRenderer = NULL;
    gWindow = NULL;
    gGameController = NULL;
    gBackgroundTexture = NULL;

    IMG_Quit();
    SDL_Quit();
}

void render() {
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

    if (gBackgroundTexture != NULL) {
        SDL_RenderCopy(gRenderer, gBackgroundTexture, NULL, NULL);
    }

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 150);
    SDL_Rect darkenRect = { BOARD_X, BOARD_Y, BOARD_PIXEL_WIDTH, BOARD_PIXEL_HEIGHT };
    SDL_RenderFillRect(gRenderer, &darkenRect);

    SDL_SetRenderDrawColor(gRenderer, 0, 255, 0, 255);
    int borderThickness = 2;
    SDL_Rect topBorder = { BOARD_X, BOARD_Y, BOARD_PIXEL_WIDTH, borderThickness };
    SDL_Rect bottomBorder = { BOARD_X, BOARD_Y + BOARD_PIXEL_HEIGHT, BOARD_PIXEL_WIDTH, borderThickness };
    SDL_RenderFillRect(gRenderer, &topBorder);
    SDL_RenderFillRect(gRenderer, &bottomBorder);

    SDL_Rect leftBorder = { BOARD_X, BOARD_Y, borderThickness, BOARD_PIXEL_HEIGHT };
    SDL_Rect rightBorder = { BOARD_X + BOARD_PIXEL_WIDTH - borderThickness, BOARD_Y, borderThickness, BOARD_PIXEL_HEIGHT };
    SDL_RenderFillRect(gRenderer, &leftBorder);
    SDL_RenderFillRect(gRenderer, &rightBorder);

    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j]) {
                SDL_Rect fillRect = { BOARD_X + j * BLOCK_SIZE, BOARD_Y + i * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
                SDL_RenderFillRect(gRenderer, &fillRect);
            }
        }
    }

    SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect fillRect = { BOARD_X + (current.blocks[i].x + current.x) * BLOCK_SIZE, BOARD_Y + (current.blocks[i].y + current.y) * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
        SDL_RenderFillRect(gRenderer, &fillRect);
    }

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 150);
    SDL_Rect textBgRect = { TEXT_X - 10, BOARD_Y, 500, 310 };
    SDL_RenderFillRect(gRenderer, &textBgRect);

    if (gFont != NULL) {
        render_score();
        render_best_score();
        render_time();
        render_best_time();
        render_lines();
        render_best_lines();
    }

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 150);
    SDL_Rect nextBoxRect = { NEXT_BOX_X - 10, NEXT_BOX_Y - 10, NEXT_BOX_SIZE + 20, NEXT_BOX_SIZE + 20 };
    SDL_RenderFillRect(gRenderer, &nextBoxRect);
    render_next_tetromino();

    SDL_RenderPresent(gRenderer);
}

void render_text(const char* text, int xPos, int yPos) {
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, text, textColor);
    if (textSurface == NULL) {
        printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
    if (textTexture == NULL) {
        printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_Rect renderQuad = { xPos, yPos, textWidth, textHeight };
    SDL_RenderCopy(gRenderer, textTexture, NULL, &renderQuad);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void render_time() {
    char timeText[20];
    sprintf(timeText, "Time: %d", currentTime / 1000);
    render_text(timeText, TEXT_X, BOARD_Y);
}

void render_best_time() {
    char bestTimeText[20];
    sprintf(bestTimeText, "Best Time: %d", bestTime / 1000);
    render_text(bestTimeText, TEXT_X, BOARD_Y + 40);
}


void render_score() {
    char scoreText[20];
    sprintf(scoreText, "Score: %d", score);
    render_text(scoreText, TEXT_X, BOARD_Y + 120);
}

void render_best_score() {
    char bestScoreText[20];
    sprintf(bestScoreText, "Best Score: %d", bestScore);
    render_text(bestScoreText, TEXT_X, BOARD_Y + 160);
}


void render_lines() {
    char linesText[20];
    sprintf(linesText, "Lines: %d", linesCleared);
    render_text(linesText, TEXT_X, BOARD_Y + 240);
}

void render_best_lines() {
    char bestLinesText[20];
    sprintf(bestLinesText, "Best Lines: %d", bestLines);
    render_text(bestLinesText, TEXT_X, BOARD_Y + 280);
}

void render_next_tetromino() {
    SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect fillRect = { NEXT_BOX_X + next.blocks[i].x * BLOCK_SIZE, NEXT_BOX_Y + next.blocks[i].y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
        SDL_RenderFillRect(gRenderer, &fillRect);
    }
}

void move_down() {
    if (!check_collision(0, 1)) {
        current.y++;
    } else {
        merge_tetromino();
        clear_lines();
        spawn_tetromino();
    }
}

bool check_collision(int dx, int dy) {
    for (int i = 0; i < 4; ++i) {
        int newX = current.blocks[i].x + current.x + dx;
        int newY = current.blocks[i].y + current.y + dy;
        if (newX < 0 || newX >= BOARD_WIDTH || newY >= BOARD_HEIGHT || board[newY][newX]) {
            return true;
        }
    }
    return false;
}

void merge_tetromino() {
    for (int i = 0; i < 4; ++i) {
        board[current.blocks[i].y + current.y][current.blocks[i].x + current.x] = 1;
    }
}

void clear_lines() {
    int linesThisClear = 0;
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        bool full = true;
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (!board[i][j]) {
                full = false;
                break;
            }
        }
        if (full) {
            for (int k = i; k > 0; --k) {
                for (int j = 0; j < BOARD_WIDTH; ++j) {
                    board[k][j] = board[k - 1][j];
                }
            }
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                board[0][j] = 0;
            }
            score += 100;
            linesCleared++;
            linesThisClear++;
        }
    }
}

void spawn_tetromino() {
    current = next;
    next = tetrominoes[rand() % 7];
    current.x = BOARD_WIDTH / 2 - 2;
    current.y = 0;
    current.rotation = 0;
    if (check_collision(0, 0)) {
        if (currentTime > bestTime) {
            bestTime = currentTime;
        }

        if (score > bestScore) {
            bestScore = score;
        }

        if (linesCleared > bestLines) {
            bestLines = linesCleared;
        }

        currentTime = 0;
        startTime = SDL_GetTicks();

        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                board[i][j] = 0;
            }
        }

        score = 0;
        linesCleared = 0;

        spawn_tetromino();
    }
}

bool check_rotation_collision() {
    Tetromino temp = current;
    for (int i = 0; i < 4; ++i) {
        int x = current.blocks[i].x;
        int y = current.blocks[i].y;
        temp.blocks[i].x = temp.blocks[i].y;
        temp.blocks[i].y = -x;
    }
    return check_collision(0, 0);
}

void rotate_tetromino() {
    Tetromino temp = current;
    Point pivot = { current.blocks[1].x, current.blocks[1].y };

    for (int i = 0; i < 4; ++i) {
        int relX = current.blocks[i].x - pivot.x;
        int relY = current.blocks[i].y - pivot.y;
        current.blocks[i].x = pivot.x - relY;
        current.blocks[i].y = pivot.y + relX;
    }

    if (check_rotation_collision()) {
        current = temp;
    }
}

void handle_input(SDL_Event* e) {
    if (e->type == SDL_CONTROLLERBUTTONDOWN) {
        switch (e->cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                rotate_tetromino();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                move_down();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                if (!check_collision(-1, 0)) {
                    current.x--;
                }
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                if (!check_collision(1, 0)) {
                    current.x++;
                }
                break;
            case SDL_CONTROLLER_BUTTON_A:
                rotate_tetromino();
                break;
            case SDL_CONTROLLER_BUTTON_B:
                while (!check_collision(0, 1)) {
                    current.y++;
                }
                merge_tetromino();
                clear_lines();
                spawn_tetromino();
                break;
            default:
                break;
        }
    }
}

void game_loop() {
    SDL_Event e;
    bool quit = false;
    Uint32 lastMoveDown = SDL_GetTicks();
    const Uint32 moveDownInterval = 1000;

    next = tetrominoes[rand() % 7];
    spawn_tetromino();

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP) {
                handle_input(&e);
            }
        }

        if (SDL_GetTicks() - lastMoveDown >= moveDownInterval) {
            move_down();
            lastMoveDown = SDL_GetTicks();
        }

        currentTime = SDL_GetTicks() - startTime;

        render();
        SDL_Delay(16);
    }
}
