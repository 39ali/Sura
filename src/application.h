#pragma once
#include "events/applicationEvent.h"
#include "events/event.h"
#include "layerStack.h"
#include "window.h"
namespace Sura {

class Application {
 public:
  Application();
  virtual ~Application();
  void run();
  void onEvent(Event& e);
  bool onWindowClose(WindowCloseEvent& e);
  void pushLayer(Layer* layer);
  void pushOverlay(Layer* layer);
  virtual void onUpdate();


 private:
  std::unique_ptr<Window> m_window;
  bool m_running = true;
  LayerStack m_layerStack;
};

}  // namespace Sura
