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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eetg.h"
#include "ei.h"
#include "macros.h"

#define EI_NR_LIVES 3

#define EI_ALIEN_STARTING_ROW 3

#define EI_PLAYER_MISSILE_SPEED 25
#define EI_ALIENS_SPEED 10
#define EI_ALIEN_MISSILE_SPEED 10
#define EI_FIRST_ALIEN_MISSILE_DELAY 2
#define EI_UFO_SPEED 10

#define EI_SCORE_MISSILE 40
#define EI_SCORE_ALIENS0 30
#define EI_SCORE_ALIENS12 20
#define EI_SCORE_ALIENS34 10
#define EI_SCORE_UFO_BASE 100

#define EI_TITLE_SPRITE                                                     \
" _____                                                   _____ \n"         \
"( ___ )-------------------------------------------------( ___ )\n"         \
" |   |                                                   |   | \n"         \
" |   |        ____      __          __   __       __     |   | \n"         \
" |   |       / __/_ _  / /  ___ ___/ /__/ /__ ___/ /     |   | \n"         \
" |   |      / _//  ' \\/ _ \\/ -_) _  / _  / -_) _  /      |   | \n"       \
" |   |     /___/_/_/_/_.__/\\__/\\_,_/\\_,_/\\__/\\_,_/       |   | \n"    \
" |   |           _                  __                   |   | \n"         \
" |   |          (_)__ _  _____ ____/ /__ _______         |   | \n"         \
" |   |         / / _ \\ |/ / _ `/ _  / -_) __(_-<         |   | \n"        \
" |   |        /_/_//_/___/\\_,_/\\_,_/\\__/_/ /___/         |   | \n"      \
" |   |                                                   |   | \n"         \
" |___|                                                   |___| \n"         \
"(_____)-------------------------------------------------(_____)\n"

#define EI_HELP_SPRITE      \
"       s = left     \n"    \
"       f = right    \n"    \
"   space = shoot    \n"

#define EI_START_SPRITE     \
"Press SPACE to start\n"    \
"Press X to leave    \n"

#define EI_PLAYER_SPRITE "/-^-\\\n"

#define EI_BUNKER_SPRITE    \
"  ###  \n"                 \
" ##### \n"                 \
"#######\n"                 \
"##   ##\n"

#define EI_ALIENS0_SPRITE_1     ",^,\n"
#define EI_ALIENS0_SPRITE_2     ".-.\n"
#define EI_ALIENS12_SPRITE_1    "-O_\n"
#define EI_ALIENS12_SPRITE_2    "_O-\n"
#define EI_ALIENS34_SPRITE_1    "/^\\\n"
#define EI_ALIENS34_SPRITE_2    "-^-\n"

#define EI_STATUS_SPRITE_FORMAT "SCORE: %08d   Lives: %d\n"

#define EI_END_TITLE_SPRITE                                 \
"  ________   __  _______         ____ _   _________ \n"    \
" / ___/ _ | /  |/  / __/        / __ \\ | / / __/ _ \\\n"  \
"/ (_ / __ |/ /|_/ / _/         / /_/ / |/ / _// , _/\n"    \
"\\___/_/ |_/_/  /_/___/         \\____/|___/___/_/|_| \n"

#define EI_TYPE_TITLE           0
#define EI_TYPE_HELP            1
#define EI_TYPE_START           2
#define EI_TYPE_PLAYER          3
#define EI_TYPE_PLAYER_MISSILE  4
#define EI_TYPE_BUNKER          5
#define EI_TYPE_ALIEN           6
#define EI_TYPE_ALIEN_MISSILE   7
#define EI_TYPE_UFO             8
#define EI_TYPE_STATUS          9
#define EI_TYPE_END_TITLE       10

static const char *
ei_get_group_sprite1(size_t group)
{
    const char *sprite;

    if (group == 0) {
        sprite = EI_ALIENS0_SPRITE_1;
    } else if ((group == 1) || (group == 2)) {
        sprite = EI_ALIENS12_SPRITE_1;
    } else {
        sprite = EI_ALIENS34_SPRITE_1;
    }

    return sprite;
}

