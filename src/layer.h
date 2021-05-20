#pragma once
#include <events/event.h>

namespace Sura {

	class Layer {
	public:

		Layer(const std::string& name = "Layer") :m_name(name) {};
		virtual ~Layer() {};

		virtual void onUpdate() {}
		virtual void onEvent(Event& event) {}
	protected:
		std::string m_name;
	};


}
