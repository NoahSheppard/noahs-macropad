#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define W 64
#define H 32

#define BOARD_W 10
#define BOARD_H 14

#define CELL_W 2
#define CELL_H 2

#define BOARD_BORDER_W (BOARD_W * CELL_W + 2)
#define BOARD_BORDER_H (BOARD_H * CELL_H + 2)

#define BOARD_X (W - BOARD_BORDER_W - 1)
#define BOARD_Y ((H - BOARD_BORDER_H) / 2)

#define FRAME_NS 33000000L
#define FALL_FRAMES 15

#define PIX_OFF ((char)0)
#define PIX_ON ((char)1)
#define BLOCK_GLYPH "█"

typedef struct {
    int type;
    int rot;
    int x;
    int y;
} Piece;

static char fb[H][W + 1];
static struct termios original_termios;
static int board[BOARD_H][BOARD_W];

static const int piece_blocks[7][4][4][2] = {
    {
        {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
        {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
        {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
        {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
    },
    {
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
    },
    {
        {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {1, 2}},
    },
    {
        {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
        {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
        {{0, 0}, {0, 1}, {1, 1}, {1, 2}},
    },
    {
        {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
        {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
        {{1, 0}, {0, 1}, {1, 1}, {0, 2}},
    },
    {
        {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
        {{1, 0}, {1, 1}, {0, 2}, {1, 2}},
    },
    {
        {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
        {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
        {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {1, 2}},
    },
};

static void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    printf("\x1b[?25h");
    fflush(stdout);
}

static int enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        return -1;
    }

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
        return -1;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }

    atexit(restore_terminal);
    return 0;
}

static void clear_fb(void) {
    for (int y = 0; y < H; y++) {
        memset(fb[y], PIX_OFF, W);
    }
}

static void put_px(int x, int y, char on) {
    if (x >= 0 && x < W && y >= 0 && y < H) {
        fb[y][x] = on;
    }
}

static void draw_frame(void) {
    for (int x = 0; x < W; x++) {
        put_px(x, 0, PIX_ON);
        put_px(x, H - 1, PIX_ON);
    }

    for (int y = 0; y < H; y++) {
        put_px(0, y, PIX_ON);
        put_px(W - 1, y, PIX_ON);
    }
}

static void render(void) {
    printf("\x1b[H");
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            if (fb[y][x] == PIX_ON) {
                fputs(BLOCK_GLYPH, stdout);
            } else {
                putchar(' ');
            }
        }
        putchar('\n');
    }
    fflush(stdout);
}

static int piece_collides(Piece piece) {
    for (int i = 0; i < 4; i++) {
        int bx = piece.x + piece_blocks[piece.type][piece.rot][i][0];
        int by = piece.y + piece_blocks[piece.type][piece.rot][i][1];

        if (bx < 0 || bx >= BOARD_W || by < 0 || by >= BOARD_H) {
            return 1;
        }

        if (board[by][bx] != 0) {
            return 1;
        }
    }

    return 0;
}

static int try_move(Piece *piece, int dx, int dy) {
    Piece candidate = *piece;
    candidate.x += dx;
    candidate.y += dy;

    if (!piece_collides(candidate)) {
        *piece = candidate;
        return 1;
    }

    return 0;
}

static void try_rotate(Piece *piece) {
    Piece candidate = *piece;
    candidate.rot = (candidate.rot + 1) % 4;

    if (!piece_collides(candidate)) {
        *piece = candidate;
        return;
    }

    candidate.x = piece->x - 1;
    if (!piece_collides(candidate)) {
        *piece = candidate;
        return;
    }

    candidate.x = piece->x + 1;
    if (!piece_collides(candidate)) {
        *piece = candidate;
    }
}

static void lock_piece(const Piece *piece) {
    for (int i = 0; i < 4; i++) {
        int bx = piece->x + piece_blocks[piece->type][piece->rot][i][0];
        int by = piece->y + piece_blocks[piece->type][piece->rot][i][1];

        if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
            board[by][bx] = piece->type + 1;
        }
    }
}

static int clear_lines(void) {
    int cleared = 0;

    for (int y = BOARD_H - 1; y >= 0; y--) {
        int full = 1;

        for (int x = 0; x < BOARD_W; x++) {
            if (board[y][x] == 0) {
                full = 0;
                break;
            }
        }

        if (!full) {
            continue;
        }

        cleared++;
        for (int row = y; row > 0; row--) {
            memcpy(board[row], board[row - 1], sizeof(board[row]));
        }
        memset(board[0], 0, sizeof(board[0]));
        y++;
    }

    return cleared;
}

static void spawn_piece(Piece *current, int *next_piece, int *game_over) {
    current->type = *next_piece;
    current->rot = 0;
    current->x = (BOARD_W / 2) - 2;
    current->y = 0;
    *next_piece = rand() % 7;

    if (piece_collides(*current)) {
        *game_over = 1;
    }
}

static void draw_cell(int bx, int by, char fill) {
    int px = BOARD_X + 1 + (bx * CELL_W);
    int py = BOARD_Y + 1 + (by * CELL_H);

    for (int dy = 0; dy < CELL_H; dy++) {
        for (int dx = 0; dx < CELL_W; dx++) {
            put_px(px + dx, py + dy, fill);
        }
    }
}

static void draw_board(const Piece *current) {
    int border_w = BOARD_BORDER_W;
    int border_h = BOARD_BORDER_H;

    for (int x = 0; x < border_w; x++) {
        put_px(BOARD_X + x, BOARD_Y, PIX_ON);
        put_px(BOARD_X + x, BOARD_Y + border_h - 1, PIX_ON);
    }

    for (int y = 0; y < border_h; y++) {
        put_px(BOARD_X, BOARD_Y + y, PIX_ON);
        put_px(BOARD_X + border_w - 1, BOARD_Y + y, PIX_ON);
    }

    for (int y = 0; y < BOARD_H; y++) {
        for (int x = 0; x < BOARD_W; x++) {
            if (board[y][x] != 0) {
                draw_cell(x, y, PIX_ON);
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        int bx = current->x + piece_blocks[current->type][current->rot][i][0];
        int by = current->y + piece_blocks[current->type][current->rot][i][1];

        if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
            draw_cell(bx, by, PIX_ON);
        }
    }
}

static void draw_next_piece(int next_piece, int ox, int oy) {

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 10; x++) {
            put_px(ox + x, oy + y, PIX_OFF);
        }
    }

    for (int i = 0; i < 4; i++) {
        int px = piece_blocks[next_piece][0][i][0];
        int py = piece_blocks[next_piece][0][i][1];
        int sx = ox + (px * 2);
        int sy = oy + (py * 2);
        put_px(sx, sy, PIX_ON);
        put_px(sx + 1, sy, PIX_ON);
        put_px(sx, sy + 1, PIX_ON);
        put_px(sx + 1, sy + 1, PIX_ON);
    }
}

static void draw_glyph_3x5(int ox, int oy, const char glyph[5][4]) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 3; x++) {
            if (glyph[y][x] == '1') {
                put_px(ox + x, oy + y, PIX_ON);
            }
        }
    }
}

