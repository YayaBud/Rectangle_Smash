// game_update.cpp  â€”  all per-frame subsystem update functions
#include "game.h"

//  MENU
void game::updateMenu()
{
	updateDemoBackground();

	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!this->mouseHeld) {
			this->mouseHeld = true;

			// FIX: helper lambda to start a fade transition
			auto fadeToState = [&](GameState target) {
				transitionTarget = target;
				transitionFadingOut = true;
				transitionAlpha = 0.f;
				};

			if (PlayText.getGlobalBounds().contains(mousePosView)) {
				fadeToState(GameState::DIFFICULTY_SELECT);
				debugLog("State changed: DIFFICULTY_SELECT (fade)");
			}
			else if (ShipSelectText.getGlobalBounds().contains(mousePosView)) {
				fadeToState(GameState::SHIP_SELECT);
				debugLog("State changed: SHIP_SELECT (fade)");
			}
			else if (ShopText.getGlobalBounds().contains(mousePosView)) {
				fadeToState(GameState::SHOP);
				debugLog("State changed: SHOP (fade)");
			}
			else if (BlackMarketText.getGlobalBounds().contains(mousePosView)) {
				fadeToState(GameState::BLACK_MARKET);
				debugLog("State changed: BLACK_MARKET (fade)");
			}
			else if (SettingsText.getGlobalBounds().contains(mousePosView)) {
				fadeToState(GameState::SETTINGS);
				debugLog("State changed: SETTINGS (fade)");
			}
			else if (SkinsText.getGlobalBounds().contains(mousePosView)) {
				fadeToState(GameState::SKINS);
				debugLog("State changed: SKINS (fade)");
			}
			else if (QuitText.getGlobalBounds().contains(mousePosView)) {
				debugLog("Quit selected; closing window");
				window->close();
			}
		}
	}
	else { mouseHeld = false; }

	auto updateHover = [&](sf::Text& t, sf::Color hColor) {
		bool h = t.getGlobalBounds().contains(mousePosView);
		t.setFillColor(h ? hColor : sf::Color(220, 220, 220));
		};
	updateHover(PlayText, sf::Color(255, 210, 60));
	updateHover(ShipSelectText, sf::Color(255, 210, 60));
	updateHover(ShopText, sf::Color(255, 210, 60));
	updateHover(BlackMarketText, sf::Color(255, 210, 60));
	updateHover(SettingsText, sf::Color(255, 210, 60));
	updateHover(SkinsText, sf::Color(255, 210, 60));
	updateHover(QuitText, sf::Color(255, 90, 90));
}

//  SHIP SELECT
void game::updateShipSelect()
{
	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;
	float cardH = 380.f;
	float cardW = 300.f;
	float spacing = 350.f;
	// Back button is rendered at cy + cardH/2 + 90
	sf::FloatRect backHitbox(cx - 110.f, cy + cardH / 2.f + 60.f, 220.f, 60.f);

	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			if (backHitbox.contains(mousePosView)) { state = GameState::MENU; return; }

			for (int i = 0; i < 3; i++) {
				float bx = cx - spacing + i * spacing;
				sf::FloatRect bounds(bx - cardW / 2.f, cy + 30.f - cardH / 2.f, cardW, cardH);
				if (bounds.contains(mousePosView)) {
					selectedShipClass = i;
					powerUpSound.play();
				}
			}
		}
	}
	else { mouseHeld = false; }
	// Tint the back text based on hover
	BackText.setFillColor(backHitbox.contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
}

//  SHOP
void game::updateShop()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			if (BackText.getGlobalBounds().contains(mousePosView)) { state = GameState::MENU; return; }

			float cx = (float)window->getSize().x / 2.f;
			float startY = 250.f;
			int* upgrades[] = { &upgWeaponLevel, &upgMoveSpeed, &upgFireRate, &upgDashCooldown, &upgMaxHealth };
			int maxLevel[] = { 5, 5, 5, 5, 5 };
			int baseCost[] = { 100, 50, 60, 50, 80 }; // Adjusted costs

			for (int i = 0; i < 5; i++) {
				sf::FloatRect btnBounds(cx + 175.f - 75.f, startY + i * 70.f - 25.f, 150.f, 50.f);
				if (btnBounds.contains(mousePosView)) {
					if (*upgrades[i] < maxLevel[i]) {
						int cost = baseCost[i] * (*upgrades[i] + 1);
						if (totalGears >= cost) {
							totalGears -= cost;
							(*upgrades[i])++;
							saveGameData();
							powerUpSound.play();
						}
						else {
							hitSound.play(); // Error sound
						}
					}
				}
			}
		}
	}
	else { mouseHeld = false; }
	BackText.setFillColor(BackText.getGlobalBounds().contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
}

//  DIFFICULTY SELECT
void game::updateDifficultySelect()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			int chosen = -1;
			float dcx = (float)window->getSize().x / 2.f;
			float dcy = (float)window->getSize().y / 2.f;
			// Row positions must match renderDifficultySelect: rowY starts at cy-100, steps +150
			float rowYs[3] = { dcy - 100.f, dcy + 50.f, dcy + 200.f };
			for (int i = 0; i < 3; i++) {
				sf::FloatRect rowHit(dcx - 420.f, rowYs[i] - 45.f, 840.f, 90.f);
				if (rowHit.contains(mousePosView)) { chosen = i; break; }
			}
			// Back button at cy+300
			sf::FloatRect backHit(dcx - 110.f, dcy + 280.f, 220.f, 60.f);
			if (chosen < 0 && backHit.contains(mousePosView)) { state = GameState::MENU; mouseHeld = false; return; }
			if (chosen >= 0) {
				difficulty = chosen;
				applyDifficultySettings(chosen);
				bossEntity.initBoss(window, chosen);
				boss2Entity.initBoss2(window, chosen);
				// Reset game state
				points = 0; currentWave = 0; isBossStage = false; bossWaveDefeated = false;
				isBoss2Stage = false; boss2Defeated = false;
				boss2DeathSequence = false; boss2DeathTimer = 0.f;
				bossDeathSequence = false; bossDeathTimer = 0.f;
				bossArrivalPending = false;
				enemies.clear(); lasers.clear(); particles.clear(); powerUps.clear();
				enemyBullets.clear(); spawnWarnings.clear(); scorePopups.clear(); dashTrail.clear();
				asteroids.clear(); blackHoles.clear();
				comboCount = 0; heatLevel = 0.f; isOverheated = false;
				shieldActive = true; shieldRechargeTimer = 0.f;
				isDashing = false; dashCooldown = 0.f;
				invincibilityTimer = 0.f; deathRayTimer = 0.f;
				fastFireTimer = 0.f; slowTimeTimer = 0.f;
				waveEnemySpeedBonus = 0.f; EndGame = false;
				boss2LastBeamState = false;
				player.setPosition((float)window->getSize().x / 2.f, (float)window->getSize().y - 100.f);
				playerSprite.setPosition(player.getPosition());
				// FIX: use fade transition into playing
				transitionTarget = GameState::PLAYING;
				transitionFadingOut = true;
				transitionAlpha = 0.f;
				state = GameState::DIFFICULTY_SELECT; // stay until fade complete
				debugLog("Started game | difficulty=" + std::to_string(difficulty) +
					" health=" + std::to_string(health) +
					" selectedSkin=" + std::to_string(selectedSkin));
			}
		}
	}
	else { mouseHeld = false; }

	{
		float dcx2 = (float)window->getSize().x / 2.f;
		float dcy2 = (float)window->getSize().y / 2.f;
		float rowYs2[3] = { dcy2 - 100.f, dcy2 + 50.f, dcy2 + 200.f };
		sf::Color cols[3] = { sf::Color::Green, sf::Color::Yellow, sf::Color::Red };
		sf::Text* texts[3] = { &DiffEasyText, &DiffNormalText, &DiffHardText };
		for (int i = 0; i < 3; i++) {
			sf::FloatRect rowHit(dcx2 - 420.f, rowYs2[i] - 45.f, 840.f, 90.f);
			texts[i]->setFillColor(rowHit.contains(mousePosView) ? cols[i] : sf::Color::White);
		}
		sf::FloatRect backHit2(dcx2 - 110.f, dcy2 + 280.f, 220.f, 60.f);
		BackText.setFillColor(backHit2.contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
	}
}

