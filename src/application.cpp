#include "application.h"

namespace Sura {

Application::Application() {
  WindowInfo info{"Sura", 1280, 720};
  m_window = std::unique_ptr<Window>{Window::create(info)};
  m_window->setEventCallbackFn(BIND_EVENT(Application::onEvent));
}
Application::~Application() {}

void Application::onEvent(Event& e) {
  EventDispatcher dispatcher(e);

  dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT(Application::onWindowClose));

  m_layerStack.onEvent(e);
}

bool Application::onWindowClose(WindowCloseEvent& e) {
  m_running = false;
  return true;
}

void Application::pushLayer(Layer* layer) {
  m_layerStack.pushLayer(layer);
}
void Application::pushOverlay(Layer* layer) {
  m_layerStack.pushOverlay(layer);
}

void Application::onUpdate() {}

void Application::run() {
  WindowResizeEvent e{300, 300};

  while (m_running) {
    m_layerStack.onUpdate();
    m_window->onUpdate();
    this->onUpdate();
  }
}

}  // namespace Sura