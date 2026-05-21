#include "game.h"

//private functions

void game::initvariable()
{
	this -> window = nullptr;

	this->state = GameState::MENU;
	this->difficulty = 1;

	//game logic
	this->points = 0;
	this->EndGame = false;
	this->health = 50; // increased health
	this->enemiesSpawnTimerMax = 2.f; // slowed down spawning
	this->enemiesSpawnTimer = this->enemiesSpawnTimerMax;
	this->maxEnemies = 5; // fewer enemies at once for slower pace
	this->mouseHeld = false;
	
	// Add laser fire timer logic
	this->laserFireTimerMax = 15.f; // The lower this value, the faster the rate of fire
	this->laserFireTimer = this->laserFireTimerMax;

	// Abilities
	this->invincibilityTimer = 0.f;
	this->deathRayTimer = 0.f;
	this->fastFireTimer = 0.f;
	this->slowTimeTimer = 0.f;

	this->isBossStage = false;
}

void game::initwindow()
{
	this->dimensions.height = 800;
	this->dimensions.width = 800;
	this->window = new sf::RenderWindow(this->dimensions, "game 1", sf::Style::Default);
	this->window->setFramerateLimit(120);
}

void game::initPlayer()
{
	this->player.setPointCount(3);
	this->player.setPoint(0, sf::Vector2f(0.f, -30.f));
	this->player.setPoint(1, sf::Vector2f(-20.f, 20.f));
	this->player.setPoint(2, sf::Vector2f(20.f, 20.f));
	this->player.setFillColor(sf::Color::Cyan);
	this->player.setPosition(this->window->getSize().x / 2.f, this->window->getSize().y - 50.f);
}