//  SETTINGS
void game::updateSettings()
{
	std::stringstream mss; mss << basePlayerMoveSpeed; MoveSpeedValText.setString(mss.str());
	std::stringstream fss; fss << fireSpeedLevelFromTimer(baseLaserFireTimerMax); FireSpeedValText.setString(fss.str());
	FollowMouseSettingText.setString(followMouse ? "MOVEMENT: MOUSE" : "MOVEMENT: WASD");
	AutoFireSettingText.setString(autoFire ? "FIRE: AUTO" : "FIRE: MANUAL (L-CLICK)");
	OverheatSettingText.setString(overheatEnabled ? "OVERHEAT: ON" : "OVERHEAT: OFF");
	auto centerDynamicText = [&](sf::Text& t) {
		sf::FloatRect bounds = t.getLocalBounds();
		t.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
		};
	centerDynamicText(FollowMouseSettingText);
	centerDynamicText(AutoFireSettingText);
	centerDynamicText(OverheatSettingText);

	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			if (BackText.getGlobalBounds().contains(mousePosView)) {
				state = (points > 0 || health < maxHealth) ? GameState::PAUSED : GameState::MENU;
				debugLog("Settings back selected; state changed");
			}
			else if (MoveSpeedMinus.getGlobalBounds().contains(mousePosView)) {
				basePlayerMoveSpeed -= 1.f; if (basePlayerMoveSpeed < 1.f) basePlayerMoveSpeed = 1.f;
				debugLog("Move speed changed: " + std::to_string(basePlayerMoveSpeed));
			}
			else if (MoveSpeedPlus.getGlobalBounds().contains(mousePosView)) {
				basePlayerMoveSpeed += 1.f; if (basePlayerMoveSpeed > 10.f) basePlayerMoveSpeed = 10.f;
				debugLog("Move speed changed: " + std::to_string(basePlayerMoveSpeed));
			}
			else if (FireSpeedMinus.getGlobalBounds().contains(mousePosView)) {
				baseLaserFireTimerMax += 2.f; if (baseLaserFireTimerMax > 25.f) baseLaserFireTimerMax = 25.f;
				laserFireTimerMax = baseLaserFireTimerMax;
				debugLog("Fire speed changed: " + std::to_string(fireSpeedLevelFromTimer(baseLaserFireTimerMax)) +
					" timer=" + std::to_string(baseLaserFireTimerMax));
			}
			else if (FireSpeedPlus.getGlobalBounds().contains(mousePosView)) {
				baseLaserFireTimerMax -= 2.f; if (baseLaserFireTimerMax < 5.f) baseLaserFireTimerMax = 5.f;
				laserFireTimerMax = baseLaserFireTimerMax;
				debugLog("Fire speed changed: " + std::to_string(fireSpeedLevelFromTimer(baseLaserFireTimerMax)) +
					" timer=" + std::to_string(baseLaserFireTimerMax));
			}
			else if (FollowMouseSettingText.getGlobalBounds().contains(mousePosView)) {
				followMouse = !followMouse;
				debugLog(std::string("Follow mouse: ") + (followMouse ? "ON" : "OFF"));
			}
			else if (AutoFireSettingText.getGlobalBounds().contains(mousePosView)) {
				autoFire = !autoFire;
				debugLog(std::string("Auto fire: ") + (autoFire ? "ON" : "OFF"));
			}
			else if (OverheatSettingText.getGlobalBounds().contains(mousePosView)) {
				overheatEnabled = !overheatEnabled;
				debugLog(std::string("Overheat: ") + (overheatEnabled ? "ON" : "OFF"));
			}
		}
	}
	else { mouseHeld = false; }

	BackText.setFillColor(BackText.getGlobalBounds().contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
	FollowMouseSettingText.setFillColor(FollowMouseSettingText.getGlobalBounds().contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
	AutoFireSettingText.setFillColor(AutoFireSettingText.getGlobalBounds().contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
	OverheatSettingText.setFillColor(OverheatSettingText.getGlobalBounds().contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
	auto updateHover = [&](sf::Text& t, sf::Color hColor) {
		bool h = t.getGlobalBounds().contains(mousePosView);
		sf::Vector2f center(t.getPosition().x, t.getPosition().y);
		t.setFillColor(h ? hColor : sf::Color::White);
		t.setScale(h ? 1.03f : 1.f, h ? 1.03f : 1.f);
		sf::FloatRect b = t.getLocalBounds();
		t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
		t.setPosition(center);
		};
	updateHover(MoveSpeedMinus, sf::Color::Red);
	updateHover(MoveSpeedPlus, sf::Color::Green);
	updateHover(FireSpeedMinus, sf::Color::Red);
	updateHover(FireSpeedPlus, sf::Color::Green);
	MoveSpeedValText.setString(mss.str()); // ensure we center it after string change
	centerDynamicText(MoveSpeedValText);
	FireSpeedValText.setString(fss.str());
	centerDynamicText(FireSpeedValText);
}

//  PLAYER  (sprite follows collision shape)
void game::updatePlayer()
{
	if (parryWindowTimer > 0.f) parryWindowTimer -= 1.f;

	// Rotate collision shape (and sprite) toward mouse
	sf::Vector2f dir = mousePosView - player.getPosition();
	float angle = std::atan2(dir.y, dir.x) * 180.f / 3.14159f;
	player.setRotation(angle + 90.f);

	if (!isDashing && window->hasFocus()) {
		float spd = basePlayerMoveSpeed + ((comboCount >= 3) ? std::min(4.f, (float)comboCount * 0.2f) : 0.f); // Momentum buff
		if (followMouse) {
			sf::Vector2f mDir = mousePosView - player.getPosition();
			float mDist = std::sqrt(mDir.x * mDir.x + mDir.y * mDir.y);
			if (mDist > spd) {
				player.move((mDir / mDist) * spd);
			}
			else {
				player.setPosition(mousePosView);
			}
		}
		else {
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  player.move(-spd, 0.f);
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) player.move(spd, 0.f);
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    player.move(0.f, -spd);
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  player.move(0.f, spd);
		}
	}

	// GHOST VISIBILITY LOGIC
	for (auto& e : enemies) {
		if (e.isGhost) {
			sf::Vector2f ePos = e.shape.getPosition() + e.shape.getSize() / 2.f;
			float distToMouse = (float)std::sqrt(std::pow(ePos.x - mousePosView.x, 2) + std::pow(ePos.y - mousePosView.y, 2));
			if (distToMouse < 80.f) e.ghostAlpha = std::min(255.f, e.ghostAlpha + 25.f);
			else e.ghostAlpha = std::max(40.f, e.ghostAlpha - 5.f);
		}
	}

	// Clamp to window
	sf::Vector2f pos = player.getPosition();
	pos.x = std::max(20.f, std::min(pos.x, (float)window->getSize().x - 20.f));
	pos.y = std::max(20.f, std::min(pos.y, (float)window->getSize().y - 20.f));
	player.setPosition(pos);

	// Sync sprite position & rotation
	playerSprite.setPosition(pos);
	playerSprite.setRotation(angle + 90.f);

	// Power-up color tint on sprite
	if (invincibilityTimer > 0.f) playerSprite.setColor(sf::Color(255, 220, 0));
	else if (deathRayTimer > 0.f)      playerSprite.setColor(sf::Color(255, 80, 80));
	else if (fastFireTimer > 0.f)      playerSprite.setColor(sf::Color(80, 255, 255));
	else if (slowTimeTimer > 0.f)      playerSprite.setColor(sf::Color(150, 150, 255));
	else                               playerSprite.setColor(sf::Color::White);

	// D1: damage flash — alternate red/white for playerDamageFlashTimer frames
	if (playerDamageFlashTimer > 0.f) {
		playerDamageFlashTimer -= 1.f;
		if ((int)playerDamageFlashTimer % 2 == 0)
			playerSprite.setColor(sf::Color(255, 50, 50));
	}
}

//  LASERS
void game::updateLasers()
{
	if (laserFireTimer < laserFireTimerMax) laserFireTimer += 1.f;

	bool shouldFire = autoFire
		? (laserFireTimer >= laserFireTimerMax)
		: (isFiring && laserFireTimer >= laserFireTimerMax);

	if (!isOverheated && shouldFire && state == GameState::PLAYING) {
		laserFireTimer = 0.f;

		sf::Vector2f d = mousePosView - player.getPosition();
		float len = std::sqrt(d.x * d.x + d.y * d.y);
		if (len > 0.f) d /= len; else d = sf::Vector2f(0.f, -1.f);
		float baseAngle = std::atan2(d.y, d.x);

		int numShots = 1;
		if (weaponLevel == 2) numShots = 2;
		else if (weaponLevel == 3) numShots = 3;
		else if (weaponLevel == 4) numShots = 5;
		else if (weaponLevel >= 5) numShots = 7;

		for (int s = 0; s < numShots; s++) {
			Laser nl;
			float bonusWidth = (comboCount >= 3) ? std::min(10.f, (float)comboCount * 0.8f) : 0.f; // Combo Momentum buff
			if (deathRayTimer > 0.f) { nl.shape.setSize(sf::Vector2f(16.f + bonusWidth, 120.f)); nl.shape.setFillColor(sf::Color::Red); }
			else { nl.shape.setSize(sf::Vector2f(4.f + bonusWidth, 150.f));  nl.shape.setFillColor(sf::Color::White); } // Long white core
			nl.shape.setPosition(player.getPosition().x - nl.shape.getSize().x / 2.f, player.getPosition().y - 30.f);
			nl.pierceCount = (selectedShipClass == 2) ? 3 : 0; // Sniper piercing

			float angleOffset = 0.f;
			if (numShots > 1) {
				if (numShots == 2) {
					angleOffset = (s == 0) ? -0.1f : 0.1f;
				}
				else {
					angleOffset = (s - numShots / 2) * 0.15f;
				}
			}
			float finalAngle = baseAngle + angleOffset;
			nl.velocity = sf::Vector2f(std::cos(finalAngle), std::sin(finalAngle)) * 25.f; // Very fast
			nl.shape.setRotation(finalAngle * 180.f / 3.14159f + 90.f);
			lasers.push_back(nl);
		}

		laserSounds[currentLaserSoundIndex].setPitch(deathRayTimer > 0.f ? 0.88f : (comboCount >= 5 ? 1.08f : 1.f));
		laserSounds[currentLaserSoundIndex].setVolume(deathRayTimer > 0.f ? 60.f : (numShots >= 5 ? 42.f : 45.f));
		laserSounds[currentLaserSoundIndex].play();		currentLaserSoundIndex = (currentLaserSoundIndex + 1) % MAX_LASER_SOUNDS;

		// C5: auto-fire now generates heat at half rate (was completely exempt)
		if (overheatEnabled) {
			heatLevel += autoFire ? 6.f : 12.f;
			if (heatLevel >= heatMax) {
				heatLevel = heatMax;
				isOverheated = true;
				overheatTimer = 0.f;
				overheatSound.play();
			}
		}
	}

	for (size_t i = 0; i < lasers.size(); i++) {
		lasers[i].shape.move(lasers[i].velocity);
		float px = lasers[i].shape.getPosition().x, py = lasers[i].shape.getPosition().y;

		// PERK 0: BOUNCY LASERS — reflect off screen edges
		bool hasBouncy = false;
		for (int pid : activePerkIds) if (pid == 0) { hasBouncy = true; break; }
		if (hasBouncy) {
			if (px < 0 || px > window->getSize().x) {
				lasers[i].velocity.x = -lasers[i].velocity.x;
				lasers[i].shape.move(lasers[i].velocity * 2.f);
			}
			if (py < -100.f || py > window->getSize().y) {
				lasers.erase(lasers.begin() + i); i--; continue;
			}
		}
		else {
			if (py < 0 || py > window->getSize().y || px < 0 || px > window->getSize().x) {
				lasers.erase(lasers.begin() + i); i--; continue;
			}
		}
		bool del = false;

		// Boss hit
		if (bossEntity.isActive() && lasers[i].shape.getGlobalBounds().intersects(bossEntity.getBounds())) {
			int dmg = (lasers[i].shape.getFillColor() == sf::Color::Red) ? 5 : 1;
			int scaledDmg = std::max(1, (int)std::round(dmg * diffBossDamageMult));
			bossEntity.takeDamage(scaledDmg);
			debugLog("Boss hit | damage=" + std::to_string(scaledDmg) +
				" hp=" + std::to_string(bossEntity.getHp()) +
				"/" + std::to_string(bossEntity.getMaxHp()));
			Particle p; p.shape.setSize(sf::Vector2f(4.f, 4.f)); p.shape.setFillColor(sf::Color::White);
			p.shape.setPosition(lasers[i].shape.getPosition());
			p.velocity = sf::Vector2f((rand() % 40 - 20) / 10.f, (rand() % 40 - 20) / 10.f);
			p.lifetime = 0; p.maxLifetime = 15; particles.push_back(p);
			lasers.erase(lasers.begin() + i); i--; del = true; continue;
		}

		// Boss 2 hit
		if (boss2Entity.isActive() && lasers[i].shape.getGlobalBounds().intersects(boss2Entity.getBounds())) {
			int dmg = (lasers[i].shape.getFillColor() == sf::Color::Red) ? 5 : 1;
			int scaledDmg = std::max(1, (int)std::round(dmg * diffBossDamageMult));
			boss2Entity.takeDamage(scaledDmg);
			Particle p; p.shape.setSize(sf::Vector2f(4.f, 4.f)); p.shape.setFillColor(sf::Color::White);
			p.shape.setPosition(lasers[i].shape.getPosition());
			p.velocity = sf::Vector2f((rand() % 40 - 20) / 10.f, (rand() % 40 - 20) / 10.f);
			p.lifetime = 0; p.maxLifetime = 15; particles.push_back(p);
			lasers.erase(lasers.begin() + i); i--; del = true; continue;
		}

		// Asteroid hit
		for (size_t a = 0; a < asteroids.size(); a++) {
			if (lasers[i].shape.getGlobalBounds().intersects(asteroids[a].shape.getGlobalBounds())) {
				asteroids[a].hp--;
				Particle p; p.shape.setSize(sf::Vector2f(3.f, 3.f)); p.shape.setFillColor(sf::Color(150, 150, 150));
				p.shape.setPosition(lasers[i].shape.getPosition());
				p.velocity = sf::Vector2f((rand() % 30 - 15) / 10.f, (rand() % 30 - 15) / 10.f);
				p.lifetime = 0; p.maxLifetime = 8; particles.push_back(p);
				lasers.erase(lasers.begin() + i); i--; del = true; break;
			}
		}
		if (del) continue;

		// Minion hit
		if (bossEntity.isActive() && !del) {
			auto& minions = bossEntity.getMinions();
			for (size_t m = 0; m < minions.size() && !del; m++) {
				if (lasers[i].shape.getGlobalBounds().intersects(minions[m].shape.getGlobalBounds())) {
					int dmg = (lasers[i].shape.getFillColor() == sf::Color::Red) ? 5 : 1;
					minions[m].hp -= dmg;
					if (minions[m].hp <= 0) {
						gainPoints(15, sf::Vector2f(minions[m].shape.getPosition().x + 15.f, minions[m].shape.getPosition().y + 15.f));
						addExplosionParticles(sf::Vector2f(minions[m].shape.getPosition().x + 15.f, minions[m].shape.getPosition().y + 15.f), sf::Color(255, 140, 0), 20);
						explosionSound.play();
						minions.erase(minions.begin() + m);
					}
					lasers.erase(lasers.begin() + i); i--; del = true;
				}
			}
		}

		// Boss 2 Minion hit
		if (boss2Entity.isActive() && !del) {
			auto& minions2 = boss2Entity.getMinions();
			for (size_t m = 0; m < minions2.size() && !del; m++) {
				if (lasers[i].shape.getGlobalBounds().intersects(minions2[m].shape.getGlobalBounds())) {
					int dmg = (lasers[i].shape.getFillColor() == sf::Color::Red) ? 5 : 1;
					minions2[m].hp -= dmg;
					if (minions2[m].hp <= 0) {
						// RING FIX: shape origin is now centered so getPosition() is already the center
						sf::Vector2f mCenter = minions2[m].shape.getPosition();
						gainPoints(20, mCenter);
						addExplosionParticles(mCenter, sf::Color(255, 100, 50), 25);
						explosionSound.play();
						minions2.erase(minions2.begin() + m);
					}
					lasers.erase(lasers.begin() + i); i--; del = true;
				}
			}
		}
		if (del) continue;

		// Normal enemy hit
		for (size_t k = 0; k < enemies.size() && !del; k++) {
			if (lasers[i].shape.getGlobalBounds().intersects(enemies[k].shape.getGlobalBounds())) {
				int dmg = (lasers[i].shape.getFillColor() == sf::Color::Red) ? 5 : 1;

				// PERK 1: GLASS CANNON — double damage
				for (int pid : activePerkIds) if (pid == 1) { dmg *= 2; break; }

				if (enemies[k].hasShield) {
					enemies[k].shieldHp -= dmg;
					if (enemies[k].shieldHp <= 0) {
						enemies[k].hasShield = false;
						explosionSound.play();
						addExplosionParticles(lasers[i].shape.getPosition(), sf::Color::Cyan, 15);
					}
					else {
						Particle p; p.shape.setSize(sf::Vector2f(3.f, 3.f)); p.shape.setFillColor(sf::Color::Cyan);
						p.shape.setPosition(lasers[i].shape.getPosition());
						p.velocity = sf::Vector2f((rand() % 20 - 10) / 10.f, (rand() % 20 - 10) / 10.f);
						p.lifetime = 0; p.maxLifetime = 10; particles.push_back(p);
					}
				}
				else {
					enemies[k].hp -= dmg;
					if (enemies[k].hp <= 0) {
						int baseP = (enemies[k].enemyType == 0) ? 5 : (enemies[k].enemyType == 1) ? 10 : (enemies[k].enemyType == 2) ? 20 : 3;
						sf::Vector2f ec(enemies[k].shape.getPosition().x + enemies[k].shape.getSize().x / 2.f,
							enemies[k].shape.getPosition().y + enemies[k].shape.getSize().y / 2.f);
						gainPoints(baseP, ec);
						enemiesKilledInWave++;
						spawnPowerUp(ec);
						if (rand() % 100 < 5) spawnPowerUp(ec, true);
						if (enemiesKilledInWave % 5 == 0) spawnPowerUp(ec, false); // guaranteed every 5th kill

						// PERK 2: VAMPIRISM — 5% chance heal on kill
						for (int pid : activePerkIds) if (pid == 2) { if (rand() % 100 < 5) { health = std::min(health + 1, maxHealth); powerUpSound.play(); } break; }

						// PERK 4: SEEKING SHARDS — spawn 2 small homing shards on kill
						for (int pid : activePerkIds) if (pid == 4) {
							for (int s = 0; s < 2; s++) {
								EnemyData shard;
								shard.enemyType = 3; // swarm type (small, fast)
								shard.shape.setSize(sf::Vector2f(20.f, 20.f));
								shard.shape.setFillColor(sf::Color::Transparent);
								shard.baseColor = sf::Color(255, 200, 50);
								shard.hp = 1; shard.maxHp = 1;
								shard.flashTimer = 0; shard.zigzagTimer = 0.f;
								shard.zigzagDir = (s == 0) ? -1.f : 1.f;
								shard.shootTimer = 0.f; shard.shootTimerMax = 9999.f;
								shard.hasShield = false; shard.shieldHp = 0;
								shard.stopAndShoot = false; shard.stopXDir = 1.f;
								shard.isGhost = false; shard.ghostAlpha = 255.f;
								shard.shape.setPosition(ec.x - 10.f + s * 20.f, ec.y);
								int texIdx = 3;
								sf::Vector2u texSize = this->enemyTextures[texIdx].getSize();
								shard.sprite.setTexture(this->enemyTextures[texIdx]);
								shard.sprite.setOrigin((float)texSize.x / 2.f, (float)texSize.y / 2.f);
								float sc = 20.f / (float)std::max(texSize.x, texSize.y);
								shard.sprite.setScale(sc, -sc);
								shard.sprite.setColor(shard.baseColor);
								shard.sprite.setPosition(ec);
								enemies.push_back(shard);
							}
							break;
						}

						addExplosionParticles(ec, sf::Color(enemies[k].baseColor.r, enemies[k].baseColor.g, enemies[k].baseColor.b), 30);
						explosionSound.play();
						hitStopTimer = 2.f;
						screenShake += 3.f;
						spawnExplosion(ec.x, ec.y, 1.2f);
						enemies.erase(enemies.begin() + k);
					}
					else {
						enemies[k].flashTimer = 4;
						Particle p; p.shape.setSize(sf::Vector2f(3.f, 3.f)); p.shape.setFillColor(sf::Color::White);
						p.shape.setPosition(lasers[i].shape.getPosition());
						p.velocity = sf::Vector2f((rand() % 20 - 10) / 10.f, (rand() % 20 - 10) / 10.f);
						p.lifetime = 0; p.maxLifetime = 10; particles.push_back(p);
					}
				}

				if (lasers[i].pierceCount > 0) {
					lasers[i].pierceCount--;
				}
				else {
					lasers.erase(lasers.begin() + i); i--; del = true;
				}
			}
		}
	}
}

//  ENEMIES  (sprite synced each frame)
void game::updateEnemies()
{
	if (isBossStage) { enemies.clear(); return; }
	float speedMult = (slowTimeTimer > 0.f) ? 0.5f : 1.0f;
	float baseSpeed = (1.0f + waveEnemySpeedBonus) * diffEnemySpeedMult * speedMult;

	for (size_t i = 0; i < enemies.size(); i++) {
		EnemyData& e = enemies[i];

		// Flash timer
		if (e.flashTimer > 0) e.flashTimer--;

		// Movement per type
		switch (e.enemyType) {
		case 0: // Zigzagger
			e.zigzagTimer += 0.05f;
			e.shape.move(std::sin(e.zigzagTimer) * 2.f * baseSpeed, baseSpeed);
			break;
		case 1: // Shooter
			if (e.stopAndShoot && e.shape.getPosition().y > window->getSize().y / 5.f) {
				e.shape.move(e.stopXDir * baseSpeed * 0.5f, 0.f);
				if (e.shape.getPosition().x < 0 || e.shape.getPosition().x + e.shape.getSize().x > window->getSize().x) {
					e.stopXDir *= -1.f;
				}
				e.shootTimerMax = 60.f / diffEnemySpeedMult; // Shoot faster when stopped
			}
			else {
				e.shape.move(0.f, baseSpeed * 0.7f);
			}

			e.shootTimer += 1.f;
			if (e.shootTimer >= e.shootTimerMax) {
				e.shootTimer = 0.f;
				int bullets = (currentWave >= 5) ? 3 : 1;

				sf::Vector2f bdir = player.getPosition() - sf::Vector2f(e.shape.getPosition().x + e.shape.getSize().x / 2.f, e.shape.getPosition().y + e.shape.getSize().y);
				float blen = std::sqrt(bdir.x * bdir.x + bdir.y * bdir.y);
				if (blen > 0) bdir /= blen;
				float baseAngle = std::atan2(bdir.y, bdir.x);

				for (int b = 0; b < bullets; b++) {
					EnemyBullet eb;
					eb.shape.setRadius(6.f); eb.shape.setFillColor(sf::Color(255, 100, 255));
					eb.shape.setPosition(e.shape.getPosition().x + e.shape.getSize().x / 2.f,
						e.shape.getPosition().y + e.shape.getSize().y);

					float angle = baseAngle;
					if (bullets == 3) {
						if (b == 1) angle -= 0.2f;
						else if (b == 2) angle += 0.2f;
					}
					eb.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * 3.5f * diffEnemySpeedMult;
					enemyBullets.push_back(eb);
				}
			}
			break;
		case 2: // Homing
		{
			float dx = player.getPosition().x - (e.shape.getPosition().x + e.shape.getSize().x / 2.f);
			e.shape.move(dx * 0.005f * diffEnemySpeedMult, baseSpeed * 0.5f);
		}
		break;
		case 3: // Swarm fast
			e.shape.move(0.f, baseSpeed * 2.f);
			break;
		case 4: // Dodging
		{
			float ex = e.shape.getPosition().x + e.shape.getSize().x / 2.f;
			float dx = (ex < mousePosView.x) ? -1.f : 1.f;
			e.shape.move(dx * baseSpeed * 1.5f, baseSpeed);
		}
		break;
		case 5: // Shielded
			e.shape.move(0.f, baseSpeed * 0.4f);
			break;
		}

		// Sync sprite center + apply flash tint
		e.sprite.setPosition(
			e.shape.getPosition().x + e.shape.getSize().x / 2.f,
			e.shape.getPosition().y + e.shape.getSize().y / 2.f);
		// Flash = bright white, normal = original type tint
		e.sprite.setColor(e.flashTimer > 0 ? sf::Color(255, 255, 255) : e.baseColor);

		// Player collision
		if (e.shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
			playerTakeDamage(3);
			enemiesKilledInWave++;
			addExplosionParticles(
				sf::Vector2f(e.shape.getPosition().x + e.shape.getSize().x / 2.f,
					e.shape.getPosition().y + e.shape.getSize().y / 2.f),
				sf::Color(e.baseColor.r, e.baseColor.g, e.baseColor.b), 15);
			enemies.erase(enemies.begin() + i); i--; continue;
		}

		// Off-screen: B3 — damage scales with enemy danger level
		if (e.shape.getPosition().y > window->getSize().y) {
			int escDmg = (e.enemyType == 3) ? 1 : (e.enemyType == 0) ? 1 : (e.enemyType == 1) ? 2 : 3;
			playerTakeDamage(escDmg);
			enemiesKilledInWave++;
			enemies.erase(enemies.begin() + i); i--;
		}
	}
}

//  ENEMY BULLETS
void game::updateEnemyBullets()
{
	for (size_t i = 0; i < enemyBullets.size(); i++) {
		enemyBullets[i].shape.move(enemyBullets[i].velocity);
		float px = enemyBullets[i].shape.getPosition().x, py = enemyBullets[i].shape.getPosition().y;
		if (py > window->getSize().y || py < -20.f || px < -20.f || px > window->getSize().x) {
			enemyBullets.erase(enemyBullets.begin() + i); i--; continue;
		}
		if (enemyBullets[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
			playerTakeDamage(2);
			addExplosionParticles(enemyBullets[i].shape.getPosition(), sf::Color(255, 90, 255), 6);
			spawnExplosion(enemyBullets[i].shape.getPosition().x, enemyBullets[i].shape.getPosition().y, 0.7f);
			enemyBullets.erase(enemyBullets.begin() + i); i--;
		}
	}
}

//  SPAWN WARNINGS
void game::updateSpawnWarnings()
{
	for (size_t i = 0; i < spawnWarnings.size(); i++) {
		spawnWarnings[i].timer += 1.f;
		if (spawnWarnings[i].timer >= spawnWarnings[i].timerMax) {
			spawnEnemyFromWarning(spawnWarnings[i].enemyType, spawnWarnings[i].xPos);
			spawnWarnings.erase(spawnWarnings.begin() + i); i--;
		}
	}
}

//  POWER-UPS
void game::updatePowerUps()
{
	if (invincibilityTimer > 0.f) invincibilityTimer -= 1.f;
	if (deathRayTimer > 0.f)      deathRayTimer -= 1.f;
	if (slowTimeTimer > 0.f)      slowTimeTimer -= 1.f;
	if (fastFireTimer > 0.f) { fastFireTimer -= 1.f; laserFireTimerMax = 5.f; }
	else { laserFireTimerMax = baseLaserFireTimerMax; }

	for (size_t i = 0; i < powerUps.size(); i++) {
		powerUps[i].shape.move(0.f, 2.f);
		if (powerUps[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
			int t = powerUps[i].type;
			if (t == 0) { invincibilityTimer = 600.f; powerUpSound.setPitch(1.05f); }
			else if (t == 1) { deathRayTimer = 600.f; powerUpSound.setPitch(0.9f); }
			else if (t == 2) {
				// B5: cap nuke at 10 enemies; full screen-clear was too powerful
				int nukeCap = std::min((int)enemies.size(), 10);
				for (int ni = 0; ni < nukeCap; ni++) {
					auto& e = enemies[ni];
					int bp = (e.enemyType == 0) ? 5 : (e.enemyType == 1) ? 10 : (e.enemyType == 2) ? 20 : 3;
					sf::Vector2f c(e.shape.getPosition().x + e.shape.getSize().x / 2.f,
						e.shape.getPosition().y + e.shape.getSize().y / 2.f);
					gainPoints(bp, c);
					addExplosionParticles(c, sf::Color(e.baseColor.r, e.baseColor.g, e.baseColor.b), 20);
					if (rand() % 100 < 30) {
						CurrencyDrop d;
						d.shape.setRadius(5.f); d.shape.setFillColor(sf::Color(255, 215, 0));
						d.shape.setPosition(c);
						d.velocity = sf::Vector2f(((rand() % 100) / 50.f) - 1.f, -2.f);
						d.timer = 600.f;
						drops.push_back(d);
					}
					enemiesKilledInWave++;
				}
				if ((int)enemies.size() > nukeCap)
					enemies.erase(enemies.begin(), enemies.begin() + nukeCap);
				else
					enemies.clear();
				explosionSound.play();
				if (bossEntity.isActive()) bossEntity.takeDamage(20); // B5: reduced from 50
			}
			else if (t == 3) { fastFireTimer = 600.f; powerUpSound.setPitch(1.2f); }
			else if (t == 4) { slowTimeTimer = 600.f; powerUpSound.setPitch(0.85f); }
			else if (t == 5) { health = std::min(health + 1, maxHealth); powerUpSound.setPitch(1.1f); }
			powerUpSound.play();
			powerUps.erase(powerUps.begin() + i); i--; continue;
		}
		if (powerUps[i].shape.getPosition().y > window->getSize().y) {
			powerUps.erase(powerUps.begin() + i); i--;
		}
	}
}

//  PARTICLES
void game::updateParticles()
{
	for (size_t i = 0; i < particles.size(); i++) {
		particles[i].shape.move(particles[i].velocity);
		particles[i].lifetime++;
		int alpha = (int)(255 - 255 * (particles[i].lifetime / particles[i].maxLifetime));
		sf::Color c = particles[i].shape.getFillColor();
		c.a = (sf::Uint8)(alpha > 0 ? alpha : 0);
		particles[i].shape.setFillColor(c);
		if (particles[i].lifetime >= particles[i].maxLifetime) { particles.erase(particles.begin() + i); i--; }
	}
}

//  SCORE POPUPS
void game::updateScorePopups()
{
	for (size_t i = 0; i < scorePopups.size(); i++) {
		scorePopups[i].text.move(scorePopups[i].velocity);
		scorePopups[i].lifetime += 1.f;
		float alpha = 255.f * (1.f - scorePopups[i].lifetime / scorePopups[i].maxLifetime);
		sf::Color c = scorePopups[i].text.getFillColor();
		c.a = (sf::Uint8)std::max(0.f, alpha);
		scorePopups[i].text.setFillColor(c);
		if (scorePopups[i].lifetime >= scorePopups[i].maxLifetime) { scorePopups.erase(scorePopups.begin() + i); i--; }
	}
}

//  BACKGROUND STARS
void game::updateBackground()
{
	// BULLET TIME (1 HP = slow motion)
	if (health == 1 && state == GameState::PLAYING) {
		timeScale = std::max(0.25f, timeScale - 0.05f);
	}
	else {
		timeScale = std::min(1.0f, timeScale + 0.05f);
	}

	// HYPERSPACE (Breather = star stretch)
	if (inBreather) {
		hyperspaceProgress = std::min(1.0f, hyperspaceProgress + 0.02f);
	}
	else {
		hyperspaceProgress = std::max(0.0f, hyperspaceProgress - 0.04f);
	}

	for (auto& star : stars) {
		float speed = star.speed * (inBreather ? (1.f + hyperspaceProgress * 20.f) : 1.f);
		star.shape.move(0.f, speed * timeScale);
		if (star.shape.getPosition().y > window->getSize().y) {
			star.shape.setPosition((float)(rand() % window->getSize().x), -5.f);
		}
	}
}

void game::updateMusic()
{
	bool inBossSequence = (state == GameState::BOSS_ARRIVAL) || isBossStage || isBoss2Stage || bossDeathSequence || boss2DeathSequence;
	bool wantBossMusic = inBossSequence && !EndGame;
	bool wantMenuMusic = !wantBossMusic && !EndGame;

	float bgTarget = wantMenuMusic ? 18.f : 0.f;
	float bossTarget = wantBossMusic ? 24.f : 0.f;
	float fadeStep = 0.08f;

	if (bgTarget > 0.f) {
		if (bgMusic.getStatus() != sf::Music::Playing) bgMusic.play();
		float v = bgMusic.getVolume();
		if (v < bgTarget) v = std::min(bgTarget, v + fadeStep);
		else v = std::max(bgTarget, v - fadeStep);
		bgMusic.setVolume(v);
	}
	else if (bgMusic.getStatus() != sf::Music::Stopped) {
		float v = bgMusic.getVolume();
		if (v > 0.2f) {
			bgMusic.setVolume(std::max(0.f, v - fadeStep));
		}
		else {
			bgMusic.stop();
		}
	}

	if (bossTarget > 0.f) {
		if (bossBgMusic.getStatus() != sf::Music::Playing) bossBgMusic.play();
		float v = bossBgMusic.getVolume();
		if (v < bossTarget) v = std::min(bossTarget, v + fadeStep);
		else v = std::max(bossTarget, v - fadeStep);
		bossBgMusic.setVolume(v);
	}
	else if (bossBgMusic.getStatus() != sf::Music::Stopped) {
		float v = bossBgMusic.getVolume();
		if (v > 0.2f) {
			bossBgMusic.setVolume(std::max(0.f, v - fadeStep));
		}
		else {
			bossBgMusic.stop();
		}
	}
}
//  WAVE SYSTEM
void game::updateWave()
{
	if (isBossStage || isBoss2Stage || bossDeathSequence || EndGame) return;

	if (inBreather) {
		breatherTimer += 1.f;
		if (breatherTimer >= breatherTimerMax) {
			inBreather = false; breatherTimer = 0.f;
			currentWave++;
			// Allow boss to trigger again on the next boss wave (multiples of 5, not wave 10)
			bool nextIsBossWave = (currentWave % 5 == 0) && (currentWave != 10) && (currentWave > 0);
			if (bossWaveDefeated && !nextIsBossWave) bossWaveDefeated = false;
			if (currentWave % 2 == 0) waveEnemySpeedBonus = std::min(waveEnemySpeedBonus + 0.3f, 3.0f); // B4: cap speed
			enemiesKilledInWave = 0; totalEnemiesSpawnedInWave = 0;
			enemiesNeededInWave = std::min(4 + currentWave * 2, 30); // B4: cap enemies per wave at 30
			std::stringstream ws; ws << "WAVE " << currentWave;
			waveBannerText.setString(ws.str());
			sf::FloatRect wb = waveBannerText.getLocalBounds();
			waveBannerText.setOrigin(wb.left + wb.width / 2.f, wb.top + wb.height / 2.f);
			waveBannerTimer = waveBannerTimerMax;
			waveCompleteSound.play();
			debugLog("Wave started: " + std::to_string(currentWave) +
				" enemiesNeeded=" + std::to_string(enemiesNeededInWave) +
				" speedBonus=" + std::to_string(waveEnemySpeedBonus));
		}
		return;
	}

	if (currentWave == 0) {
		currentWave = 1; enemiesNeededInWave = 6;
		enemiesKilledInWave = 0; totalEnemiesSpawnedInWave = 0;
		std::stringstream ws; ws << "WAVE " << currentWave;
		waveBannerText.setString(ws.str());
		sf::FloatRect wb = waveBannerText.getLocalBounds();
		waveBannerText.setOrigin(wb.left + wb.width / 2.f, wb.top + wb.height / 2.f);
		waveBannerTimer = waveBannerTimerMax;
		waveCompleteSound.play();
		debugLog("Wave started: " + std::to_string(currentWave) +
			" enemiesNeeded=" + std::to_string(enemiesNeededInWave));
	}

	if (totalEnemiesSpawnedInWave < enemiesNeededInWave && (int)enemies.size() < 6 && (int)spawnWarnings.size() < 3)
		spawnEnemyWithoutOverlap();

	if (enemiesKilledInWave >= enemiesNeededInWave && enemies.empty() && spawnWarnings.empty()) {
		inBreather = true; breatherTimer = 0.f;

		// ROGUELITE: Trigger perk select only after a boss wave is defeated
		// (bossWaveDefeated is set after boss death sequence completes)
		// Regular wave completion just resumes, no perk pick

		if (currentWave > highestWaveReached) {
			highestWaveReached = currentWave;
			saveGameData();
		}

		std::stringstream ws; ws << "WAVE " << currentWave << " COMPLETE!";
		waveBannerText.setString(ws.str());
		sf::FloatRect wb = waveBannerText.getLocalBounds();
		waveBannerText.setOrigin(wb.left + wb.width / 2.f, wb.top + wb.height / 2.f);
		waveBannerTimer = waveBannerTimerMax;
		waveCompleteSound.play();
		debugLog("Wave complete: " + std::to_string(currentWave) +
			" killed=" + std::to_string(enemiesKilledInWave) +
			" spawned=" + std::to_string(totalEnemiesSpawnedInWave));
	}

	if (waveBannerTimer > 0.f) waveBannerTimer -= 1.f;
}

//  COMBO
void game::updateCombo()
{
	if (comboCount > 0) {
		comboTimer -= 1.f;
		if (comboTimer <= 0.f) {
			// Risk penalty
			if (comboCount >= 5 && overheatEnabled) {
				isOverheated = true;
				overheatTimer = 0.f;
				heatLevel = heatMax;
				overheatSound.setPitch(0.9f);
				overheatSound.play();
				debugLog("Combo dropped! Weapon overheated.");
			}
			comboCount = 0;
			comboTimer = 0.f;
		}
	}
}

//  HEAT
void game::updateHeat()
{
	if (isOverheated) {
		overheatTimer += 1.f;
		heatLevel = std::max(0.f, heatLevel - (heatMax / overheatTimerMax));
		if (overheatTimer >= overheatTimerMax) { isOverheated = false; overheatTimer = 0.f; heatLevel = 0.f; }
	}
	else { heatLevel = std::max(0.f, heatLevel - 0.4f); }
}

//  SHIELD
void game::updateShield()
{
	if (!shieldActive) {
		shieldRechargeTimer += 1.f;
		if (shieldRechargeTimer >= shieldRechargeMax) { shieldActive = true; shieldRechargeTimer = 0.f; }
	}
	shieldShape.setPosition(player.getPosition());
	if (shieldActive) {
		float pulse = (std::sin(shieldRechargeTimer * 0.1f) + 1.f) * 0.5f;
		shieldShape.setFillColor(sf::Color(0, 200, 255, (sf::Uint8)(30 + pulse * 40)));
		shieldShape.setOutlineColor(sf::Color(0, 200, 255, (sf::Uint8)(150 + pulse * 80)));
	}
	else {
		float pct = shieldRechargeTimer / shieldRechargeMax;
		shieldShape.setFillColor(sf::Color(0, 0, 100, 20));
		shieldShape.setOutlineColor(sf::Color(100, 100, 200, (sf::Uint8)(60 + pct * 80)));
	}
}

//  DASH  (ghost trail uses sprites)
void game::updateDash()
{
	if (dashCooldown > 0.f) dashCooldown -= 1.f;

	if (isDashing) {
		dashTimer += 1.f;
		player.move(dashVelocity);

		// SHIELD RAMMING
		if (shieldActive) {
			for (auto& e : enemies) {
				if (player.getGlobalBounds().intersects(e.shape.getGlobalBounds())) {
					e.hp -= 10; // Massive contact damage
					if (e.hp > 0) e.flashTimer = 5;
					screenShake += 5.f;
					hitStopTimer = 3.f;
				}
			}
		}

		// Spawn ghost every 4 frames
		if ((int)dashTimer % 4 == 0) {
			DashGhost ghost;
			ghost.sprite = playerSprite;
			ghost.sprite.setColor(sf::Color(0, 220, 255, 150));
			ghost.alpha = 150.f;
			ghost.lifetime = 0.f;
			ghost.maxLifetime = 20.f;
			dashTrail.push_back(ghost);

			// PERK 3: TOXIC EXHAUST — dash trail poisons nearby enemies
			for (int pid : activePerkIds) if (pid == 3) {
				for (auto& e : enemies) {
					sf::Vector2f eCenter(e.shape.getPosition().x + e.shape.getSize().x / 2.f,
						e.shape.getPosition().y + e.shape.getSize().y / 2.f);
					sf::Vector2f pPos = player.getPosition();
					float dx = eCenter.x - pPos.x, dy = eCenter.y - pPos.y;
					if (dx * dx + dy * dy < 80.f * 80.f) {
						e.hp--;
						e.flashTimer = 3;
					}
				}
				break;
			}
		}

		if (dashTimer == 1.f) { dashSound.setPitch(0.95f + (float)(rand() % 11) * 0.01f); dashSound.play(); }
		if (dashTimer >= dashTimerMax) { isDashing = false; dashTimer = 0.f; dashCooldown = dashCooldownMax; }
	}

	// Fade ghosts
	for (size_t i = 0; i < dashTrail.size(); i++) {
		dashTrail[i].lifetime += 1.f;
		dashTrail[i].alpha = 150.f * (1.f - dashTrail[i].lifetime / dashTrail[i].maxLifetime);
		sf::Color c = dashTrail[i].sprite.getColor();
		c.a = (sf::Uint8)std::max(0.f, dashTrail[i].alpha);
		dashTrail[i].sprite.setColor(c);
		if (dashTrail[i].lifetime >= dashTrail[i].maxLifetime) { dashTrail.erase(dashTrail.begin() + i); i--; }
	}
}


void game::updateParticleTrails()
{
	// Lasers leave a small cyan/red trail
	for (auto& l : lasers) {
		if (rand() % 100 < 30) {
			Particle p;
			p.shape.setSize(sf::Vector2f(3.f, 3.f));
			p.shape.setFillColor(l.shape.getFillColor());
			p.shape.setPosition(l.shape.getPosition().x + (rand() % 5), l.shape.getPosition().y + (rand() % 5));
			p.velocity = sf::Vector2f(0.f, 0.f);
			p.maxLifetime = 15.f;
			p.lifetime = 0.f;
			particles.push_back(p);
		}
	}

	// Enemies leave trailing particles depending on type
	for (auto& e : enemies) {
		if (rand() % 100 < 20 && e.hp <= e.maxHp / 2) {
			Particle p;
			p.shape.setSize(sf::Vector2f(4.f, 4.f));
			p.shape.setFillColor(sf::Color(e.baseColor.r, e.baseColor.g, e.baseColor.b, 150));
			p.shape.setPosition(e.sprite.getPosition().x + (rand() % 20) - 10.f, e.sprite.getPosition().y + (rand() % 20) - 10.f);
			p.velocity = sf::Vector2f(0.f, 0.f);
			p.maxLifetime = 20.f;
			p.lifetime = 0.f;
			particles.push_back(p);
		}
	}

	// D4: engine exhaust trail — particles drift from the rear of the player ship
	if (state == GameState::PLAYING && !EndGame && !isDashing) {
		if (rand() % 100 < 65) {
			// Compute rear of ship: opposite direction from aim
			sf::Vector2f dir = mousePosView - player.getPosition();
			float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (len > 0.f) dir /= len;
			float rx = player.getPosition().x - dir.x * 18.f + (rand() % 8 - 4);
			float ry = player.getPosition().y - dir.y * 18.f + (rand() % 8 - 4);
			Particle ep;
			ep.shape.setSize(sf::Vector2f(4.f, 4.f));
			sf::Color exCol = (invincibilityTimer > 0.f) ? sf::Color(255, 200, 0, 200)
				: (fastFireTimer > 0.f) ? sf::Color(80, 255, 255, 200)
				: sf::Color(0, 160, 255, 180);
			ep.shape.setFillColor(exCol);
			ep.shape.setPosition(rx, ry);
			ep.velocity = sf::Vector2f(-dir.x * 1.5f + (rand() % 20 - 10) * 0.1f,
				-dir.y * 1.5f + (rand() % 20 - 10) * 0.1f);
			ep.lifetime = 0.f;
			ep.maxLifetime = 20.f;
			particles.push_back(ep);
		}
	}
}

void game::updateDrops()
{
	for (size_t i = 0; i < drops.size(); i++) {
		drops[i].shape.move(drops[i].velocity);
		drops[i].timer -= 1.f;

		// Move slowly downwards
		drops[i].velocity.y += 0.05f;

		// Collect
		if (drops[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
			totalGears++;
			saveGameData();
			powerUpSound.play(); // Reuse powerup sound for coin collect for now
			drops.erase(drops.begin() + i); i--; continue;
		}

		// Despawn
		if (drops[i].timer <= 0.f || drops[i].shape.getPosition().y > window->getSize().y) {
			drops.erase(drops.begin() + i); i--; continue;
		}
	}
}

//  SKINS
void game::updateSkins()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			if (BackText.getGlobalBounds().contains(mousePosView))
				state = GameState::MENU;

			for (int i = 0; i < 6; i++) {
				if (skinPreviews[i].getGlobalBounds().contains(mousePosView)) {
					selectedSkin = i;
					playerSprite.setTexture(skinTextures[i], true);
					sf::Vector2u nSz = skinTextures[i].getSize();
					playerSprite.setOrigin((float)nSz.x / 2.f, (float)nSz.y / 2.f);
					float scl = 60.f / (float)std::max(nSz.x, nSz.y);
					playerSprite.setScale(scl, scl);
					debugLog("Selected skin " + std::to_string(selectedSkin) +
						" textureSize=" + std::to_string(nSz.x) + "x" + std::to_string(nSz.y) +
						" scale=" + std::to_string(scl));
				}
			}
		}
	}
	else { mouseHeld = false; }

	auto updateHover = [&](sf::Text& t, sf::Color hColor) {
		bool h = t.getGlobalBounds().contains(mousePosView);
		t.setFillColor(h ? hColor : sf::Color::White);
		};
	updateHover(BackText, sf::Color(255, 210, 60));

	for (int i = 0; i < 6; i++) {
		bool h = skinPreviews[i].getGlobalBounds().contains(mousePosView);
		sf::Vector2u sz = skinTextures[i].getSize();
		float baseScale = 120.f / (float)std::max(sz.x, sz.y);
		float s = h ? baseScale * 1.1f : baseScale;
		skinPreviews[i].setScale(s, s);
	}
}

// ---- DEMO BACKGROUND ----
void game::updateDemoBackground()
{
	demoShipPos += demoShipVel;

	// Random erratic movement
	if (rand() % 60 == 0) {
		demoShipVel.y = (rand() % 40 - 20) / 10.f;
	}

	// Bounds wrap
	if (demoShipPos.x > window->getSize().x + 100.f) {
		demoShipPos.x = -100.f;
		demoShipPos.y = 100.f + rand() % (int)(window->getSize().y - 400.f);
	}
	if (demoShipPos.y < 50.f) demoShipVel.y = 1.f;
	if (demoShipPos.y > window->getSize().y - 250.f) demoShipVel.y = -1.f;

	float angle = std::atan2(demoShipVel.y, demoShipVel.x) * 180.f / 3.14159f;
	demoShipSprite.setPosition(demoShipPos);
	demoShipSprite.setRotation(angle + 90.f);

	demoShootTimer += 1.f;
	if (demoShootTimer > 20.f) {
		demoShootTimer = 0.f;
		DemoLaser l;
		l.shape.setSize(sf::Vector2f(2.f, 20.f));
		l.shape.setFillColor(sf::Color(0, 255, 255, 100));
		l.shape.setOrigin(1.f, 10.f);
		l.shape.setPosition(demoShipPos);
		l.shape.setRotation(angle + 90.f);
		l.vel = sf::Vector2f(std::cos(angle * 3.14159f / 180.f) * 15.f, std::sin(angle * 3.14159f / 180.f) * 15.f);
		demoLasers.push_back(l);
	}

	for (size_t i = 0; i < demoLasers.size(); i++) {
		demoLasers[i].shape.move(demoLasers[i].vel);
		float lx = demoLasers[i].shape.getPosition().x;
		float ly = demoLasers[i].shape.getPosition().y;
		if (lx < -50.f || lx > window->getSize().x + 50.f || ly < -50.f || ly > window->getSize().y + 50.f) {
			demoLasers.erase(demoLasers.begin() + i);
			i--;
		}
	}
}

// ---- INTRO SPLASH ----
void game::updateIntro()
{
	updateDemoBackground();

	introTimer += 1.f;

	// Moon rises with smooth ease-out cubic
	float t = std::min(1.f, introTimer / 180.f);
	float easedT = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
	float newY = introStartY - (introStartY - introTargetY) * easedT;
	introMoonY = newY;
	moonSprite.setPosition((float)window->getSize().x / 2.f, introMoonY);
	moonSprite.rotate(0.04f); // slow continuous revolution

	// Title alpha fades in after frame 60
	introTextAlpha = (introTimer > 60.f) ? std::min(255.f, (introTimer - 60.f) * 4.f) : 0.f;

	// Any key/click after frame 30
	if (introTimer > 30.f) {
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape) ||
			sf::Keyboard::isKeyPressed(sf::Keyboard::Return) ||
			sf::Keyboard::isKeyPressed(sf::Keyboard::Space) ||
			sf::Mouse::isButtonPressed(sf::Mouse::Left))
		{
			state = GameState::HOW_TO_PLAY;
			howToPage = 0;
			mouseHeld = true;
			debugLog("Intro -> HOW_TO_PLAY");
		}
	}
}

