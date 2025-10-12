#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapreduce.h"

void Map(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);

    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
            if (token[0] == '\0') continue;
            MR_Emit(token, "1");
        }
    }
    free(line);
    fclose(fp);
}

void Reduce(char *key, Getter get_next, int partition_number) {
    int count = 0;
    char *val;
    while ((val = get_next(key, partition_number)) != NULL) {
        (void)val;
        count++;
    }
    printf("%s %d\n", key, count);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s input1 [input2 ...]\n", argv[0]);
        return 1;
    }
    MR_Run(argc, argv, Map, argc-1, Reduce, 10, MR_DefaultHashPartition);
    return 0;
}
