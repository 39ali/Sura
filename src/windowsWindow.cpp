#include "windowsWindows.h"

#pragma warning(disable : 26812)
#include <SDL.h>
#include <SDL_vulkan.h>

namespace Sura {

	Window* Window::create(const WindowInfo& info) {

		return new WindowsWindow(info);
	}

	WindowsWindow::WindowsWindow(const WindowInfo& info) {
		init(info);
	}

	WindowsWindow::~WindowsWindow() {
		printf("~WindowsWindow");
		shutdown();
	}

	void WindowsWindow::onUpdate() {

	}

	void WindowsWindow::init(const WindowInfo& info) {
		m_info = info;

		// We initialize SDL and create a window with it.
		SDL_Init(SDL_INIT_VIDEO);

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

		m_window = SDL_CreateWindow(m_info.title.c_str(), SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, m_info.width,
			m_info.height, window_flags);

		assert(m_window && "SDL window creation error");

	}
	void WindowsWindow::shutdown() {

		SDL_DestroyWindow(m_window);
	}

}