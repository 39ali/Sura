#pragma once

#define ATTACH_APP(APP)			\
Sura::Application* createApp() {\
								\
	return new APP();			\
}								\


extern Sura::Application* createApp();


int main(int argc, char* args[])
{
	Sura::Application* app = createApp();
	app->run();
	delete app;

	return 0;
};


