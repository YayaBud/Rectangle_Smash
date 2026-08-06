// game_spawn.cpp  â€”  spawn functions + score/combo/damage helpers
#include "game.h"

//  POWER-UP SPAWN
void game::spawnPowerUp(sf::Vector2f pos, bool forceHealth)
{
	int roll = rand() % 100;
	if (!forceHealth && roll >= (int)diffPowerUpDropRate) return;

	PowerUp p;
	p.type = forceHealth ? 5 : rand() % 5;
	p.shape.setRadius(10.f);
	p.shape.setPosition(pos);
	switch (p.type) {
	case 0: p.shape.setFillColor(sf::Color::Yellow);  break;
	case 1: p.shape.setFillColor(sf::Color::Red);     break;
	case 2: p.shape.setFillColor(sf::Color::Magenta); break;
	case 3: p.shape.setFillColor(sf::Color::Cyan);    break;
	case 4: p.shape.setFillColor(sf::Color::Blue);    break;
	case 5: p.shape.setFillColor(sf::Color::Green);   break;
	}
	this->powerUps.push_back(p);
}

//  SPAWN WARNING
void game::queueSpawnWarning(int enemyType)
{
	float w = (enemyType == 0) ? 50.f : (enemyType == 1) ? 70.f : (enemyType == 2) ? 100.f : (enemyType == 4) ? 45.f : 30.f;
	float xPos = (float)(rand() % (int)(this->window->getSize().x - w));
	SpawnWarning sw;
	sw.xPos = xPos; sw.width = w; sw.enemyType = enemyType;
	sw.timer = 0.f; sw.timerMax = 60.f;
	this->spawnWarnings.push_back(sw);
	debugLog("Queued spawn warning | type=" + std::to_string(enemyType) +
		" x=" + std::to_string((int)xPos) +
		" pendingWarnings=" + std::to_string(spawnWarnings.size()));
}

void game::spawnEnemyFromWarning(int enemyType, float xPos)
{
	spawnEnemyGroup(enemyType, xPos, (enemyType == 3) ? (rand() % 3 + 2) : 1);
}

