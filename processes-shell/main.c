#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "Vector.h"
#include <linux/limits.h>

StringVector PATHS;

int hasWord(char *line) {
    for (size_t i = 0; i < strlen(line); i++) {
        if (line[i] != ' ' && line[i] != '\n') {
            return 1;
        }
    }
    return 0;
}

char *preprocess(char *line) {
    static char buffer[1024];
    int j = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '>' || line[i] == '<' || line[i] == '&') {
            buffer[j++] = ' ';
            buffer[j++] = line[i];
            buffer[j++] = ' ';
        } else {
            buffer[j++] = line[i];
        }
    }
    buffer[j] = '\0';
    return buffer;
}

StringVector GetArgs(char *line) {
    StringVector args;
    initVector(&args);
    char *token;
    while ((token = strsep(&line, " ")) != NULL) {
        if (strlen(token) > 0)
            pushBack(&args, token);
    }
    return args;
}

void errorMSG() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void changeDirectory(StringVector *args) {
    if (args->size != 2) {
        errorMSG();
        return;
    }
    char *path = get(args, 1);
    if (chdir(path) != 0) {
        errorMSG();
    }
}

char *concat(char *a, char *b) {
    char *ans = (char *)malloc((strlen(a) + strlen(b) + 1) * sizeof(char));
    strcpy(ans, a);
    strcat(ans, b);
    return ans;
}

int isDelimeter(char *str) {
    return (strcmp(str, "|") == 0 || strcmp(str, ">") == 0);
}

int isContainRedirectOutput(StringVector *args) {
    for (size_t i = 0; i < args->size; i++) {
        if (strcmp(get(args, i), ">") == 0) {
            return 1;
        }
    }
    return 0;
}

int isValidContainRedirectOutput(StringVector *args) {
    if (args->size < 2) return 0;
    int cnt = 0;
    for (size_t i = 0; i < args->size; i++) {
        if (strcmp(get(args, i), ">") == 0) cnt++;
    }
    return strcmp(get(args, args->size - 2), ">") == 0 && cnt == 1;
}

pid_t executeCommand(StringVector args) {
    if (args.size == 0) return -1;

    if (strcmp(get(&args, 0), "cd") == 0) {
        changeDirectory(&args);
        return -1;
    }
    if (strcmp(get(&args, 0), "exit") == 0) {
        if (args.size != 1) errorMSG();
        else exit(0);
        return -1;
    }
    if (strcmp(get(&args, 0), "path") == 0) {
        freeVector(&PATHS);
        initVector(&PATHS);
        for (size_t i = 1; i < args.size; i++) {
            pushBack(&PATHS, get(&args, i));
        }
        return -1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        int idx = -1;
        for (int i = 0; i < args.size; i++) {
            if (strcmp(get(&args, i), ">") == 0) {
                idx = i;
                break;
            }
        }
        if (idx != -1 && idx + 2 == args.size) {
            int fd = open(get(&args, idx + 1),
                          O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args.size = idx; // cut command before >
        }

        // build argv
        char *Args[args.size + 1];
        for (size_t i = 0; i < args.size; i++) {
            Args[i] = get(&args, i);
        }
        Args[args.size] = NULL;

        // search PATH
        for (size_t i = 0; i < PATHS.size; i++) {
            char *fullPath = concat(concat(get(&PATHS, i), "/"), Args[0]);
            if (access(fullPath, X_OK) == 0) {
                execv(fullPath, Args);
            }
            free(fullPath);
        }
        errorMSG();
        exit(1);
    }

    return pid; 
}

int main(int argc, char *argv[]) {
    initVector(&PATHS);
    pushBack(&PATHS, "/bin");

    if (argc > 2) {
        errorMSG();
        exit(1);
    }

    FILE *inputFile = NULL;
    if (argc == 2) {
        inputFile = fopen(argv[1], "r");
        if (!inputFile) {
            errorMSG();
            exit(1);
        }
    }

    while (1) {
        if (!inputFile) printf("Wish> ");
        char *line = NULL;
        size_t len = 0;
        if (getline(&line, &len, inputFile ? inputFile : stdin) == -1) {
            free(line);
            break;
        }
        line[strcspn(line, "\n")] = '\0';
        if (!hasWord(line) || strcmp(line, "&") == 0) {
            free(line);
            continue;
        }

        StringVector args = GetArgs(preprocess(line));

        int idx = 0;
        pid_t pids[1024];
        int pcount = 0;

        while (idx < args.size) {
            StringVector newArgs;
            initVector(&newArgs);

            while (idx < args.size && strcmp(get(&args, idx), "&") != 0) {
                pushBack(&newArgs, get(&args, idx));
                idx++;
            }

            if (idx < args.size && strcmp(get(&args, idx), "&") == 0) idx++;

            if (newArgs.size == 0) {
                freeVector(&newArgs);
                continue;
            }

            if (isDelimeter(get(&newArgs, newArgs.size - 1)) ||
                isDelimeter(get(&newArgs, 0))) {
                errorMSG();
                freeVector(&newArgs);
                continue;
            }

            int ok = isValidContainRedirectOutput(&newArgs);
            if (!ok && isContainRedirectOutput(&newArgs)) {
                errorMSG();
                freeVector(&newArgs);
                continue;
            }

            pid_t pid = executeCommand(newArgs);
            if (pid != -1) pids[pcount++] = pid;

            freeVector(&newArgs);
        }

        for (int i = 0; i < pcount; i++) {
            waitpid(pids[i], NULL, 0);
        }

        free(line);
        freeVector(&args);
    }

    if (inputFile) fclose(inputFile);
    return 0;
}
