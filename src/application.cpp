#include "application.h"

#include "events/applicationEvent.h"

namespace Sura {


	Application::Application() {}
	Application::~Application() {}

	void Application::run() {
	
		WindowResizeEvent e{ 300,300 };

		if (e.isInCategory( EventCategoryInput)) {

			printf("lol");
		}

		while (true);
	
	}

}