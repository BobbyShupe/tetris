#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

// Adjustable parameters
#define CELL_SIZE 15
#define GRID_WIDTH 12
#define GRID_HEIGHT 30
#define GRID_X_OFFSET 20
#define GRID_Y_OFFSET 20
#define INITIAL_FALL_DELAY 700
#define LINES_PER_LEVEL 10
#define PREVIEW_SIZE 50
#define FONT_SIZE 16

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

// Game state
int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
int current_shape, next_shape, current_rotation;
int current_x, current_y;
int score = 0;
int level = 1;
int lines_cleared = 0;
Uint32 last_fall = 0;
int fall_delay = INITIAL_FALL_DELAY;
bool quit = false;
bool game_over = false;

// Color schemes for levels (RGBA) - Bright colors only
SDL_Color color_schemes[][7] = {
    {{255,100,100,255}, {100,255,100,255}, {100,100,255,255},
    {255,255,100,255}, {255,100,255,255}, {100,255,255,255}, {200,200,200,255}},
    {{255,150,150,255}, {150,255,150,255}, {150,150,255,255},
    {255,255,150,255}, {255,150,255,255}, {150,255,255,255}, {220,220,220,255}},
    {{255,200,200,255}, {200,255,200,255}, {200,200,255,255},
    {255,255,200,255}, {255,200,255,255}, {200,255,255,255}, {240,240,240,255}},
    {{255,128,0,255}, {128,255,0,255}, {0,255,128,255},
    {128,0,255,255}, {255,0,128,255}, {128,255,128,255}, {255,255,255,255}}
};

// Tetromino shapes (7 types × 4 rotations × 4x4 blocks)
const int SHAPES[7][4][4][4] = {
    {{{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
     {{0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}},
     {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
     {{0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0}}},
    {{{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,1,0}, {0,0,0,0}}},
    {{{0,0,0,0}, {0,1,0,0}, {1,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,0,0}, {0,1,1,0}, {0,1,0,0}},
     {{0,0,0,0}, {0,0,0,0}, {1,1,1,0}, {0,1,0,0}},
     {{0,0,0,0}, {0,1,0,0}, {1,1,0,0}, {0,1,0,0}}},
    {{{0,0,0,0}, {0,0,1,0}, {1,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,1,0}},
     {{0,0,0,0}, {0,0,0,0}, {1,1,1,0}, {1,0,0,0}},
     {{0,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,1,0,0}}},
    {{{0,0,0,0}, {1,0,0,0}, {1,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,1,0}, {0,1,0,0}, {0,1,0,0}},
     {{0,0,0,0}, {0,0,0,0}, {1,1,1,0}, {0,0,1,0}},
     {{0,0,0,0}, {0,1,0,0}, {0,1,0,0}, {1,1,0,0}}},
    {{{0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,1,0}},
     {{0,0,0,0}, {0,0,0,0}, {0,1,1,0}, {1,1,0,0}},
     {{0,0,0,0}, {1,0,0,0}, {1,1,0,0}, {0,1,0,0}}},
    {{{0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0}},
     {{0,0,0,0}, {0,0,1,0}, {0,1,1,0}, {0,1,0,0}},
     {{0,0,0,0}, {0,0,0,0}, {1,1,0,0}, {0,1,1,0}},
     {{0,0,0,0}, {0,1,0,0}, {1,1,0,0}, {1,0,0,0}}},
};

// Function declarations
void draw_game();
void update_level();
void draw_text(const char* text, int x, int y, SDL_Color color);
void draw_block(int x, int y, SDL_Color color);
int check_collision(int x, int y, int rotation);
void new_piece();
void merge_piece();
void clear_lines();
void draw_preview();
void reset_game();
void handle_input();

// Helper function to invert color
SDL_Color invert_color(SDL_Color color) {
    return (SDL_Color){
        255 - color.r,
        255 - color.g,
        255 - color.b,
        color.a
    };
}

void update_level() {
    level = 1 + (lines_cleared / LINES_PER_LEVEL);
    fall_delay = INITIAL_FALL_DELAY * pow(0.9, level-1);
}

void draw_text(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        fprintf(stderr, "Text render failed: %s\n", TTF_GetError());
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_block(int x, int y, SDL_Color color) {
    SDL_Rect rect = {
        GRID_X_OFFSET + x * CELL_SIZE,
        GRID_Y_OFFSET + y * CELL_SIZE,
        CELL_SIZE - 1,
        CELL_SIZE - 1
    };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

int check_collision(int x, int y, int rotation) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (SHAPES[current_shape][rotation][i][j]) {
                int grid_x = x + j;
                int grid_y = y + i;
                if (grid_x < 0 || grid_x >= GRID_WIDTH || grid_y >= GRID_HEIGHT)
                    return 1;
                if (grid_y >= 0 && grid[grid_y][grid_x])
                    return 1;
            }
        }
    }
    return 0;
}

void new_piece() {
    current_shape = next_shape;
    next_shape = rand() % 7;
    current_rotation = 0;
    current_x = GRID_WIDTH / 2 - 2;
    current_y = -2;
    
    if (check_collision(current_x, current_y, current_rotation)) {
        game_over = true;
    }
}

void merge_piece() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (SHAPES[current_shape][current_rotation][i][j]) {
                int grid_x = current_x + j;
                int grid_y = current_y + i;
                if (grid_y >= 0 && grid_x >= 0 && grid_x < GRID_WIDTH)
                    grid[grid_y][grid_x] = current_shape + 1;
            }
        }
    }
}

