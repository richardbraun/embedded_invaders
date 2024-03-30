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
 * Engine for embedded text-based games.
 */

#ifndef EETG_H
#define EETG_H

#include <stdbool.h>
#include <stdint.h>

#define EETG_COLUMNS 80
#define EETG_ROWS    24

#define EETG_RAND_MAX 32767

#define EETG_COLOR_BLACK    0
#define EETG_COLOR_RED      1
#define EETG_COLOR_GREEN    2
#define EETG_COLOR_YELLOW   3
#define EETG_COLOR_BLUE     4
#define EETG_COLOR_MAGENTA  5
#define EETG_COLOR_CYAN     6
#define EETG_COLOR_WHITE    7

struct eetg_world;

struct eetg_object;

typedef void (*eetg_write_fn)(const void *buffer, size_t size, void *arg);

typedef void (*eetg_handle_collision_fn)(struct eetg_object *object1,
                                         struct eetg_object *object2,
                                         int x, int y, void *arg);

struct eetg_object {
    struct eetg_world *world;
    struct eetg_object *next;
    const char *sprite;
    int8_t color;
    int8_t type;
    int8_t x;
    int8_t y;
    int8_t width;
    int8_t height;
};

struct eetg_view_cell {
    char c;
    int8_t color;
};

struct eetg_view_row {
    struct eetg_view_cell columns[EETG_COLUMNS];
};

struct eetg_view {
    struct eetg_view_row rows[EETG_ROWS];
};

struct eetg_world {
    eetg_write_fn write_fn;
    void *write_fn_arg;
    eetg_handle_collision_fn handle_collision_fn;
    void *handle_collision_fn_arg;
    struct eetg_object *objects;
    struct eetg_view views[2];
    struct eetg_view *view;
    struct eetg_view *prev_view;
    int8_t cursor_row;
    int8_t cursor_column;
    int8_t current_color;
};

void eetg_world_init(struct eetg_world *world,
                     eetg_write_fn write_fn, void *arg);
void eetg_world_clear(struct eetg_world *world);
void eetg_world_register_collision_fn(struct eetg_world *world,
         eetg_handle_collision_fn handle_collision_fn, void *arg);
void eetg_world_add(struct eetg_world *world, struct eetg_object *object,
                    int x, int y);
void eetg_world_remove(struct eetg_world *world, struct eetg_object *object);
void eetg_world_render(struct eetg_world *world, bool sync);

void eetg_object_init(struct eetg_object *object, int type, const char *sprite);
void eetg_object_set_color(struct eetg_object *object, int color);
int eetg_object_get_type(const struct eetg_object *object);
const char *eetg_object_get_sprite(const struct eetg_object *object);
int eetg_object_get_x(const struct eetg_object *object);
int eetg_object_get_y(const struct eetg_object *object);
int eetg_object_get_width(const struct eetg_object *object);
int eetg_object_get_height(const struct eetg_object *object);
bool eetg_object_is_empty(const struct eetg_object *object);
void eetg_object_move(struct eetg_object *object, int x, int y);
int eetg_object_get_cell(const struct eetg_object *object, int x, int y);
struct eetg_world *eetg_object_get_world(const struct eetg_object *object);

void eetg_init_rand(unsigned int seed);
int eetg_rand(void);

#endif /* EETG_H */
