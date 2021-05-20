#pragma once
#include "Sura.h"


class ExampleLayer : public Sura::Layer {
public:
	ExampleLayer() : Layer("example") {

	}

	void onUpdate() override {

	}

	void onEvent(Sura::Event& event) override {
		std::cout << "layer:" << event.toString() << std::endl;
		event.m_handled = true;
	}
};


class Sandbox : public Sura::Application {

public:
	Sandbox() {

		pushLayer(new ExampleLayer());
	}
	~Sandbox() {}

};

ATTACH_APP(Sandbox);


