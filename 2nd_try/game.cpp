#include "game.h"

//private functions

void game::initvariable()
{
	this -> window = nullptr;

	//game logic
	this->points = 0;
	this->EndGame = false;
	this->health = 20;
	this->enemiesSpawnTimerMax = 1.f;
	this->enemiesSpawnTimer = this->enemiesSpawnTimerMax;
	this->maxEnemies = 7;
	this->mouseHeld = false;
}

void game::initwindow()
{
	this->dimensions.height = 800;
	this->dimensions.width = 800;
	this->window = new sf::RenderWindow(this->dimensions, "game 1", sf::Style::Default);
	this->window->setFramerateLimit(120);
}

void game::initEnemies()
{
	this->enemy.setPosition(10.f,10.f);
	this->enemy.setSize(sf::Vector2f(100.0f,100.0f));
	this->enemy.setFillColor(sf::Color::Black);
	//this->enemy.setOutlineColor(sf::Color::Red);
	//this->enemy.setOutlineThickness(1.0f);

}
void game::initfonts()
{
	if (this->font.loadFromFile("fonts/RushDriver-Italic.otf")==false)
	{
		std::cout << "failed to load fonts " << std::endl;
	}
}
void game::initText()
{
	this->UiText.setFont(this->font);
	this->UiText.setCharacterSize(24);
	this->UiText.setFillColor(sf::Color::White);
	this->UiText.setString("");
}

void game::initGameOverText() 
{
	this->GameOverText.setFont(this->font);
	this->GameOverText.setCharacterSize(40);
	this->GameOverText.setFillColor(sf::Color::Red);
	this->GameOverText.setString("     GAME OVER\nPress ESC to exit");
	

	sf::FloatRect textbound = this->GameOverText.getLocalBounds();
	this->GameOverText.setOrigin(
		textbound.left + textbound.width / 2.0f,
		textbound.top + textbound.height / 2.0f
	);
	this->GameOverText.setPosition(
		this->window->getSize().x/2.0f,
		this->window->getSize().y/2.0f
	);

}



void game::spawnEnemyWithoutOverlap()
{
	sf::RectangleShape newEnemy; // Temporary enemy to check for overlaps

	// Randomly assign size and color based on enemy type
	int enemyType = rand() % 5;

	switch (enemyType)
	{
	case 0:
		newEnemy.setSize(sf::Vector2f(10.f, 10.f));
		newEnemy.setFillColor(sf::Color::Blue);
		break;
	case 1:
		newEnemy.setSize(sf::Vector2f(30.f, 30.f));
		newEnemy.setFillColor(sf::Color::Green);
		break;
	case 2:
		newEnemy.setSize(sf::Vector2f(50.f, 50.f));
		newEnemy.setFillColor(sf::Color::Yellow);
		break;
	case 3:
		newEnemy.setSize(sf::Vector2f(70.f, 70.f));
		newEnemy.setFillColor(sf::Color::Magenta);
		break;
	case 4:
		newEnemy.setSize(sf::Vector2f(100.f, 100.f));
		newEnemy.setFillColor(sf::Color::Red);
		break;
	}

	// We'll try up to 10 times to find a valid non-overlapping position
	const int maxSpawnAttempts = 10;
	bool validSpawnFound = false;

	for (int attempt = 0; attempt < maxSpawnAttempts && !validSpawnFound; ++attempt)
	{
		// Generate a random X position within window width - enemy width
		float spawnPosX = static_cast<float>(rand() % static_cast<int>(this->window->getSize().x - newEnemy.getSize().x));
		float spawnPosY = 0.f; // Always spawn at the top

		newEnemy.setPosition(spawnPosX, spawnPosY);

		// Assume it's valid unless we find a collision
		validSpawnFound = true;

		for (const auto& existingEnemy : this->enemies)
		{
			if (newEnemy.getGlobalBounds().intersects(existingEnemy.getGlobalBounds()))
			{
				validSpawnFound = false;
				break; // Overlap found, break inner loop
			}
		}
	}

	// If we found a valid spot, add it to the list of enemies
	if (validSpawnFound)
	{
		this->enemies.push_back(newEnemy);
	}
	// If no valid spawn point found in all attempts, we skip spawning this frame
}


