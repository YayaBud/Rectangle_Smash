#include "game.h"
#include <iostream>
#include <cmath>

// ─────────────────────────────────────────────
//  SHOCKWAVES
//  Shared "something big happened" ring. Bombs, ultimates and boss deaths all
//  use it so the player learns one visual language instead of three.
// ─────────────────────────────────────────────
void game::spawnShockwave(sf::Vector2f pos, float maxRadius, sf::Color color, float life)
{
	Shockwave w;
	w.pos = pos;
	w.radius = 0.f;
	w.maxRadius = maxRadius;
	w.thickness = 22.f;
	w.lifetime = 0.f;
	w.maxLifetime = life;
	w.color = color;
	shockwaves.push_back(w);
}

void game::updateShockwaves()
{
	for (size_t i = 0; i < shockwaves.size(); i++) {
		Shockwave& w = shockwaves[i];
		w.lifetime += 1.f;

		// Ease-out: fast expansion that decelerates. A linear ring reads like a
		// growing circle; this one reads like a blast.
		float t = std::min(1.f, w.lifetime / w.maxLifetime);
		float eased = 1.f - (1.f - t) * (1.f - t) * (1.f - t);
		w.radius = w.maxRadius * eased;
		w.thickness = 4.f + 24.f * (1.f - t);

		if (w.lifetime >= w.maxLifetime) {
			shockwaves.erase(shockwaves.begin() + i);
			i--;
		}
	}
}

// ─────────────────────────────────────────────
//  BOMB
// ─────────────────────────────────────────────
void game::triggerBomb()
{
	if (bombCount <= 0 || bombCooldownTimer > 0.f) {
		// Audible refusal — silence just reads as an unresponsive key.
		if (bombCount <= 0) { hitSound.setPitch(0.6f); hitSound.setVolume(22.f); hitSound.play(); hitSound.setVolume(50.f); }
		return;
	}

	bombCount--;
	bombCooldownTimer = bombCooldownMax;

	sf::Vector2f p = player.getPosition();

	// 1. Clear all enemy projectiles — each one becomes a particle burst so the
	//    screen visibly *empties* rather than the bullets just blinking out.
	for (auto& b : enemyBullets)
		addExplosionParticles(b.shape.getPosition(), sf::Color(255, 120, 255), 3);
	enemyBullets.clear();
	if (bossEntity.isActive())  bossEntity.getProjectiles().clear();
	if (boss2Entity.isActive()) boss2Entity.getProjectiles().clear();

	// 2. Damage everything on screen
	for (auto& enemy : enemies) {
		enemy.hp -= 30;
		enemy.flashTimer = 8;
		addExplosionParticles(enemy.sprite.getPosition(), enemy.baseColor, 6);
	}
	if (isBossStage && bossEntity.isActive())   bossEntity.takeDamage(50);
	if (isBoss2Stage && boss2Entity.isActive()) boss2Entity.takeDamage(50);

	// 3. Feedback: ring, freeze, shake, sound. The old version had screen shake
	//    and nothing else, which is why a screen-clearing ability felt like
	//    pressing a dead key.
	spawnShockwave(p, 900.f, sf::Color(255, 210, 90), 40.f);
	addExplosionParticles(p, sf::Color(255, 220, 120), 40);
	hitStopTimer = 8.f;
	screenShake = 26.f;
	bombSound.setPitch(0.95f + (float)(rand() % 11) * 0.01f);
	bombSound.play();

	debugLog("Bomb detonated, " + std::to_string(bombCount) + " left");
}

void game::updateBombs()
{
	if (bombCooldownTimer > 0.f) bombCooldownTimer -= 1.f;
	// Firing is edge-triggered from pollevent(). Polling isKeyPressed here meant
	// holding Shift re-fired the instant the cooldown expired.
}

// ─────────────────────────────────────────────
//  COMPANION DRONES
// ─────────────────────────────────────────────
void game::grantDrone()
{
	if (drones.size() >= 3) return;

	Drone d;
	d.shape.setRadius(7.f);
	d.shape.setOrigin(7.f, 7.f);
	d.shape.setFillColor(sf::Color(120, 235, 255));
	d.shape.setOutlineColor(sf::Color(20, 60, 90));
	d.shape.setOutlineThickness(2.f);
	d.shape.setPosition(player.getPosition());
	// Spread evenly around the orbit so a second drone is visibly a second drone.
	d.orbitAngle = 6.2831853f * (float)drones.size() / 3.f;
	d.shootTimer = 0;
	d.shootTimerMax = 45;
	drones.push_back(d);

	droneLevel = (int)drones.size();
	powerUpSound.setPitch(1.25f);
	powerUpSound.play();
	powerUpSound.setPitch(1.f);
	debugLog("Drone granted, total " + std::to_string(drones.size()));
}

