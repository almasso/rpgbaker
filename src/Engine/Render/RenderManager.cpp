#include "RenderManager.h"
#include "TextTexture.h"
#include "TextureLoader.h"
#include "Texture.h"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_init.h>
#include <Utils/Error.h>
#include <SDL3_image/SDL_image.h>



RenderManager::RenderManager() : _screenScale(0), _window(nullptr), _renderer(nullptr), _width(0), _height(0) {
}

bool RenderManager::init(const int& width, const int& height, std::string const& gameName, std::string const& gameIcon) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        {
            Error::ShowError("Error al inicializar SDL_VIDEO", SDL_GetError());
            return false;
        }
    }
    if (!TTF_Init()) {
        {
            Error::ShowError("Error al inicializar TTF", SDL_GetError());
            return false;
        }
    }
    if (!SDL_CreateWindowAndRenderer(gameName.c_str(), width, height, SDL_WINDOW_MAXIMIZED, &_window, &_renderer))
    {
        Error::ShowError("Error al crear la ventana", SDL_GetError());
        return false;
    }
    int w, h;
    getWindowSize(&w, &h);
    _width = static_cast<float>(w);
    _height = static_cast<float>(h);
    _viewOffset = {w/2.f,h/2.f};
    _screenOffset = {0,0};
    _screenScale = 1;
    if (!gameIcon.empty()) {
        auto icon = IMG_Load(gameIcon.c_str());
        SDL_SetWindowIcon(_window, icon);
        SDL_DestroySurface(icon);
    }

    return TextureLoader::Init(_renderer);
}

void RenderManager::present() const {
    SDL_RenderPresent(_renderer);
}

void RenderManager::clear() const {
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
    SDL_RenderClear(_renderer);
}

bool RenderManager::drawRect(const Rect &rect, const Color& color) const {
    Rect drawRect = convertRect(rect);
    SDL_SetRenderDrawColor(_renderer, color.r, color.g, color.b, color.a);
    return SDL_RenderFillRect(_renderer, &drawRect);
}

bool RenderManager::drawSprite(const Rect &rect, const Sprite *sprite, float rotation) const {
    Rect drawRect = convertRect(rect);
    if (drawRect.w == 0 || drawRect.h == 0) {
        return true;
    }
    return SDL_RenderTextureRotated(_renderer, sprite->getTexture()->texture, &sprite->getRect(), &drawRect, rotation, nullptr, SDL_FLIP_NONE);
}

bool RenderManager::drawText(const Rect &rect, const TextTexture *text, float rotation) const {
    Rect drawRect = convertRect(rect);
    if (drawRect.w == 0 || drawRect.h == 0) {
        return true;
    }
    return SDL_RenderTextureRotated(_renderer, text->getTexture(), nullptr, &drawRect, rotation, nullptr, SDL_FLIP_NONE);
}

void RenderManager::getWindowSize(int *width, int *height) const {
    SDL_GetWindowSize(_window,width,height);
}

void RenderManager::shutdown() const {
    SDL_DestroyRenderer(_renderer);
    SDL_DestroyWindow(_window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

std::pair<float, const Vector2&> RenderManager::setViewRect(const Vector2 &viewPosition, const Vector2 &viewSize)  {
    _viewOffset = viewPosition;
    int w, h;
    getWindowSize(&w, &h);
    _width = static_cast<float>(w);
    _height = static_cast<float>(h);
    float tempScaleX = _width / viewSize.getX();
    float tempScaleY = _height / viewSize.getY();
    _screenScale = std::min(tempScaleY, tempScaleX);
    _screenOffset = Vector2(_width - viewSize.getX() * _screenScale, _height - viewSize.getY() * _screenScale)/2.f;
    return std::pair<float, Vector2&>(_screenScale, _screenOffset);
}

Rect RenderManager::convertRect(const Rect &rect) const {
    Rect tempRect = {
        (rect.x - _viewOffset.getX()) * _screenScale + _screenOffset.getX(),
        (rect.y - _viewOffset.getY()) * _screenScale + _screenOffset.getY(),
        rect.w * _screenScale,
        rect.h * _screenScale
    };
    if (tempRect.x >= _width || tempRect.x < -tempRect.w || tempRect.y >= _height || tempRect.y < -tempRect.h)
        return {0,0,0,0};
    return tempRect;
}