static const char *
ei_get_group_sprite2(size_t group)
{
    const char *sprite;

    if (group == 0) {
        sprite = EI_ALIENS0_SPRITE_2;
    } else if ((group == 1) || (group == 2)) {
        sprite = EI_ALIENS12_SPRITE_2;
    } else {
        sprite = EI_ALIENS34_SPRITE_2;
    }

    return sprite;
}

static int
ei_get_group_color(size_t group)
{
    int color;

    if (group == 0) {
        color = EETG_COLOR_RED;
    } else if ((group == 1) || (group == 2)) {
        color = EETG_COLOR_GREEN;
    } else {
        color = EETG_COLOR_BLUE;
    }

    return color;
}

static struct eetg_object *
ei_get_object(struct eetg_object *object1,
              struct eetg_object *object2,
              int type)
{
    struct eetg_object *object = NULL;

    assert(object1);
    assert(object2);

    if (eetg_object_get_type(object1) == type) {
        object = object1;
    } else if (eetg_object_get_type(object2) == type) {
        object = object2;
    }

    return object;
}

static bool
ei_has_type(const struct eetg_object *object1,
            const struct eetg_object *object2,
            int type)
{
    return (eetg_object_get_type(object1) == type)
           || (eetg_object_get_type(object2) == type);
}

static void
ei_bunker_reset_sprite(struct ei_bunker *bunker)
{
    assert(bunker);

    snprintf(bunker->sprite, sizeof(bunker->sprite), "%s", EI_BUNKER_SPRITE);
}

static void
ei_bunker_init(struct ei_bunker *bunker)
{
    assert(bunker);

    ei_bunker_reset_sprite(bunker);

    eetg_object_init(&bunker->object, EI_TYPE_BUNKER, bunker->sprite);
    eetg_object_set_color(&bunker->object, EETG_COLOR_CYAN);
}

static struct ei_bunker *
ei_bunker_get(struct eetg_object *object)
{
    assert(eetg_object_get_type(object) == EI_TYPE_BUNKER);

    return structof(object, struct ei_bunker, object);
}

static struct eetg_object *
ei_bunker_get_object(struct ei_bunker *bunker)
{
    assert(bunker);

    return &bunker->object;
}

static bool
ei_bunker_damage(struct ei_bunker *bunker, int x, int y)
{
    char *sprite;
    int index;

    assert(bunker);

    index = eetg_object_get_cell(&bunker->object, x, y);

    sprite = (char *)eetg_object_get_sprite(&bunker->object);

    sprite[index] = ' ';

    return eetg_object_is_empty(&bunker->object);
}

static void
ei_alien_init(struct ei_alien *alien, const char *sprite, int color)
{
    assert(alien);

    eetg_object_init(&alien->object, EI_TYPE_ALIEN, sprite);
    eetg_object_set_color(&alien->object, color);
}

static struct ei_alien *
ei_alien_get(struct eetg_object *object)
{
    assert(eetg_object_get_type(object) == EI_TYPE_ALIEN);

    return structof(object, struct ei_alien, object);
}

static struct eetg_object *
ei_alien_get_object(struct ei_alien *alien)
{
    assert(alien);

    return &alien->object;
}

static bool
ei_alien_is_dead(const struct ei_alien *alien)
{
    assert(alien);

    return (eetg_object_get_world(&alien->object) == NULL);
}

static int
ei_alien_get_x(const struct ei_alien *alien)
{
    assert(alien);

    return eetg_object_get_x(&alien->object);
}

static int
ei_alien_get_width(const struct ei_alien *alien)
{
    assert(alien);

    return eetg_object_get_width(&alien->object);
}

static bool
ei_alien_move_down(struct ei_alien *alien)
{
    struct eetg_object *object;
    bool game_over = false;
    int y;

    assert(alien);

    object = &alien->object;

    y = eetg_object_get_y(object) + 1;

    eetg_object_move(object, eetg_object_get_x(object), y);

    if (y >= (EETG_ROWS - 1)) {
        game_over = true;
    }

    return game_over;
}

