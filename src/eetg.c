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

#include <stdio.h>

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "eetg.h"
#include "macros.h"

#define EETG_RENDERING_DISABLED 0

#define EETG_BG_COLOR EETG_COLOR_BLACK
#define EETG_FG_COLOR EETG_COLOR_WHITE

#define EETG_CSI "\e["

static unsigned int eetg_rand_next = 1;

static void
eetg_object_set(struct eetg_object *object, struct eetg_world *world,
                int x, int y)
{
    assert(object);
    assert(!object->world);
    assert(world);

    object->world = world;
    object->x = x;
    object->y = y;
}

static void
eetg_object_unset(struct eetg_object *object)
{
    assert(object);
    assert(object->world);

    object->world = NULL;
    object->next = NULL;
}

static char
eetg_object_get_char(const struct eetg_object *object, int x, int y)
{
    int index;

    index = eetg_object_get_cell(object, x, y);

    if (index == -1)
    {
        return index;
    }

    return object->sprite[index];
}

static void
eetg_object_check_collision(struct eetg_object *object1,
                            struct eetg_object *object2,
                            eetg_handle_collision_fn handle_collision_fn,
                            void *handle_collision_fn_arg)
{
    int o1xbr, o1ybr, o2xbr, o2ybr;
    int xtl, ytl, xbr, ybr;

    assert(object1);
    assert(object2);
    assert(handle_collision_fn);

    o1xbr = object1->x + object1->width - 1;
    o1ybr = object1->y + object1->height - 1;

    o2xbr = object2->x + object2->width - 1;
    o2ybr = object2->y + object2->height - 1;

    if ((object2->x > o1xbr) || (object1->x > o2xbr) ||
        (object2->y > o1ybr) || (object1->y > o2ybr)) {
        return;
    }

    xtl = (object1->x > object2->x) ? object1->x : object2->x;
    ytl = (object1->y > object2->y) ? object1->y : object2->y;
    xbr = (o1xbr < o2xbr) ? o1xbr : o2xbr;
    ybr = (o1ybr < o2ybr) ? o1ybr : o2ybr;

    assert(xtl <= xbr);
    assert(ytl <= ybr);

    for (int i = xtl; i <= xbr; i++) {
        for (int j = ytl; j <= ybr; j++) {
            char c1, c2;

            c1 = eetg_object_get_char(object1, i, j);
            c2 = eetg_object_get_char(object2, i, j);

            if ((c1 <= 0) || (c2 <= 0)) {
                return;
            } else if ((c1 == ' ') || (c2 == ' ')) {
                continue;
            }

            handle_collision_fn(object1, object2,
                                i, j, handle_collision_fn_arg);
            return;
        }
    }
}

static void
eetg_view_cell_set(struct eetg_view_cell *view_cell,
                   char c, int color)
{
    assert(view_cell);

    view_cell->c = c;
    view_cell->color = color;
}

static char
eetg_view_cell_get_c(const struct eetg_view_cell *view_cell)
{
    assert(view_cell);

    return view_cell->c;
}

static int
eetg_view_cell_get_color(const struct eetg_view_cell *view_cell)
{
    assert(view_cell);

    return view_cell->color;
}

static struct eetg_view_cell *
eetg_view_row_get_cell(struct eetg_view_row *view_row, int index)
{
    assert(view_row);
    assert(index >= 0);
    assert(index < (int)ARRAY_SIZE(view_row->columns));

    return &view_row->columns[index];
}

static void
eetg_view_row_init(struct eetg_view_row *view_row)
{
    assert(view_row);

    for (size_t i = 0; i < ARRAY_SIZE(view_row->columns); i++) {
        struct eetg_view_cell *view_cell;

        view_cell = eetg_view_row_get_cell(view_row, (int)i);
        eetg_view_cell_set(view_cell, ' ', EETG_BG_COLOR);
    }
}

static struct eetg_view_row *
eetg_view_get_row(struct eetg_view *view, int index)
{
    assert(view);
    assert(index >= 0);
    assert(index < (int)ARRAY_SIZE(view->rows));

    return &view->rows[index];
}

