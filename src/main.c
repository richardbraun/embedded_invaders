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
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "eetg.h"
#include "ei.h"

static struct termios orig_ios;

static struct ei_game game;

static void
restore_termios(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_ios);
    write(STDOUT_FILENO, "\ec", 2);
}

static void
setup_io(void)
{
    struct termios ios;

    setvbuf(stdin, NULL, _IONBF, 0);

    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

    tcgetattr(STDIN_FILENO, &orig_ios);
    atexit(restore_termios);
    ios = orig_ios;
    ios.c_lflag &= ~(ICANON | ECHO);
    ios.c_cc[VMIN] = 1;
    ios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &ios);
}

static void
write_terminal(const void *buffer, size_t size, void *arg)
{
    (void)arg;

    write(STDOUT_FILENO, buffer, size);
}

int
main(void)
{
    bool leave;

    setup_io();

    eetg_init_rand(time(NULL));

    ei_game_init(&game, write_terminal, NULL);

    do {
        ssize_t nr_bytes;
        int8_t c;

        usleep(1000000 / EI_FPS);

        nr_bytes = read(STDIN_FILENO, &c, 1);

        if (nr_bytes == -1) {
            if (errno == EAGAIN) {
                c = -1;
            } else {
                break;
            }
        }

        leave = ei_game_process(&game, c);
    } while (!leave);

    return EXIT_SUCCESS;
}