// ---- HOW-TO-PLAY ----
void game::updateHowToPlay()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			float cx = (float)window->getSize().x / 2.f;
			float wy = (float)window->getSize().y;
			// FIX: button areas match new render positions (cx±160, wy-90, size 200x60)
			sf::FloatRect nextBtn(cx + 160.f - 100.f, wy - 90.f - 30.f, 200.f, 60.f);
			sf::FloatRect backBtn(cx - 160.f - 100.f, wy - 90.f - 30.f, 200.f, 60.f);

			if (nextBtn.contains(mousePosView)) {
				if (howToPage < 3) howToPage++;
				else { state = GameState::MENU; debugLog("HowToPlay -> MENU"); }
			}
			else if (backBtn.contains(mousePosView)) {
				if (howToPage > 0) howToPage--;
				else { state = GameState::MENU; debugLog("HowToPlay -> MENU"); }
			}
		}
	}
	else { mouseHeld = false; }
}

// ---- BOSS ARRIVAL CINEMATIC ----
void game::updateBossArrival()
{
	bossArrivalTimer -= 1.f;

	// Screen shake ramps up
	if (bossArrivalTimer > 120.f)
		screenShake = 5.f;
	else
		screenShake = 18.f;

	// Start boss music a bit early
	if (bossArrivalTimer <= 240.f &&
		bossBgMusic.getStatus() != sf::Music::Playing) {
		bgMusic.setVolume(12.f);
		if (bossBgMusic.getVolume() <= 0.1f) bossBgMusic.setVolume(0.f);
		bossBgMusic.play();
	}

	if (waveBannerTimer > 0.f) waveBannerTimer -= 1.f;

	if (bossArrivalTimer <= 0.f) {
		state = GameState::PLAYING;
		if (bossArrivalWave == 1) {
			isBossStage = true;
			debugLog("Boss 1 arrival complete, activating boss stage");
		}
		else if (bossArrivalWave == 2) {
			isBoss2Stage = true;
			boss2Entity.initBoss2(window, difficulty);
			debugLog("Boss 2 arrival complete, activating boss2 stage");
		}
		bossArrivalPending = false;
		screenShake = 0.f;
	}
}

