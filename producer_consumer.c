#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define MAX_LINE_LENGTH 1024
#define QUEUE_SIZE 100

// Queue implementation (using a circular array)
char queue[QUEUE_SIZE][MAX_LINE_LENGTH];
int head = 0, tail = 0;

omp_lock_t queue_lock;

void enqueue(char *line) {
    omp_set_lock(&queue_lock);
    if ((tail + 1) % QUEUE_SIZE != head) {
        strcpy(queue[tail], line);
        tail = (tail + 1) % QUEUE_SIZE;
    } else {
        printf("Queue full!\n");
    }
    omp_unset_lock(&queue_lock);
}

char *dequeue() {
    omp_set_lock(&queue_lock);
    if (head != tail) {
        char *line = queue[head];
        head = (head + 1) % QUEUE_SIZE;
        omp_unset_lock(&queue_lock);
        return line;
    } else {
        omp_unset_lock(&queue_lock);
        return NULL;
    }
}

void producer(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        enqueue(line);
    }

    fclose(fp);
}

void consumer() {
    char *line;
    while ((line = dequeue()) != NULL) {
        char *token = strtok(line, " \t\n");
        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, " \t\n");
        }
    }
}

int main() {
    double start_time = omp_get_wtime();

    omp_init_lock(&queue_lock);

    // Read input file name
    char filename[MAX_LINE_LENGTH];
    printf("Enter input file name: ");
    scanf("%s", filename);

    // Use a single producer for one input file
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            producer(filename);
        }

        // Use multiple consumers for tokenization
        #pragma omp section
        {
            #pragma omp parallel for
            for (int i = 0; i < omp_get_num_threads(); i++) {
                consumer();
            }
        }
    }

    omp_destroy_lock(&queue_lock);

    double end_time = omp_get_wtime();
    printf("Execution time: %f seconds\n", end_time - start_time);

    return 0;
}
