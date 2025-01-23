#include <stdint.h>
#include <stdio.h>
#include <sys/resource.h>
#include <termios.h>
#include <unistd.h>

#define RLIMIT_SIZE (1024 * 1024 * 128)
#define BUFFER_SIZE (1024 * 1024 * 2)

enum bool {
    FALSE,
    TRUE,
};
enum result {
    RESULT_OK,
    RESULT_ERR,
};

struct constvec {
    const char* data;
    int32_t size;
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
        printf("getrlimit\n");
        return RESULT_ERR;
    }
    if (rl.rlim_cur >= kstacksize) {
        return RESULT_OK;
    }
    if (setrlimit(RLIMIT_STACK, &rl) != 0) {
        printf("setrlimit\n");
        return RESULT_ERR;
    }
}
enum result term_update(struct vec* dst) {
    dst->size = read(STDIN_FILENO, dst->data, BUFFER_SIZE);
    return RESULT_OK;
}
enum result term_deinit(struct termios* term_orig) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, term_orig) < 0) {
        printf("err: tcsetattr\n");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result term_init(struct termios* term_orig) {
    if (tcgetattr(STDIN_FILENO, term_orig) == -1) {
        printf("err: tcgetattr\n");
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
        printf("err: tcsetattr\n");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result input_update(struct vec* src, enum bool* isescape) {
    for (int32_t i = 0; i < src->size; i++) {
        if (src->data[i] == 'q') {
            *isescape = TRUE;
            return RESULT_OK;
        }
    }
    return RESULT_OK;
}
enum result main2(struct termios* term_orig) {
    char term_data[BUFFER_SIZE];
    struct vec term_vec = {.data = term_data, .size = 0};
    enum bool isescape = FALSE;
    while (isescape == FALSE) {
        if (term_update(&term_vec) == RESULT_ERR) {
            printf("err: _update\n");
            return RESULT_ERR;
        }
        if (input_update(&term_vec, &isescape) == RESULT_ERR) {
            printf("err: _update\n");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
int main() {
    struct termios term_orig;
    if (limit_init() == RESULT_ERR) {
        printf("err: limit_init\n\n");
        return 0;
    }
    if (term_init(&term_orig) == RESULT_ERR) {
        printf("err: term_init\n\n");
        return 0;
    }
    if (main2(&term_orig) == RESULT_ERR) {
        printf("err: main2\n\n");
    }
    if (term_deinit(&term_orig) == RESULT_ERR) {
        printf("err: term_deinit\n\n");
    }
    return 0;
}
