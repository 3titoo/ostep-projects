#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **data;     // array of strings
    size_t size;     // number of elements
    size_t capacity; // allocated capacity
} StringVector;

// Initialize vector
void initVector(StringVector *vec) {
    vec->size = 0;
    vec->capacity = 4; // start small
    vec->data = malloc(vec->capacity * sizeof(char *));
    if (!vec->data) {
        perror("malloc failed");
        exit(1);
    }
}

// Push a new string into the vector
void pushBack(StringVector *vec, const char *str) {
    if (vec->size == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, vec->capacity * sizeof(char *));
        if (!vec->data) {
            perror("realloc failed");
            exit(1);
        }
    }
    vec->data[vec->size] = strdup(str); // duplicate the string
    if (!vec->data[vec->size]) {
        perror("strdup failed");
        exit(1);
    }
    vec->size++;
}

// Get element at index
char *get(StringVector *vec, size_t index) {
    if (index >= vec->size) return NULL;
    return vec->data[index];
}

// Free all memory
void freeVector(StringVector *vec) {
    for (size_t i = 0; i < vec->size; i++) {
        free(vec->data[i]);
    }
    free(vec->data);
    vec->size = vec->capacity = 0;
    vec->data = NULL;
}
