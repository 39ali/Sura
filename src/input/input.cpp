#include "input.h"
#include "SDL.h"
namespace Sura {

bool Input::isKeyPressed(KEYCODE keyCode) {
  const Uint8* keystate = SDL_GetKeyboardState(NULL);
  return keystate[static_cast<int>(keyCode)];
}

bool Input::isMouseButtonPressed(MOUSE_BUTTON btn) {
  return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON((int)btn);
}

std::tuple<int, int> Input::getMousePos() {
  int x = 0, y = 0;
  SDL_GetMouseState(&x, &y);
  return {x, y};
}

}  // namespace Sura
