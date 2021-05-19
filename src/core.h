#pragma once


#define BIT(x) (1 << x)

#define BIND_EVENT(func) std::bind(&func , this,std::placeholders::_1)
