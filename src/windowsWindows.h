#pragma once
#include "window.h"

struct SDL_Window;
namespace Sura {

class WindowsWindow : public Window {
 public:
  WindowsWindow(const WindowInfo& info);
  ~WindowsWindow();

  void onUpdate() override;

  inline unsigned int getWidth() const override { return m_info.width; };
  inline unsigned int getHeight() const override { return m_info.height; };
  void setEventCallbackFn(const EventCallbackFn& fn) override {
    m_eventCallbackFn = fn;
  };
  void pollEvents();
  virtual const void* getNativeWindow() override { return m_window; };

 private:
  void init(const WindowInfo& info);
  void shutdown();

 private:
  WindowInfo m_info;
  SDL_Window* m_window{nullptr};
};

}  // namespace Sura
