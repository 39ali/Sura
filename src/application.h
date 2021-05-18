#pragma once
#include "window.h"
namespace Sura {

	class Application {
	public:
		Application();
		virtual ~Application();
		void run();

	private:
		std::unique_ptr<Window> m_window;
		bool m_running = true;
	};

}
