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
    int32_t node_i;
    int32_t x;
    int32_t y;
};
struct global {
    char block_data[BUFFER_SIZE][BLOCK_SIZE];
    char term_input_data[BUFFER_SIZE];
    char term_output_data[BUFFER_SIZE];
    struct vec block_vec[BUFFER_SIZE];
    struct vec term_input_vec;
    struct vec term_output_vec;
    struct termios term_orig;
    int32_t tick_count;
    bool isescape;
};

struct const_vec const_vec_make(const char* data, int32_t size) {
    return (struct const_vec){.data = data, .size = size};
}
struct const_vec vec_to_const_vec(struct vec* src) {
    return (struct const_vec){.data = src->data, .size = src->size};
}
struct vec vec_make(char* data, int32_t size) {
    return (struct vec){.data = data, .size = size};
}
enum result term_update(struct vec* term_input) {
    term_input->size = read(STDIN_FILENO, term_input->data, BUFFER_SIZE);
    if (term_input->size == -1) {
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
enum result input_ch(struct const_vec src, bool* isescape) {
    if (src.data[0] == 'q') {
        *isescape = true;
        return RESULT_OK;
    }
    if (src.size == strlen("🤔") && memcmp(src.data, "🤔", strlen("🤔")) == 0) {
        printf("err: testerr\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result input_update(struct vec* term_input_vec, bool* isescape) {
    char* ch_data[BUFFER_SIZE];
    int32_t i = 0;
    while (i < term_input_vec->size) {
        enum result result;
        if ((term_input_vec->data[i] & 0b11111000) == 0b11110000) {
            result = input_ch(const_vec_make(term_input_vec->data + i, 4), isescape);
            i += 4;
        } else if ((term_input_vec->data[i] & 0b11110000) == 0b11100000) {
            result = input_ch(const_vec_make(term_input_vec->data + i, 3), isescape);
            i += 3;
        } else if ((term_input_vec->data[i] & 0b11100000) == 0b11000000) {
            result = input_ch(const_vec_make(term_input_vec->data + i, 2), isescape);
            i += 2;
        } else {
            result = input_ch(const_vec_make(term_input_vec->data + i, 1), isescape);
            i += 1;
        }
        if (result == RESULT_ERR) {
            printf("at: input_ch\n\r");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
enum result global_update(struct global* global) {
    global->tick_count += 1;
    return RESULT_OK;
}
enum result global_init(struct global* global) {
    global->tick_count = 0;
    for(int32_t i=0;i<BUFFER_SIZE;i++) {
        global->block_vec[i] = vec_make(global->block_data[i], 0);
    }
    global->term_input_vec = vec_make(global->term_input_data, 0);
    global->term_output_vec = vec_make(global->term_output_data, 0);
    return RESULT_OK;
}
enum result loop(struct global* global) {
    while (global->isescape == false) {
        if (global_update(global) == RESULT_ERR) {
            printf("at: global_update\n\r");
            return RESULT_ERR;
        }
        if (term_update(&global->term_input_vec) == RESULT_ERR) {
            printf("at: term_update\n\r");
            return RESULT_ERR;
        }
        if (input_update(&global->term_input_vec, &global->isescape) == RESULT_ERR) {
            printf("at: input_update\n\r");
            return RESULT_ERR;
        }
    }
    return RESULT_OK;
}
enum result deinit(struct global* global) {
    if (term_deinit(&global->term_orig) == RESULT_ERR) {
        printf("at: term_deinit\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
enum result init(struct global* global) {
    if(global_init(global) == RESULT_ERR) {
        printf("at: global_init\n\r");
        return RESULT_ERR;
    }
    if (term_init(&global->term_orig) == RESULT_ERR) {
        printf("at: term_init\n\r");
        return RESULT_ERR;
    }
    return RESULT_OK;
}
int main() {
    static struct global global;
    if (init(&global) == RESULT_ERR) {
        printf("at: init\n\r\n\r");
    }
    if (loop(&global) == RESULT_ERR) {
        printf("at: loop\n\r\n\r");
    }
    if (deinit(&global) == RESULT_ERR) {
        printf("at: deinit\n\r\n\r");
    }
    return 0;
}
