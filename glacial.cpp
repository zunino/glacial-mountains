#include <array>
#include <string_view>
#include <tuple>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

struct TextureSlice {
    int x, y, w, h;
};

namespace {
    const char* const TITLE = "Glacial Mountains - Parallax Example";
    constexpr int WINDOW_WIDTH = 768;
    constexpr int WINDOW_HEIGHT = 432;

    const char* const TEXTURES_PATH = "rsc/glacial_mountains_textures.png"; 
    constexpr int DEFAULT_WIDTH = 384;
    constexpr int DEFAULT_HEIGHT = 216;

    constexpr TextureSlice bg_clouds{0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr TextureSlice mountains{384, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr TextureSlice fg_clouds_2{0, 216, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr TextureSlice fg_clouds_1{384, 216, DEFAULT_WIDTH, DEFAULT_HEIGHT};
    constexpr TextureSlice credits{384, 432, DEFAULT_WIDTH, DEFAULT_HEIGHT};
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

struct SideScrolling {
    float speed_factor = 0.0f; // 1: camera speed; 0.5: half that etc.
    float screen_x = 0.0f;
};

enum class TitleEffectStatus {
    INITIAL,
    SCROLLING_IN,
    AT_DESTINATION,
    SCROLLING_OUT
};

struct TitleEffect {
    float speed = 0.0f;
    float screen_y = 0.0f;
    uint32_t on_timeout = 0;
    uint32_t off_timeout = 0;
    uint32_t base_ticks = SDL_GetTicks();
    TitleEffectStatus status = TitleEffectStatus::INITIAL;
};

struct Layer {
    Layer(const TextureSlice& t_slice, SideScrolling side_scrolling) :
        t_slice{t_slice},
        side_scrolling{side_scrolling}
    {
    }
    Layer(const TextureSlice& t_slice, TitleEffect title_effect) :
        t_slice{t_slice},
        title_effect{title_effect}
    {
    }
    TextureSlice t_slice;
    SideScrolling side_scrolling;
    TitleEffect title_effect;
};

struct Camera {
    float x_speed = 0;
};

struct Scene {
    Scene(Sdl& sdl) : 
        texture{load_texture(sdl.renderer, TEXTURES_PATH)},
        layers{
            Layer{bg_clouds, SideScrolling{0.15f}}, 
            Layer{mountains, SideScrolling{0.25f}},
            Layer{credits, TitleEffect{-2.0f, WINDOW_HEIGHT, 2000, 3000}},
            Layer{fg_clouds_2, SideScrolling{0.50f}},
            Layer{fg_clouds_1, SideScrolling{0.75f}}
        }
    {
    }
    ~Scene() {
        SDL_DestroyTexture(texture);
    }
    SDL_Texture* texture;
    std::array<Layer, 5> layers;
    Camera camera{4.0f};
};

struct Timer {
    Timer() : start{SDL_GetTicks()} {}
    uint32_t elapsed() { return SDL_GetTicks() - start; }
    uint32_t start;
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

inline bool has_side_scrolling(const Layer& layer) {
    return layer.side_scrolling.speed_factor != 0.0f;
}

inline bool has_title_effect(const Layer& layer) {
    return layer.title_effect.speed != 0.0f;
}

inline bool reached_destination(const TitleEffect& title_effect, float dest_y) {
    return title_effect.speed < 0.0f && dest_y <= 0.0f
            || title_effect.speed > 0.0f && dest_y >= 0.0f;
}

inline bool went_off_screen(const TitleEffect& title_effect, float dest_y) {
    return title_effect.speed < 0.0f && dest_y <= -WINDOW_HEIGHT
            ||title_effect.speed > 0.0f && dest_y >= WINDOW_HEIGHT;
}

void update_scene(Scene& scene, Timer& timer) {
    for (Layer& layer : scene.layers) {
        // side scrolling
        if (has_side_scrolling(layer)) {
            layer.side_scrolling.screen_x -= scene.camera.x_speed * layer.side_scrolling.speed_factor;
            if (layer.side_scrolling.screen_x < -WINDOW_WIDTH) {
                layer.side_scrolling.screen_x += WINDOW_WIDTH;
            }
        }
        // title effect
        if (has_title_effect(layer)) {
            float elapsed = SDL_GetTicks() - layer.title_effect.base_ticks;
            switch (layer.title_effect.status) {
            case TitleEffectStatus::INITIAL:
                if (elapsed >= layer.title_effect.on_timeout) {
                    layer.title_effect.status = TitleEffectStatus::SCROLLING_IN;
                }
                break;
            case TitleEffectStatus::SCROLLING_IN: {
                float dest_y = layer.title_effect.screen_y + layer.title_effect.speed;
                if (reached_destination(layer.title_effect, dest_y)) {
                    dest_y = 0.0f;
                    layer.title_effect.status = TitleEffectStatus::AT_DESTINATION;
                    layer.title_effect.base_ticks = SDL_GetTicks();
                }
                layer.title_effect.screen_y = dest_y;
                break;
            }
            case TitleEffectStatus::AT_DESTINATION:
                if (elapsed >= layer.title_effect.off_timeout) {
                    layer.title_effect.status = TitleEffectStatus::SCROLLING_OUT;
                }
                break;
            case TitleEffectStatus::SCROLLING_OUT: {
                float dest_y = layer.title_effect.screen_y + layer.title_effect.speed;
                if (went_off_screen(layer.title_effect, dest_y)) {
                    dest_y = layer.title_effect.speed < 0.0f ? WINDOW_HEIGHT : -WINDOW_HEIGHT;
                    layer.title_effect.status = TitleEffectStatus::INITIAL;
                    layer.title_effect.base_ticks = SDL_GetTicks();
                }
                layer.title_effect.screen_y = dest_y;
                break;
            }}
        }
    }
}

inline void render_layer(const Layer& layer, SDL_Renderer* renderer,
        SDL_Texture* texture, int dest_x, int dest_y) {
    SDL_Rect src{layer.t_slice.x, layer.t_slice.y, layer.t_slice.w, layer.t_slice.h};
    SDL_Rect dst{dest_x, dest_y, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderCopy(renderer, texture, &src, &dst);
}

void render_scene(Sdl& sdl, Scene& scene) {
    // clear
    SDL_SetRenderDrawColor(sdl.renderer, 0x08, 0xa9, 0xfc, 0xff);
    SDL_RenderClear(sdl.renderer);
    // layers
    for (const Layer& layer : scene.layers) {
        render_layer(layer, sdl.renderer, scene.texture, layer.side_scrolling.screen_x,
                layer.title_effect.screen_y);
        if (has_side_scrolling(layer)) {
            int dest_x = layer.side_scrolling.screen_x;
            for (int i = 0; i < 2; ++i) {
                dest_x += WINDOW_WIDTH;
                render_layer(layer, sdl.renderer, scene.texture, dest_x,
                        layer.title_effect.screen_y);
            }
        }
    }
    SDL_RenderPresent(sdl.renderer);
}

int main() {
    Sdl sdl;
    Scene scene{sdl};
    Timer timer;
    bool run = true;
    while (run) {
        run = handle_input();
        update_scene(scene, timer);
        render_scene(sdl, scene);
    }
}

