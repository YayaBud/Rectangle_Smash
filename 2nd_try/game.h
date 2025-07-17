#pragma once

#include <iostream>
#include <vector>
#include <ctime>	
#include <sstream>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>


//making things move and rendering them 

class game
{
private:

	//variable window
	//window make 

	sf::RenderWindow * window;
	sf::VideoMode dimensions;
	sf::Event ev;

	//fonts
	sf::Font font;
	
	//text
	sf::Text UiText;
	sf::Text GameOverText;

	//mouse position

	sf::Vector2i mousePoswindow;
	sf::Vector2f mousePosView;

	//game logic
	
	unsigned points;
	int health;
	bool EndGame;
	float enemiesSpawnTimer;
	float enemiesSpawnTimerMax;
	int maxEnemies;
	bool mouseHeld;
	
	//functions for window

	void initvariable();
	void initwindow();
	void initEnemies();
	void initfonts();
	void initText();
	void initGameOverText();


	//game objects

	std::vector<sf::RectangleShape> enemies;
	sf::RectangleShape enemy;



public:
	//constructors and destructors

	game();
	virtual ~game();

	//tells the program its open
	const bool running() const;

	//tell program when to end
	const bool getEndGame() const;


	//final functions

	//void spawnEnemy();
	void spawnEnemyWithoutOverlap(); 


	/*pollevent() function:

	Handles events

	Moves enemies

	Checks for mouse click on enemies 

	basically all the events happening 
	*/
	void pollevent();

	//track the current position of the mouse relative to the game window in SFML
	void updatemousePositions();
	void updateText();
	void updateEnemies();
	//single function to make use of all in main function
	void update();
	

	void renderText(sf::RenderTarget& target);
	void renderEnemies(sf::RenderTarget& target);
	void render();

};