static void draw_next_label(int ox, int oy) {
    static const char g_n[5][4] = {"101", "111", "111", "111", "101"};
    static const char g_e[5][4] = {"111", "100", "110", "100", "111"};
    static const char g_x[5][4] = {"101", "101", "010", "101", "101"};
    static const char g_t[5][4] = {"111", "010", "010", "010", "010"};
    static const char g_colon[5][4] = {"000", "010", "000", "010", "000"};

    draw_glyph_3x5(ox + 0, oy, g_n);
    draw_glyph_3x5(ox + 4, oy, g_e);
    draw_glyph_3x5(ox + 8, oy, g_x);
    draw_glyph_3x5(ox + 12, oy, g_t);
    draw_glyph_3x5(ox + 16, oy, g_colon);
}

static void draw_digit_3x5(int digit, int ox, int oy) {
    static const char digits[10][5][4] = {
        {"111", "101", "101", "101", "111"},
        {"010", "110", "010", "010", "111"},
        {"111", "001", "111", "100", "111"},
        {"111", "001", "111", "001", "111"},
        {"101", "101", "111", "001", "001"},
        {"111", "100", "111", "001", "111"},
        {"111", "100", "111", "101", "111"},
        {"111", "001", "010", "100", "100"},
        {"111", "101", "111", "101", "111"},
        {"111", "101", "111", "001", "111"},
    };

    if (digit < 0 || digit > 9) {
        return;
    }

    draw_glyph_3x5(ox, oy, digits[digit]);
}

static void draw_score_value(int score, int ox, int oy) {
    char score_buf[16];
    snprintf(score_buf, sizeof(score_buf), "%d", score);

    int x = ox;
    for (int i = 0; score_buf[i] != '\0'; i++) {
        int digit = score_buf[i] - '0';
        draw_digit_3x5(digit, x, oy);
        x += 4;
    }
}

