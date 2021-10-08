#pragma warning(disable : 26812)
#include <SDL.h>
#include <SDL_vulkan.h>
#include "events/applicationEvent.h"
#include "events/keyEvent.h"
#include "events/mouseEvent.h"

#include "windowsWindows.h"

namespace Sura {

Window* Window::m_instance = nullptr;
Window* Window::create(const WindowInfo& info) {
  Window::m_instance = new WindowsWindow(info);
  return Window::m_instance;
}

WindowsWindow::WindowsWindow(const WindowInfo& info) {
  init(info);
}

WindowsWindow::~WindowsWindow() {
  shutdown();
}

void WindowsWindow::onUpdate() {
  pollEvents();
}

void WindowsWindow::init(const WindowInfo& info) {
  m_info = info;

  // We initialize SDL and create a window with it.
  SDL_Init(SDL_INIT_VIDEO);

  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

  m_window = SDL_CreateWindow(m_info.title.c_str(), SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, m_info.width,
                              m_info.height, window_flags);

  assert(m_window && "SDL window creation error");
}

void WindowsWindow::pollEvents() {
  if (!m_eventCallbackFn) {
    return;
  }

  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    switch (e.type) {
      case SDL_KEYDOWN: {
        KeyPressedEvent ev(e.key.keysym.scancode, e.key.repeat);
        m_eventCallbackFn(ev);
        break;
      }

      case SDL_KEYUP: {
        KeyReleasedEvent ev(e.key.keysym.scancode);
        m_eventCallbackFn(ev);
        break;
      }

      case SDL_MOUSEBUTTONUP: {
        MouseButtonReleasedEvent ev(e.button.button);
        m_eventCallbackFn(ev);
        break;
      }
      case SDL_MOUSEBUTTONDOWN: {
        MouseButtonPressedEvent ev(e.button.button);
        m_eventCallbackFn(ev);
        break;
      }

      case SDL_MOUSEMOTION: {
        MouseMovedEvent ev(e.button.x, e.button.y);
        m_eventCallbackFn(ev);
        break;
      }

      case SDL_MOUSEWHEEL: {
        MouseScrolledEvent ev(e.wheel.x, e.wheel.y);
        m_eventCallbackFn(ev);
        break;
      }

      case SDL_WINDOWEVENT: {
        switch (e.window.event) {
          case SDL_WINDOWEVENT_CLOSE: {
            WindowCloseEvent ev;
            m_eventCallbackFn(ev);
            break;
          }
          case SDL_WINDOWEVENT_RESIZED: {
            WindowResizeEvent ev(e.window.data1, e.window.data2);
            m_eventCallbackFn(ev);
            break;
          }
        }
      }
    }
  }
}

void WindowsWindow::shutdown() {
  SDL_DestroyWindow(m_window);
}

}  // namespace Sura