// ---- BOSS 2 UPDATE (called from master update) ----
void game::updateBoss2()
{
	boss2Entity.update(health, player, points, isBoss2Stage,
		invincibilityTimer > 0.f, slowTimeTimer > 0.f,
		invincibilityTimer, deathRayTimer, fastFireTimer,
		laserFireTimerMax, screenShake, maxHealth,
		[this](int d) { this->playerTakeDamage(d); });

	if (boss2Entity.hasStageTransitioned()) {
		spawnExplosion(boss2Entity.getBounds().left + 100.f, boss2Entity.getBounds().top + 100.f, 3.0f);
		explosionSound.play();
		boss2Entity.clearStageTransition();
	}

	if (!isBoss2Stage && boss2Entity.isDying() && !boss2DeathSequence) {
		boss2DeathSequence = true;
		boss2DeathTimer = 0.f;
		timeScale = 0.3f; // Dramatic slow-mo
		debugLog("Boss 2 death sequence started");
	}
}

// ---- EXPLOSION ANIMATIONS ----
void game::updateExplosions()
{
	for (size_t i = 0; i < activeExplosions.size(); i++) {
		auto& ex = activeExplosions[i];
		ex.frameTick += 1.f;
		if (ex.frameTick >= 6.f) {
			ex.frameTick = 0.f;
			ex.frame++;
		}
		if (ex.frame >= EXPL_FRAMES) { ex.done = true; }
		if (ex.done) { activeExplosions.erase(activeExplosions.begin() + i); i--; continue; }

		// Fade alpha on last 2 frames
		float alphaScale = (ex.frame >= EXPL_FRAMES - 2)
			? (float)(EXPL_FRAMES - ex.frame) / 2.f : 1.f;
		ex.sprite.setColor(sf::Color(255, 255, 255, (sf::Uint8)(255 * alphaScale)));
		ex.sprite.setTextureRect(sf::IntRect(ex.frame * EXPL_FRAME_W, 0, EXPL_FRAME_W, EXPL_FRAME_H));
		ex.sprite.setPosition(ex.x, ex.y);
	}

	// Boss death sequence for Boss 1 — 660 frames (~5.5 seconds at 120 fps)
	if (bossDeathSequence) {
		bossDeathTimer += 1.f;
		
		// Move the boss slowly downwards as it explodes
		bossEntity.move(0.f, 0.6f);

		// Sustained screen shake that pulses
		screenShake = 6.f + std::sin(bossDeathTimer * 0.15f) * 4.f;

		// FIX: duck boss music to silence during death sequence for dramatic silence
		if (bossDeathTimer < 5.f) {
			bossBgMusic.setVolume(std::max(0.f, bossBgMusic.getVolume() - 2.f));
			if (bossBgMusic.getVolume() <= 0.1f) bossBgMusic.stop();
		}

		// --- explosion burst schedule ---
		sf::FloatRect bb = bossEntity.getBounds();
		float cx = bb.left + bb.width * 0.5f;
		float cy = bb.top + bb.height * 0.5f;

		if (bossDeathTimer == 1.f) {
			destructionSound.setPitch(0.85f);
			destructionSound.play();
			spawnExplosion(cx, cy, 4.5f);
		}
		else if (bossDeathTimer == 80.f) {
			spawnExplosion(cx - 80.f, cy + 60.f, 3.5f);
		}
		else if (bossDeathTimer == 160.f) {
			spawnExplosion(cx + 90.f, cy - 40.f, 3.5f);
			// second sound hit for extra drama
			destructionSound.play();
		}
		else if (bossDeathTimer == 240.f) {
			spawnExplosion(cx - 50.f, cy - 70.f, 3.0f);
		}
		else if (bossDeathTimer == 320.f) {
			spawnExplosion(cx + 60.f, cy + 80.f, 3.0f);
		}
		else if (bossDeathTimer == 400.f) {
			spawnExplosion(cx, cy + 50.f, 3.5f);
			destructionSound.play();
		}
		else if (bossDeathTimer == 480.f) {
			spawnExplosion(cx - 100.f, cy, 2.5f);
			spawnExplosion(cx + 100.f, cy, 2.5f);
		}
		else if (bossDeathTimer == 560.f) {
			// Final mega-explosion
			spawnExplosion(cx, cy, 5.5f);
			spawnExplosion(cx - 60.f, cy - 60.f, 3.0f);
			spawnExplosion(cx + 60.f, cy + 60.f, 3.0f);
			destructionSound.play();
			screenShake = 30.f; // big final shockwave
		}

		// Sequence complete after 660 frames (~5.5s)
		if (bossDeathTimer >= 660.f) {
			bossDeathSequence = false;
			bossDeathTimer = 0.f;
			bossWaveDefeated = true;
			isBossStage = false;
			bossEntity.setActive(false);
			screenShake = 0.f;
			inBreather = true; breatherTimer = 0.f;
			// Offer a perk after defeating the boss
			state = GameState::PERK_SELECT;
			mouseHeld = true; // Prevent accidental instant selection if player was holding fire
			generatePerkChoices();
			std::stringstream bws;
			bws << "BOSS DEFEATED! WAVE " << currentWave;
			waveBannerText.setString(bws.str());
			sf::FloatRect bwb = waveBannerText.getLocalBounds();
			waveBannerText.setOrigin(bwb.left + bwb.width / 2.f, bwb.top + bwb.height / 2.f);
			waveBannerTimer = waveBannerTimerMax;
			bossBgMusic.stop();
			bgMusic.setVolume(18.f);
			bgMusic.play();
			debugLog("Boss death sequence complete");
		}
	}

	// Boss 2 death sequence
	if (boss2DeathSequence) {
		boss2DeathTimer += 1.f;
		screenShake = 6.f + std::sin(boss2DeathTimer * 0.15f) * 4.f;

		// FIX: silence boss music immediately on death
		if (boss2DeathTimer < 5.f) {
			bossBgMusic.setVolume(std::max(0.f, bossBgMusic.getVolume() - 2.f));
			if (bossBgMusic.getVolume() <= 0.1f) bossBgMusic.stop();
		}

		sf::FloatRect bb = boss2Entity.getBounds();
		float cx = bb.left + bb.width * 0.5f;
		float cy = bb.top + bb.height * 0.5f;

		if (boss2DeathTimer == 1.f) {
			destructionSound.setPitch(0.8f);
			destructionSound.play();
			spawnExplosion(cx, cy, 5.5f);
		}
		else if (boss2DeathTimer == 80.f) {
			spawnExplosion(cx - 90.f, cy + 70.f, 4.5f);
		}
		else if (boss2DeathTimer == 160.f) {
			spawnExplosion(cx + 100.f, cy - 50.f, 4.5f);
			destructionSound.play();
		}
		else if (boss2DeathTimer == 240.f) {
			spawnExplosion(cx - 60.f, cy - 80.f, 4.0f);
		}
		else if (boss2DeathTimer == 320.f) {
			spawnExplosion(cx + 70.f, cy + 90.f, 4.0f);
		}
		else if (boss2DeathTimer == 400.f) {
			spawnExplosion(cx, cy + 60.f, 4.5f);
			destructionSound.play();
		}
		else if (boss2DeathTimer == 480.f) {
			spawnExplosion(cx - 120.f, cy, 3.5f);
			spawnExplosion(cx + 120.f, cy, 3.5f);
		}
		else if (boss2DeathTimer == 560.f) {
			spawnExplosion(cx, cy, 6.5f);
			spawnExplosion(cx - 70.f, cy - 70.f, 4.0f);
			spawnExplosion(cx + 70.f, cy + 70.f, 4.0f);
			destructionSound.play();
			screenShake = 35.f; // big final shockwave
		}
		else if (boss2DeathTimer >= 660.f) {
			boss2DeathSequence = false;
			boss2DeathTimer = 0.f;
			boss2Defeated = true;
			screenShake = 0.f;
			inBreather = true; breatherTimer = 0.f;
			// Offer a perk after defeating the boss
			state = GameState::PERK_SELECT;
			mouseHeld = true; // Prevent accidental instant selection if player was holding fire
			generatePerkChoices();
			std::stringstream bws;
			bws << "SURT DEFEATED! WAVE " << currentWave;
			waveBannerText.setString(bws.str());
			sf::FloatRect bwb = waveBannerText.getLocalBounds();
			waveBannerText.setOrigin(bwb.left + bwb.width / 2.f, bwb.top + bwb.height / 2.f);
			waveBannerTimer = waveBannerTimerMax;
			bossBgMusic.stop();
			bgMusic.setVolume(18.f);
			bgMusic.play();
			debugLog("Boss 2 death sequence complete");
		}
	}
}

