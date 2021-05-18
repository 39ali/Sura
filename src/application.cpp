#include "application.h"

#include "events/applicationEvent.h"

namespace Sura {


	Application::Application() {

		WindowInfo info{ "Sura",1920 , 720 };
		m_window = std::unique_ptr<Window>{ Window::create(info) };
	}
	Application::~Application() {}

	void Application::run() {

		WindowResizeEvent e{ 300,300 };

		if (e.isInCategory(EventCategoryInput)) {

			printf("lol");
		}

		while (m_running&&0) {
			m_window->onUpdate();

		};

	}

}