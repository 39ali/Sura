#pragma once
#include "window.h"
#include "events/event.h"
#include "events/applicationEvent.h"

namespace Sura {

	class Application {
	public:
		Application();
		virtual ~Application();
		void run();
		void onEvent(Event& e);
		bool onWindowClose(WindowCloseEvent& e);
	private:
		std::unique_ptr<Window> m_window;
		bool m_running = true;
	};

}