void game::spawnExplosion(float x, float y, float scale)
{
	if (explosionTex.getSize().x == 0) return;
	ExplosionAnim ex;
	ex.frame = 0;
	ex.frameTick = 0.f;
	ex.x = x - (EXPL_FRAME_W * scale) / 2.f;
	ex.y = y - (EXPL_FRAME_H * scale) / 2.f;
	ex.scale = scale;
	ex.done = false;
	ex.sprite.setTexture(explosionTex);
	ex.sprite.setTextureRect(sf::IntRect(0, 0, EXPL_FRAME_W, EXPL_FRAME_H));
	ex.sprite.setScale(scale, scale);
	ex.sprite.setPosition(ex.x, ex.y);
	activeExplosions.push_back(ex);
}

// ---- HAZARDS ----
void game::updateHazards()
{
	// Random Spawn Asteroid
	if (rand() % 600 == 0) {
		Asteroid a;
		float radius = (float)(rand() % 40 + 20);
		a.shape.setRadius(radius);
		a.shape.setPointCount(8);
		a.shape.setFillColor(sf::Color(80, 80, 90));
		a.shape.setOutlineColor(sf::Color(120, 120, 130));
		a.shape.setOutlineThickness(2.f);
		a.shape.setOrigin(radius, radius);

		int side = rand() % 4;
		float x, y;
		if (side == 0) { x = -radius; y = (float)(rand() % window->getSize().y); a.velocity = sf::Vector2f(1.5f, 0.5f); }
		else if (side == 1) { x = (float)window->getSize().x + radius; y = (float)(rand() % window->getSize().y); a.velocity = sf::Vector2f(-1.5f, -0.5f); }
		else if (side == 2) { y = -radius; x = (float)(rand() % window->getSize().x); a.velocity = sf::Vector2f(0.5f, 1.5f); }
		else { y = (float)window->getSize().y + radius; x = (float)(rand() % window->getSize().x); a.velocity = sf::Vector2f(-0.5f, -1.5f); }

		a.shape.setPosition(x, y);
		a.hp = 20;
		a.rotationSpeed = (rand() % 20 - 10) / 10.f;
		asteroids.push_back(a);
	}

	// Random Spawn Black Hole
	if (rand() % 2500 == 0 && blackHoles.empty()) {
		BlackHole bh;
		float r = 50.f;
		bh.core.setRadius(r);
		bh.core.setFillColor(sf::Color::Black);
		bh.core.setOutlineColor(sf::Color(100, 0, 255, 150));
		bh.core.setOutlineThickness(4.f);
		bh.core.setOrigin(r, r);
		bh.core.setPosition((float)window->getSize().x / 2.f, (float)window->getSize().y / 2.f);

		bh.eventHorizon.setRadius(300.f);
		bh.eventHorizon.setOrigin(300.f, 300.f);
		bh.eventHorizon.setPosition(bh.core.getPosition());
		bh.eventHorizon.setFillColor(sf::Color(50, 0, 100, 25));

		bh.gravityRadius = 300.f;
		bh.pullStrength = 0.6f;
		bh.lifetime = 0.f;
		bh.maxLifetime = 500.f;
		blackHoles.push_back(bh);
	}

	// Update Asteroids
	for (size_t i = 0; i < asteroids.size(); i++) {
		asteroids[i].shape.move(asteroids[i].velocity);
		asteroids[i].shape.rotate(asteroids[i].rotationSpeed);

		// Player Collision
		if (asteroids[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
			playerTakeDamage(2);
			asteroids[i].hp -= 5;
		}

		// Enemy Collision
		for (auto& e : enemies) {
			if (asteroids[i].shape.getGlobalBounds().intersects(e.shape.getGlobalBounds())) {
				e.hp--;
			}
		}

		if (asteroids[i].hp <= 0) {
			addExplosionParticles(asteroids[i].shape.getPosition(), sf::Color(100, 100, 100), 20);
			asteroids.erase(asteroids.begin() + i); i--; continue;
		}

		// Out of bounds check
		sf::Vector2f p = asteroids[i].shape.getPosition();
		float r = asteroids[i].shape.getRadius();
		if (p.x < -r * 3 || p.x > window->getSize().x + r * 3 || p.y < -r * 3 || p.y > window->getSize().y + r * 3) {
			asteroids.erase(asteroids.begin() + i); i--; continue;
		}
	}

	// Update Black Holes
	for (size_t i = 0; i < blackHoles.size(); i++) {
		blackHoles[i].lifetime += 1.f;
		sf::Vector2f bhPos = blackHoles[i].core.getPosition();

		// Gravity effect
		auto pull = [&](sf::Vector2f pos, float factor) -> sf::Vector2f {
			sf::Vector2f dir = bhPos - pos;
			float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (dist < blackHoles[i].gravityRadius && dist > 10.f) {
				return (dir / dist) * blackHoles[i].pullStrength * factor;
			}
			return sf::Vector2f(0.f, 0.f);
			};

		player.move(pull(player.getPosition(), 1.0f));
		for (auto& e : enemies) e.shape.move(pull(e.shape.getPosition(), 0.5f));
		for (auto& l : lasers) l.shape.move(pull(l.shape.getPosition(), 2.0f));
		for (auto& b : enemyBullets) b.shape.move(pull(b.shape.getPosition(), 1.5f));

		if (blackHoles[i].lifetime >= blackHoles[i].maxLifetime) {
			blackHoles.erase(blackHoles.begin() + i); i--; continue;
		}
	}
}

void game::updateSecondaryWeapons()
{
	if (state != GameState::PLAYING) return;

	secondaryEnergy = std::min(secondaryEnergyMax, secondaryEnergy + 0.5f);

	if (rightMouseHeld && secondaryEnergy >= 30.f) {
		secondaryEnergy -= 30.f;
		if (selectedShipClass == 0) { // Tank: Mines
			SpaceMine m;
			m.shape.setRadius(15.f);
			m.shape.setFillColor(sf::Color::Yellow);
			m.shape.setOrigin(15.f, 15.f);
			m.shape.setPosition(player.getPosition());
			m.timer = 300.f;
			m.detonated = false;
			mines.push_back(m);
		}
		else if (selectedShipClass == 1) { // Speedster: Missiles
			for (int i = 0; i < 4; i++) {
				HomingMissile m;
				m.shape.setRadius(5.f);
				m.shape.setFillColor(sf::Color::Cyan);
				m.shape.setPosition(player.getPosition());
				float angle = (float)(rand() % 360) * 3.14159f / 180.f;
				m.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * 5.f;
				m.lifetime = 0.f;
				m.maxLifetime = 200.f;
				missiles.push_back(m);
			}
		}
		else if (selectedShipClass == 2) { // Sniper: Shotgun
			for (int i = 0; i < 12; i++) {
				Laser l;
				l.shape.setSize(sf::Vector2f(4.f, 30.f));
				l.shape.setFillColor(sf::Color::White);
				l.shape.setOrigin(2.f, 15.f);
				l.shape.setPosition(player.getPosition());
				float baseAngle = std::atan2(mousePosView.y - player.getPosition().y, mousePosView.x - player.getPosition().x);
				float angle = baseAngle + (i - 6) * 0.1f;
				l.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * 15.f;
				l.shape.setRotation(angle * 180.f / 3.14159f + 90.f);
				lasers.push_back(l);
			}
		}
		powerUpSound.play();
	}

	// Update Missiles
	for (size_t i = 0; i < missiles.size(); i++) {
		missiles[i].lifetime += 1.f;
		if (!enemies.empty()) {
			sf::Vector2f targetPos = enemies[0].shape.getPosition();
			sf::Vector2f dir = targetPos - missiles[i].shape.getPosition();
			float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
			if (dist > 0) {
				sf::Vector2f desired = (dir / dist) * 7.f;
				missiles[i].velocity += (desired - missiles[i].velocity) * 0.1f;
			}
		}
		missiles[i].shape.move(missiles[i].velocity);
		if (missiles[i].lifetime >= missiles[i].maxLifetime) {
			missiles.erase(missiles.begin() + i); i--;
		}
	}

	// Update Mines
	for (size_t i = 0; i < mines.size(); i++) {
		mines[i].timer -= 1.f;

		// Proximity detonation
		bool detonate = false;
		for (auto& e : enemies) {
			if (mines[i].shape.getGlobalBounds().intersects(e.shape.getGlobalBounds())) { detonate = true; break; }
		}

		if (detonate || mines[i].timer <= 0) {
			sf::Vector2f mpos = mines[i].shape.getPosition();
			explosionSound.play();
			screenShake += 10.f;
			addExplosionParticles(mpos, sf::Color::Yellow, 40);
			spawnExplosion(mpos.x, mpos.y, 2.5f);

			// AoE Damage
			for (size_t k = 0; k < enemies.size(); k++) {
				sf::Vector2f ep = enemies[k].shape.getPosition();
				float d = (float)std::sqrt(std::pow(ep.x - mpos.x, 2) + std::pow(ep.y - mpos.y, 2));
				if (d < 150.f) {
					enemies[k].hp -= 15;
					if (enemies[k].hp <= 0) {
						gainPoints(10, ep);
						enemiesKilledInWave++;
						enemies.erase(enemies.begin() + k); k--;
					}
				}
			}
			mines.erase(mines.begin() + i); i--;
		}
	}

	// Missile collision
	for (size_t i = 0; i < missiles.size(); i++) {
		for (size_t k = 0; k < enemies.size(); k++) {
			if (missiles[i].shape.getGlobalBounds().intersects(enemies[k].shape.getGlobalBounds())) {
				enemies[k].hp -= 8;
				explosionSound.play();
				addExplosionParticles(missiles[i].shape.getPosition(), sf::Color::Cyan, 15);
				spawnExplosion(missiles[i].shape.getPosition().x, missiles[i].shape.getPosition().y, 1.2f);
				if (enemies[k].hp <= 0) {
					gainPoints(10, enemies[k].shape.getPosition());
					enemiesKilledInWave++;
					enemies.erase(enemies.begin() + k);
				}
				missiles.erase(missiles.begin() + i); i--; break;
			}
		}
	}
}

void game::updatePerkSelect()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			float cx = (float)window->getSize().x / 2.f;
			float cy = (float)window->getSize().y / 2.f;

			for (int i = 0; i < 3; i++) {
				sf::FloatRect cardBounds(cx - 400.f + i * 400.f - 180.f, cy - 120.f, 360.f, 240.f);
				if (cardBounds.contains(mousePosView)) {
					int chosenId = perkChoices[i];
					activePerkIds.push_back(chosenId);
					powerUpSound.play();
					// Immediate perk effects
					if (chosenId == 1) { health = 1; maxHealth = 1; } // GLASS CANNON
					state = GameState::PLAYING;
					debugLog("Perk selected: " + perkPool[chosenId].name);
					return;
				}
			}
		}
	}
	else { mouseHeld = false; }
}

