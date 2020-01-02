all:
	g++ main.cpp object.cpp body.cpp loot.cpp game.cpp ship.cpp space.cpp -O3 -o shippy -lallegro -lallegro_primitives -lallegro_font -lallegro_ttf
