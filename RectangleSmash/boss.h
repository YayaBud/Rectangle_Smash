#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct BossProj {
	sf::CircleShape shape;
	sf::Vector2f velocity;
	int hp;
};

struct BossData {
	sf::RectangleShape shape;
	int hp;
	int maxHp;
	float attackTimer;
	int state; // 0: normal, 1: beam warning, 2: beam active
	float stateTimer;
	sf::Vector2f moveDir;
};

struct BossMinion {
	sf::CircleShape shape;
	sf::Vector2f velocity;
	float teleportTimer;
	float teleportTimerMax;
	float shootTimer;
	float shootTimerMax;
	int hp;
};

struct BossPowerUp {
	sf::CircleShape shape;
	int type; // 0: health+10, 1: invincibility, 2: deathray, 3: fastfire
};

class Boss
{
private:
	BossData bossData;
	bool bossActive;
	std::vector<BossProj> bossProjectiles;
	std::vector<BossMinion> bossMinions;
	std::vector<BossProj> minionBullets;
	std::vector<BossPowerUp> bossPowerUps;
	sf::RectangleShape bossBeam;
	sf::RenderWindow* window;
	int difficulty; // 0=easy, 1=normal, 2=hard

	static const int MAX_MINIONS = 2;

public:
	Boss();
	virtual ~Boss();
	void initBoss(sf::RenderWindow* window, int difficulty = 1);
	void update(int& health, sf::ConvexShape& player,
		unsigned& points, bool& isBossStage, bool isInvincible, bool isSlowed,
		float& invincibilityTimer, float& deathRayTimer, float& fastFireTimer,
		float& laserFireTimerMax);
	void render(sf::RenderTarget& target);

	bool isActive() const { return bossActive; }
	void setActive(bool active) { bossActive = active; }
	sf::FloatRect getBounds() const { return bossData.shape.getGlobalBounds(); }
	void takeDamage(int dmg);
	std::vector<BossMinion>& getMinions() { return bossMinions; }
};