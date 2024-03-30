/*
 * Copyright (c) 2024 Richard Braun.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * Embedded invaders.
 */

#ifndef EI_H
#define EI_H

#include <stdbool.h>
#include <stdint.h>

#include "eetg.h"

#define EI_FPS 50

#define EI_NR_ALIEN_GROUPS 5
#define EI_ALIEN_GROUP_SIZE 10
#define EI_ALIEN_WIDTH 3

#define EI_STATE_INTRO      0
#define EI_STATE_PREPARED   1
#define EI_STATE_PLAYING    2
#define EI_STATE_GAME_OVER  3

struct ei_bunker {
    struct eetg_object object;
    char sprite[33];
};

struct ei_alien {
    struct eetg_object object;
};

struct ei_alien_group {
    struct ei_alien aliens[EI_ALIEN_GROUP_SIZE];
    const char *sprites[2];
    char sprite[EI_ALIEN_WIDTH + 2];
    int8_t sprite_index;
};

struct ei_game {
    struct eetg_world world;
    struct eetg_object title;
    struct eetg_object help;
    struct eetg_object start;
    struct eetg_object player;
    struct eetg_object player_missile;
    struct ei_bunker bunkers[4];
    struct ei_alien_group aliens[EI_NR_ALIEN_GROUPS];
    struct eetg_object alien_missile;
    struct eetg_object ufo;
    struct eetg_object status;
    struct eetg_object end_title;
    int score;
    int8_t sync_counter_reload;
    int8_t sync_counter;
    int8_t nr_lives;
    int8_t player_missile_counter_reload;
    int8_t player_missile_counter;
    int8_t aliens_speed_counter_reload;
    int8_t aliens_speed_counter;
    int8_t first_alien_missile_counter;
    int8_t alien_missile_counter_reload;
    int8_t alien_missile_counter;
    int8_t ufo_counter_reload;
    int8_t ufo_counter;
    int8_t nr_dead_aliens;
    int8_t state;
    bool aliens_move_left;
    bool aliens_move_down;
    bool ufo_moves_left;
    char status_sprite[32];
};

void ei_game_init(struct ei_game *game, eetg_write_fn write_fn, void *arg);
bool ei_game_process(struct ei_game *game, int8_t c);

#endif /* EI_H */
