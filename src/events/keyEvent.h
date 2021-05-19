#pragma once

#include "event.h"

namespace Sura {

	class KeyEvent : public Event {
	public:
		inline int getKeycode() const { return m_keycode; }

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryApplication)
	protected:
		KeyEvent(int keycode) :m_keycode(keycode) {}

		int m_keycode;
	};

	class KeyPressedEvent : public KeyEvent {
	public:
		KeyPressedEvent(int keycode, int repeatCount) :
			KeyEvent(keycode), m_repeatCount(repeatCount) {}

		inline int getRepeatCount() const { return m_repeatCount; }
		std::string toString() override {
			std::stringstream ss;
			ss << "KeyPressedEvent:" << m_keycode << ",repeat:" << m_repeatCount;

			return ss.str();
		}
		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_repeatCount;
	};



	class KeyReleasedEvent : public KeyEvent {
	public:
		KeyReleasedEvent(int keycode) :
			KeyEvent(keycode) {}

		std::string toString() override {
			std::stringstream ss;
			ss << "KeyReleasedEvent:" << m_keycode;

			return ss.str();
		}
		EVENT_CLASS_TYPE(KeyReleased)
	};

}
