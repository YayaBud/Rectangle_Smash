// game.cpp  â€”  pollevent, updateText, master update(), master render()
// All other functions live in: game_init.cpp, game_spawn.cpp, game_update.cpp, game_render.cpp
#include "game.h"

//  POLL EVENT
void game::pollevent()
{
	while (this->window->pollEvent(this->ev)) {
		switch (this->ev.type) {
		case sf::Event::Closed:
			this->window->close();
			break;
		case sf::Event::KeyPressed:
			if (ev.key.code == sf::Keyboard::Escape || (this->EndGame && ev.key.code == sf::Keyboard::Enter)) {
				if (this->EndGame) { 
					this->EndGame = false;
					this->state = GameState::MENU;
					debugLog("Returned to MENU from GAME OVER");
				}
				else if (this->state == GameState::PLAYING) { this->state = GameState::PAUSED; debugLog("State changed: PAUSED"); }
				else if (this->state == GameState::PAUSED) { this->state = GameState::PLAYING; debugLog("State changed: PLAYING"); }
				else if (this->state == GameState::DIFFICULTY_SELECT || this->state == GameState::SETTINGS || this->state == GameState::SKINS) { this->state = GameState::MENU; debugLog("State changed: MENU"); }
				else if (this->state == GameState::INTRO) { this->state = GameState::HOW_TO_PLAY; howToPage = 0; }
			}
			if (ev.key.code == sf::Keyboard::M && this->state == GameState::PAUSED) {
				this->state = GameState::MENU;
				debugLog("State changed: MENU from PAUSED");
			}
			// Bomb + Ultimate are edge-triggered here rather than polled with
			// isKeyPressed in their update functions: polling meant holding the
			// key re-fired the instant the cooldown lapsed.
			if (this->state == GameState::PLAYING && !this->EndGame) {
				if (ev.key.code == sf::Keyboard::LShift || ev.key.code == sf::Keyboard::RShift ||
					ev.key.code == sf::Keyboard::F2)
					triggerBomb();
				if (ev.key.code == sf::Keyboard::Q || ev.key.code == sf::Keyboard::E)
					triggerUltimate();
			}
			// Space = Dash
			if (ev.key.code == sf::Keyboard::Space &&
				!isDashing && dashCooldown <= 0.f &&
				this->state == GameState::PLAYING && !this->EndGame) {
				isDashing = true; dashTimer = 0.f;
				parryWindowTimer = 12.f; // Perfect Parry window initiated
				sf::Vector2f dir(0.f, 0.f);
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) dir.x -= 1.f;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) dir.x += 1.f;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) dir.y -= 1.f;
				if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) dir.y += 1.f;
				if (dir.x == 0.f && dir.y == 0.f) {
					dir = mousePosView - player.getPosition();
					float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
					if (len > 0.f) dir /= len; else dir = sf::Vector2f(0.f, -1.f);
				}
				else {
					float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
					if (len > 0.f) dir /= len;
				}
				dashVelocity = dir * 15.f;
				dashSound.play();
				screenShake += 3.f; // Juice on dash
			}
			break;
		case sf::Event::MouseButtonPressed:
			if (ev.mouseButton.button == sf::Mouse::Left && !autoFire) isFiring = true;
			if (ev.mouseButton.button == sf::Mouse::Right) rightMouseHeld = true;
			break;
		case sf::Event::MouseButtonReleased:
			if (ev.mouseButton.button == sf::Mouse::Left) isFiring = false;
			if (ev.mouseButton.button == sf::Mouse::Right) rightMouseHeld = false;
			break;
		}
	}

	if (this->EndGame || this->state == GameState::PAUSED) return;
	if (this->state == GameState::INTRO || this->state == GameState::HOW_TO_PLAY ||
		this->state == GameState::BOSS_ARRIVAL) return;

	// Boss 1 trigger: every 5 waves (waves 5, 15, 20…), not wave 10
	bool isBossWave = (currentWave % 5 == 0) && (currentWave != 10) && (currentWave > 0);
	if (isBossWave && !isBossStage && !bossWaveDefeated && !bossArrivalPending && !bossEntity.isActive() && !isBoss2Stage && !bossDeathSequence) {
		bossArrivalPending = true;
		bossArrivalTimer = 300.f;
		bossArrivalWave = 1;
		state = GameState::BOSS_ARRIVAL;
		bossWaveDefeated = true;  // mark so it doesn't re-trigger this wave; cleared after boss dies
		debugLog("Boss 1 arrival triggered at wave " + std::to_string(currentWave));
	}

	// Boss 2 trigger: wave 10
	if (currentWave == 10 && !isBoss2Stage && !boss2Defeated && !bossArrivalPending && !boss2DeathSequence) {
		bossArrivalPending = true;
		bossArrivalTimer = 300.f;
		bossArrivalWave = 2;
		state = GameState::BOSS_ARRIVAL;
		debugLog("Boss 2 (Surt) arrival triggered at wave 10");
	}
	// Debug cheat key (F10) removed to prevent accidental wave skipping.
}

//  MOUSE POSITIONS
void game::updatemousePositions()
{
	this->mousePoswindow = sf::Mouse::getPosition(*this->window);
	this->mousePosView = this->window->mapPixelToCoords(this->mousePoswindow);
}

//  TEXT (HUD strings)
void game::updateText()
{
	std::stringstream ss;
	ss << "POINTS: " << this->points << "\n"
		<< "HEALTH: " << this->health << "/" << this->maxHealth << "\n"
		<< "WAVE:   " << this->currentWave;
	this->UiText.setString(ss.str());

	if (comboCount > 1) {
		std::stringstream cs; cs << "x" << comboCount << " COMBO";
		comboText.setString(cs.str());
		float t = std::min((float)(comboCount - 1) / 9.f, 1.f);
		comboText.setFillColor(sf::Color(255, (sf::Uint8)(255 * (1.f - t)), 0));
	}
	else { comboText.setString(""); }

	for (int i = 0; i < MAX_HIGH_SCORES; i++) {
		std::stringstream hs;
		hs << "#" << (i + 1) << "  " << highScores[i] << " pts  - W" << highScoreWaves[i];
		highScoreEntryTexts[i].setString(hs.str());
	}
}

//  MASTER UPDATE
void game::update()
{
	updateTransition();
	this->pollevent();
	this->updatemousePositions();
	this->updateBackground();
	this->updateMusic(); // FIX: was never called — boss music now works

	if (state == GameState::INTRO) {
		updateIntro(); return;
	}
	else if (state == GameState::HOW_TO_PLAY) {
		updateHowToPlay(); return;
	}
	else if (state == GameState::BOSS_ARRIVAL) {
		updateBossArrival(); return;
	}
	else if (state == GameState::MENU) {
		updateMenu(); updateText();
	}
	else if (state == GameState::SHIP_SELECT) {
		updateShipSelect();
	}
	else if (state == GameState::SHOP) {
		updateShop();
	}
	else if (state == GameState::BLACK_MARKET) {
		updateBlackMarket();
	}
	else if (state == GameState::PERK_SELECT) {
		updatePerkSelect();
	}
	else if (state == GameState::DIFFICULTY_SELECT) {
		updateDifficultySelect();
	}
	else if (state == GameState::SETTINGS) {
		updateSettings();
	}
	else if (state == GameState::SKINS) {
		updateSkins();
	}
	else if (state == GameState::PAUSED) {
	}
	else if (state == GameState::PLAYING && !EndGame) {
		updateText();
		updateWave();
		updateCombo();
		updateHeat();
		updateShield();
		updateDash();
		updateSecondaryWeapons();
		updatePowerUps();
		updateDrops();

		if (!isBossStage && !isBoss2Stage && !bossDeathSequence) {
			updateEnemies();
			updateSpawnWarnings();
			updateHazards();
		}
		else if (isBossStage && !bossDeathSequence) {
			bossEntity.update(health, player, points, isBossStage,
				invincibilityTimer > 0.f, slowTimeTimer > 0.f,
				invincibilityTimer, deathRayTimer, fastFireTimer,
				laserFireTimerMax, screenShake, maxHealth,
				[this](int d) { this->playerTakeDamage(d); });

			int currentBossState = bossEntity.getBossState();
			if (currentBossState == 2 && bossLastState != 2) {
				bossBeamSound.play();
				bossLazerSound2.play();
			}
			bossLastState = currentBossState;

			// Boss 1 died — start explosion sequence instead of immediate deactivation
			if (!isBossStage && bossEntity.isDying() && !bossDeathSequence) {
				isBossStage = true; // Keep the stage active so the boss sprite renders
				bossDeathSequence = true;
				bossDeathTimer = 0.f;
				timeScale = 0.3f; // Dramatic slow-mo
				debugLog("Boss 1 death sequence started");
			}
		}
		else if (isBoss2Stage) {
			updateBoss2();

			// laser sound on beam
			if (boss2Entity.isActive()) {
				bool currentBoss2Beam = boss2Entity.isBeamActive();
				if (currentBoss2Beam && !boss2LastBeamState) {
					bossBeamSound.play();
					bossLazerSound2.play();
				}
				boss2LastBeamState = currentBoss2Beam;
			}
		}

		// Boss death explosion animation
		updateExplosions();

		if (hitStopTimer > 0.f) {
			hitStopTimer -= 1.f;
			updateBackground(); // Keep background moving slightly
			// still do screen shake
			if (screenShake > 0.f) { screenShake -= 0.5f; if (screenShake < 0.f) screenShake = 0.f; }
			return; // Skip other updates
		}

		if (!playerDeathSequence) updatePlayer();
		updateBombs();
		updateDrones();
		updateLasers();
		updateEnemyBullets();
		// These three were implemented but never called, so the ultimate meter,
		// graze streak and combo pop did nothing all game.
		updateGraze();
		updateUltimate();
		updateComboAnnouncer();
		updateShockwaves();
		updateParticles();
		updateParticleTrails();
		updateScorePopups();

		if (killBannerTimer > 0.f) killBannerTimer -= 1.f;
		if (screenShake > 0.f) { screenShake -= 0.5f; if (screenShake < 0.f) screenShake = 0.f; }

		if (health <= 0 && !playerDeathSequence && !EndGame) {
			playerDeathSequence = true;
			playerDeathTimer = 0.f;
			timeScale = 0.3f;
			destructionSound.play();
			screenShake = 30.f;
		}

		if (playerDeathSequence) {
			playerDeathTimer += 1.f;
			
			if (playerDeathTimer == 1.f) spawnExplosion(player.getPosition().x, player.getPosition().y, 5.0f);
			if (playerDeathTimer == 20.f) spawnExplosion(player.getPosition().x + 20.f, player.getPosition().y - 20.f, 3.5f);
			if (playerDeathTimer == 40.f) spawnExplosion(player.getPosition().x - 20.f, player.getPosition().y + 20.f, 3.5f);
			if (playerDeathTimer == 60.f) { spawnExplosion(player.getPosition().x, player.getPosition().y, 6.0f); destructionSound.play(); screenShake = 40.f; }

			if (playerDeathTimer >= 150.f) {
				EndGame = true; state = GameState::GAME_OVER;
				playerDeathSequence = false;
				timeScale = 1.0f;
				std::stringstream fs; fs << "Score: " << points << "  -  Wave: " << currentWave << "\n\nPRESS ENTER TO CONTINUE";
				FinalScoreText.setString(fs.str());
				sf::FloatRect fb = FinalScoreText.getLocalBounds();
				FinalScoreText.setOrigin(fb.left + fb.width / 2.f, fb.top + fb.height / 2.f);
				FinalScoreText.setPosition((float)window->getSize().x / 2.f, (float)window->getSize().y / 2.f - 30.f);
				// Insert score into high score table
				for (int i = 0; i < MAX_HIGH_SCORES; i++) {
					if ((int)points > highScores[i]) {
						for (int j = MAX_HIGH_SCORES - 1; j > i; j--) {
							highScores[j] = highScores[j - 1]; highScoreWaves[j] = highScoreWaves[j - 1];
						}
						highScores[i] = (int)points; highScoreWaves[i] = currentWave; break;
					}
				}
				saveHighScore();
			}
		}
	}
}

//  MASTER RENDER
void game::render()
{
	window->clear(sf::Color::Black);

	// Screen shake
	sf::View view = window->getDefaultView();
	if (screenShake > 0.f && state == GameState::PLAYING) {
		float ox = (rand() % 100 - 50) / 100.f * screenShake;
		float oy = (rand() % 100 - 50) / 100.f * screenShake;
		view.move(ox, oy);
	}
	window->setView(view);

	renderBackground(*window);

	if (state == GameState::INTRO) {
		renderIntro(*window);
	}
	else if (state == GameState::HOW_TO_PLAY) {
		renderHowToPlay(*window);
	}
	else if (state == GameState::BOSS_ARRIVAL) {
		renderBackground(*window);
		renderBossArrival(*window);
	}
	else if (state == GameState::MENU) {
		renderMenu(*window); renderHighScores(*window);
	}
	else if (state == GameState::SHIP_SELECT) {
		renderShipSelect(*window);
	}
	else if (state == GameState::SHOP) {
		renderShop(*window);
	}
	else if (state == GameState::BLACK_MARKET) {
		renderBlackMarket(*window);
	}
	else if (state == GameState::PERK_SELECT) {
		renderPerkSelect(*window);
	}
	else if (state == GameState::DIFFICULTY_SELECT) {
		renderDifficultySelect(*window);
	}
	else if (state == GameState::SETTINGS) {
		renderSettings(*window);
	}
	else if (state == GameState::SKINS) {
		renderSkins(*window);
	}
	else if (state == GameState::PAUSED) {
		if (!isBossStage && !isBoss2Stage) {
			renderEnemies(*window);
			renderHazards(*window);
		}
		else if (isBossStage) { bossEntity.render(*window); bossEntity.renderPhaseAnnouncement(*window, font); }
		else if (isBoss2Stage) { renderBoss2(*window); }
		renderSpawnWarnings(*window); renderPowerUps(*window);
		renderLasers(*window); renderSecondaryWeapons(*window); renderEnemyBullets(*window);
		renderParticles(*window); renderShockwaves(*window); renderDashTrail(*window);
		renderDrones(*window);
		renderPlayer(*window); renderHUD(*window); renderText(*window);
		renderScorePopups(*window);

		sf::RectangleShape pauseWash(sf::Vector2f((float)window->getSize().x, (float)window->getSize().y));
		pauseWash.setFillColor(sf::Color(0, 0, 10, 145));
		window->draw(pauseWash);

		sf::RectangleShape pauseFrame(sf::Vector2f(520.f, 250.f));
		pauseFrame.setOrigin(260.f, 125.f);
		pauseFrame.setPosition((float)window->getSize().x / 2.f, 190.f);
		pauseFrame.setFillColor(sf::Color(8, 18, 38, 180));
		pauseFrame.setOutlineColor(sf::Color(255, 230, 0, 180));
		pauseFrame.setOutlineThickness(3.f);
		window->draw(pauseFrame);
		window->draw(PausedText);
	}
	else if (state == GameState::PLAYING && !EndGame) {
		renderSpawnWarnings(*window);
		if (!isBossStage && !isBoss2Stage) renderEnemies(*window);
		else if (isBossStage) { bossEntity.render(*window); bossEntity.renderPhaseAnnouncement(*window, font); }
		else if (isBoss2Stage) { renderBoss2(*window); }
		renderPowerUps(*window); renderLasers(*window); renderSecondaryWeapons(*window); renderEnemyBullets(*window);
		renderParticles(*window); renderShockwaves(*window);
		renderDashTrail(*window); renderDrones(*window); renderPlayer(*window);
		renderExplosions(*window);

		// FIX: white flash on boss death — computed directly from timer, no new member needed
		{
			float flashT = 0.f;
			if (bossDeathSequence && bossDeathTimer < 10.f) flashT = 1.f - bossDeathTimer / 10.f;
			if (boss2DeathSequence && boss2DeathTimer < 10.f) flashT = 1.f - boss2DeathTimer / 10.f;
			if (flashT > 0.f) {
				sf::RectangleShape flash(sf::Vector2f((float)window->getSize().x, (float)window->getSize().y));
				flash.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)(flashT * 230.f)));
				window->draw(flash);
			}
		}
		renderHUD(*window); renderText(*window); renderScorePopups(*window);
		renderVignette(*window); // D2: vignette was defined but never called

		if (comboCount > 1) window->draw(comboText);

		if (killBannerTimer > 0.f) {
			sf::Color kc = killBannerText.getFillColor();
			kc.a = (sf::Uint8)(killBannerTimer / killBannerTimerMax * 255.f);
			killBannerText.setFillColor(kc); window->draw(killBannerText);
		}
		if (waveBannerTimer > 0.f) {
			float alpha = std::min(waveBannerTimer / 20.f, 1.f) * 255.f;
			sf::Color wc = waveBannerText.getFillColor();
			wc.a = (sf::Uint8)alpha; waveBannerText.setFillColor(wc);
			window->draw(waveBannerText);
		}
	}
	else if (state == GameState::GAME_OVER || EndGame) {
		sf::RectangleShape endWash(sf::Vector2f((float)window->getSize().x, (float)window->getSize().y));
		endWash.setFillColor(sf::Color(40, 0, 18, 185));
		window->draw(endWash);

		sf::RectangleShape panel(sf::Vector2f(760.f, 420.f));
		panel.setOrigin(380.f, 210.f);
		panel.setPosition((float)window->getSize().x / 2.f, (float)window->getSize().y / 2.f - 20.f);
		panel.setFillColor(sf::Color(8, 12, 28, 210));
		panel.setOutlineColor(sf::Color(255, 70, 90, 180));
		panel.setOutlineThickness(3.f);
		window->draw(panel);

		window->draw(GameOverText);
		window->draw(FinalScoreText);
		renderHighScores(*window);
	}

	// D5: draw crosshair on top of everything, every state (system cursor is hidden)
	cursorVert.setPosition(mousePosView);
	cursorHoriz.setPosition(mousePosView);
	window->draw(cursorVert);
	window->draw(cursorHoriz);

	window->setView(window->getDefaultView());

	renderTransitionFade(*window);

	window->display();
}