static void
ei_alien_move_left(struct ei_alien *alien)
{
    struct eetg_object *object;

    assert(alien);

    object = &alien->object;

    eetg_object_move(object, eetg_object_get_x(object) - 1,
                     eetg_object_get_y(object));
}

static void
ei_alien_move_right(struct ei_alien *alien)
{
    struct eetg_object *object;

    assert(alien);

    object = &alien->object;

    eetg_object_move(object, eetg_object_get_x(object) + 1,
                     eetg_object_get_y(object));
}

static void
ei_alien_group_init(struct ei_alien_group *group, const char *sprite1,
                    const char *sprite2, int color)
{
    assert(group);
    assert(sprite1);
    assert(strlen(sprite1) == (EI_ALIEN_WIDTH + 1));
    assert(sprite2);
    assert(strlen(sprite2) == (EI_ALIEN_WIDTH + 1));

    group->sprites[0] = sprite1;
    group->sprites[1] = sprite2;
    group->sprite_index = 0;

    memcpy(group->sprite, group->sprites[group->sprite_index],
           sizeof(group->sprite));

    for (size_t i = 0; i < ARRAY_SIZE(group->aliens); i++) {
        ei_alien_init(&group->aliens[i], group->sprite, color);
    }
}

static void
ei_alien_group_attach(struct ei_alien_group *group,
                      struct eetg_world *world, int y)
{
    int x = 0;

    assert(group);

    for (size_t i = 0; i < ARRAY_SIZE(group->aliens); i++) {
        struct ei_alien *alien = &group->aliens[i];

        eetg_world_add(world, ei_alien_get_object(alien), x, y);

        x += EI_ALIEN_WIDTH;
    }
}

static bool
ei_alien_group_has_alien(const struct ei_alien_group *group,
                         const struct ei_alien *alien)
{
    bool alien_present = false;
    const struct ei_alien *end;

    assert(group);

    end = &group->aliens[ARRAY_SIZE(group->aliens)];

    if ((alien >= group->aliens) && (alien < end)) {
        alien_present = true;
    }

    return alien_present;
}

static struct ei_alien *
ei_alien_group_get(struct ei_alien_group *group, size_t index)
{
    assert(group);
    assert(index < ARRAY_SIZE(group->aliens));

    return &group->aliens[index];
}

static void
ei_alien_group_twerk(struct ei_alien_group *group)
{
    assert(group);

    group->sprite_index = (group->sprite_index + 1) & 1;
    memcpy(group->sprite, group->sprites[group->sprite_index],
           sizeof(group->sprite));
}

static bool
ei_alien_group_move_down(struct ei_alien_group *group)
{
    bool game_over = false;

    ei_alien_group_twerk(group);

    for (size_t i = 0; i < ARRAY_SIZE(group->aliens); i++) {
        struct ei_alien *alien = &group->aliens[i];

        if (!ei_alien_is_dead(alien)) {
            bool tmp;

            tmp = ei_alien_move_down(alien);

            if (tmp) {
                game_over = true;
            }
        }
    }

    return game_over;
}

static bool
ei_alien_group_move_left(struct ei_alien_group *group)
{
    bool border_reached = false;

    ei_alien_group_twerk(group);

    for (size_t i = 0; i < ARRAY_SIZE(group->aliens); i++) {
        struct ei_alien *alien = &group->aliens[i];

        if (!ei_alien_is_dead(alien)) {
            ei_alien_move_left(alien);

            if (!border_reached && (ei_alien_get_x(alien) == 0)) {
                border_reached = true;
            }
        }
    }

    return border_reached;
}

static bool
ei_alien_group_move_right(struct ei_alien_group *group)
{
    bool border_reached = false;

    ei_alien_group_twerk(group);

    for (size_t i = ARRAY_SIZE(group->aliens) - 1;
         i < ARRAY_SIZE(group->aliens); i--) {
        struct ei_alien *alien = &group->aliens[i];

        if (!ei_alien_is_dead(alien)) {
            ei_alien_move_right(alien);

            if (!border_reached) {
                int x;

                x = ei_alien_get_x(alien) + ei_alien_get_width(alien) - 1;

                if (x == (EETG_COLUMNS - 1)) {
                    border_reached = true;
                }
            }
        }
    }

    return border_reached;
}

