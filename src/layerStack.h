#pragma once
#include "layer.h"

namespace Sura {

	class LayerStack {
	public:

		LayerStack();
		~LayerStack();

		void pushLayer(Layer* layer);
		void popLayer(Layer* layer);
		void pushOverlay(Layer* overlay);
		void popOverlay(Layer* overlay);

		void onUpdate();
		void onEvent(Event& event);
	private:
		std::vector<Layer*> m_layers;
		std::vector<Layer*> m_overlays;
	};


}
