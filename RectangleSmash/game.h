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

#include "boss.h"

enum class GameState {
	MENU,
	DIFFICULTY_SELECT,
	PLAYING,
	GAME_OVER
};

//making things move and rendering them 

class game
{
private:

	//variable window
	//window make 

	sf::RenderWindow * window;
	sf::VideoMode dimensions;
	sf::Event ev;

	GameState state;
	int difficulty; // 0=Easy, 1=Normal, 2=Hard

	//fonts
	sf::Font font;
	
	//text
	sf::Text UiText;
	sf::Text GameOverText;
	sf::Text PlayText;
	sf::Text QuitText;
	sf::Text DiffEasyText;
	sf::Text DiffNormalText;
	sf::Text DiffHardText;

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

	// Fire rate controls
	float laserFireTimer;
	float laserFireTimerMax;
	
	// Abilities
	struct PowerUp {
		sf::CircleShape shape;
		int type; // 0: Invincibility, 1: DeathRay, 2: CircularBlast, 3: FireRate, 4: SlowTime
	};
	std::vector<PowerUp> powerUps;
	
	float invincibilityTimer;
	float deathRayTimer;
	float fastFireTimer;
	float slowTimeTimer;

	bool isBossStage;

	Boss bossEntity;

	//functions for window

	void initvariable();
	void initwindow();
	void initEnemies();
	void initfonts();
	void initText();
	void initMenuTexts();
	void initGameOverText();


	//game objects

	struct EnemyData {
		sf::RectangleShape shape;
		int hp;
	};
	std::vector<EnemyData> enemies;

	sf::ConvexShape player;
	std::vector<sf::RectangleShape> lasers;

	struct Particle {
		sf::RectangleShape shape;
		sf::Vector2f velocity;
		float lifetime;
		float maxLifetime;
	};
	std::vector<Particle> particles;

	struct Star {
		sf::CircleShape shape;
		float speed;
	};
	std::vector<Star> stars;

	void initPlayer();
	void initBackground();
	void spawnPowerUp(sf::Vector2f pos);

	void updateMenu();
	void updateDifficultySelect();
	void updatePlayer();
	void updateLasers();
	void updateParticles();
	void updatePowerUps();
	void updateBackground();
	
	void renderMenu(sf::RenderTarget& target);
	void renderDifficultySelect(sf::RenderTarget& target);
	void renderPlayer(sf::RenderTarget& target);
	void renderLasers(sf::RenderTarget& target);
	void renderParticles(sf::RenderTarget& target);
	void renderPowerUps(sf::RenderTarget& target);
	void renderBackground(sf::RenderTarget& target);

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