void game::pollevent()
{
	//event polling

	while (this->window->pollEvent(this->ev))
	{
		switch (this->ev.type)
		{
		case sf::Event::Closed:
			//this->window->close();
			break;
		case sf::Event::KeyPressed:
			if (ev.key.code == sf::Keyboard::Escape &&this->EndGame)
				this->window->close();
			break;	
		}
	}
	if (this->EndGame)
		return;
	//move the enemies
	/*loops from 0 to maxium no of enemies and moves enemies induvidully */

	for (int i = 0; i < this->enemies.size(); i++)
	{
		bool deleted = false;

		this->enemies[i].move(0.f, 2.f);

		if (this->enemies[i].getPosition().y > this->window->getSize().y)
		{
			this->enemies.erase(this->enemies.begin() + i);
			this->health -= 1;
		}

	}
	//check if clicked upon
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{
		if (this->mouseHeld == false)
		{
			mouseHeld = true;
			bool deleted = false;
			for (size_t i = 0; i < this->enemies.size() && deleted == false;i++)
			{
				if (this->enemies[i].getGlobalBounds().contains(this->mousePosView))
				{

					//gain points
					if(this->enemies[i].getFillColor()==sf::Color::Blue)
					{
						this->points += 10.f;
					}
					else if (this->enemies[i].getFillColor() == sf::Color::Green)
					{
						this->points += 7.f;
					}
					else if (this->enemies[i].getFillColor() == sf::Color::Yellow)
					{
						this->points += 5.f;
					}
					else if (this->enemies[i].getFillColor() == sf::Color::Magenta)
					{
						this->points += 3.f;
					}
					else if (this->enemies[i].getFillColor() == sf::Color::Red)
					{
						this->points += 1.f;
					}
					else if (this->enemies[i].getFillColor() == sf::Color::White)
					{
						this->points += 1.f;
					}
					
					deleted = true;
					this->enemies.erase(this->enemies.begin() + i);

					
				}
			}
		}
	}
	else
	{
		this->mouseHeld = false;
	}

}

//constructors and destructors

game::game()
{
	this->initvariable();        // Sets defaults
	this->initwindow();          // Creates the window
	this->initfonts();           // Loads the font file
	this->initText();            // Uses font (needs font to be loaded!)
	this->initGameOverText();    // Uses window size (needs window to exist!)
	this->initEnemies();         // Optional enemy init
}


game::~game()
{
	delete this->window;
}

//tells window status

const bool game::running() const
{
	return this->window->isOpen();

}

const bool game::getEndGame() const
{
	return this->EndGame;
}


void game::updatemousePositions()
{
	/*updates the mouse postion relatred to the window*/
	this->mousePoswindow = sf::Mouse::getPosition(*this->window);
	this->mousePosView = this->window->mapPixelToCoords(this->mousePoswindow);
}


void game::updateText()
{
	std::stringstream ss;
	ss << "POINTS: " << this->points << std::endl
		<< "HEALTH: " << this->health << std::endl;
	this->UiText.setString(ss.str());

}

void game::updateEnemies()
{
	/*
		first checks if there is more enemies than allowed
		secondly checks to time spawn new enemy or not 
		if not adds 1 to to the current time till enemiesSpawnTimerMax (1000 time)
		moves the enemy downwards
	*/
	//updating the timer for enemy spawning 

	if (this->enemies.size() < this->maxEnemies) {

		//spawn enemy and reset the timer
		if (this->enemiesSpawnTimer >= this->enemiesSpawnTimerMax)
		{
		
			this->spawnEnemyWithoutOverlap();
			this->enemiesSpawnTimer = 0.0f;
		}
		else
			this->enemiesSpawnTimer += 1.f;
	}
}

void game::update()
{
	this->pollevent();

	if (!this->EndGame)
	{
		this->updatemousePositions();
		this->updateText();
		this->updateEnemies();	
	}
	if (this->health <= 0)
	{
		this->EndGame = true;
	}
	
	
}

void game::renderText(sf::RenderTarget& target)
{
	target.draw(this->UiText);
}

void game::renderEnemies(sf::RenderTarget& target)
{
	//rendering all the enemies
	for (auto& e : this->enemies)
	{
		target.draw(e);
	}
}

void game::render()
{
	/*
	
	1)clears old frames
	2)render new objects
	3)display frames in window

	renders the game objects
	*/

	this->window->clear(sf::Color::Black);
	if (!this->EndGame) 
	{
		this->renderEnemies(*this->window);
		this->renderText(*this->window);
	}
	else
	{
		this->window->draw(this->GameOverText);
	}
	this->window->display();
}
