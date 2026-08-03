#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <conio.h>
#include <windows.h>

#define PI 3.14159265358979323846f

// --- Map ---
#define MAP_WIDTH  8
#define MAP_HEIGHT 8

int map[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,1,0,1,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,0,1},
    {1,1,1,1,1,1,1,1}
};

// --- Player ---
typedef struct {
    float x, y;
    float angle;
} Player;

// --- Enemies ---
typedef struct {
    float x, y;
    int alive;
} Enemy;

#define NUM_ENEMIES 3
Enemy enemies[NUM_ENEMIES] = {
    { 2.5f, 1.5f, 1 },
    { 5.5f, 1.5f, 1 },
    { 1.5f, 5.5f, 1 }
};

// --- Screen settings ---
#define SCREEN_WIDTH  79
#define SCREEN_HEIGHT 18
#define FOV 1.0472f
#define MAX_DEPTH 16.0f

// --- Movement settings ---
#define MOVE_SPEED 0.1f
#define ROTATE_SPEED 0.08f

// --- Shooting settings ---
#define SHOOT_RANGE 6.0f
#define SHOOT_CONE  0.15f   // radians - how narrow the "aim" is

char screen[SCREEN_HEIGHT][SCREEN_WIDTH];
WORD colorGrid[SCREEN_HEIGHT][SCREEN_WIDTH];
float zbuffer[SCREEN_WIDTH];

// Casts a ray and reports both the distance to the wall hit AND
// which "type" of wall face it hit (0 = a north/south-facing wall,
// 1 = an east/west-facing wall).
float cast_ray(float x, float y, float angle, int *side_out) {
    float step_size = 0.05f;
    float distance = 0.0f;
    float ray_x = x;
    float ray_y = y;

    while (distance < MAX_DEPTH) {
        ray_x += cosf(angle) * step_size;
        ray_y += sinf(angle) * step_size;
        distance += step_size;

        int map_x = (int)ray_x;
        int map_y = (int)ray_y;

        if (map_x < 0 || map_x >= MAP_WIDTH || map_y < 0 || map_y >= MAP_HEIGHT) {
            *side_out = 0;
            return distance;
        }
        if (map[map_y][map_x] == 1) {
            float frac_x = ray_x - floorf(ray_x);
            float frac_y = ray_y - floorf(ray_y);
            float edge_x = fminf(frac_x, 1.0f - frac_x);
            float edge_y = fminf(frac_y, 1.0f - frac_y);
            *side_out = (edge_x < edge_y) ? 0 : 1;
            return distance;
        }
    }

    *side_out = 0;
    return distance;
}

WORD get_wall_color(float dist, int side) {
    WORD color = side == 0 ? FOREGROUND_GREEN : (FOREGROUND_GREEN | FOREGROUND_BLUE);
    if (dist < MAX_DEPTH / 3.0f) {
        color |= FOREGROUND_INTENSITY;
    }
    return color;
}

int is_walkable(float x, float y) {
    int map_x = (int)x;
    int map_y = (int)y;
    if (map_x < 0 || map_x >= MAP_WIDTH || map_y < 0 || map_y >= MAP_HEIGHT) {
        return 0;
    }
    return map[map_y][map_x] == 0;
}

