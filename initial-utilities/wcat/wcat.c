#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
            FILE *ar = fopen(argv[i], "r");
            if (!ar)
            {
                printf("wcat: cannot open file\n");
                exit(1);
            }
            int ch;
            while ((ch = fgetc(ar)) != EOF)
            {
                putchar(ch);
            }
            fclose(ar);
    }
    return 0;
}