static struct ei_alien_group *
ei_game_find_alien_group(struct ei_game *game, struct ei_alien *alien)
{
    struct ei_alien_group *group = NULL;

    assert(alien);

    for (size_t i = 0; i < ARRAY_SIZE(game->aliens); i++) {
        struct ei_alien_group *tmp = &game->aliens[i];

        if (ei_alien_group_has_alien(tmp, alien)) {
            group = tmp;
            break;
        }
    }

    return group;
}

static void
ei_game_add_bunkers(struct ei_game *game)
{
    int x = 6;

    assert(game);

    for (size_t i = 0; i < ARRAY_SIZE(game->bunkers); i++) {
        struct ei_bunker *bunker = &game->bunkers[i];

        ei_bunker_reset_sprite(bunker);
        eetg_world_add(&game->world, ei_bunker_get_object(bunker), x, 17);

        x += 20;
    }
}

static void
ei_game_add_aliens(struct ei_game *game)
{
    assert(game);

    for (size_t i = 0; i < ARRAY_SIZE(game->aliens); i++) {
        struct ei_alien_group *group = &game->aliens[i];

        ei_alien_group_attach(group, &game->world,
                              EI_ALIEN_STARTING_ROW + (i * 2));
    }
}

static void
ei_game_update_status(struct ei_game *game)
{
    assert(game);

    snprintf(game->status_sprite, sizeof(game->status_sprite),
             EI_STATUS_SPRITE_FORMAT, game->score, game->nr_lives);
}

static void
ei_game_prepare(struct ei_game *game)
{
    assert(game);

    eetg_world_clear(&game->world);

    game->state = EI_STATE_PREPARED;
}

static void
ei_game_start(struct ei_game *game)
{
    assert(game);

    eetg_world_clear(&game->world);

    eetg_world_add(&game->world, &game->player, 37, 23);

    ei_game_add_bunkers(game);
    ei_game_add_aliens(game);

    eetg_world_add(&game->world, &game->status, 26, 0);

    game->player_missile_counter_reload = EI_FPS / EI_PLAYER_MISSILE_SPEED;
    game->player_missile_counter = game->player_missile_counter_reload;
    game->aliens_speed_counter_reload = EI_FPS / EI_ALIENS_SPEED;
    game->aliens_speed_counter = game->aliens_speed_counter_reload;
    game->first_alien_missile_counter = EI_FPS * EI_FIRST_ALIEN_MISSILE_DELAY;
    game->alien_missile_counter_reload = EI_FPS / EI_ALIEN_MISSILE_SPEED;
    game->ufo_counter_reload = EI_FPS / EI_UFO_SPEED;
    game->nr_dead_aliens = 0;

    game->aliens_move_left = false;
    game->aliens_move_down = false;

    game->state = EI_STATE_PLAYING;
}

static void
ei_game_kill_alien(struct ei_game *game, struct ei_alien *alien)
{
    int group, score;

    assert(alien);

    group = ei_game_find_alien_group(game, alien) - game->aliens;

    if (group == 0) {
        score = EI_SCORE_ALIENS0;
    } else if ((group == 1) || (group == 2)) {
        score = EI_SCORE_ALIENS12;
    } else {
        score = EI_SCORE_ALIENS34;
    }

    game->score += score;

    ei_game_update_status(game);

    eetg_world_remove(&game->world, ei_alien_get_object(alien));

    game->nr_dead_aliens++;

    if (game->nr_dead_aliens == (EI_NR_ALIEN_GROUPS * EI_ALIEN_GROUP_SIZE)) {
        ei_game_prepare(game);
    } else {
        int aliens_speed;

        aliens_speed = game->nr_dead_aliens / 2;

        if (aliens_speed < (EI_FPS / 5)) {
            aliens_speed = EI_FPS / 5;
        } else if (aliens_speed > ((EI_FPS * 4) / 5)) {
            aliens_speed = ((EI_FPS * 4) / 5);
        }

        game->aliens_speed_counter_reload = EI_FPS / aliens_speed;
    }
}

