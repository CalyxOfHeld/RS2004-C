#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "core/arena.h"
#include "core/jagfile.h"
#include "gfx/pix2d.h"
#include "gfx/pix3d.h"
#include "gfx/model.h"

#define CLIENT_WIDTH  765
#define CLIENT_HEIGHT 503

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("[2004scape 3D Model Viewer] Starting...\n");

    Arena arena = arena_create(64 * 1024 * 1024);

    const char *models_path = "../engine/data/pack/client/models";
    Jagfile *models_jag = jagfile_load_file(models_path, &arena);
    if (!models_jag) {
        models_path = "engine/data/pack/client/models";
        models_jag = jagfile_load_file(models_path, &arena);
    }
    if (!models_jag) {
        fprintf(stderr, "Failed to load models archive!\n");
        return 1;
    }

    model_init_repo(models_jag, &arena);

    int32_t current_model_id = 0;
    Model *model = model_load(current_model_id, &arena);
    if (model) {
        model_calculate_normals(model, 64, 768, -50, -10, -50);
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "2004scape C Rewrite - 3D Model Viewer (Rev 225)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        CLIENT_WIDTH,
        CLIENT_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *framebuffer = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        CLIENT_WIDTH,
        CLIENT_HEIGHT
    );

    uint32_t *pixels = (uint32_t *)arena_alloc_zero(&arena, CLIENT_WIDTH * CLIENT_HEIGHT * sizeof(uint32_t));
    pix2d_bind(pixels, CLIENT_WIDTH, CLIENT_HEIGHT);

    pix3d_init();
    pix3d_init_colour_table(0.8);
    pix3d_init_2d();

    int32_t yaw = 0;
    int32_t pitch = 0;
    int32_t zoom = 350;
    bool running = true;
    SDL_Event event;

    printf("\nControls:\n");
    printf("  [Left/Right Arrows] Change Model ID\n");
    printf("  [Up/Down Arrows]    Adjust Pitch\n");
    printf("  [W / S]             Zoom In / Out\n");
    printf("  [Space]             Toggle Auto-Rotation\n\n");

    bool auto_rotate = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_SPACE:
                        auto_rotate = !auto_rotate;
                        break;
                    case SDLK_UP:
                        pitch = (pitch - 32) & 2047;
                        break;
                    case SDLK_DOWN:
                        pitch = (pitch + 32) & 2047;
                        break;
                    case SDLK_w:
                        zoom -= 25;
                        if (zoom < 50) zoom = 50;
                        break;
                    case SDLK_s:
                        zoom += 25;
                        break;
                    case SDLK_RIGHT: {
                        int32_t next_id = current_model_id + 1;
                        Model *m = model_load(next_id, &arena);
                        if (m) {
                            current_model_id = next_id;
                            model = m;
                            model_calculate_normals(model, 64, 768, -50, -10, -50);
                            printf("Loaded Model ID: %d (points: %d, faces: %d)\n",
                                   current_model_id, model->num_points, model->num_faces);
                        }
                        break;
                    }
                    case SDLK_LEFT: {
                        if (current_model_id > 0) {
                            int32_t prev_id = current_model_id - 1;
                            Model *m = model_load(prev_id, &arena);
                            if (m) {
                                current_model_id = prev_id;
                                model = m;
                                model_calculate_normals(model, 64, 768, -50, -10, -50);
                                printf("Loaded Model ID: %d (points: %d, faces: %d)\n",
                                       current_model_id, model->num_points, model->num_faces);
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (auto_rotate) {
            yaw = (yaw + 8) & 2047;
        }

        // Render frame
        pix2d_cls();

        if (model) {
            model_render_obj(model, pitch, yaw, 0, 0, 0, 0, zoom);
        }

        SDL_UpdateTexture(framebuffer, NULL, pixels, CLIENT_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(20);
    }

    SDL_DestroyTexture(framebuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    arena_free(&arena);
    printf("[2004scape 3D Model Viewer] Terminated cleanly.\n");
    return 0;
}
