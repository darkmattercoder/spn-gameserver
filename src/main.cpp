#include <thread>
#include <chrono>

#include <iostream>

#include "Bot.h"
#include "Field.h"

int main(void)
{
	std::shared_ptr<Field> f = std::make_shared<Field>(60, 20, 30);

	f->newBot("testBot");

	for(std::size_t i = 0; /*forever*/ ; i++) {
		std::cout << "Frame number #" << i << std::endl;

		f->updateFood();
		f->consumeFood();

		f->moveAllBots();
		f->debugVisualization();

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	return 0;
}
