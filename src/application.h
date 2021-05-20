#pragma once
#include "window.h"
#include "events/event.h"
#include "events/applicationEvent.h"
#include "layerStack.h"
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
	private:
		std::unique_ptr<Window> m_window;
		bool m_running = true;
		LayerStack m_layerStack; 

	};

}
