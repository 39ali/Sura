#pragma once
#include "event.h"

namespace Sura {

	class MouseMovedEvent : public Event {
	public:
		MouseMovedEvent(float x, float y) : m_x(x), m_y(y)
		{}
		inline float getX() { return m_x; }
		inline float getY() { return m_y; }

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_x, m_y;
	};

	class MouseScrolledEvent : public Event {
	public:
		MouseScrolledEvent(float xOffset, float yOffset) : m_xOffset(xOffset), m_yOffset(yOffset) {}
		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	private:
		float m_xOffset, m_yOffset;
	};


	class MouseButtonEvent : public Event {
	public : 
		inline int getMouseButton() const {
			return m_button;}
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	protected : 
		MouseButtonEvent(int button) : m_button(button) {}
		int m_button;
	};


	class MouseButtonPressedEvent : public MouseButtonEvent {
	public:
		MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}
		EVENT_CLASS_TYPE(MouseButtonPressed)
	};


	class MouseButtonReleasedEvent : public MouseButtonEvent {
	public:
		MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}
		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}