void clear_lines() {
    int lines_removed = 0;
    int rows_to_clear[4] = {-1, -1, -1, -1};

    // Identify full rows
    for (int row = GRID_HEIGHT - 1; row >= 0; row--) {
        int full = 1;
        for (int col = 0; col < GRID_WIDTH; col++) {
            if (!grid[row][col]) {
                full = 0;
                break;
            }
        }
        if (full) {
            rows_to_clear[lines_removed] = row;
            lines_removed++;
        }
    }

    if (lines_removed > 0) {
        lines_cleared += lines_removed;
        score += lines_removed * 100 * level;
        update_level();

        // Clear full rows
        for (int i = 0; i < lines_removed; i++) {
            memset(grid[rows_to_clear[i]], 0, sizeof(grid[0]));
        }

        // Shift non-cleared rows down
        int temp_grid[GRID_HEIGHT][GRID_WIDTH] = {0};
        int dst = GRID_HEIGHT - 1;
        for (int src = GRID_HEIGHT - 1; src >= 0; src--) {
            bool is_cleared = false;
            for (int i = 0; i < lines_removed; i++) {
                if (src == rows_to_clear[i]) {
                    is_cleared = true;
                    break;
                }
            }
            if (!is_cleared) {
                memcpy(temp_grid[dst], grid[src], sizeof(grid[0]));
                dst--;
            }
        }

        // Copy shifted grid back
        memcpy(grid, temp_grid, sizeof(grid));
    }
}