static void
ei_game_damage_bunker(struct ei_game *game, struct ei_bunker *bunker,
                      int x, int y)
{
    bool destroyed;

    destroyed = ei_bunker_damage(bunker, x, y);

    if (destroyed) {
        eetg_world_remove(&game->world, ei_bunker_get_object(bunker));
    }
}

static void
ei_game_terminate(struct ei_game *game)
{
    assert(game);

    eetg_world_clear(&game->world);

    eetg_world_add(&game->world, &game->end_title, 12, 10);
    eetg_world_add(&game->world, &game->status, 26, 6);
    eetg_world_add(&game->world, &game->start, 30, 20);

    game->state = EI_STATE_GAME_OVER;
}

static void
ei_game_kill_player(struct ei_game *game, bool game_over)
{
    assert(game);
    assert(game->nr_lives > 0);

    game->nr_lives--;

    ei_game_update_status(game);

    if (game_over || (game->nr_lives == 0))
    {
        ei_game_terminate(game);
    }
}

static void
ei_game_handle_player_missile_collision(struct ei_game *game,
                                        struct eetg_object *missile,
                                        struct eetg_object *object,
                                        int x, int y)
{
    assert(game);

    eetg_world_remove(&game->world, missile);

    if (eetg_object_get_type(object) == EI_TYPE_BUNKER) {
        ei_game_damage_bunker(game, ei_bunker_get(object), x, y);
    } else if (eetg_object_get_type(object) == EI_TYPE_ALIEN_MISSILE) {
        game->score += EI_SCORE_MISSILE;
        eetg_world_remove(&game->world, object);
    } else if (eetg_object_get_type(object) == EI_TYPE_ALIEN) {
        ei_game_kill_alien(game, ei_alien_get(object));
    } else if (eetg_object_get_type(object) == EI_TYPE_UFO) {
        game->score += EI_SCORE_UFO_BASE * ((eetg_rand() % 5) + 1);
        eetg_world_remove(&game->world, object);
    }
}

static void
ei_game_handle_alien_collision(struct ei_game *game,
                               struct eetg_object *object,
                               int x, int y)
{
    if (eetg_object_get_type(object) == EI_TYPE_BUNKER) {
        ei_game_damage_bunker(game, ei_bunker_get(object), x, y);
    } else if (eetg_object_get_type(object) == EI_TYPE_PLAYER) {
        ei_game_kill_player(game, true);
    }
}

static void
ei_game_handle_alien_missile_collision(struct ei_game *game,
                                       struct eetg_object *missile,
                                       struct eetg_object *object,
                                       int x, int y)
{
    assert(game);

    if (eetg_object_get_type(object) == EI_TYPE_ALIEN) {
        return;
    }

    eetg_world_remove(&game->world, missile);

    if (eetg_object_get_type(object) == EI_TYPE_BUNKER) {
        ei_game_damage_bunker(game, ei_bunker_get(object), x, y);
    } else if (eetg_object_get_type(object) == EI_TYPE_PLAYER) {
        ei_game_kill_player(game, false);
    }
}

static void
ei_game_handle_collision(struct eetg_object *object1,
                         struct eetg_object *object2,
                         int x, int y, void *arg)
{
    if (ei_has_type(object1, object2, EI_TYPE_PLAYER_MISSILE)) {
        struct eetg_object *missile, *other;

        missile = ei_get_object(object1, object2, EI_TYPE_PLAYER_MISSILE);
        other = (object1 == missile) ? object2 : object1;

        ei_game_handle_player_missile_collision(arg, missile, other, x, y);
    } else if (ei_has_type(object1, object2, EI_TYPE_ALIEN)) {
        struct eetg_object *alien, *other;

        alien = ei_get_object(object1, object2, EI_TYPE_ALIEN);
        other = (object1 == alien) ? object2 : object1;

        ei_game_handle_alien_collision(arg, other, x, y);
    } else if (ei_has_type(object1, object2, EI_TYPE_ALIEN_MISSILE)) {
        struct eetg_object *missile, *other;

        missile = ei_get_object(object1, object2, EI_TYPE_ALIEN_MISSILE);
        other = (object1 == missile) ? object2 : object1;

        ei_game_handle_alien_missile_collision(arg, missile, other, x, y);
    }
}

