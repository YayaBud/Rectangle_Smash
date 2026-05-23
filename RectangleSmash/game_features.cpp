#include "game.h"
#include <iostream>
#include <cmath>

void game::triggerBomb()
{
	if (bombCount > 0 && bombCooldownTimer <= 0.f) {
		bombCount--;
		bombCooldownTimer = bombCooldownMax; // 10 seconds at 60 fps

		// 1. Clear all enemy projectiles
		enemyBullets.clear();
		if (bossEntity.isActive()) bossEntity.getProjectiles().clear();
		if (boss2Entity.isActive()) boss2Entity.getProjectiles().clear();

		// 2. Damage all enemies on screen
		for (auto& enemy : enemies) {
			enemy.hp -= 30; // heavy damage
		}

		// 3. Boss damage
		if (isBossStage && bossEntity.isActive()) {
			bossEntity.takeDamage(50);
		}
		if (isBoss2Stage && boss2Entity.isActive()) {
			boss2Entity.takeDamage(50);
		}

		// 4. Visuals: white flash
		screenShake = 30.f;

		debugLog("Bomb triggered!");
	}
}

void game::updateBombs()
{
	if (bombCooldownTimer > 0.f) {
		bombCooldownTimer -= 1.f;
	}

	// Trigger bomb with Shift key
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
		triggerBomb();
	}
}

void game::updateDrones()
{
	for (auto& drone : drones) {
		// Orbit player
		drone.orbitAngle += 0.05f;
		float targetX = player.getPosition().x + std::cos(drone.orbitAngle) * 80.f;
		float targetY = player.getPosition().y + std::sin(drone.orbitAngle) * 80.f;

		drone.shape.setPosition(
			drone.shape.getPosition().x + (targetX - drone.shape.getPosition().x) * 0.1f,
			drone.shape.getPosition().y + (targetY - drone.shape.getPosition().y) * 0.1f
		);

		if (drone.shootTimer > 0) drone.shootTimer--;
		else {
			// Find nearest enemy to shoot
			float nearestDist = 999999.f;
			sf::Vector2f targetPos = drone.shape.getPosition();
			bool found = false;

			for (auto& e : enemies) {
				float dx = e.shape.getPosition().x - drone.shape.getPosition().x;
				float dy = e.shape.getPosition().y - drone.shape.getPosition().y;
				float dist = dx * dx + dy * dy;
				if (dist < nearestDist && dist < 40000.f) { // range ~200
					nearestDist = dist;
					targetPos = e.shape.getPosition();
					found = true;
				}
			}

			if (found) {
				// Fire laser from drone
				Laser l;
				l.shape.setSize(sf::Vector2f(8.f, 2.f));
				l.shape.setFillColor(sf::Color::Cyan);
				l.shape.setPosition(drone.shape.getPosition());

				float angle = std::atan2(targetPos.y - drone.shape.getPosition().y, targetPos.x - drone.shape.getPosition().x);
				l.velocity = sf::Vector2f(std::cos(angle) * 15.f, std::sin(angle) * 15.f);
				l.shape.setRotation(angle * 180.f / 3.14159f);
				l.pierceCount = 0;
				lasers.push_back(l);

				drone.shootTimer = drone.shootTimerMax; // drone cooldown
			}
		}
	}
}

void game::updateComboAnnouncer()
{
	// Implementation placeholder
}

void game::updateUltimate()
{
	// Implementation placeholder
}

void game::updateGraze()
{
	// Implementation placeholder
}