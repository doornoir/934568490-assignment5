#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/*
 * Assignment 5: Multi-threaded Producer Consumer Pipeline
 *
 * Pipeline:
 * Input Thread -> Line Separator Thread -> Plus Sign Thread -> Output Thread
 *
 * The buffers are sized large enough for the assignment limits, so they are
 * treated as unbounded producer-consumer buffers: producers never wait for
 * space, and consumers only wait when their input buffer is empty.
 */

#define BUFFER_SIZE 50000
#define END_MARKER '\0'
#define OUTPUT_WIDTH 80

/* Producer-consumer character buffer. */
typedef struct {
    char data[BUFFER_SIZE];
    int count;
    int prod_idx;
    int con_idx;
    pthread_mutex_t mutex;
    pthread_cond_t full;
} Buffer;

Buffer buffer1 = { .count = 0, .prod_idx = 0, .con_idx = 0,
                   .mutex = PTHREAD_MUTEX_INITIALIZER,
                   .full = PTHREAD_COND_INITIALIZER };

Buffer buffer2 = { .count = 0, .prod_idx = 0, .con_idx = 0,
                   .mutex = PTHREAD_MUTEX_INITIALIZER,
                   .full = PTHREAD_COND_INITIALIZER };

Buffer buffer3 = { .count = 0, .prod_idx = 0, .con_idx = 0,
                   .mutex = PTHREAD_MUTEX_INITIALIZER,
                   .full = PTHREAD_COND_INITIALIZER };

/* Put one character into a buffer. Called by producer side of a thread. */
void put_char(Buffer *buffer, char value) {
    pthread_mutex_lock(&buffer->mutex);

    buffer->data[buffer->prod_idx] = value;
    buffer->prod_idx = (buffer->prod_idx + 1) % BUFFER_SIZE;
    buffer->count++;

    pthread_cond_signal(&buffer->full);
    pthread_mutex_unlock(&buffer->mutex);
}

/* Get one character from a buffer. Called by consumer side of a thread. */
char get_char(Buffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count == 0) {
        pthread_cond_wait(&buffer->full, &buffer->mutex);
    }

    char value = buffer->data[buffer->con_idx];
    buffer->con_idx = (buffer->con_idx + 1) % BUFFER_SIZE;
    buffer->count--;

    pthread_mutex_unlock(&buffer->mutex);
    return value;
}

/*
 * Thread 1: reads lines from standard input until it sees exactly STOP\n.
 * It sends all characters before the stop-processing line into buffer1.
 */
void *input_thread(void *args) {
    char *line = NULL;
    size_t size = 0;

    while (getline(&line, &size, stdin) != -1) {
        if (strcmp(line, "STOP\n") == 0) {
            break;
        }

        for (int i = 0; line[i] != '\0'; i++) {
            put_char(&buffer1, line[i]);
        }
    }

    free(line);
    put_char(&buffer1, END_MARKER);
    return NULL;
}

/*
 * Thread 2: replaces every line separator character '\n' with a space.
 * It reads from buffer1 and writes to buffer2.
 */
void *line_separator_thread(void *args) {
    while (1) {
        char current = get_char(&buffer1);

        if (current == END_MARKER) {
            put_char(&buffer2, END_MARKER);
            break;
        }

        if (current == '\n') {
            current = ' ';
        }

        put_char(&buffer2, current);
    }

    return NULL;
}

/*
 * Thread 3: replaces each pair of plus signs "++" with one caret '^'.
 * It reads from buffer2 and writes to buffer3.
 */
void *plus_sign_thread(void *args) {
    while (1) {
        char current = get_char(&buffer2);

        if (current == END_MARKER) {
            put_char(&buffer3, END_MARKER);
            break;
        }

        if (current == '+') {
            char next = get_char(&buffer2);

            if (next == END_MARKER) {
                put_char(&buffer3, current);
                put_char(&buffer3, END_MARKER);
                break;
            }

            if (next == '+') {
                put_char(&buffer3, '^');
            } else {
                put_char(&buffer3, current);
                put_char(&buffer3, next);
            }
        } else {
            put_char(&buffer3, current);
        }
    }

    return NULL;
}

/*
 * Thread 4: writes processed output to standard output in lines of exactly
 * 80 characters. Any leftover characters fewer than 80 are not printed.
 */
void *output_thread(void *args) {
    char output_line[OUTPUT_WIDTH];
    int index = 0;

    while (1) {
        char current = get_char(&buffer3);

        if (current == END_MARKER) {
            break;
        }

        output_line[index] = current;
        index++;

        if (index == OUTPUT_WIDTH) {
            for (int i = 0; i < OUTPUT_WIDTH; i++) {
                putchar(output_line[i]);
            }
            putchar('\n');
            fflush(stdout);
            index = 0;
        }
    }

    return NULL;
}

int main(void) {
    pthread_t input_t;
    pthread_t line_separator_t;
    pthread_t plus_sign_t;
    pthread_t output_t;

    pthread_create(&input_t, NULL, input_thread, NULL);
    pthread_create(&line_separator_t, NULL, line_separator_thread, NULL);
    pthread_create(&plus_sign_t, NULL, plus_sign_thread, NULL);
    pthread_create(&output_t, NULL, output_thread, NULL);

    pthread_join(input_t, NULL);
    pthread_join(line_separator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);

    return 0;
}
