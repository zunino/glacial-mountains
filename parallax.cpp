#include <array>
#include <tuple>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

struct Texture_Slice {
    int x, y, w, h;
};

namespace {
    const char* const TITLE = "Glacial Mountains - Parallax Example";
    constexpr int WINDOW_WIDTH = 768;
    constexpr int WINDOW_HEIGHT = 432;

    const char* const TEXTURES_PATH = "rsc/glacial_mountains_textures.png"; 
    constexpr int DEFAULT_WIDTH = 384;
    constexpr int DEFAULT_HEIGHT = 216;

    constexpr Texture_Slice bg_clouds{0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr Texture_Slice mountains{384, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr Texture_Slice fg_clouds_2{0, 216, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr Texture_Slice fg_clouds_1{384, 216, DEFAULT_WIDTH, DEFAULT_HEIGHT};
}

SDL_Texture* load_texture(SDL_Renderer* renderer, std::string_view path) {
    SDL_Surface* surface = IMG_Load(path.data());
    if (surface == nullptr) {
        throw SDL_GetError();
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

struct Sdl {
    Sdl() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw SDL_GetError();
        }
        window = SDL_CreateWindow(
                TITLE,
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                WINDOW_WIDTH,
                WINDOW_HEIGHT,
                SDL_WINDOW_SHOWN// | SDL_WINDOW_FULLSCREEN_DESKTOP
                );
        if (window == nullptr) {
            throw SDL_GetError();
        }
        renderer = SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
                );
        if (renderer == nullptr) {
            throw SDL_GetError();
        }
    }
    ~Sdl() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    SDL_Window* window;
    SDL_Renderer* renderer;
};

struct Layer {
    Layer(const Texture_Slice& t_slice, float speed_factor) :
        speed_factor{speed_factor},
        t_slice{t_slice},
        dest_x{0.0f}
    {
    }
    Texture_Slice t_slice;
    float speed_factor; // 1: camera speed; 0.5: half that etc.
    float dest_x;
};

struct Camera {
    float x_speed = 0;
};

struct Scene {
    Scene(Sdl& sdl) : 
        texture{load_texture(sdl.renderer, TEXTURES_PATH)},
        layers{
            Layer{bg_clouds, 0.15f},
            Layer{mountains, 0.25f},
            Layer{fg_clouds_2, 0.50f},
            Layer{fg_clouds_1, 0.75f}
        }
    {
    }
    ~Scene() {
        SDL_DestroyTexture(texture);
    }
    SDL_Texture* texture;
    std::array<Layer, 4> layers;
    Camera camera{4.0f};
};

bool handle_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                return false;
                break;
            }
        }
    }
    return true;
}

void update_scene(Scene& scene) {
    for (Layer& layer : scene.layers) {
        layer.dest_x -= scene.camera.x_speed * layer.speed_factor;
        if (layer.dest_x < -WINDOW_WIDTH) {
            layer.dest_x += WINDOW_WIDTH;
        }
    }
}

void render_scene(Sdl& sdl, Scene& scene) {
    SDL_SetRenderDrawColor(sdl.renderer, 0x08, 0xa9, 0xfc, 0xff);
    SDL_RenderClear(sdl.renderer);
    for (const Layer& layer : scene.layers) {
        SDL_Rect src{layer.t_slice.x, layer.t_slice.y, layer.t_slice.w, layer.t_slice.h};
        for (int i = 0, dest_x = layer.dest_x; i < 3; ++i) {
            SDL_Rect dst{dest_x, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderCopy(sdl.renderer, scene.texture, &src, &dst);
            dest_x += WINDOW_WIDTH;
        }
    }
    SDL_RenderPresent(sdl.renderer);
}

int main() {
    Sdl sdl;
    Scene scene{sdl};
    bool run = true;
    while (run) {
        run = handle_input();
        update_scene(scene);
        render_scene(sdl, scene);
    }
}

