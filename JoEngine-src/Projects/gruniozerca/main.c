/*
** Gruniożerca Saturn — Tier 18: SCSP Sound Effects
*/

#include <jo/jo.h>

typedef enum {
    COLOR_RED   = 0,
    COLOR_BLUE  = 1,
    COLOR_GREEN = 2,
    COLOR_GRAY  = 3,
    COLOR_COUNT = 4
} Color;

typedef enum {
    STATE_TITLE     = 0,
    STATE_PLAYING   = 1,
    STATE_PAUSED    = 2,
    STATE_GAME_OVER = 3
} GameState;

GameState game_state = STATE_TITLE;

int         grunt_sprite_ids[COLOR_COUNT];
int         grunt_walk_sprite_ids[COLOR_COUNT][2];  // [color][frame]
int         carrot_sprite_ids[COLOR_COUNT];
int         ground_sprite_id;
int         heart_sprite_id;
int         heart_empty_id;
int         grunt_x, grunt_y;
int         carrot_x, carrot_y;
Color       grunt_color;
Color       carrot_color;
int         frame_tick;
int         score;
int         lives;
int         facing_right = 1;
static int  debug_mode = 0;

/* Walk animation */
static int  walk_frame = 0;        // 0=WK1, 1=WK2
static int  walk_tick = 0;         // frame counter
static int  is_moving = 0;         // czy Grunio się rusza
static int  move_timeout = 0;      // jak długo pokazać chodzenie po stopie
#define     WALK_TIMEOUT  15       // klatki tail animation (~0.25s)
#define     WALK_INTERVAL 6        // klatki na zmianę klatki (~10 FPS animacja)

/* Sound effects */
static jo_sound snd_catch;
static jo_sound snd_miss;
static jo_sound snd_color_change;
static jo_sound snd_game_over;
static int color_snd_cooldown = 0;  // anti-spam na trzymanie A/B

/* Combo scoring (NES original) */
static int combo = 0;

/* Carrot fall speed (acceleration) */
#define CARROT_START_SPEED  1
#define CARROT_MAX_SPEED    4
#define CARROT_ACCEL        1
static int carrot_fall_speed = CARROT_START_SPEED;

/* Grunio movement speed (acceleration) */
#define GRUNT_START_SPEED   2
#define GRUNT_MAX_SPEED     4
static int grunt_move_speed = GRUNT_START_SPEED;
const int   CARROT_W     = 16;
const int   CARROT_H     = 16;
const int   GROUND_H     = 48;
const int   GROUND_Y     = 205;

#define SPRITE_W  24
#define SPRITE_H  17

/* Pseudo-random (fast LCG) */
static unsigned int seed = 12345;
static unsigned int next_rand(void)
{
    seed = seed * 1103515245 + 12345;
    return (seed >> 16) & 0x7FFF;
}

static const char *color_names[COLOR_COUNT] = { "RED", "BLU", "GRN", "GRY" };

/* Combo scoring table: index 0-8, 8 = cap */
static const int scoring_combo[9] = { 10, 50, 100, 200, 500, 1000, 2000, 5000, 5000 };

/* VDP2 jo_color for each color (carrot squares + HUD indicator) */
static jo_color color_vdp2[COLOR_COUNT] = {
    JO_COLOR_RGB(255, 0, 0),     /* Red */
    JO_COLOR_RGB(0, 68, 255),    /* Blue */
    JO_COLOR_RGB(0, 204, 0),     /* Green */
    JO_COLOR_RGB(136, 136, 136), /* Gray */
};

static void respawn_carrot(void);
static void reset_game(void);
static void draw_background_elements(void);

static void draw_background_elements(void)
{
    // Gwiazdki (30 losowych białych/szarych kropel 2x2)
    for (int i = 0; i < 30; i++) {
        int x = next_rand() % 320;
        int y = next_rand() % 220;
        jo_color c;
        int r = next_rand() % 3;
        if (r == 0)      c = JO_COLOR_RGB(255, 255, 255);
        else if (r == 1) c = JO_COLOR_RGB(180, 180, 180);
        else             c = JO_COLOR_RGB(100, 100, 100);
        jo_draw_background_square(x, y, 2, 2, c);
    }

    // Czerwone krzyżyki (10 sztuk, losowe pozycje)
    for (int i = 0; i < 10; i++) {
        int x = next_rand() % 300;
        int y = next_rand() % 190;
        jo_color red = JO_COLOR_RGB(255, 0, 0);
        jo_draw_background_square(x, y, 6, 2, red);   // poziomo
        jo_draw_background_square(x + 2, y + 2, 2, 6, red); // pionowo
    }
}

