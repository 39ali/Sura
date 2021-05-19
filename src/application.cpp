#include "application.h"

namespace Sura {


	Application::Application() {

		WindowInfo info{ "Sura",1280 , 720 };
		m_window = std::unique_ptr<Window>{ Window::create(info) };
		m_window->setEventCallbackFn(BIND_EVENT(Application::onEvent));
	}
	Application::~Application() {}

	void Application::onEvent(Event& e) {

		EventDispatcher dispatcher(e);

		dispatcher.dispatch<WindowCloseEvent>(BIND_EVENT(Application::onWindowClose));


	}

	bool Application::onWindowClose(WindowCloseEvent& e) {

		m_running = false;

		return true;
	}

	void Application::run() {

		WindowResizeEvent e{ 300,300 };

		if (e.isInCategory(EventCategoryInput)) {

			printf("lol");
		}

		while (m_running ) {
			m_window->onUpdate();

		};

	}

}