static void
eetg_view_init(struct eetg_view *view)
{
    assert(view);

    for (size_t i = 0; i < ARRAY_SIZE(view->rows); i++) {
        eetg_view_row_init(eetg_view_get_row(view, (int)i));
    }
}

void
eetg_world_init(struct eetg_world *world, eetg_write_fn write_fn, void *arg)
{
    assert(world);
    assert(write_fn);

    world->write_fn = write_fn;
    world->write_fn_arg = arg;

    world->handle_collision_fn = NULL;
    world->objects = NULL;

    eetg_view_init(&world->views[0]);
    eetg_view_init(&world->views[1]);

    world->view = &world->views[0];
    world->prev_view = &world->views[1];

    world->cursor_row = -1;
    world->cursor_column = -1;
    world->current_color = EETG_FG_COLOR;
}

void
eetg_world_clear(struct eetg_world *world)
{
    struct eetg_object *object;

    assert(world);

    object = world->objects;

    while (object) {
        struct eetg_object *next = object->next;

        eetg_object_unset(object);
        object = next;
    }

    world->objects = NULL;
}

void
eetg_world_register_collision_fn(struct eetg_world *world,
                                 eetg_handle_collision_fn handle_collision_fn,
                                 void *arg)
{
    assert(world);
    assert(handle_collision_fn);

    world->handle_collision_fn = handle_collision_fn;
    world->handle_collision_fn_arg = arg;
}

static void
eetg_world_scan_collisions(struct eetg_world *world, struct eetg_object *object)
{
    assert(world);

    if (!world->handle_collision_fn) {
        return;
    }

    for (struct eetg_object *tmp = world->objects; tmp; tmp = tmp->next) {
        if (tmp == object) {
            continue;
        }

        eetg_object_check_collision(object, tmp,
                                    world->handle_collision_fn,
                                    world->handle_collision_fn_arg);
    }
}

void
eetg_world_add(struct eetg_world *world, struct eetg_object *object,
               int x, int y)
{
    assert(world);
    assert(eetg_object_get_world(object) == NULL);

    object->next = world->objects;
    world->objects = object;

    eetg_object_set(object, world, x, y);
    eetg_world_scan_collisions(world, object);
}

void
eetg_world_remove(struct eetg_world *world, struct eetg_object *object)
{
    assert(world);
    assert(eetg_object_get_world(object) == world);

    if (world->objects == object) {
        world->objects = object->next;
        goto out;
    }

    for (struct eetg_object *tmp = world->objects; tmp->next; tmp = tmp->next) {
        if (tmp->next == object) {
            tmp->next = tmp->next->next;
            break;
        }
    }

out:
    eetg_object_unset(object);
}

static void
eetg_object_render_row(struct eetg_object *object, int row,
                       struct eetg_view_row *view_row)
{
    const char *line;

    assert(object);
    assert(row < object->height);

    line = &object->sprite[(object->width + 1) * row];

    for (int obj_column = 0; obj_column < object->width; obj_column++) {
        int column = object->x + obj_column;
        char c;

        if ((column < 0) || (column >= EETG_COLUMNS)) {
            continue;
        }

        c = line[obj_column];

        if (c != ' ') {
            struct eetg_view_cell *view_cell;

            view_cell = eetg_view_row_get_cell(view_row, column);
            eetg_view_cell_set(view_cell, c, object->color);
        }
    }
}

static void
eetg_object_render(struct eetg_object *object, struct eetg_view *view)
{
    assert(object);

    for (int obj_row = 0; obj_row < object->height; obj_row++) {
        int row = object->y + obj_row;

        if ((row < 0) || (row >= EETG_ROWS)) {
            continue;
        }

        eetg_object_render_row(object, obj_row, eetg_view_get_row(view, row));
    }
}

static void
eetg_world_write(struct eetg_world *world, const void *buffer, size_t size)
{
    assert(world);
    assert(world->write_fn);
    assert(buffer);

#if EETG_RENDERING_DISABLED
    (void)buffer;
    (void)size;
#else
    world->write_fn(buffer, size, world->write_fn_arg);
#endif
}

