#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 800;
const int BLOCK_SIZE = 40;
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

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
int board[BOARD_HEIGHT][BOARD_WIDTH] = {0};
Tetromino current;

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

int main(int argc, char* args[]) {
    srand((unsigned int)time(NULL));

    if (!init()) {
        printf("Failed to initialize!\n");
    } else {
        Mix_Chunk* backgroundSound = NULL;
        if (argc >= 2) {
            backgroundSound = load_audio(args[1]);
            if (backgroundSound == NULL) {
                printf("Failed to load audio!\n");
                return 1;
            }
        }

        game_loop();

        if (backgroundSound != NULL) {
            close_audio(backgroundSound);
        }
    }

    close();
    return 0;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

// version
  SDL_version version;
  SDL_version compiledVersion;
  SDL_GetVersion(&version);
  SDL_VERSION(&compiledVersion);
  printf(
    "Tetris PS5 - Running with SDL %d.%d.%d (compiled with %d.%d.%d)\n",
    version.major, version.minor, version.patch, compiledVersion.major, compiledVersion.minor, compiledVersion.patch
  );


    gWindow = SDL_CreateWindow("Joystick Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (gWindow == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_PRESENTVSYNC);
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

    return true;
}

void close() {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    if (gGameController != NULL) {
        SDL_GameControllerClose(gGameController);
    }
    gRenderer = NULL;
    gWindow = NULL;
    gGameController = NULL;
    SDL_Quit();
}

void render() {

    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);


    SDL_SetRenderDrawColor(gRenderer, 0, 255, 0, 255);
    
    int borderThickness = 2;
    int borderTop = (BOARD_HEIGHT - 1) * BLOCK_SIZE - borderThickness;
    int borderBottom = SCREEN_HEIGHT;
    int borderLeft = borderThickness;
    int borderRight = BOARD_WIDTH * BLOCK_SIZE + borderThickness;
    
    SDL_Rect topBorder = { 0, 0, SCREEN_WIDTH, borderThickness };
    SDL_Rect bottomBorder = { 0, borderBottom - borderThickness, SCREEN_WIDTH, borderThickness };
    SDL_RenderFillRect(gRenderer, &topBorder);
    SDL_RenderFillRect(gRenderer, &bottomBorder);
    
    SDL_Rect leftBorder = { 0, borderThickness, borderThickness, borderTop - borderThickness + BLOCK_SIZE };
    SDL_Rect rightBorder = { borderRight - borderThickness, borderThickness, borderThickness, borderTop - borderThickness + BLOCK_SIZE };
    SDL_RenderFillRect(gRenderer, &leftBorder);
    SDL_RenderFillRect(gRenderer, &rightBorder);

    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j]) {
                SDL_Rect fillRect = { j * BLOCK_SIZE, i * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
                SDL_RenderFillRect(gRenderer, &fillRect);
            }
        }
    }

    SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect fillRect = { (current.blocks[i].x + current.x) * BLOCK_SIZE, (current.blocks[i].y + current.y) * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
        SDL_RenderFillRect(gRenderer, &fillRect);
    }

    SDL_RenderPresent(gRenderer);
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
        }
    }
}

void spawn_tetromino() {
    current = tetrominoes[rand() % 7];
    current.x = BOARD_WIDTH / 2 - 2;
    current.y = 0;
    current.rotation = 0;
    if (check_collision(0, 0)) {

        for (int i = 0; i < BOARD_HEIGHT; ++i) {
            for (int j = 0; j < BOARD_WIDTH; ++j) {
                board[i][j] = 0;
            }
        }

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

    if (check_collision(0, 0)) {

        int dx = 0;
        while (check_collision(dx, 0)) {
            if (dx > 0) {
                dx = -dx - 1;
            } else {
                dx = -dx + 1;
            }
        }
        current.x += dx;
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

        render();
        SDL_Delay(16);
    }
}
