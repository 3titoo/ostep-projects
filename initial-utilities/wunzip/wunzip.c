#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }

    for (int i = 1; i < argc; i++)
    {
        FILE *ar = fopen(argv[i], "r");
        if (!ar)
        {
            printf("wunzip: cannot open file: %s\n", argv[i]);
            continue;
        }

        int32_t cnt;
        char c;

        while (fread(&cnt, sizeof(int32_t), 1, ar) == 1 &&
               fread(&c, sizeof(char), 1, ar) == 1)
        {
            for (int j = 0; j < cnt; j++)
            {
                putchar(c);
            }
        }

        fclose(ar);
    }

    return 0;
}
