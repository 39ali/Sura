#pragma once
#include "event.h"
namespace Sura
{
	class WindowResizeEvent : public Event {

	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_width(width), m_height(height) {};

		inline unsigned int getWidth() const { return m_width; }
		inline unsigned int getHeight() const { return m_height; }


		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		unsigned int m_width, m_height;
	};



	class WindowCloseEvent : public Event {

	public:
		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};


}