static void
eetg_world_write_str(struct eetg_world *world, const char *str)
{
    eetg_world_write(world, str, strlen(str));
}

static void
eetg_world_set_cursor(struct eetg_world *world, int row, int column)
{
    char str[32];

    assert(world);
    assert(row >= 0);
    assert(row < EETG_ROWS);
    assert(column >= 0);
    assert(column < EETG_COLUMNS);

    if ((world->cursor_row == row) && (world->cursor_column == column)) {
        return;
    }

    snprintf(str, sizeof(str), EETG_CSI "%d;%dH", row + 1, column + 1);
    eetg_world_write_str(world, str);

    world->cursor_row = row;
    world->cursor_column = column;
}

static int
eetg_convert_fg_color(int color)
{
    return color + 30;
}

static int
eetg_convert_bg_color(int color)
{
    return eetg_convert_fg_color(color) + 10;
}

static void
eetg_world_set_color(struct eetg_world *world, int color, bool force)
{
    char str[16];

    if ((color == world->current_color) && !force) {
        return;
    }

    snprintf(str, sizeof(str), EETG_CSI "%d;%dm",
             eetg_convert_fg_color(color),
             eetg_convert_bg_color(EETG_BG_COLOR));

    eetg_world_write_str(world, str);

    world->current_color = color;
}

static void
eetg_world_write_char(struct eetg_world *world, char c)
{
    eetg_world_write(world, &c, sizeof(c));

    world->cursor_column++;

    if (world->cursor_column == EETG_COLUMNS) {
        if (world->cursor_row == EETG_ROWS) {
            world->cursor_column = EETG_COLUMNS - 1;
        } else {
            world->cursor_column = 0;
            world->cursor_row++;
        }
    }
}

static void
eetg_world_swap_views(struct eetg_world *world)
{
    struct eetg_view *tmp;

    assert(world);

    tmp = world->view;
    world->view = world->prev_view;
    world->prev_view = tmp;
}

static void
eetg_world_render_sync(struct eetg_world *world)
{
    assert(world);

    eetg_world_write_str(world, EETG_CSI "?25l"); /* cursor invisible */
    eetg_world_set_color(world, EETG_FG_COLOR, true);
    eetg_world_write_str(world, EETG_CSI "2J"); /* clear screen */
    eetg_world_set_cursor(world, 0, 0);

    for (int row = 0; row < EETG_ROWS; row++) {
        struct eetg_view_row *view_row;

        view_row = eetg_view_get_row(world->view, row);

        for (int column = 0; column < EETG_COLUMNS; column++) {
            struct eetg_view_cell *view_cell;
            int color;
            char c;

            view_cell = eetg_view_row_get_cell(view_row, column);
            color = eetg_view_cell_get_color(view_cell);
            c = eetg_view_cell_get_c(view_cell);

            if (c == ' ') {
                continue;
            }

            eetg_world_set_cursor(world, row, column);
            eetg_world_set_color(world, color, false);
            eetg_world_write_char(world, c);
        }
    }
}

static void
eetg_world_render_delta(struct eetg_world *world)
{
    assert(world);

    for (int row = 0; row < EETG_ROWS; row++) {
        struct eetg_view_row *view_row, *prev_view_row;

        view_row = eetg_view_get_row(world->view, row);
        prev_view_row = eetg_view_get_row(world->prev_view, row);

        for (int column = 0; column < EETG_COLUMNS; column++) {
            struct eetg_view_cell *view_cell, *prev_view_cell;
            int color, prev_color;
            char c, prev_c;

            view_cell = eetg_view_row_get_cell(view_row, column);
            color = eetg_view_cell_get_color(view_cell);
            c = eetg_view_cell_get_c(view_cell);

            prev_view_cell = eetg_view_row_get_cell(prev_view_row, column);
            prev_color = eetg_view_cell_get_color(prev_view_cell);
            prev_c = eetg_view_cell_get_c(prev_view_cell);

            if ((color != prev_color) || (c != prev_c)) {
                eetg_world_set_cursor(world, row, column);
                eetg_world_set_color(world, color, false);
                eetg_world_write_char(world, c);
            }
        }
    }
}