void game::generatePerkChoices()
{
	std::vector<int> allIndices;
	for (int i = 0; i < (int)perkPool.size(); i++) allIndices.push_back(i);
	std::random_shuffle(allIndices.begin(), allIndices.end());

	for (int i = 0; i < 3; i++) {
		perkChoices[i] = allIndices[i % allIndices.size()];
	}
	debugLog("Generated new perk choices");
}

void game::updateBlackMarket()
{
	if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
		if (!mouseHeld) {
			mouseHeld = true;
			if (BackText.getGlobalBounds().contains(mousePosView)) { state = GameState::MENU; return; }

			float cx = (float)window->getSize().x / 2.f;
			float startY = 250.f;
			int* blkUpgrades[] = { &blkStartingExtraHealth, &blkPermanentSpeedBoost, &blkDamageMultiplier, &blkStartingGears };
			int baseCost[] = { 1000, 1500, 2000, 500 };

			for (int i = 0; i < 4; i++) {
				sf::FloatRect btnBounds(cx + 100.f, startY + i * 80.f - 25.f, 150.f, 50.f);
				if (btnBounds.contains(mousePosView)) {
					int cost = baseCost[i] * (*blkUpgrades[i] + 1);
					if (totalGears >= cost) {
						totalGears -= cost;
						(*blkUpgrades[i])++;
						saveGameData();
						powerUpSound.play();
					}
					else {
						hitSound.play();
					}
				}
			}
		}
	}
	else { mouseHeld = false; }
	BackText.setFillColor(BackText.getGlobalBounds().contains(mousePosView) ? sf::Color::Cyan : sf::Color::White);
}

void game::updateTransition()
{
	if (transitionFadingOut) {
		transitionAlpha += 5.f;
		if (transitionAlpha >= 255.f) {
			transitionAlpha = 255.f;
			transitionFadingOut = false;
			transitionFadingIn = true;
			state = transitionTarget;
		}
	}
	else if (transitionFadingIn) {
		transitionAlpha -= 5.f;
		if (transitionAlpha <= 0.f) {
			transitionAlpha = 0.f;
			transitionFadingIn = false;
		}
	}
}