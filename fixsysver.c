#include <stdio.h>

int main()
{
    FILE *f = fopen("win32yank.exe", "rb+");
    int   r = 0;
    if (!f)
        r = 2;
    else
    {
        fseek(f, 0x3C, SEEK_SET);
        unsigned int x;
        if (fread(&x, sizeof x, 1, f) != 1)
            r = 1;
        else
        {
            fseek(f, x + 0x40, SEEK_SET);
            unsigned int v = 4;
            fwrite(&v, sizeof v, 1, f);
            fseek(f, x + 0x48, SEEK_SET);
            fwrite(&v, sizeof v, 1, f);
        }
        fclose(f);
    }
    return r;
}