void game::initBackground()
{
	for (int i = 0; i < 100; ++i) {
		Star star;
		star.shape.setRadius(rand() % 2 + 1);
		star.shape.setFillColor(sf::Color(255, 255, 255, rand() % 150 + 50));
		star.shape.setPosition(rand() % this->window->getSize().x, rand() % this->window->getSize().y);
		star.speed = (rand() % 50 + 10) / 10.f;
		this->stars.push_back(star);
	}
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

void game::initMenuTexts()
{
	this->PlayText.setFont(this->font);
	this->PlayText.setCharacterSize(50);
	this->PlayText.setFillColor(sf::Color::White);
	this->PlayText.setString("PLAY");
	this->PlayText.setPosition(this->window->getSize().x / 2.f - 50.f, 300.f);

	this->QuitText.setFont(this->font);
	this->QuitText.setCharacterSize(50);
	this->QuitText.setFillColor(sf::Color::White);
	this->QuitText.setString("QUIT");
	this->QuitText.setPosition(this->window->getSize().x / 2.f - 50.f, 400.f);

	this->DiffEasyText.setFont(this->font);
	this->DiffEasyText.setCharacterSize(40);
	this->DiffEasyText.setFillColor(sf::Color::White);
	this->DiffEasyText.setString("EASY");
	this->DiffEasyText.setPosition(this->window->getSize().x / 2.f - 50.f, 250.f);

	this->DiffNormalText.setFont(this->font);
	this->DiffNormalText.setCharacterSize(40);
	this->DiffNormalText.setFillColor(sf::Color::White);
	this->DiffNormalText.setString("NORMAL");
	this->DiffNormalText.setPosition(this->window->getSize().x / 2.f - 70.f, 350.f);

	this->DiffHardText.setFont(this->font);
	this->DiffHardText.setCharacterSize(40);
	this->DiffHardText.setFillColor(sf::Color::White);
	this->DiffHardText.setString("HARD");
	this->DiffHardText.setPosition(this->window->getSize().x / 2.f - 50.f, 450.f);
}




void game::spawnEnemyWithoutOverlap()
{
	// Temporary enemy to check for overlaps
	EnemyData newEnemy; 

	// Randomly assign size, color and hp based on enemy type (removed the smallest ones)
	int enemyType = rand() % 3;

	switch (enemyType)
	{
	case 0:
		newEnemy.shape.setSize(sf::Vector2f(50.f, 50.f));
		newEnemy.shape.setFillColor(sf::Color::Yellow);
		newEnemy.hp = 2;
		break;
	case 1:
		newEnemy.shape.setSize(sf::Vector2f(70.f, 70.f));
		newEnemy.shape.setFillColor(sf::Color::Magenta);
		newEnemy.hp = 3;
		break;
	case 2:
		newEnemy.shape.setSize(sf::Vector2f(100.f, 100.f));
		newEnemy.shape.setFillColor(sf::Color::Red);
		newEnemy.hp = 5;
		break;
	}

	// We'll try up to 10 times to find a valid non-overlapping position
	const int maxSpawnAttempts = 10;
	bool validSpawnFound = false;

	for (int attempt = 0; attempt < maxSpawnAttempts && !validSpawnFound; ++attempt)
	{
		// Generate a random X position within window width - enemy width
		float spawnPosX = static_cast<float>(rand() % static_cast<int>(this->window->getSize().x - newEnemy.shape.getSize().x));
		float spawnPosY = 0.f; // Always spawn at the top

		newEnemy.shape.setPosition(spawnPosX, spawnPosY);

		// Assume it's valid unless we find a collision
		validSpawnFound = true;

		for (const auto& existingEnemy : this->enemies)
		{
			if (newEnemy.shape.getGlobalBounds().intersects(existingEnemy.shape.getGlobalBounds()))	
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
		
	// Boss trigger check
	if (this->points >= 500 && !this->bossEntity.isActive()) {
		this->isBossStage = true;
	}

	if (!this->isBossStage) {
		//move the enemies
		/*loops from 0 to maxium no of enemies and moves enemies induvidually */

		float speedMult = (this->slowTimeTimer > 0.f) ? 0.5f : 1.0f;
		for (int i = 0; i < this->enemies.size(); i++)
		{
			this->enemies[i].shape.move(0.f, 1.0f * speedMult); // slower speed

			// Check intersection with player
			if (this->enemies[i].shape.getGlobalBounds().intersects(this->player.getGlobalBounds())) {
				if (this->invincibilityTimer <= 0.f) this->health -= 5; // minus health when hitting player
				this->enemies.erase(this->enemies.begin() + i);
				i--;
				continue;
			}

			if (this->enemies[i].shape.getPosition().y > this->window->getSize().y)
			{
				this->health -= 1;
				this->enemies.erase(this->enemies.begin() + i);
				i--;
			}
		}
	}

	// Increment laser fire timer
	if (this->laserFireTimer < this->laserFireTimerMax)
	{
		this->laserFireTimer += 1.f;
	}

	//check if clicked upon (now shoots laser!)
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{
		if (this->laserFireTimer >= this->laserFireTimerMax)
		{
			this->laserFireTimer = 0.f; // Reset timer
			
			sf::RectangleShape laser;
			if (this->deathRayTimer > 0.f) {
				laser.setSize(sf::Vector2f(20.f, 40.f));
				laser.setFillColor(sf::Color::Red);
			} else {
				laser.setSize(sf::Vector2f(4.f, 20.f));
				laser.setFillColor(sf::Color::Cyan);
			}
			laser.setPosition(this->player.getPosition().x - laser.getSize().x / 2.f, this->player.getPosition().y - 30.f);
			this->lasers.push_back(laser);
		}
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
	this->initMenuTexts();
	this->initPlayer();
	this->initBackground();
	this->bossEntity.initBoss(this->window);
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

void game::updateMenu()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{
		if (!this->mouseHeld)
		{
			this->mouseHeld = true;
			if (this->PlayText.getGlobalBounds().contains(this->mousePosView))
			{
				this->state = GameState::DIFFICULTY_SELECT;
			}
			else if (this->QuitText.getGlobalBounds().contains(this->mousePosView))
			{
				this->window->close();
			}
		}
	}
	else
	{
		this->mouseHeld = false;
	}
	
	// Hover effects
	if (this->PlayText.getGlobalBounds().contains(this->mousePosView))
		this->PlayText.setFillColor(sf::Color::Cyan);
	else
		this->PlayText.setFillColor(sf::Color::White);

	if (this->QuitText.getGlobalBounds().contains(this->mousePosView))
		this->QuitText.setFillColor(sf::Color::Red);
	else
		this->QuitText.setFillColor(sf::Color::White);
}

void game::updateDifficultySelect()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
	{
		if (!this->mouseHeld)
		{
			this->mouseHeld = true;
			if (this->DiffEasyText.getGlobalBounds().contains(this->mousePosView))
			{
				this->difficulty = 0;
				this->enemiesSpawnTimerMax = 3.f;
				this->maxEnemies = 3;
				this->state = GameState::PLAYING;
			}
			else if (this->DiffNormalText.getGlobalBounds().contains(this->mousePosView))
			{
				this->difficulty = 1;
				this->enemiesSpawnTimerMax = 2.f;
				this->maxEnemies = 5;
				this->state = GameState::PLAYING;
			}
			else if (this->DiffHardText.getGlobalBounds().contains(this->mousePosView))
			{
				this->difficulty = 2;
				this->enemiesSpawnTimerMax = 1.f;
				this->maxEnemies = 8;
				this->state = GameState::PLAYING;
			}
		}
	}
	else
	{
		this->mouseHeld = false;
	}

	// Hover effects
	if (this->DiffEasyText.getGlobalBounds().contains(this->mousePosView))
		this->DiffEasyText.setFillColor(sf::Color::Green);
	else
		this->DiffEasyText.setFillColor(sf::Color::White);

	if (this->DiffNormalText.getGlobalBounds().contains(this->mousePosView))
		this->DiffNormalText.setFillColor(sf::Color::Yellow);
	else
		this->DiffNormalText.setFillColor(sf::Color::White);

	if (this->DiffHardText.getGlobalBounds().contains(this->mousePosView))
		this->DiffHardText.setFillColor(sf::Color::Red);
	else
		this->DiffHardText.setFillColor(sf::Color::White);
}

void game::updatemousePositions()
{
	/*updates the mouse postion relatded to the window*/
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

void game::spawnPowerUp(sf::Vector2f pos)
{
	// 15% chance to drop a power up when an enemy dies
	if (rand() % 100 < 15) {
		PowerUp p;
		p.type = rand() % 5;
		p.shape.setRadius(10.f);
		p.shape.setPosition(pos);
		
		switch (p.type) {
			case 0: p.shape.setFillColor(sf::Color::Yellow); break; // Invincibility
			case 1: p.shape.setFillColor(sf::Color::Red); break; // DeathRay
			case 2: p.shape.setFillColor(sf::Color::Magenta); break; // CircularBlast
			case 3: p.shape.setFillColor(sf::Color::Cyan); break; // FastFire
			case 4: p.shape.setFillColor(sf::Color::Blue); break; // SlowTime
		}
		this->powerUps.push_back(p);
	}
}

void game::updatePowerUps()
{
	bool isInvincible = this->invincibilityTimer > 0.f;
	
	if (this->invincibilityTimer > 0.f) this->invincibilityTimer -= 1.f;
	
	if (this->fastFireTimer > 0.f) {
		this->fastFireTimer -= 1.f;
		this->laserFireTimerMax = 5.f;
	} else {
		// Reset based on difficulty or base value
		if (this->difficulty == 0) this->laserFireTimerMax = 15.f;
		else if (this->difficulty == 1) this->laserFireTimerMax = 15.f;
		else this->laserFireTimerMax = 15.f; 
	}
	
	if (this->deathRayTimer > 0.f) this->deathRayTimer -= 1.f;
	if (this->slowTimeTimer > 0.f) this->slowTimeTimer -= 1.f;

	if (this->invincibilityTimer > 0.f) this->player.setFillColor(sf::Color::Yellow);
	else this->player.setFillColor(sf::Color::Cyan);

	for (size_t i = 0; i < this->powerUps.size(); i++) {
		this->powerUps[i].shape.move(0.f, 2.f);
		
		if (this->powerUps[i].shape.getGlobalBounds().intersects(this->player.getGlobalBounds())) {
			int t = this->powerUps[i].type;
			if (t == 0) this->invincibilityTimer = 600.f;
			else if (t == 1) this->deathRayTimer = 600.f;
			else if (t == 2) {
				// Circular blast effect instantly
				for (auto& e : this->enemies) {
					this->points += 10;
					for (int p = 0; p < 15; p++) {
						Particle particle;
						particle.shape.setSize(sf::Vector2f(4.f, 4.f));
						particle.shape.setFillColor(e.shape.getFillColor());
						particle.shape.setPosition(
							e.shape.getPosition().x + e.shape.getSize().x / 2.f, 
							e.shape.getPosition().y + e.shape.getSize().y / 2.f);
						float angle = (rand() % 360) * 3.14159f / 180.f;
						float speed = (rand() % 50 + 10) / 10.f;
						particle.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
						particle.lifetime = 0;
						particle.maxLifetime = rand() % 30 + 20;
						this->particles.push_back(particle);
					}
				}
				this->enemies.clear();
				if (this->bossEntity.isActive()) {
					this->bossEntity.takeDamage(50);
				}
			}
			else if (t == 3) this->fastFireTimer = 600.f;
			else if (t == 4) this->slowTimeTimer = 600.f;

			this->powerUps.erase(this->powerUps.begin() + i);
			i--;
			continue;
		}

		if (this->powerUps[i].shape.getPosition().y > this->window->getSize().y) {
			this->powerUps.erase(this->powerUps.begin() + i);
			i--;
		}
	}
}

void game::renderPowerUps(sf::RenderTarget& target)
{
	for (auto& p : this->powerUps) {
		target.draw(p.shape);
	}
}

void game::updateEnemies()
{
	if (this->isBossStage) {
		// Clear existing normal enemies
		this->enemies.clear();
		return;
	}

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

void game::updatePlayer()
{
	this->player.setPosition(this->mousePosView.x, this->window->getSize().y - 50.f);
	
	// Keep player in bounds
	if (this->player.getPosition().x < 20.f)
		this->player.setPosition(20.f, this->player.getPosition().y);
	if (this->player.getPosition().x > this->window->getSize().x - 20.f)
		this->player.setPosition(this->window->getSize().x - 20.f, this->player.getPosition().y);
}

void game::updateLasers()
{
	for (size_t i = 0; i < this->lasers.size(); i++)
	{
		this->lasers[i].move(0.f, -8.f); // slightly slower lasers
		if (this->lasers[i].getPosition().y < 0)
		{
			this->lasers.erase(this->lasers.begin() + i);
			i--;
			continue;
		}

		bool laserDeleted = false;

		// Boss hit check
		if (this->bossEntity.isActive() && this->lasers[i].getGlobalBounds().intersects(this->bossEntity.getBounds())) {
			if (this->lasers[i].getFillColor() == sf::Color::Red) {
				this->bossEntity.takeDamage(5); // Death Ray
			} else {
				this->bossEntity.takeDamage(1);
			}
			
			// hit particles
			Particle p;
			p.shape.setSize(sf::Vector2f(4.f, 4.f));
			p.shape.setFillColor(sf::Color::White);
			p.shape.setPosition(this->lasers[i].getPosition());
			p.velocity = sf::Vector2f((rand() % 40 - 20) / 10.f, (rand() % 40 - 20) / 10.f);
			p.lifetime = 0;
			p.maxLifetime = 15;
			this->particles.push_back(p);
			
			this->lasers.erase(this->lasers.begin() + i);
			i--;
			laserDeleted = true;
			continue;
		}

		for (size_t k = 0; k < this->enemies.size() && !laserDeleted; k++)
		{
			if (this->lasers[i].getGlobalBounds().intersects(this->enemies[k].shape.getGlobalBounds()))
			{
				if (this->lasers[i].getFillColor() == sf::Color::Red) {
					this->enemies[k].hp -= 5;
				} else {
					this->enemies[k].hp -= 1;
				}
				
				if (this->enemies[k].hp <= 0) {
					//gain points
					if (this->enemies[k].shape.getFillColor() == sf::Color::Yellow)
					{
						this->points += 5.f;
					}
					else if (this->enemies[k].shape.getFillColor() == sf::Color::Magenta)
					{
						this->points += 10.f;
					}
					else if (this->enemies[k].shape.getFillColor() == sf::Color::Red)
					{
						this->points += 20.f;
					}

					// Spawn power-up
					this->spawnPowerUp(this->enemies[k].shape.getPosition());

					// Create destroy particles
					for (int p = 0; p < 15; p++)
					{
						Particle particle;
						particle.shape.setSize(sf::Vector2f(4.f, 4.f));
						particle.shape.setFillColor(this->enemies[k].shape.getFillColor());
						particle.shape.setPosition(
							this->enemies[k].shape.getPosition().x + this->enemies[k].shape.getSize().x / 2.f, 
							this->enemies[k].shape.getPosition().y + this->enemies[k].shape.getSize().y / 2.f);
						float angle = (rand() % 360) * 3.14159f / 180.f;
						float speed = (rand() % 50 + 10) / 10.f;
						particle.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
						particle.lifetime = 0;
						particle.maxLifetime = rand() % 30 + 20;
						this->particles.push_back(particle);
					}
					
					this->enemies.erase(this->enemies.begin() + k);
				} else {
					// Hit particle
					Particle p;
					p.shape.setSize(sf::Vector2f(3.f, 3.f));
					p.shape.setFillColor(sf::Color::White);
					p.shape.setPosition(this->lasers[i].getPosition());
					p.velocity = sf::Vector2f((rand()%20-10)/10.f, (rand()%20-10)/10.f);
					p.lifetime = 0;
					p.maxLifetime = 10;
					this->particles.push_back(p);
				}

				this->lasers.erase(this->lasers.begin() + i);
				i--;
				laserDeleted = true;
			}
		}
	}
}

void game::updateParticles()
{
	for (size_t i = 0; i < this->particles.size(); i++)
	{
		this->particles[i].shape.move(this->particles[i].velocity);
		this->particles[i].lifetime++;
		
		int alpha = 255 - (255 * (this->particles[i].lifetime / this->particles[i].maxLifetime));
		sf::Color color = this->particles[i].shape.getFillColor();
		color.a = alpha > 0 ? alpha : 0;
		this->particles[i].shape.setFillColor(color);

		if (this->particles[i].lifetime >= this->particles[i].maxLifetime)
		{
			this->particles.erase(this->particles.begin() + i);
			i--;
		}
	}
}

void game::updateBackground()
{
	for (auto& star : this->stars)
	{
		star.shape.move(0.f, star.speed);
		if (star.shape.getPosition().y > this->window->getSize().y)
		{
			star.shape.setPosition(rand() % this->window->getSize().x, 0.f);
		}
	}
}

void game::update()
{
	this->pollevent();
	this->updatemousePositions();
	this->updateBackground();

	if (this->state == GameState::MENU)
	{
		this->updateMenu();
	}
	else if (this->state == GameState::DIFFICULTY_SELECT)
	{
		this->updateDifficultySelect();
	}
	else if (this->state == GameState::PLAYING && !this->EndGame)
	{
		this->updateText();
		this->updatePowerUps();
		if (!this->isBossStage) {
			this->updateEnemies();
		} else {
			this->bossEntity.update(this->health, this->player, this->points, this->isBossStage, this->invincibilityTimer > 0.f, this->slowTimeTimer > 0.f);
		}
		this->updatePlayer();
		this->updateLasers();
		this->updateParticles();

		if (this->health <= 0)
		{
			this->EndGame = true;
			this->state = GameState::GAME_OVER;
		}
	}
}

void game::renderMenu(sf::RenderTarget& target)
{
	target.draw(this->PlayText);
	target.draw(this->QuitText);
}

void game::renderDifficultySelect(sf::RenderTarget& target)
{
	target.draw(this->DiffEasyText);
	target.draw(this->DiffNormalText);
	target.draw(this->DiffHardText);
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
		target.draw(e.shape);
	}
}

void game::renderPlayer(sf::RenderTarget& target)
{
	target.draw(this->player);
}

void game::renderLasers(sf::RenderTarget& target)
{
	for (auto& l : this->lasers) {
		target.draw(l);
	}
}

void game::renderParticles(sf::RenderTarget& target)
{
	for (auto& p : this->particles) {
		target.draw(p.shape);
	}
}

void game::renderBackground(sf::RenderTarget& target)
{
	sf::RectangleShape bg(sf::Vector2f(this->window->getSize().x, this->window->getSize().y));
	bg.setFillColor(sf::Color(10, 10, 25)); // Dark blue background
	target.draw(bg);

	for (auto& star : this->stars) {
		target.draw(star.shape);
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
	this->renderBackground(*this->window);

	if (this->state == GameState::MENU)
	{
		this->renderMenu(*this->window);
	}
	else if (this->state == GameState::DIFFICULTY_SELECT)
	{
		this->renderDifficultySelect(*this->window);
	}
	else if (this->state == GameState::PLAYING && !this->EndGame) 
	{
		if (!this->isBossStage) {
			this->renderEnemies(*this->window);
		} else {
			this->bossEntity.render(*this->window);
		}
		this->renderPowerUps(*this->window);
		this->renderLasers(*this->window);
		this->renderParticles(*this->window);
		this->renderPlayer(*this->window);
		this->renderText(*this->window);
	}
	else if (this->state == GameState::GAME_OVER || this->EndGame)
	{
		this->window->draw(this->GameOverText);
	}
	this->window->display();
}

