#pragma once

#include "events/event.h"


namespace Sura {

	using EventCallbackFn = std::function<void(Event&)>;

	struct WindowInfo {
		std::string title;
		unsigned int width;
		unsigned int height;
		WindowInfo() {};
		WindowInfo(const std::string& _title, unsigned int _width,
			unsigned int _height) : title(_title), height(_height), width(_width) {}
	};


	class Window {
	public:
		virtual ~Window() { printf("\nWindow"); };
		virtual void onUpdate() = 0;

		virtual unsigned int getWidth() const = 0;
		virtual unsigned int getHeight() const = 0;
		virtual void setEventCallbackFn(const EventCallbackFn& fn) = 0;
		static Window* create(const WindowInfo& info);
	protected:
		EventCallbackFn m_eventCallbackFn = nullptr;
	};


}
