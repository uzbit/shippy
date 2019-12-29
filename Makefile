all:
	g++ main.cpp object.cpp body.cpp loot.cpp game.cpp ship.cpp space.cpp -ggdb -o shippy -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf
