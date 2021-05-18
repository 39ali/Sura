#pragma once
#include "core.h"

namespace Sura {

	enum class EventType {
		None = 0,
		WindowClose,WindowResize,
		KeyPressed, KeyReleased,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

#pragma warning(disable : 26812)
	enum  EventCategory {
		None = 0,
		EventCategoryKeyboard = BIT(0),
		EventCategoryMouse = BIT(1),
		EventCategoryInput = BIT(2),
		EventCategoryApplication = BIT(3),
	};


#define EVENT_CLASS_TYPE(t) static EventType getStaticType() {return EventType::##t;}\
							virtual EventType getEventType() const override {return getStaticType();}\
							virtual const char* getName()const override{ return #t ;}


#define EVENT_CLASS_CATEGORY(category) virtual int getCategoryFlags() const override {return category;}

	class Event {

	public:
		virtual EventType getEventType() const = 0;
		virtual const char* getName() const = 0;
		virtual int getCategoryFlags() const = 0;
		inline bool isInCategory(EventCategory category) {
			return getCategoryFlags() & (int)category;
		}
	protected:
		bool m_handled = false; 
	};



	class EventDispatcher {
		template <typename T>
		using EventCallback = std::function<bool(T&)>;
	public:
		EventDispatcher(Event& event): m_event(event){}

		template<typename T> 
		bool dispatch(EventCallback<T> func) {
		
			if (m_event.getEventType() == T::getStaticType()) {
				m_event.m_handled = func(m_event); //func(*(T*)(&m_event));
				return true; 
			}
			return false;
		}

	private:
	Event& m_event; 
	};

}