// Normalizes an angle to the range -PI..PI
float normalize_angle(float a) {
    while (a > PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}

// Renders walls + floor + ceiling into screen/colorGrid, and fills
// zbuffer with the wall distance for each column (used later for
// enemy occlusion).
void render_walls(Player player) {
    const char *shades = " .:-=+*#%@";
    int num_shades = 9;

    for (int col = 0; col < SCREEN_WIDTH; col++) {
        float ray_angle = (player.angle - FOV / 2.0f) +
                           ((float)col / (float)SCREEN_WIDTH) * FOV;

        int side;
        float dist = cast_ray(player.x, player.y, ray_angle, &side);
        zbuffer[col] = dist;

        int wall_height = (int)(SCREEN_HEIGHT / (dist + 0.0001f));
        if (wall_height > SCREEN_HEIGHT) wall_height = SCREEN_HEIGHT;

        int ceiling = (SCREEN_HEIGHT / 2) - (wall_height / 2);
        int floor   = SCREEN_HEIGHT - ceiling;

        int shade_index = (int)((1.0f - (dist / MAX_DEPTH)) * (num_shades - 1));
        if (shade_index < 0) shade_index = 0;
        if (shade_index >= num_shades) shade_index = num_shades - 1;
        char wall_char = shades[shade_index];
        WORD wall_color = get_wall_color(dist, side);

        for (int row = 0; row < SCREEN_HEIGHT; row++) {
            if (row < ceiling) {
                screen[row][col] = ' ';
                colorGrid[row][col] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            } else if (row <= floor) {
                screen[row][col] = wall_char;
                colorGrid[row][col] = wall_color;
            } else {
                screen[row][col] = '.';
                colorGrid[row][col] = FOREGROUND_RED | FOREGROUND_GREEN;
            }
        }
    }
}

// Draws alive enemies on top of the wall grid, using distance and
// the zbuffer to keep them from showing through walls in front of them.
void render_enemies(Player player) {
    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (!enemies[i].alive) continue;

        float dx = enemies[i].x - player.x;
        float dy = enemies[i].y - player.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 0.2f || dist > MAX_DEPTH) continue;

        float angle_to_enemy = atan2f(dy, dx);
        float rel_angle = normalize_angle(angle_to_enemy - player.angle);

        if (fabsf(rel_angle) > FOV / 2.0f) continue; // outside field of view

        int screen_x = (int)((0.5f + rel_angle / FOV) * SCREEN_WIDTH);

        int sprite_height = (int)(SCREEN_HEIGHT / (dist + 0.0001f));
        if (sprite_height > SCREEN_HEIGHT) sprite_height = SCREEN_HEIGHT;
        int sprite_width = sprite_height / 2;
        if (sprite_width < 1) sprite_width = 1;

        int top = (SCREEN_HEIGHT / 2) - (sprite_height / 2);
        int bottom = top + sprite_height;
        int left = screen_x - sprite_width / 2;
        int right = left + sprite_width;

        for (int col = left; col < right; col++) {
            if (col < 0 || col >= SCREEN_WIDTH) continue;
            if (dist >= zbuffer[col]) continue; // wall is closer, enemy is hidden

            for (int row = top; row < bottom; row++) {
                if (row < 0 || row >= SCREEN_HEIGHT) continue;
                screen[row][col] = '#';
                colorGrid[row][col] = FOREGROUND_RED | FOREGROUND_INTENSITY;
            }
        }
    }
}

// Draws the built screen/colorGrid to the actual console.
void draw_to_console(HANDLE console) {
    WORD current_color = (WORD)0xFFFF;
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            if (colorGrid[row][col] != current_color) {
                SetConsoleTextAttribute(console, colorGrid[row][col]);
                current_color = colorGrid[row][col];
            }
            putchar(screen[row][col]);
        }
        putchar('\n');
    }
}

// Fires a shot: finds the nearest alive enemy within the aim cone
// and shooting range, kills it if found.
void shoot(Player player) {
    int target = -1;
    float best_dist = SHOOT_RANGE + 1.0f;

    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (!enemies[i].alive) continue;

        float dx = enemies[i].x - player.x;
        float dy = enemies[i].y - player.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > SHOOT_RANGE) continue;

        float angle_to_enemy = atan2f(dy, dx);
        float rel_angle = normalize_angle(angle_to_enemy - player.angle);

        if (fabsf(rel_angle) <= SHOOT_CONE && dist < best_dist) {
            best_dist = dist;
            target = i;
        }
    }

    if (target != -1) {
        enemies[target].alive = 0;
        Beep(880, 150); // hit sound
    } else {
        Beep(220, 100); // miss sound
    }
}

void handle_input(Player *player) {
    if (!_kbhit()) {
        return;
    }

    char key = _getch();
    float new_x = player->x;
    float new_y = player->y;

    switch (key) {
        case 'w': case 'W':
            new_x += cosf(player->angle) * MOVE_SPEED;
            new_y += sinf(player->angle) * MOVE_SPEED;
            break;
        case 's': case 'S':
            new_x -= cosf(player->angle) * MOVE_SPEED;
            new_y -= sinf(player->angle) * MOVE_SPEED;
            break;
        case 'a': case 'A':
            player->angle -= ROTATE_SPEED;
            break;
        case 'd': case 'D':
            player->angle += ROTATE_SPEED;
            break;
        case ' ':
            shoot(*player);
            break;
        case 'q': case 'Q':
            exit(0);
            break;
        default:
            break;
    }

    if (is_walkable(new_x, new_y)) {
        player->x = new_x;
        player->y = new_y;
    }
}

void move_cursor_home(void) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD topLeft = { 0, 0 };
    SetConsoleCursorPosition(console, topLeft);
}

void hide_cursor(void) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(console, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(console, &cursorInfo);
}

int main(void) {
    Player player = { 3.5f, 3.5f, 0.0f };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

    system("cls");
    hide_cursor();

    while (1) {
        handle_input(&player);

        render_walls(player);
        render_enemies(player);

        move_cursor_home();
        draw_to_console(console);

        Sleep(30);
    }

    return 0;
}