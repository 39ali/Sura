#pragma once
#include "Sura.h"

class ExampleLayer : public Sura::Layer {
 public:
  ExampleLayer() : Layer("example") {}

  void onUpdate() override {}

  void onEvent(Sura::Event& event) override {
    std::cout << "layer:" << event.toString() << std::endl;
    event.m_handled = true;
  }
};

class Sandbox : public Sura::Application {
 public:
  Sandbox() {
    pushLayer(new ExampleLayer());

    init();
  }

  void init() {}

  void onUpdate() override {
    const bool g = (Sura::Input::isKeyPressed(Sura::KEYCODE::A));

    if (g) {
      l++;
    }

    if (Sura::Input::isMouseButtonPressed(Sura::MOUSE_BUTTON::BUTTON_LEFT)) {
    }

    auto [x, y] = Sura::Input::getMousePos();

    S_TRACE("lol{0}", 3);
  }
  ~Sandbox() {}

 private:
  int l = 0;
};

ATTACH_APP(Sandbox);