static void
ei_game_reset_history(struct ei_game *game)
{
    assert(game);

    game->score = 0;
    game->nr_lives = EI_NR_LIVES;

    ei_game_update_status(game);
}

static bool
ei_game_process_intro_input(struct ei_game *game, char c)
{
    assert(game);

    if (c == 'x') {
        return true;
    }

    if (c != ' ') {
        return false;
    }

    ei_game_reset_history(game);
    ei_game_prepare(game);

    return false;
}

static bool
ei_game_process_game_input(struct ei_game *game, char c)
{
    assert(game);

    if (c == 'x') {
        return true;
    }

    if (c == 's') {
        struct eetg_object *player_object = &game->player;
        int x;

        x = eetg_object_get_x(player_object);

        if (x > 0) {
            eetg_object_move(player_object, x - 1,
                             eetg_object_get_y(player_object));
        }
    } else if (c == 'f') {
        struct eetg_object *player_object = &game->player;
        int x, width;

        x = eetg_object_get_x(player_object);
        width = eetg_object_get_width(player_object);

        if ((x + width) < EETG_COLUMNS) {
            eetg_object_move(player_object, x + 1,
                             eetg_object_get_y(player_object));
        }
    } else if (c == ' ') {
        struct eetg_object *player_missile = &game->player_missile;

        if (eetg_object_get_world(player_missile) == NULL) {
            struct eetg_object *player_object = &game->player;
            int x, y;

            x = eetg_object_get_x(player_object);
            y = eetg_object_get_y(player_object);

            eetg_world_add(&game->world, player_missile, x + 2, y - 1);

            game->player_missile_counter = game->player_missile_counter_reload;
        }
    }

    return false;
}

static void
ei_game_process_player_missile(struct ei_game *game)
{
    int x, y;

    assert(game);

    if (eetg_object_get_world(&game->player_missile) == NULL) {
        return;
    }

    assert(game->player_missile_counter > 0);
    game->player_missile_counter--;

    if (game->player_missile_counter != 0) {
        return;
    }

    game->player_missile_counter = game->player_missile_counter_reload;

    x = eetg_object_get_x(&game->player_missile);
    y = eetg_object_get_y(&game->player_missile) - 1;

    if (y == 0) {
        eetg_world_remove(&game->world, &game->player_missile);
    } else {
        eetg_object_move(&game->player_missile, x, y);
    }
}

static struct ei_alien *
ei_game_select_firing_alien(struct ei_game *game)
{
    struct ei_alien *alien = NULL;
    int nr_firing_aliens = 0;

    assert(game);

    for (size_t i = 0; i < EI_ALIEN_GROUP_SIZE; i++) {
        for (size_t j = 0; j < ARRAY_SIZE(game->aliens); j++) {
            struct ei_alien *tmp = ei_alien_group_get(&game->aliens[j], i);

            if (!ei_alien_is_dead(tmp)) {
                nr_firing_aliens++;
                break;
            }
        }
    }

    if (nr_firing_aliens > 0) {
        int index;

        index = eetg_rand() % nr_firing_aliens;

        for (size_t i = ARRAY_SIZE(game->aliens) - 1;
             i < ARRAY_SIZE(game->aliens); i--) {
            struct ei_alien *tmp = ei_alien_group_get(&game->aliens[i], index);

            if (!ei_alien_is_dead(tmp)) {
                alien = tmp;
                break;
            }
        }
    }

    return alien;
}