void game::updateDrones()
{
	for (auto& drone : drones) {
		drone.orbitAngle += 0.03f;
		float targetX = player.getPosition().x + std::cos(drone.orbitAngle) * 80.f;
		float targetY = player.getPosition().y + std::sin(drone.orbitAngle) * 80.f;

		drone.shape.setPosition(
			drone.shape.getPosition().x + (targetX - drone.shape.getPosition().x) * 0.1f,
			drone.shape.getPosition().y + (targetY - drone.shape.getPosition().y) * 0.1f
		);

		if (drone.shootTimer > 0) { drone.shootTimer--; continue; }

		// Nearest enemy in range
		float nearestDist = 999999.f;
		sf::Vector2f targetPos = drone.shape.getPosition();
		bool found = false;

		for (auto& e : enemies) {
			float dx = e.shape.getPosition().x - drone.shape.getPosition().x;
			float dy = e.shape.getPosition().y - drone.shape.getPosition().y;
			float dist = dx * dx + dy * dy;
			if (dist < nearestDist && dist < 90000.f) { // range ~300
				nearestDist = dist;
				targetPos = e.shape.getPosition();
				found = true;
			}
		}
		// Drones also help against bosses, where `enemies` is empty.
		if (!found && isBossStage && bossEntity.isActive()) {
			sf::FloatRect bb = bossEntity.getBounds();
			targetPos = sf::Vector2f(bb.left + bb.width / 2.f, bb.top + bb.height / 2.f);
			found = true;
		}

		if (found) {
			Laser l;
			l.shape.setSize(sf::Vector2f(3.f, 14.f));
			l.shape.setFillColor(sf::Color(120, 235, 255));
			l.shape.setPosition(drone.shape.getPosition());

			float angle = std::atan2(targetPos.y - drone.shape.getPosition().y,
				targetPos.x - drone.shape.getPosition().x);
			l.velocity = sf::Vector2f(std::cos(angle) * 18.f, std::sin(angle) * 18.f);
			l.shape.setRotation(angle * 180.f / 3.14159f + 90.f);
			l.pierceCount = 0;
			lasers.push_back(l);

			drone.shootTimer = drone.shootTimerMax;
		}
	}
}

// ─────────────────────────────────────────────
//  COMBO ANNOUNCER
//  Drives the scale pop on the combo counter. Was a stub, so the combo text
//  just sat there at a fixed size no matter how big the streak got.
// ─────────────────────────────────────────────
void game::updateComboAnnouncer()
{
	if (comboAnnouncerTimer > 0.f) {
		comboAnnouncerTimer -= 1.f;
		// Overshoot then settle
		float t = comboAnnouncerTimer / 22.f;
		comboAnnouncerScale = 1.f + 0.55f * t * t;
	}
	else {
		comboAnnouncerScale += (1.f - comboAnnouncerScale) * 0.2f;
	}
	comboText.setScale(comboAnnouncerScale, comboAnnouncerScale);
}

// ─────────────────────────────────────────────
//  ULTIMATE
//  Charged by grazing, killing and taking hits. Effect depends on ship class.
// ─────────────────────────────────────────────
void game::addUltimateCharge(float amount)
{
	if (ultimateActive) return;
	if (ultimateCharge >= ultimateChargeMax) return;

	ultimateCharge = std::min(ultimateChargeMax, ultimateCharge + amount);

	if (ultimateCharge >= ultimateChargeMax && !ultimateReadyAnnounced) {
		ultimateReadyAnnounced = true;
		ultReadySound.play();
		ultimateFlashTimer = 45.f;
	}
}

void game::triggerUltimate()
{
	if (ultimateActive || ultimateCharge < ultimateChargeMax) {
		if (!ultimateActive) { hitSound.setPitch(0.6f); hitSound.setVolume(22.f); hitSound.play(); hitSound.setVolume(50.f); }
		return;
	}

	ultimateCharge = 0.f;
	ultimateActive = true;
	ultimateReadyAnnounced = false;
	ultimateActiveTimer = ultimateActiveMax;

	sf::Vector2f p = player.getPosition();
	ultFireSound.play();
	hitStopTimer = 10.f;
	screenShake = 24.f;
	ultimateFlashTimer = 30.f;

	// Ship-specific payload. Each one leans on the timer that already exists for
	// that effect rather than inventing parallel state.
	switch (selectedShipClass) {
	case 1: // TANK — BULWARK: ride out the next few seconds untouchable
		invincibilityTimer = std::max(invincibilityTimer, ultimateActiveMax);
		shieldActive = true;
		spawnShockwave(p, 520.f, sf::Color(120, 220, 255), 38.f);
		addExplosionParticles(p, sf::Color(140, 220, 255), 46);
		break;

	case 2: // SNIPER — LANCE: heavy piercing damage, everything on screen
		deathRayTimer = std::max(deathRayTimer, ultimateActiveMax);
		for (auto& e : enemies) {
			e.hp -= 25;
			e.flashTimer = 10;
			addExplosionParticles(e.sprite.getPosition(), sf::Color(255, 90, 90), 8);
		}
		if (isBossStage && bossEntity.isActive())   bossEntity.takeDamage(70);
		if (isBoss2Stage && boss2Entity.isActive()) boss2Entity.takeDamage(70);
		spawnShockwave(p, 1100.f, sf::Color(255, 90, 90), 34.f);
		break;

	default: // BALANCED — OVERDRIVE: fire rate, no heat, cleared screen
		fastFireTimer = std::max(fastFireTimer, ultimateActiveMax);
		heatLevel = 0.f;
		isOverheated = false;
		for (auto& b : enemyBullets)
			addExplosionParticles(b.shape.getPosition(), sf::Color(255, 220, 120), 2);
		enemyBullets.clear();
		spawnShockwave(p, 760.f, sf::Color(255, 220, 120), 36.f);
		addExplosionParticles(p, sf::Color(255, 235, 160), 40);
		break;
	}

	debugLog("Ultimate fired (ship class " + std::to_string(selectedShipClass) + ")");
}

