#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define BUFFER_SIZE (1024 * 4)
#define BLOCK_SIZE 128

enum result {
    RESULT_OK,
    RESULT_ERR,
};

struct const_vec {
    const char* const data;
    const int32_t size;
};
struct vec {
    char* data;
    int32_t size;
};
struct node {
    struct vec data;
    struct node* prev;
    struct node* next;
};
struct cursor {
    struct node* node;
    int32_t i;
    int32_t x;
    int32_t y;
};

static char block_data[BUFFER_SIZE][BLOCK_SIZE];
static char term_input_data[BUFFER_SIZE];
static struct vec term_input_vec;
static char term_output_data[BUFFER_SIZE];
static struct vec term_output_vec;
static struct node node_data[BUFFER_SIZE];
static struct node* node_free;
static struct cursor cursor_main;
static struct cursor cursor_cmd;
static struct termios term_orig;
static int32_t tick_count;
static bool isescape;

struct const_vec const_vec_make(const char* data, int32_t size) {
    return (struct const_vec){.data = data, .size = size};
}
struct const_vec vec_to_const_vec(struct vec* src) {
    return (struct const_vec){.data = src->data, .size = src->size};
}
struct vec vec_make(char* data, int32_t size) {
    return (struct vec){.data = data, .size = size};
}
void node_release(struct node* node) {
    node->prev = node_free;
    node_free->next = node;
    node_free = node;
}
struct node* node_allocate() {
    struct node* node = node_free;
    node_free = node_free->prev;
    return node;
}
void node_delete(struct node* node) {
    struct node* next = node->next;
    struct node* prev = node->prev;
    node_release(node);
    if (next != NULL) {
        next->prev = prev;
    }
    if (prev != NULL) {
        prev->next = next;
    }
}
struct node* node_insert(struct node* next) {
    struct node* node = node_allocate();
    struct node* prev = next->prev;
    node->next = next;
    node->prev = prev;
    next->prev = node;
    if (prev != NULL) {
        prev->next = node;
    }
    return node;
}
enum result term_update() {
    term_input_vec.size = read(STDIN_FILENO, term_input_vec.data, BUFFER_SIZE);
    if (term_input_vec.size == -1) {
        printf("err: read\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result term_deinit() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term_orig) < 0) {
        printf("err: tcsetattr\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result term_init() {
    if (tcgetattr(STDIN_FILENO, &term_orig) == -1) {
        printf("err: tcgetattr\n\r");
        return RESULT_ERR;
    }
    struct termios termios_new = term_orig;
    termios_new.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    termios_new.c_oflag &= ~(OPOST);
    termios_new.c_cflag |= (CS8);
    termios_new.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    termios_new.c_cc[VMIN] = 0;
    termios_new.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &termios_new) < 0) {
        printf("err: tcsetattr\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result input_ch(struct const_vec src) {
    if (src.data[0] == 'q') {
        isescape = true;
        return RESULT_OK;
    }
    if (src.size == strlen("🤔") && memcmp(src.data, "🤔", strlen("🤔")) == 0) {
        printf("err: testerr\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result input_update() {
    char* ch_data[BUFFER_SIZE];
    int32_t i = 0;
    while (i < term_input_vec.size) {
        enum result result;
        if ((term_input_vec.data[i] & 0b11111000) == 0b11110000) {
            result = input_ch(const_vec_make(term_input_vec.data + i, 4));
            i += 4;
        } else if ((term_input_vec.data[i] & 0b11110000) == 0b11100000) {
            result = input_ch(const_vec_make(term_input_vec.data + i, 3));
            i += 3;
        } else if ((term_input_vec.data[i] & 0b11100000) == 0b11000000) {
            result = input_ch(const_vec_make(term_input_vec.data + i, 2));
            i += 2;
        } else {
            result = input_ch(const_vec_make(term_input_vec.data + i, 1));
            i += 1;
        }
        if (result == RESULT_ERR) {
            printf("at: input_ch\n\r");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
enum result global_update() {
    tick_count += 1;
    return RESULT_OK;
}
enum result global_init() {
    tick_count = 0;
    term_input_vec = vec_make(term_input_data, 0);
    term_output_vec = vec_make(term_output_data, 0);
    for (int32_t i = 0; i < BUFFER_SIZE; i++) {
        node_data[i] = (struct node){.data = vec_make(block_data[i], 0)};
    }
    node_free = node_data;
    for (int32_t i = 1; i < BUFFER_SIZE; i++) {
        node_release(&node_data[i]);
    }
    cursor_main = (struct cursor){.node = node_allocate(), .i = 0, .x = 0, .y = 0};
    cursor_cmd = (struct cursor){.node = node_allocate(), .i = 0, .x = 0, .y = 0};
    return RESULT_OK;
}
enum result loop() {
    while (isescape == false) {
        if (global_update() == RESULT_ERR) {
            printf("at: global_update\n\r");
            return RESULT_ERR;
        }
        if (term_update() == RESULT_ERR) {
            printf("at: term_update\n\r");
            return RESULT_ERR;
        }
        if (input_update() == RESULT_ERR) {
            printf("at: input_update\n\r");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
enum result deinit() {
    if (term_deinit() == RESULT_ERR) {
        printf("at: term_deinit\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result init() {
    if (global_init() == RESULT_ERR) {
        printf("at: global_init\n\r");
        return RESULT_ERR;
    }
    if (term_init() == RESULT_ERR) {
        printf("at: term_init\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
int main() {
    if (init() == RESULT_ERR) {
        printf("at: init\n\r\n\r");
    }
    if (loop() == RESULT_ERR) {
        printf("at: loop\n\r\n\r");
    }
    if (deinit() == RESULT_ERR) {
        printf("at: deinit\n\r\n\r");
    }
    return 0;
}