static void
ei_game_process_alien_missile(struct ei_game *game)
{
    assert(game);

    if (game->first_alien_missile_counter != 0) {
        assert(game->first_alien_missile_counter > 0);
        game->first_alien_missile_counter--;
        return;
    }

    if (eetg_object_get_world(&game->alien_missile) != NULL) {
        assert(game->alien_missile_counter > 0);
        game->alien_missile_counter--;

        if (game->alien_missile_counter == 0) {
            int x, y;

            game->alien_missile_counter = game->alien_missile_counter_reload;

            x = eetg_object_get_x(&game->alien_missile);
            y = eetg_object_get_y(&game->alien_missile);

            if (y == EETG_ROWS) {
                eetg_world_remove(&game->world, &game->alien_missile);
            } else {
                eetg_object_move(&game->alien_missile, x, y + 1);
            }
        }
    } else {
        struct ei_alien *alien;
        int x, y;

        alien = ei_game_select_firing_alien(game);

        if (alien) {
            x = eetg_object_get_x(&alien->object);
            y = eetg_object_get_y(&alien->object);

            eetg_world_add(&game->world, &game->alien_missile, x, y + 1);

            game->alien_missile_counter = game->alien_missile_counter_reload;
        }
    }
}

static void
ei_game_process_aliens(struct ei_game *game)
{
    assert(game);
    assert(game->aliens_speed_counter > 0);

    game->aliens_speed_counter--;

    if (game->aliens_speed_counter != 0) {
        return;
    }

    game->aliens_speed_counter = game->aliens_speed_counter_reload;

    if (game->aliens_move_down) {
        for (size_t i = ARRAY_SIZE(game->aliens) - 1;
             i < ARRAY_SIZE(game->aliens); i--) {
            bool game_over;

            game_over = ei_alien_group_move_down(&game->aliens[i]);

            if (game_over) {
                ei_game_terminate(game);
            }
        }

        game->aliens_move_down = false;

        if (eetg_object_get_world(&game->ufo) == NULL) {
            int n;

            n = eetg_rand() % 3;

            if (n == 0) {
                int x;

                n = eetg_rand() % 2;

                if (n == 0) {
                    x = EETG_COLUMNS;
                    game->ufo_moves_left = true;
                } else {
                    x = -eetg_object_get_width(&game->ufo);
                    game->ufo_moves_left = false;
                }

                eetg_world_add(&game->world, &game->ufo, x, 2);

                game->ufo_counter = game->ufo_counter_reload;
            }
        }
    } else {
        bool border_reached = false;

        for (size_t i = 0; i < ARRAY_SIZE(game->aliens); i++) {
            struct ei_alien_group *group = &game->aliens[i];
            bool border_reached_by_group;

            if (game->aliens_move_left) {
                border_reached_by_group = ei_alien_group_move_left(group);
            } else {
                border_reached_by_group = ei_alien_group_move_right(group);
            }

            if (border_reached_by_group) {
                border_reached = true;
            }
        }

        if (border_reached) {
            game->aliens_move_down = true;
            game->aliens_move_left = !game->aliens_move_left;
        }
    }
}

static void
ei_game_process_ufo(struct ei_game *game)
{
    struct eetg_object *ufo;

    assert(game);

    ufo = &game->ufo;

    if (eetg_object_get_world(ufo) == NULL) {
        return;
    }

    assert(game->ufo_counter > 0);

    game->ufo_counter--;

    if (game->ufo_counter != 0) {
        return;
    }

    game->ufo_counter = game->ufo_counter_reload;

    if (game->ufo_moves_left) {
        int x;

        x = eetg_object_get_x(ufo) - 1;

        if ((x + eetg_object_get_width(ufo)) <= 0) {
            eetg_world_remove(&game->world, ufo);
        } else {
            eetg_object_move(ufo, x, eetg_object_get_y(ufo));
        }
    } else {
        int x;

        x = eetg_object_get_x(ufo) + 1;

        if (x >= EETG_COLUMNS) {
            eetg_world_remove(&game->world, ufo);
        } else {
            eetg_object_move(ufo, x, eetg_object_get_y(ufo));
        }
    }
}