void draw_preview() {
    int preview_x = GRID_X_OFFSET + GRID_WIDTH * CELL_SIZE + 50;
    int preview_y = GRID_Y_OFFSET + 250;
    
    SDL_Rect border = {preview_x - 10, preview_y - 10, PREVIEW_SIZE + 20, PREVIEW_SIZE + 20};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &border);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &border);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (SHAPES[next_shape][0][i][j]) {
                SDL_Rect rect = {
                    preview_x + j * CELL_SIZE + (PREVIEW_SIZE - CELL_SIZE*4)/2,
                    preview_y + i * CELL_SIZE + (PREVIEW_SIZE - CELL_SIZE*4)/2,
                    CELL_SIZE - 1,
                    CELL_SIZE - 1
                };
                SDL_Color c = color_schemes[level % 4][next_shape];
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

void draw_game() {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // Draw grid background
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            SDL_Rect rect = {
                GRID_X_OFFSET + x * CELL_SIZE,
                GRID_Y_OFFSET + y * CELL_SIZE,
                CELL_SIZE - 1,
                CELL_SIZE - 1
            };
            if (!grid[y][x]) {
                SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
                SDL_RenderFillRect(renderer, &rect);
                SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
    }

    // Draw locked pieces
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x]) {
                int shape_idx = grid[y][x] - 1;
                SDL_Color c = color_schemes[level % 4][shape_idx];
                draw_block(x, y, c);
            }
        }
    }

    // Draw current piece
    if (!game_over) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (SHAPES[current_shape][current_rotation][i][j]) {
                    int px = current_x + j;
                    int py = current_y + i;
                    if (py >= 0 && px >= 0 && px < GRID_WIDTH) {
                        SDL_Color c = color_schemes[level % 4][current_shape];
                        draw_block(px, py, c);
                    }
                }
            }
        }
    }

    // Draw UI elements
    SDL_Color text_color = {255, 255, 255, 255};
    char info_text[256];
    snprintf(info_text, sizeof(info_text), "Level: %d", level);
    draw_text(info_text, GRID_X_OFFSET + GRID_WIDTH * CELL_SIZE + 50, GRID_Y_OFFSET, text_color);
    snprintf(info_text, sizeof(info_text), "Score: %d", score);
    draw_text(info_text, GRID_X_OFFSET + GRID_WIDTH * CELL_SIZE + 50, GRID_Y_OFFSET + (FONT_SIZE + 2) * 2, text_color);
    snprintf(info_text, sizeof(info_text), "Lines: %d", lines_cleared);
    draw_text(info_text, GRID_X_OFFSET + GRID_WIDTH * CELL_SIZE + 50, GRID_Y_OFFSET + (FONT_SIZE + 2) * 4, text_color);
    draw_text("Next:", GRID_X_OFFSET + GRID_WIDTH * CELL_SIZE + 50, GRID_Y_OFFSET + 200, text_color);
    draw_preview();

    if (game_over) {
        draw_text("Game Over!", GRID_X_OFFSET + 50, GRID_Y_OFFSET + 200, (SDL_Color){255, 0, 0, 255});
        draw_text("Press R to restart", GRID_X_OFFSET + 30, GRID_Y_OFFSET + 250, text_color);
    }

    SDL_RenderPresent(renderer);
}

void reset_game() {
    memset(grid, 0, sizeof(grid));
    score = 0;
    level = 1;
    lines_cleared = 0;
    fall_delay = INITIAL_FALL_DELAY;
    game_over = false;
    next_shape = rand() % 7;
    new_piece();
}

void handle_input() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
        else if (e.type == SDL_KEYDOWN) {
            if (game_over) {
                if (e.key.keysym.sym == SDLK_r) {
                    reset_game();
                }
                continue;
            }
            
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    if (!check_collision(current_x - 1, current_y, current_rotation))
                        current_x--;
                    break;
                case SDLK_RIGHT:
                    if (!check_collision(current_x + 1, current_y, current_rotation))
                        current_x++;
                    break;
                case SDLK_UP: {
                    int new_rot = (current_rotation + 1) % 4;
                    if (!check_collision(current_x, current_y, new_rot))
                        current_rotation = new_rot;
                    break;
                }
                case SDLK_DOWN:
                    fall_delay = INITIAL_FALL_DELAY / 8;
                    break;
                case SDLK_SPACE:
                    while (!check_collision(current_x, current_y + 1, current_rotation))
                        current_y++;
                    merge_piece();
                    clear_lines();
                    new_piece();
                    break;
                case SDLK_q:
                case SDLK_ESCAPE:
                    quit = true;
                    break;
            }
        }
        else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_DOWN) {
            fall_delay = INITIAL_FALL_DELAY * pow(0.9, level-1);
        }
    }
}

int main() {
    srand(time(NULL));
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("Tetris",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        GRID_X_OFFSET * 2 + GRID_WIDTH * CELL_SIZE + 200,
        GRID_Y_OFFSET * 2 + GRID_HEIGHT * CELL_SIZE,
        SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font = TTF_OpenFont("arial.ttf", FONT_SIZE);
    if (!font) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    next_shape = rand() % 7;
    new_piece();

    while (!quit) {
        handle_input();
        
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_fall > fall_delay) {
            if (!check_collision(current_x, current_y + 1, current_rotation)) {
                current_y++;
            } else {
                merge_piece();
                clear_lines();
                new_piece();
            }
            last_fall = current_time;
        }

        draw_game();
        SDL_Delay(10);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
