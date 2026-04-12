#include <stdio.h>
#include <math.h>

#define WIDTH 60
#define HEIGHT 20

int main(void) {
    char canvas[HEIGHT][WIDTH + 1];

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++)
            canvas[y][x] = ' ';
        canvas[y][WIDTH] = '\0';
    }

    for (int x = 0; x < WIDTH; x++) {
        double t = (double)x / WIDTH * 4.0 * M_PI;
        double val = sin(t);
        int y = (int)((1.0 - val) / 2.0 * (HEIGHT - 1));
        if (y >= 0 && y < HEIGHT)
            canvas[y][x] = '#';
    }

    printf("sin(x) plot:\n");
    for (int y = 0; y < HEIGHT; y++)
        printf("|%s|\n", canvas[y]);

    printf("pi   = %.15f\n", M_PI);
    printf("e    = %.15f\n", M_E);
    printf("sqrt(2) = %.15f\n", sqrt(2.0));

    return 0;
}
