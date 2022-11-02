#include "Engine/Game.h"
#include "GameLoop.h"

// Simple main function
int main(int argc, char** argv)
{
	// Run the game
	Game<GameLoop> game;
	game.Run();

	return 0;
}
