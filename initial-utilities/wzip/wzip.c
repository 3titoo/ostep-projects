#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    char last_char = '\0';
    int cnt = 0;

    for (int i = 1; i < argc; i++)
    {
        FILE *ar = fopen(argv[i], "r");
        if (!ar)
        {
            printf("wzip: cannot open file %s\n", argv[i]);
            exit(1);
        }

        int c;
        while ((c = fgetc(ar)) != EOF)
        {
            if (cnt == 0)
            {
                last_char = c;
                cnt = 1;
            }
            else if (c == last_char)
            {
                cnt++;
            }
            else
            {
                fwrite(&cnt, sizeof(int), 1, stdout);
                fwrite(&last_char, sizeof(char), 1, stdout);
                last_char = c;
                cnt = 1;
            }
        }

        fclose(ar);
    }

    // Flush last run after processing ALL files
    if (cnt > 0)
    {
        fwrite(&cnt, sizeof(int), 1, stdout);
        fwrite(&last_char, sizeof(char), 1, stdout);
    }

    return 0;
}
