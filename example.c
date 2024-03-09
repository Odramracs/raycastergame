#include "Screen.h"
#include <stdio.h>
#include <stdint.h>

int main(int argc, char** argv) {
    InitWindow("Example Window", 800, 600, 400, 400);
    uint32_t* bitmap = InitBitmap(800, 600);
    uint32_t color = toRGB(244, 24, 100);
    int r, g, b;
    fromRGB(color, r, g, b);
    
    printf("r:%u,g:%u,b:%u\n", r, g, b);
    while (WindowActive()) {
        Update();
        
        for(int pixel = 0; pixel < 800*600; pixel++) {
            bitmap[pixel] = color;
        }

        if(IsKeyPressed("a")) {
            printf("You are pressing A!\n");
        }

        RenderBitmap();
    };

    return 0;
}