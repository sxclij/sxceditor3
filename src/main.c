#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <termios.h>
#include <unistd.h>

#define RLIMIT_SIZE (1024 * 1024 * 128)
#define BUFFER_SIZE (1024 * 4)
#define BLOCK_SIZE 128

enum result {
    RESULT_OK,
    RESULT_ERR,
};

struct constvec {
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

enum result limit_init() {
    const rlim_t kstacksize = RLIMIT_SIZE;
    struct rlimit rl = {.rlim_cur = kstacksize, .rlim_max = kstacksize};
    if (getrlimit(RLIMIT_STACK, &rl) != 0) {
        printf("getrlimit\n\r");
        return RESULT_ERR;
    }
    if (rl.rlim_cur >= kstacksize) {
        return RESULT_OK;
    }
    if (setrlimit(RLIMIT_STACK, &rl) != 0) {
        printf("setrlimit\n\r");
        return RESULT_ERR;
    }
}
enum result term_update(struct vec* dst) {
    dst->size = read(STDIN_FILENO, dst->data, BUFFER_SIZE);
    if (dst->size == -1) {
        printf("err: read\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result term_deinit(struct termios* term_orig) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, term_orig) < 0) {
        printf("err: tcsetattr\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result term_init(struct termios* term_orig) {
    if (tcgetattr(STDIN_FILENO, term_orig) == -1) {
        printf("err: tcgetattr\n\r");
        return RESULT_ERR;
    }
    struct termios termios_new = *term_orig;
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
enum result input_ch(struct vec* src, bool* isescape) {
    if (src->data[0] == 'q') {
        *isescape = true;
        return RESULT_OK;
    }
    if (src->size == strlen("🤔") && memcmp(src->data, "🤔", strlen("🤔")) == 0) {
        printf("err: testerr\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result input_update(struct vec* src, bool* isescape) {
    char* ch_data[BUFFER_SIZE];
    int32_t i = 0;
    while (i < src->size) {
        struct vec ch_vec;
        if ((src->data[i] & 0b11111000) == 0b11110000) {
            ch_vec = (struct vec){.data = src->data + i, .size = 4};
            i += 4;
        } else if ((src->data[i] & 0b11110000) == 0b11100000) {
            ch_vec = (struct vec){.data = src->data + i, .size = 3};
            i += 3;
        } else if ((src->data[i] & 0b11100000) == 0b11000000) {
            ch_vec = (struct vec){.data = src->data + i, .size = 2};
            i += 2;
        } else {
            ch_vec = (struct vec){.data = src->data + i, .size = 1};
            i += 1;
        }
        if (input_ch(&ch_vec, isescape) == RESULT_ERR) {
            printf("err: input_ch\n\r");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
enum result main2(struct termios* term_orig) {
    char term_data[BUFFER_SIZE];
    char block_data[BUFFER_SIZE][BLOCK_SIZE];
    struct vec term_vec = {.data = term_data, .size = 0};
    struct vec block_vec[BUFFER_SIZE];
    bool isescape = false;
    for (int32_t i = 0; i < BUFFER_SIZE; i++) {
        block_vec[i] = (struct vec){.data = block_data[i], .size = 0};
    }
    while (isescape == false) {
        if (term_update(&term_vec) == RESULT_ERR) {
            printf("err: term_update\n\r");
            return RESULT_ERR;
        }
        if (input_update(&term_vec, &isescape) == RESULT_ERR) {
            printf("err: input_update\n\r");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
int main() {
    struct termios term_orig;
    if (limit_init() == RESULT_ERR) {
        printf("err: limit_init\n\r\n\r");
        return 0;
    }
    if (term_init(&term_orig) == RESULT_ERR) {
        printf("err: term_init\n\r\n\r");
        return 0;
    }
    if (main2(&term_orig) == RESULT_ERR) {
        printf("err: main2\n\r\n\r");
    }
    if (term_deinit(&term_orig) == RESULT_ERR) {
        printf("err: term_deinit\n\r\n\r");
    }
    return 0;
}
