#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>   // std::function

struct BossProj {
	sf::CircleShape shape;
	sf::Vector2f velocity = {0.f, 0.f};
	int hp = 0;
	bool isMissile = false;
	sf::Sprite sprite;
	bool grazed = false;   // graze mechanic: award once per projectile
};

struct BossData {
	sf::RectangleShape shape;
	int hp = 0;
	int maxHp = 0;
	float attackTimer = 0.f;
	int state = 0; // 0: normal, 1: beam warning, 2: beam active
	float stateTimer = 0.f;
	sf::Vector2f moveDir = {0.f, 0.f};
};

struct BossMinion {
	sf::CircleShape shape;
	sf::Sprite      sprite;
	sf::Vector2f velocity = {0.f, 0.f};
	float teleportTimer = 0.f;
	float teleportTimerMax = 0.f;
	float shootTimer = 0.f;
	float shootTimerMax = 0.f;
	int hp = 0;
};

struct BossPowerUp {
	sf::CircleShape shape;
	int type = 0; // 0: health+10, 1: invincibility, 2: deathray, 3: fastfire
};

class Boss
{
private:
	BossData bossData;
	bool bossActive;
	bool bossIsDying = false;
	std::vector<BossProj>    bossProjectiles;
	std::vector<BossMinion>  bossMinions;
	std::vector<BossProj>    minionBullets;
	std::vector<BossPowerUp> bossPowerUps;
	sf::RectangleShape bossBeam;
	sf::RenderWindow* window;
	int difficulty;

	// Sprite rendering
	sf::Texture bossTexture;
	sf::Sprite  bossSprite;
	bool        textureLoaded = false;

	sf::Texture minionTexture;
	bool        minionTexLoaded = false;

	sf::Texture missileTex;
	bool        missileTexLoaded = false;

	float phaseFlashTimer = 0.f;
	float phaseAnnounceTimer = 0.f;

	// Phase system
	int   phase = 1;           // 1 = normal, 2 = half HP agro, 3 = quarter HP frenzy
	bool  phase2Triggered = false;
	bool  phase3Triggered = false;

	// Separate minion respawn timer so it's not tied to the attack cycle
	float minionRespawnTimer = 0.f;
	float minionRespawnInterval = 480.f; // ~4s at 120fps; shortened per phase

	static const int MAX_MINIONS = 2;

public:
	Boss();
	virtual ~Boss();

	void initBoss(sf::RenderWindow* window, int difficulty = 1);

	// ── Original 11-param update — matches boss.cpp exactly ──────────────
	void update(int& health,
		sf::ConvexShape& player,
		unsigned& points,
		bool& isBossStage,
		bool isInvincible,
		bool isSlowed,
		float& invincibilityTimer,
		float& deathRayTimer,
		float& fastFireTimer,
		float& laserFireTimerMax,
		float& screenShakeOut);

	// ── 13-param inline overload — called by game.cpp ────────────────────
	inline void update(int& health,
		sf::ConvexShape& player,
		unsigned& points,
		bool& isBossStage,
		bool isInvincible,
		bool isSlowed,
		float& invincibilityTimer,
		float& deathRayTimer,
		float& fastFireTimer,
		float& laserFireTimerMax,
		float& screenShakeOut,
		int&   /*maxHealth*/,
		std::function<void(int)> /*onPlayerDamage*/)
	{
		update(health, player, points, isBossStage,
			isInvincible, isSlowed,
			invincibilityTimer, deathRayTimer, fastFireTimer,
			laserFireTimerMax, screenShakeOut);
	}

	void render(sf::RenderTarget& target);

	// ── Phase announcement overlay stub ──────────────────────────────────
	inline void renderPhaseAnnouncement(sf::RenderTarget& /*target*/,
		sf::Font& /*font*/) {
	}

	// ── State / status queries ────────────────────────────────────────────
	bool isActive()     const { return bossActive; }
	bool isDying()      const { return bossIsDying; }
	int  getBossState() const { return bossData.state; }
	bool isBeamActive() const { return bossData.state == 2; }
	int  getHp()        const { return bossData.hp; }
	int  getMaxHp()     const { return bossData.maxHp; }

	void setActive(bool active) { bossActive = active; }
	sf::FloatRect getBounds() const { return bossData.shape.getGlobalBounds(); }
	void takeDamage(int dmg);
	void move(float dx, float dy) { bossData.shape.move(dx, dy); }

	std::vector<BossMinion>& getMinions() { return bossMinions; }
	std::vector<BossProj>& getProjectiles() { return bossProjectiles; }  // used by game_features.cpp
};