void
eetg_world_render(struct eetg_world *world, bool sync)
{
    for (int row = 0; row < EETG_ROWS; row++) {
        struct eetg_view_row *view_row;

        view_row = eetg_view_get_row(world->view, row);

        for (int column = 0; column < EETG_COLUMNS; column++) {
            struct eetg_view_cell *view_cell;

            view_cell = eetg_view_row_get_cell(view_row, column);
            eetg_view_cell_set(view_cell, ' ', EETG_FG_COLOR);
        }
    }

    for (struct eetg_object *obj = world->objects; obj; obj = obj->next) {
        eetg_object_render(obj, world->view);
    }

    if (sync) {
        eetg_world_render_sync(world);
    } else {
        eetg_world_render_delta(world);
    }

    eetg_world_set_cursor(world, 0, 0);
    eetg_world_swap_views(world);
}

void
eetg_object_init(struct eetg_object *object, int type, const char *sprite)
{
    size_t width;
    char *ptr;

    assert(object);

    object->world = NULL;
    object->next = NULL;
    object->sprite = sprite;
    object->type = type;
    object->x = 0;
    object->y = 0;
    object->color = EETG_FG_COLOR;

    ptr = strchr(sprite, '\n');
    assert(ptr);

    width = ptr - sprite;
    assert(width <= INT_MAX);

    object->width = (int)width;
    object->height = 1;

    for (;;) {
        char *next;

        ptr = &ptr[1];
        next = strchr(ptr, '\n');

        if (!next) {
            break;
        }

        width = next - ptr;
        assert(width <= INT_MAX);
        assert((int)width == object->width);

        assert(object->height < INT_MAX);
        object->height++;

        ptr = next;
    }
}

void
eetg_object_set_color(struct eetg_object *object, int color)
{
    assert(object);

    object->color = color;
}

int
eetg_object_get_type(const struct eetg_object *object)
{
    assert(object);

    return object->type;
}

const char *
eetg_object_get_sprite(const struct eetg_object *object)
{
    assert(object);

    return object->sprite;
}

int
eetg_object_get_x(const struct eetg_object *object)
{
    assert(object);

    return object->x;
}

int
eetg_object_get_y(const struct eetg_object *object)
{
    assert(object);

    return object->y;
}

int
eetg_object_get_width(const struct eetg_object *object)
{
    assert(object);

    return object->width;
}

int
eetg_object_get_height(const struct eetg_object *object)
{
    assert(object);

    return object->height;
}

bool
eetg_object_is_empty(const struct eetg_object *object)
{
    assert(object);

    for (const char *ptr = object->sprite; ptr; ptr++) {
        char c = *ptr;

        if (c == '\0') {
            break;
        } else if ((c != ' ') && (c != '\n')) {
            return false;
        }
    }

    return true;
}

void
eetg_object_move(struct eetg_object *object, int x, int y)
{
    assert(object);

    object->x = x;
    object->y = y;

    if (object->world) {
        eetg_world_scan_collisions(object->world, object);
    }
}

int
eetg_object_get_cell(const struct eetg_object *object, int x, int y)
{
    assert(object);

    x -= object->x;
    y -= object->y;

    if ((x < 0) || (x >= object->width) || (y < 0) || (y >= object->height)) {
        return -1;
    }

    return ((object->width + 1) * y) + (x % object->width);
}

struct eetg_world *
eetg_object_get_world(const struct eetg_object *object)
{
    assert(object);

    return object->world;
}

void eetg_init_rand(unsigned int seed)
{
    eetg_rand_next = seed;
}

int eetg_rand(void)
{
    eetg_rand_next = (eetg_rand_next * 1103515245) + 12345;
    return (eetg_rand_next / 65536) % (EETG_RAND_MAX + 1);
}