static void
ei_game_init_bunkers(struct ei_game *game)
{
    assert(game);

    for (size_t i = 0; i < ARRAY_SIZE(game->bunkers); i++) {
        ei_bunker_init(&game->bunkers[i]);
    }
}

static void
ei_game_init_aliens(struct ei_game *game)
{
    assert(game);

    for (size_t i = 0; i < ARRAY_SIZE(game->aliens); i++) {
        struct ei_alien_group *group = &game->aliens[i];
        const char *sprite1, *sprite2;
        int color;

        sprite1 = ei_get_group_sprite1(i);
        sprite2 = ei_get_group_sprite2(i);
        color = ei_get_group_color(i);

        ei_alien_group_init(group, sprite1, sprite2, color);
    }
}

void
ei_game_init(struct ei_game *game, eetg_write_fn write_fn, void *arg)
{
    assert(game);

    game->sync_counter_reload = EI_FPS * 2;
    game->sync_counter = 1;

    game->state = EI_STATE_INTRO;

    ei_game_reset_history(game);

    eetg_world_init(&game->world, write_fn, arg);
    eetg_world_register_collision_fn(&game->world,
                                     ei_game_handle_collision,
                                     game);

    eetg_object_init(&game->title, EI_TYPE_TITLE, EI_TITLE_SPRITE);
    eetg_object_set_color(&game->title, EETG_COLOR_BLUE);

    eetg_object_init(&game->help, EI_TYPE_HELP, EI_HELP_SPRITE);
    eetg_object_set_color(&game->help, EETG_COLOR_RED);

    eetg_object_init(&game->start, EI_TYPE_START, EI_START_SPRITE);
    eetg_object_set_color(&game->start, EETG_COLOR_RED);

    eetg_object_init(&game->player, EI_TYPE_PLAYER, EI_PLAYER_SPRITE);
    eetg_object_set_color(&game->player, EETG_COLOR_YELLOW);

    eetg_object_init(&game->player_missile, EI_TYPE_PLAYER_MISSILE, "!\n");
    eetg_object_set_color(&game->player_missile, EETG_COLOR_WHITE);

    ei_game_init_bunkers(game);
    ei_game_init_aliens(game);

    eetg_object_init(&game->alien_missile, EI_TYPE_ALIEN_MISSILE, ":\n");
    eetg_object_set_color(&game->alien_missile, EETG_COLOR_MAGENTA);

    eetg_object_init(&game->ufo, EI_TYPE_UFO, "<o~o>\n");
    eetg_object_set_color(&game->ufo, EETG_COLOR_MAGENTA);

    eetg_object_init(&game->status, EI_TYPE_STATUS, game->status_sprite);
    eetg_object_set_color(&game->status, EETG_COLOR_RED);

    eetg_object_init(&game->end_title, EI_TYPE_END_TITLE, EI_END_TITLE_SPRITE);
    eetg_object_set_color(&game->end_title, EETG_COLOR_WHITE);

    eetg_world_add(&game->world, &game->title, 8, 1);
    eetg_world_add(&game->world, &game->help, 30, 16);
    eetg_world_add(&game->world, &game->start, 30, 20);
}

bool
ei_game_process(struct ei_game *game, int8_t c)
{
    bool sync = false;
    bool leave = false;

    game->sync_counter--;

    if (game->sync_counter == 0) {
        sync = true;
        game->sync_counter = game->sync_counter_reload;
    }

    eetg_world_render(&game->world, sync);

    switch (game->state) {
    case EI_STATE_INTRO:
    case EI_STATE_GAME_OVER:
        if (c >= 0) {
            leave = ei_game_process_intro_input(game, (char)c);
        }

        break;
    case EI_STATE_PREPARED:
        ei_game_start(game);
        break;
    case EI_STATE_PLAYING:
        ei_game_process_player_missile(game);
        ei_game_process_aliens(game);
        ei_game_process_ufo(game);
        ei_game_process_alien_missile(game);

        if (c >= 0) {
            leave = ei_game_process_game_input(game, (char)c);
        }

        break;
    }

    return leave;
}