static void respawn_carrot(void)
{
    carrot_x = (next_rand() % (320 - CARROT_W));
    carrot_y = -32;
    carrot_color = (Color)(next_rand() % COLOR_COUNT);
}

/* Score popups (Tier 18.2 — VDP1 sprite font, zero ghosting) */
/* Uses jo_font_printf which renders each char as VDP1 sprite.
 * Sprites re-render every frame and disappear when not drawn = no ghosting. */
#define MAX_POPUPS 8
typedef struct {
    int pixel_x;      // X in pixel coords
    int pixel_y;      // Y in pixel coords
    int lifetime;     // frames remaining
    int points;       // score value to display
} ScorePopup;

static ScorePopup popups[MAX_POPUPS];
static int popup_count = 0;
static jo_font *popup_font;  // loaded in jo_main

static void spawn_popup(int pixel_x, int pixel_y, int points) {
    if (popup_count >= MAX_POPUPS) return;
    popups[popup_count].pixel_x = pixel_x;
    popups[popup_count].pixel_y = pixel_y - 16;  // 16px above carrot
    popups[popup_count].lifetime = 40;  // ~0.67s
    popups[popup_count].points = points;
    popup_count++;
}

static void draw_color_indicator(Color c, int x, int y)
{
    jo_draw_background_square(x, y, 8, 8, color_vdp2[c]);
}

static void update_and_draw_popups(void) {
    char buf[12];
    for (int i = 0; i < popup_count; i++) {
        if (popups[i].lifetime > 0) {
            popups[i].pixel_y -= 1;  // float up 1px per frame
            if (popups[i].pixel_y < 0) popups[i].pixel_y = 0;
            sprintf(buf, "+%d", popups[i].points);
            jo_font_print(popup_font, popups[i].pixel_x,
                          popups[i].pixel_y, 1.0f, buf);
            popups[i].lifetime--;
        }
    }
    // Compact: remove expired popups
    int write = 0;
    for (int i = 0; i < popup_count; i++) {
        if (popups[i].lifetime > 0) {
            if (write != i) popups[write] = popups[i];
            write++;
        }
    }
    popup_count = write;
}

static void reset_game(void)
{
    grunt_x      = 148;
    grunt_y      = GROUND_Y - SPRITE_H;
    grunt_color  = COLOR_RED;
    score        = 0;
    lives        = 3;
    frame_tick   = 0;
    facing_right = 1;
    combo        = 0;
    carrot_fall_speed = CARROT_START_SPEED;
    grunt_move_speed  = GRUNT_START_SPEED;
    walk_frame        = 0;
    walk_tick         = 0;
    is_moving         = 0;
    move_timeout      = 0;
    color_snd_cooldown = 0;
    popup_count = 0;
    game_state   = STATE_PLAYING;
    respawn_carrot();
    jo_clear_screen();
}

static void draw_title_screen(void)
{
    jo_printf(8, 10, "GRUNIOZERCA");
    jo_printf(10, 15, "PRESS START TO PLAY");

    if (jo_is_pad1_key_down(JO_KEY_START) || jo_is_pad1_key_down(JO_KEY_A)) {
        reset_game();
    }
}

static void draw_pause_screen(void)
{
    jo_printf(16, 12, "PAUSE");

    if (jo_is_pad1_key_pressed(JO_KEY_START)) {
        jo_clear_screen();
        game_state = STATE_PLAYING;
    }
}

static void draw_game_over(void)
{
    jo_printf(15, 10, "GAME  OVER");
    jo_printf(12, 12, "SCORE: %07d", score);
    jo_printf(10, 15, "PRESS START TO PLAY");

    if (jo_is_pad1_key_down(JO_KEY_START) || jo_is_pad1_key_down(JO_KEY_A)) {
        reset_game();
    }
}

