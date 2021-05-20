#include "layerStack.h"


namespace Sura {

	LayerStack::LayerStack() {

	}
	LayerStack::~LayerStack() {
		for (Layer* layer : m_layers) {
			delete layer;
		}

		for (Layer* layer : m_overlays) {
			delete layer;
		}
	}

	void LayerStack::pushLayer(Layer* layer) {
		m_layers.push_back(layer);
	}

	void LayerStack::popLayer(Layer* layer) {

		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		if (it != m_layers.end()) {
			m_layers.erase(it);
		}
	}

	void  LayerStack::pushOverlay(Layer* overlay) {
		m_overlays.push_back(overlay);
	}


	void  LayerStack::popOverlay(Layer* overlay) {
		auto it = std::find(m_overlays.begin(), m_overlays.end(), overlay);
		if (it != m_overlays.end()) {
			m_overlays.erase(it);
		}
	};

	void LayerStack::onEvent(Event& event) {

		bool handeled = false;

		size_t size = m_overlays.size() - 1;
		for (int i = size; i > -1; i--) {
			m_overlays[i]->onEvent(event);
			if (event.m_handled) {
				handeled = true;
				break;
			}
		}

		if (handeled) {
			return;
		}

		size = m_layers.size() - 1;
		for (int i = size; i > -1; i--) {
			m_layers[i]->onEvent(event);
			if (event.m_handled) {
				break;
			}
		}


	}


	void LayerStack::onUpdate() {

		for (Layer* layer : m_layers) {
			layer->onUpdate();
		}

		for (Layer* overlay : m_overlays) {
			overlay->onUpdate();
		}
	}
}