static void draw_logo_tetris(int ox, int oy) {
    static const char *letters[6][7] = {
        {"11111", "00100", "00100", "00100", "00100", "00100", "00100"},
        {"11111", "10000", "11110", "10000", "10000", "10000", "11111"},
        {"11111", "00100", "00100", "00100", "00100", "00100", "00100"},
        {"11110", "10001", "10001", "11110", "10100", "10010", "10001"},
        {"11111", "00100", "00100", "00100", "00100", "00100", "11111"},
        {"01111", "10000", "10000", "01110", "00001", "00001", "11110"},
    };

    for (int letter = 0; letter < 6; letter++) {
        int lx = ox + (letter * 6);
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (letters[letter][y][x] == '1') {
                    put_px(lx + x, oy + y, PIX_ON);
                }
            }
        }
    }
}

static void draw_ui(int score, int lines, int next_piece, int game_over) {
    int ui_w = BOARD_X - 1;
    int panel_left = 1;
    int panel_right = ui_w - 1;

    if (panel_right <= panel_left + 2) {
        return;
    }

    for (int x = panel_left; x <= panel_right; x++) {
        put_px(x, 1, PIX_ON);
        put_px(x, 10, PIX_ON);
    }
    for (int y = 1; y <= 10; y++) {
        put_px(panel_left, y, PIX_ON);
        put_px(panel_right, y, PIX_ON);
    }

    draw_next_label(panel_left + 2, 3);
    draw_next_piece(next_piece, panel_right - 10, 3);

    for (int x = panel_left; x <= panel_right; x++) {
        put_px(x, 1, PIX_ON);
        put_px(x, 10, PIX_ON);
    }
    for (int y = 1; y <= 10; y++) {
        put_px(panel_left, y, PIX_ON);
        put_px(panel_right, y, PIX_ON);
    }

    draw_logo_tetris(panel_left + 2, 12);
    draw_score_value(score, panel_left + 2, 21);

    if (game_over) {
        for (int y = 26; y < H - 2; y++) {
            for (int x = panel_left + 1; x < panel_right; x++) {
                if (((x + y) & 1) == 0) {
                    put_px(x, y, PIX_ON);
                }
            }
        }
    }
}

static void handle_input(Piece *current, int *running, int *force_drop) {
    char c;

    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'q') {
            *running = 0;
            return;
        }

        if (c == 'a') {
            try_move(current, -1, 0);
        } else if (c == 'd') {
            try_move(current, 1, 0);
        } else if (c == 's') {
            *force_drop = 1;
        } else if (c == 'w') {
            try_rotate(current);
        } else if (c == ' ') {
            while (try_move(current, 0, 1)) {
            }
            *force_drop = 1;
        }

        if (c == '\x1b') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 &&
                read(STDIN_FILENO, &seq[1], 1) == 1 && seq[0] == '[') {
                if (seq[1] == 'A') {
                    try_rotate(current);
                }
                if (seq[1] == 'B') {
                    *force_drop = 1;
                }
                if (seq[1] == 'C') {
                    try_move(current, 1, 0);
                }
                if (seq[1] == 'D') {
                    try_move(current, -1, 0);
                }
            }
        }
    }
}

int main(void) {
    setlocale(LC_ALL, "");

    if (enable_raw_mode() == -1) {
        perror("enable_raw_mode");
        return 1;
    }

    srand((unsigned int)time(NULL));

    printf("\x1b[2J\x1b[H");
    printf("\x1b[?25l");

    int running = 1;
    int game_over = 0;
    int score = 0;
    int lines = 0;
    int fall_counter = 0;

    Piece current = {0};
    int next_piece = rand() % 7;

    spawn_piece(&current, &next_piece, &game_over);

    while (running) {
        int force_drop = 0;

        handle_input(&current, &running, &force_drop);

        if (!running) {
            break;
        }

        if (!game_over) {
            fall_counter++;

            if (force_drop || fall_counter >= FALL_FRAMES) {
                if (!try_move(&current, 0, 1)) {
                    lock_piece(&current);

                    int cleared = clear_lines();
                    if (cleared > 0) {
                        lines += cleared;
                        score += cleared * 100;
                    }

                    spawn_piece(&current, &next_piece, &game_over);
                }

                fall_counter = 0;
            }
        }

        clear_fb();
        draw_frame();
        draw_board(&current);
        draw_ui(score, lines, next_piece, game_over);
        render();

        struct timespec frame_delay = {0, FRAME_NS};
        nanosleep(&frame_delay, NULL);
    }

    return 0;
}