void        my_draw(void)
{
    if (game_state == STATE_TITLE) {
        draw_title_screen();
        return;
    }

    if (game_state == STATE_GAME_OVER) {
        draw_game_over();
        return;
    }

    if (game_state == STATE_PAUSED) {
        draw_pause_screen();
        return;
    }

    // --- STATE_PLAYING ---

    // Pause toggle — only during gameplay
    if (jo_is_pad1_key_pressed(JO_KEY_START)) {
        game_state = STATE_PAUSED;
        return;
    }

    // Debug toggle — only during gameplay
    if (jo_is_pad1_key_pressed(JO_KEY_X)) {
        debug_mode = !debug_mode;
    }

    frame_tick++;

    // Movement + walk animation detection
    int moved = 0;
    if (jo_is_pad1_key_pressed(JO_KEY_LEFT))  { grunt_x -= grunt_move_speed; facing_right = 0; moved = 1; }
    if (jo_is_pad1_key_pressed(JO_KEY_RIGHT)) { grunt_x += grunt_move_speed; facing_right = 1; moved = 1; }
    if (grunt_x < 0)                        grunt_x = 0;
    if (grunt_x > (320 - SPRITE_W))         grunt_x = (320 - SPRITE_W);

    if (moved) move_timeout = WALK_TIMEOUT;
    if (move_timeout > 0) move_timeout--;
    is_moving = (move_timeout > 0);

    // Walk frame toggle
    if (is_moving) {
        walk_tick++;
        if (walk_tick >= WALK_INTERVAL) {
            walk_tick = 0;
            walk_frame = 1 - walk_frame;  // ping-pong 0↔1
        }
    } else {
        walk_frame = 0;  // reset do klatki 0, żeby następny ruch startował od początku
        walk_tick = 0;
    }

    if (jo_is_pad1_key_down(JO_KEY_A)) {
        grunt_color = (Color)((grunt_color + 1) % COLOR_COUNT);
        if (color_snd_cooldown <= 0) {
            jo_audio_play_sound(&snd_color_change);
            color_snd_cooldown = 10;
        }
    }
    if (jo_is_pad1_key_down(JO_KEY_B)) {
        int prev = grunt_color - 1;
        if (prev < 0) prev = COLOR_COUNT - 1;
        grunt_color = (Color)prev;
        if (color_snd_cooldown <= 0) {
            jo_audio_play_sound(&snd_color_change);
            color_snd_cooldown = 10;
        }
    }
    if (color_snd_cooldown > 0) color_snd_cooldown--;

    // Carrot physics — variable fall speed
    if (frame_tick % 2 == 0) {
        carrot_y += carrot_fall_speed;
    }

    if (carrot_x < grunt_x + SPRITE_W &&
        carrot_x + CARROT_W > grunt_x &&
        carrot_y < grunt_y + SPRITE_H &&
        carrot_y + CARROT_H > grunt_y) {
        if (carrot_color == grunt_color) {
            // Correct color — combo++
            combo++;
            if (combo > 8) combo = 8;
            score += scoring_combo[combo - 1];

            // Accelerate fall speed
            if (carrot_fall_speed < CARROT_MAX_SPEED)
                carrot_fall_speed += CARROT_ACCEL;

            // Accelerate Grunio movement speed
            if (grunt_move_speed < GRUNT_MAX_SPEED)
                grunt_move_speed++;

            // Play catch sound
            jo_audio_play_sound(&snd_catch);

            // Spawn score popup
            spawn_popup(carrot_x, carrot_y - 16, scoring_combo[combo - 1]);
        } else {
            // Wrong color — reset combo, lose life, reset speed
            combo = 0;
            lives--;
            if (lives < 0) lives = 0;
            carrot_fall_speed = CARROT_START_SPEED;
            grunt_move_speed  = GRUNT_START_SPEED;

            // Play miss sound
            jo_audio_play_sound(&snd_miss);

            if (lives == 0) {
                game_state = STATE_GAME_OVER;
                jo_audio_play_sound(&snd_game_over);
            }
        }
        respawn_carrot();
    }
    else if (carrot_y > GROUND_Y) {
        // Missed — reset combo, lose life, reset speed
        combo = 0;
        lives--;
        if (lives < 0) lives = 0;
        carrot_fall_speed = CARROT_START_SPEED;
        grunt_move_speed  = GRUNT_START_SPEED;

        // Play miss sound
        jo_audio_play_sound(&snd_miss);

        if (lives == 0) {
            game_state = STATE_GAME_OVER;
            jo_audio_play_sound(&snd_game_over);
        }
        respawn_carrot();
    }

    jo_sprite_disable_horizontal_flip();
    jo_sprite_draw3D2(ground_sprite_id, 0, GROUND_Y, 400);

    jo_sprite_draw3D2(carrot_sprite_ids[carrot_color], carrot_x, carrot_y, 500);

    if (facing_right) {
        jo_sprite_disable_horizontal_flip();
    } else {
        jo_sprite_enable_horizontal_flip();
    }
    if (is_moving) {
        jo_sprite_draw3D2(grunt_walk_sprite_ids[grunt_color][walk_frame], grunt_x, grunt_y, 500);
    } else {
        jo_sprite_draw3D2(grunt_sprite_ids[grunt_color], grunt_x, grunt_y, 500);
    }

    if (debug_mode) {
        draw_color_indicator(grunt_color, 4, 10);
        jo_printf(14, 8, "%s", color_names[grunt_color]);
    }

    jo_printf(0, 0, "%07d", score);

    jo_printf(300, 0, "L:%d", lives);
    jo_sprite_disable_horizontal_flip();
    for (int i = 0; i < 3; i++) {
        int id = (i < lives) ? heart_sprite_id : heart_empty_id;
        jo_sprite_draw3D2(id, 280 + (i * 12), 2, 900);
    }

    // Draw score popups
    update_and_draw_popups();
}