void game::updateUltimate()
{
	if (ultimateFlashTimer > 0.f) ultimateFlashTimer -= 1.f;

	if (ultimateActive) {
		ultimateActiveTimer -= 1.f;

		// Overdrive suppresses heat for its duration — that IS the power.
		if (selectedShipClass == 0) heatLevel = std::max(0.f, heatLevel - 8.f);

		// Trailing sparks so an active ultimate is visible on the ship itself
		if ((int)ultimateActiveTimer % 4 == 0)
			addExplosionParticles(player.getPosition(), sf::Color(255, 230, 150), 2);

		if (ultimateActiveTimer <= 0.f) {
			ultimateActive = false;
			ultimateActiveTimer = 0.f;
		}
	}
}

// ─────────────────────────────────────────────
//  GRAZE
//  Flying close to a bullet without being hit pays out in points and ultimate
//  charge. This is the risk/reward loop the whole scoring system was missing.
// ─────────────────────────────────────────────
void game::updateGraze()
{
	if (grazeFlashTimer > 0.f) grazeFlashTimer -= 1.f;

	// Streak decays if you stop grazing
	if (grazeTimer > 0.f) {
		grazeTimer -= 1.f;
		if (grazeTimer <= 0.f) { grazeCount = 0; grazeMultiplier = 1.f; }
	}

	if (state != GameState::PLAYING || EndGame || playerDeathSequence) return;

	const sf::Vector2f p = player.getPosition();
	const float GRAZE_RADIUS = 62.f;   // outer edge of the reward band
	const float HIT_RADIUS = 22.f;   // inside this you are being hit, not grazing

	int grazedThisFrame = 0;

	// One lambda, three projectile containers — the boss types are separate
	// structs, so this is the cheapest way to avoid writing the test three times.
	auto tryGraze = [&](sf::Vector2f centre, float radius, bool& flag) {
		if (flag) return;
		float dx = centre.x - p.x, dy = centre.y - p.y;
		float d = std::sqrt(dx * dx + dy * dy) - radius;
		if (d < HIT_RADIUS || d > GRAZE_RADIUS) return;
		flag = true;
		grazedThisFrame++;
		};

	for (auto& b : enemyBullets) {
		float r = b.shape.getRadius();
		tryGraze(b.shape.getPosition() + sf::Vector2f(r, r), r, b.grazed);
	}
	if (bossEntity.isActive()) {
		for (auto& b : bossEntity.getProjectiles()) {
			float r = b.shape.getRadius();
			tryGraze(b.shape.getPosition() + sf::Vector2f(r, r), r, b.grazed);
		}
	}
	if (boss2Entity.isActive()) {
		for (auto& b : boss2Entity.getProjectiles()) {
			float r = b.shape.getRadius();
			tryGraze(b.shape.getPosition() + sf::Vector2f(r, r), r, b.grazed);
		}
	}

	if (grazedThisFrame == 0) return;

	grazeCount += grazedThisFrame;
	grazeTimer = 120.f;                                        // 1s to keep the streak
	grazeMultiplier = 1.f + std::min(2.f, grazeCount * 0.02f);  // caps at 3x
	grazeFlashTimer = 10.f;

	points += (unsigned)(grazedThisFrame * 5 * grazeMultiplier);
	addUltimateCharge(grazedThisFrame * 3.f * grazeMultiplier);

	// Pitch climbs with the streak — the audio tells you the multiplier is
	// building without you having to look at the HUD.
	sf::Sound& s = grazeSounds[currentGrazeSoundIndex];
	s.setPitch(1.f + std::min(0.9f, grazeCount * 0.012f));
	s.setVolume(26.f);
	s.play();
	currentGrazeSoundIndex = (currentGrazeSoundIndex + 1) % MAX_GRAZE_SOUNDS;
}