//  SPAWN ENEMY GROUP  (sprites assigned here)
void game::spawnEnemyGroup(int enemyType, float xPos, int count)
{
	for (int c = 0; c < count; c++) {
		EnemyData e;
		e.enemyType = enemyType;
		e.flashTimer = 0;
		e.zigzagTimer = 0.f;
		e.zigzagDir = (rand() % 2 == 0) ? 1.f : -1.f;
		e.shootTimer = 0.f;
		e.shootTimerMax = 120.f / diffEnemySpeedMult;

		// AI Upgrades
		e.hasShield = false;
		e.shieldHp = 0;
		if (currentWave >= 3 && rand() % 100 < currentWave * 5) {
			e.hasShield = true;
			e.shieldHp = 5 + (int)diffEnemyHpBonus;
		}
		
		e.stopAndShoot = false;
		e.stopXDir = (rand() % 2 == 0) ? 1.f : -1.f;
		if (enemyType == 1 && currentWave >= 4) {
			e.stopAndShoot = true;
		}

		// Ghost logic (Wave 6+)
		e.isGhost = false;
		e.ghostAlpha = 255.f;
		if (currentWave >= 6 && rand() % 100 < 20) {
			e.isGhost = true;
			e.ghostAlpha = 40.f; // Minimum visibility for ghost ships
		}

		// Collision shape size per type
		switch (enemyType) {
		case 0: // Yellow Zigzagger
			e.shape.setSize(sf::Vector2f(50.f, 50.f));
			e.shape.setFillColor(sf::Color::Transparent);
			e.baseColor = sf::Color(255, 255, 100);   // yellowish tint for sprite
			e.hp = std::max(1, (int)(2 + diffEnemyHpBonus));
			break;
		case 1: // Magenta Shooter
			e.shape.setSize(sf::Vector2f(70.f, 70.f));
			e.shape.setFillColor(sf::Color::Transparent);
			e.baseColor = sf::Color(255, 150, 255);   // magenta tint
			e.hp = std::max(1, (int)(3 + diffEnemyHpBonus));
			break;
		case 2: // Red Homing
			e.shape.setSize(sf::Vector2f(100.f, 100.f));
			e.shape.setFillColor(sf::Color::Transparent);
			e.baseColor = sf::Color(255, 100, 100);   // red tint
			e.hp = std::max(1, (int)(5 + diffEnemyHpBonus));
			break;
		case 3: // Green Swarm
			e.shape.setSize(sf::Vector2f(30.f, 30.f));
			e.shape.setFillColor(sf::Color::Transparent);
			e.baseColor = sf::Color(150, 255, 150);   // green tint
			e.hp = std::max(1, (int)(1 + diffEnemyHpBonus));
			break;
		case 4: // B6: Cyan Dodger — evades the player's cursor
			e.shape.setSize(sf::Vector2f(45.f, 45.f));
			e.shape.setFillColor(sf::Color::Transparent);
			e.baseColor = sf::Color(80, 255, 255);    // cyan tint
			e.hp = std::max(1, (int)(3 + diffEnemyHpBonus));
			break;
		default:
			e.shape.setSize(sf::Vector2f(50.f, 50.f));
			e.shape.setFillColor(sf::Color::Transparent);
			e.baseColor = sf::Color::White;
			e.hp = 2;
			break;
		}
		e.maxHp = e.hp;

		// Swarm offset, collision position
		float offsetX = (float)(rand() % 60 - 30) * (c > 0 ? 1.f : 0.f);
		e.shape.setPosition(xPos + offsetX, -e.shape.getSize().y);

		// ---- Assign sprite from pre-loaded texture ----
		int texIdx = std::min(enemyType, 3); // B6: clamp so types >= 4 reuse existing textures
		sf::Vector2u texSize = this->enemyTextures[texIdx].getSize();
		e.sprite.setTexture(this->enemyTextures[texIdx]);
		e.sprite.setOrigin((float)texSize.x / 2.f, (float)texSize.y / 2.f);
		// Scale sprite so its longer axis matches the collision box size
		float boxSize = e.shape.getSize().x;
		float sprScale = boxSize / (float)std::max(texSize.x, texSize.y);
		// Flip Y (negative scale) so enemy images that point UP now face DOWN
		e.sprite.setScale(sprScale, -sprScale);
		e.sprite.setColor(e.baseColor);
		// Position sprite at center of collision box
		e.sprite.setPosition(
			e.shape.getPosition().x + e.shape.getSize().x / 2.f,
			e.shape.getPosition().y + e.shape.getSize().y / 2.f);

		// Overlap check vs existing enemies
		bool overlaps = false;
		for (auto& existing : enemies) {
			if (e.shape.getGlobalBounds().intersects(existing.shape.getGlobalBounds())) {
				overlaps = true; break;
			}
		}
		if (!overlaps) {
			this->enemies.push_back(e);
			this->totalEnemiesSpawnedInWave++;
			debugLog("Spawned enemy | type=" + std::to_string(enemyType) +
				" hp=" + std::to_string(e.hp) +
				" totalSpawned=" + std::to_string(totalEnemiesSpawnedInWave) +
				"/" + std::to_string(enemiesNeededInWave) +
				" activeEnemies=" + std::to_string(enemies.size()));
		}
		else {
			debugLog("Spawn retry needed; enemy overlapped existing enemy | type=" + std::to_string(enemyType));
		}
	}
}

void game::spawnEnemyWithoutOverlap()
{
	// B6: Introduce Dodger (type 4) from wave 6 onward
	int maxType = (currentWave < 2) ? 0 : (currentWave < 3) ? 1 : (currentWave < 4) ? 2 : (currentWave < 6) ? 3 : 4;
	if (bossWaveDefeated) maxType = std::min(maxType, 4);
	int t = rand() % (maxType + 1);
	queueSpawnWarning(t);
}

//  SCORE POPUP
void game::addScorePopup(sf::Vector2f pos, int pts)
{
	ScorePopup sp;
	sp.text.setFont(font);
	sp.text.setCharacterSize(26);
	sp.text.setString("+" + std::to_string(pts));
	sp.text.setFillColor(pts >= 50 ? sf::Color(255, 120, 80) : pts >= 20 ? sf::Color(255, 220, 90) : sf::Color::White);
	sp.text.setOutlineColor(sf::Color(0, 0, 0, 180));
	sp.text.setOutlineThickness(2.f);
	sp.text.setOrigin(sp.text.getLocalBounds().left + sp.text.getLocalBounds().width / 2.f,
		sp.text.getLocalBounds().top + sp.text.getLocalBounds().height / 2.f);
	sp.text.setPosition(pos + sf::Vector2f((float)(rand() % 20 - 10), (float)(rand() % 10 - 5)));
	sp.velocity = sf::Vector2f((rand() % 40 - 20) / 24.f, -1.2f - (pts >= 50 ? 0.4f : 0.f));
	sp.lifetime = 0.f; sp.maxLifetime = 52.f;
	scorePopups.push_back(sp);
}