void        jo_main(void)
{
    jo_core_init(JO_COLOR_Black);

    draw_background_elements();

    ground_sprite_id = jo_sprite_add_tga("TEX", "GROUND.TGA", JO_COLOR_Transparent);

    grunt_sprite_ids[COLOR_RED]   = jo_sprite_add_tga("TEX", "GRUN_RED.TGA", JO_COLOR_Transparent);
    grunt_sprite_ids[COLOR_BLUE]  = jo_sprite_add_tga("TEX", "GRUN_BLU.TGA", JO_COLOR_Transparent);
    grunt_sprite_ids[COLOR_GREEN] = jo_sprite_add_tga("TEX", "GRUN_GRN.TGA", JO_COLOR_Transparent);
    grunt_sprite_ids[COLOR_GRAY]  = jo_sprite_add_tga("TEX", "GRUN_GRY.TGA", JO_COLOR_Transparent);

    // Walk sprites
    grunt_walk_sprite_ids[COLOR_RED][0]   = jo_sprite_add_tga("TEX", "GRW_RED1.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_RED][1]   = jo_sprite_add_tga("TEX", "GRW_RED2.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_BLUE][0]  = jo_sprite_add_tga("TEX", "GRW_BLU1.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_BLUE][1]  = jo_sprite_add_tga("TEX", "GRW_BLU2.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_GREEN][0] = jo_sprite_add_tga("TEX", "GRW_GRN1.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_GREEN][1] = jo_sprite_add_tga("TEX", "GRW_GRN2.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_GRAY][0]  = jo_sprite_add_tga("TEX", "GRW_GRY1.TGA", JO_COLOR_Transparent);
    grunt_walk_sprite_ids[COLOR_GRAY][1]  = jo_sprite_add_tga("TEX", "GRW_GRY2.TGA", JO_COLOR_Transparent);

    carrot_sprite_ids[COLOR_RED]   = jo_sprite_add_tga("TEX", "CART_RED.TGA", JO_COLOR_Transparent);
    carrot_sprite_ids[COLOR_BLUE]  = jo_sprite_add_tga("TEX", "CART_BLU.TGA", JO_COLOR_Transparent);
    carrot_sprite_ids[COLOR_GREEN] = jo_sprite_add_tga("TEX", "CART_GRN.TGA", JO_COLOR_Transparent);
    carrot_sprite_ids[COLOR_GRAY]  = jo_sprite_add_tga("TEX", "CART_GRY.TGA", JO_COLOR_Transparent);

    heart_sprite_id  = jo_sprite_add_tga("TEX", "HEART.TGA",     JO_COLOR_Transparent);
    heart_empty_id   = jo_sprite_add_tga("TEX", "HRTEMPTY.TGA",  JO_COLOR_Transparent);

    // Load popup font (VDP1 sprite font — zero ghosting)
    popup_font = jo_font_load("TEX", "PFONT.TGA", JO_COLOR_Transparent, 8, 8, 0,
                              " 0123456789+");

    // Load sound effects (init volume to 0 first so JoEngine skips them if load fails)
    snd_catch.volume = JO_MIN_AUDIO_VOLUME;
    snd_miss.volume = JO_MIN_AUDIO_VOLUME;
    snd_color_change.volume = JO_MIN_AUDIO_VOLUME;
    snd_game_over.volume = JO_MIN_AUDIO_VOLUME;

    if (jo_audio_load_pcm("CATCH.PCM", JoSoundMono16Bit, &snd_catch)) {
        snd_catch.sample_rate = 32000;
        snd_catch.volume = 100;
    }
    if (jo_audio_load_pcm("MISS.PCM", JoSoundMono16Bit, &snd_miss)) {
        snd_miss.sample_rate = 32000;
        snd_miss.volume = 100;
    }
    if (jo_audio_load_pcm("CHGCOL.PCM", JoSoundMono16Bit, &snd_color_change)) {
        snd_color_change.sample_rate = 32000;
        snd_color_change.volume = 80;
    }
    if (jo_audio_load_pcm("GAMEOVR.PCM", JoSoundMono16Bit, &snd_game_over)) {
        snd_game_over.sample_rate = 32000;
        snd_game_over.volume = 100;
    }

    game_state = STATE_TITLE;

    jo_core_add_callback(my_draw);
    jo_core_run();
}
