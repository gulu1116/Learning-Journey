// cp_stdio.c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src_file dest_file\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *src = fopen(argv[1], "rb");
    if (!src) {
        perror("fopen src");
        exit(EXIT_FAILURE);
    }

    FILE *dest = fopen(argv[2], "wb");
    if (!dest) {
        perror("fopen dest");
        fclose(src);
        exit(EXIT_FAILURE);
    }

    int ch;
    while ((ch = fgetc(src)) != EOF) {
        if (fputc(ch, dest) == EOF) {
            perror("fputc");
            fclose(src);
            fclose(dest);
            exit(EXIT_FAILURE);
        }
    }

    fclose(src);
    fclose(dest);
    return 0;
}