//  KILL BANNER
void game::triggerKillBanner(int combo)
{
	if (combo < 3) return;
	std::string msg;
	if      (combo >= 10) msg = "G O D L I K E!";
	else if (combo >= 7)  msg = "UNSTOPPABLE!";
	else if (combo >= 5)  msg = "RAMPAGE!";
	else if (combo >= 3)  msg = "TRIPLE KILL!";
	killBannerText.setString(msg);
	killBannerText.setFillColor(combo >= 7 ? sf::Color(255, 80, 80) : sf::Color(255, 220, 80));
	float scale = 1.f + std::min(0.35f, combo * 0.03f);
	killBannerText.setScale(scale, scale);
	sf::FloatRect kb = killBannerText.getLocalBounds();
	killBannerText.setOrigin(kb.left + kb.width / 2.f, kb.top + kb.height / 2.f);
	killBannerText.setPosition((float)window->getSize().x / 2.f, 320.f);
	killBannerTimer = killBannerTimerMax;
}

//  GAIN POINTS (with combo multiplier)
void game::gainPoints(int base, sf::Vector2f pos)
{
	int mult = std::max(1, comboCount);
	int total = (int)(base * mult * grazeMultiplier);
	points += total;
	addScorePopup(pos, total);
	comboCount++;
	comboTimer = comboTimerMax + std::min(18.f, (float)comboCount * 1.2f);
	triggerKillBanner(comboCount);
	comboAnnouncerTimer = 22.f;          // drives the combo counter's scale pop
	addUltimateCharge(4.f + comboCount * 0.4f);
}

//  PLAYER TAKE DAMAGE
void game::playerTakeDamage(int amount)
{
	if (invincibilityTimer > 0.f) return;

	// PERFECT PARRY
	if (parryWindowTimer > 0.f) {
		parryWindowTimer = 0.f;
		invincibilityTimer = 30.f;
		powerUpSound.setPitch(1.15f);
		powerUpSound.play();
		screenShake = 18.f;
		hitStopTimer = 10.f;
		debugLog("PERFECT PARRY! Invincibility granted.");
		return;
	}

	if (isDashing) return; // dash = invincibility frames
	if (shieldActive) {
		shieldActive = false;
		shieldRechargeTimer = 0.f;
		hitSound.setPitch(0.95f);
		hitSound.play();
		screenShake = 8.f;
		playerDamageFlashTimer = 8.f;
		hitStopTimer = 3.f;
		return;
	}
	health -= amount;
	if (health < 0) health = 0;
	hitSound.setPitch(amount >= 3 ? 0.85f : 1.f);
	hitSound.play();
	screenShake = 14.f + amount * 1.5f;
	hitStopTimer = 5.f;
	playerDamageFlashTimer = 12.f;

	// Getting hit costs the graze streak but pays a little ultimate charge —
	// a bad run still builds toward the comeback button.
	grazeCount = 0; grazeTimer = 0.f; grazeMultiplier = 1.f;
	addUltimateCharge(8.f * amount);
}

//  EXPLOSION PARTICLES
void game::addExplosionParticles(sf::Vector2f pos, sf::Color color, int count)
{
	for (int p = 0; p < count; p++) {
		Particle particle;
		float size = 2.f + (rand() % 4);
		particle.shape.setSize(sf::Vector2f(size, size));
		particle.shape.setFillColor(sf::Color(color.r, color.g, color.b, (sf::Uint8)(180 + rand() % 70)));
		particle.shape.setPosition(pos);
		float angle = (rand() % 360) * 3.14159f / 180.f;
		float speed = (rand() % 80 + 20) / 14.f;
		particle.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
		particle.lifetime = 0.f; particle.maxLifetime = (float)(rand() % 24 + 18);
		this->particles.push_back(particle);
	}
}
