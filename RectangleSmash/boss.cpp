#include "boss.h"
#include <cmath>

Boss::Boss()
{
    this->bossActive = false;
    this->window = nullptr;
    this->difficulty = 1;
}

Boss::~Boss() {}

void Boss::initBoss(sf::RenderWindow* targetWindow, int diff)
{
    this->window = targetWindow;
    this->difficulty = diff;

    this->bossData.shape.setSize(sf::Vector2f(200.f, 200.f));
    this->bossData.shape.setFillColor(sf::Color::Red);
    this->bossData.shape.setPosition(this->window->getSize().x / 2.f - 100.f, -200.f);
    this->bossData.maxHp = (diff == 0) ? 300 : (diff == 1) ? 500 : 700;
    this->bossData.hp = this->bossData.maxHp;
    this->bossData.attackTimer = 0.f;
    this->bossData.state = 0;
    this->bossData.stateTimer = 0.f;
    this->bossData.moveDir = sf::Vector2f(1.f, 0.f);

    this->bossBeam.setSize(sf::Vector2f(20.f, this->window->getSize().y));
    this->bossBeam.setFillColor(sf::Color(255, 0, 0, 150));
}

void Boss::takeDamage(int dmg)
{
    this->bossData.hp -= dmg;
}

void Boss::update(int& health, sf::ConvexShape& player,
    unsigned& points, bool& isBossStage, bool isInvincible, bool isSlowed,
    float& invincibilityTimer, float& deathRayTimer, float& fastFireTimer,
    float& laserFireTimerMax)
{
    if (!this->bossActive && isBossStage) {
        this->bossActive = true;
        this->bossData.shape.setPosition(this->window->getSize().x / 2.f - 100.f, -200.f);
        this->bossData.hp = this->bossData.maxHp;
        this->bossData.state = 0;
        this->bossData.attackTimer = 0.f;
        this->bossData.stateTimer = 0.f;
        this->bossProjectiles.clear();
        this->bossMinions.clear();
        this->minionBullets.clear();
        this->bossPowerUps.clear();
    }

    if (!bossActive) return;

    float speedMult = isSlowed ? 0.5f : 1.0f;

    // Difficulty-based params
    float attackInterval = (difficulty == 0) ? 240.f : (difficulty == 1) ? 180.f : 120.f;
    float beamWarnTime = (difficulty == 0) ? 90.f : (difficulty == 1) ? 60.f : 40.f;
    float beamActiveTime = (difficulty == 0) ? 30.f : (difficulty == 1) ? 40.f : 60.f;
    float bossSpeed = (difficulty == 0) ? 1.2f : (difficulty == 1) ? 2.f : 3.f;
    float radialSpeed = (difficulty == 0) ? 2.f : (difficulty == 1) ? 3.5f : 5.f;
    int   radialCount = (difficulty == 0) ? 8 : (difficulty == 1) ? 16 : 24;
    float teleportInterval = (difficulty == 0) ? 240.f : (difficulty == 1) ? 180.f : 120.f;
    float minionShootInterval = (difficulty == 0) ? 120.f : (difficulty == 1) ? 90.f : 60.f;
    float minionBulletSpd = (difficulty == 0) ? 2.5f : (difficulty == 1) ? 4.f : 6.f;
    int   minionHp = (difficulty == 0) ? 2 : (difficulty == 1) ? 3 : 4;

    // Intro slide down
    if (this->bossData.shape.getPosition().y < 50.f) {
        this->bossData.shape.move(0.f, 1.5f * speedMult);
        return;
    }

    // Horizontal movement
    if (this->bossData.state != 2) {
        this->bossData.shape.move(this->bossData.moveDir.x * bossSpeed * speedMult, 0.f);
        if (this->bossData.shape.getPosition().x < 50.f)
            this->bossData.moveDir.x = 1.f;
        if (this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x > this->window->getSize().x - 50.f)
            this->bossData.moveDir.x = -1.f;
    }

    this->bossData.attackTimer += 1.f * speedMult;
    this->bossData.stateTimer += 1.f * speedMult;

    // ------- STATE 0: Attack -------
    if (this->bossData.state == 0) {
        if (this->bossData.attackTimer > attackInterval) {
            this->bossData.attackTimer = 0.f;

            // Spawn minions up to cap
            int toSpawn = MAX_MINIONS - (int)this->bossMinions.size();
            for (int i = 0; i < toSpawn; i++) {
                BossMinion minion;
                minion.shape.setRadius(15.f);
                minion.shape.setFillColor(sf::Color::Magenta);
                float cx = this->bossData.shape.getPosition().x + (i == 0 ? -40.f : this->bossData.shape.getSize().x + 10.f);
                float cy = this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y / 2.f;
                minion.shape.setPosition(cx, cy);
                minion.teleportTimer = 0.f;
                minion.teleportTimerMax = teleportInterval;
                minion.shootTimer = (float)(rand() % (int)minionShootInterval);
                minion.shootTimerMax = minionShootInterval;
                minion.hp = minionHp;
                this->bossMinions.push_back(minion);
            }

            // Radial burst
            float cx = this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x / 2.f;
            float cy = this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y / 2.f;
            for (int i = 0; i < radialCount; i++) {
                BossProj ball;
                ball.shape.setRadius(8.f);
                ball.shape.setFillColor(sf::Color(255, 140, 0));
                ball.shape.setPosition(cx, cy);
                float angle = (i / (float)radialCount) * 2.f * 3.14159f;
                ball.velocity = sf::Vector2f(std::cos(angle) * radialSpeed, std::sin(angle) * radialSpeed);
                ball.hp = 1;
                this->bossProjectiles.push_back(ball);
            }

            if (rand() % 3 == 0) {
                this->bossData.state = 1;
                this->bossData.stateTimer = 0.f;
            }
        }
    }
    // ------- STATE 1: Beam warning -------
    else if (this->bossData.state == 1) {
        float cx = this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x / 2.f - 10.f;
        this->bossBeam.setPosition(cx, this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y);
        this->bossBeam.setFillColor(sf::Color(255, 100, 100, 100));
        if (this->bossData.stateTimer > beamWarnTime) {
            this->bossData.state = 2;
            this->bossData.stateTimer = 0.f;
        }
    }
    // ------- STATE 2: Beam active -------
    else if (this->bossData.state == 2) {
        this->bossBeam.setFillColor(sf::Color(255, 0, 0, 255));
        if (!isInvincible && player.getGlobalBounds().intersects(this->bossBeam.getGlobalBounds())) {
            health -= 1;
        }
        if (this->bossData.stateTimer > beamActiveTime) {
            this->bossData.state = 0;
            this->bossData.stateTimer = 0.f;
            this->bossData.attackTimer = 0.f;
        }
    }

    // ------- Radial projectiles (no bounce, just die at edge) -------
    for (size_t i = 0; i < this->bossProjectiles.size(); i++) {
        this->bossProjectiles[i].shape.move(
            this->bossProjectiles[i].velocity.x * speedMult,
            this->bossProjectiles[i].velocity.y * speedMult);

        float px = this->bossProjectiles[i].shape.getPosition().x;
        float py = this->bossProjectiles[i].shape.getPosition().y;
        if (py > this->window->getSize().y + 20.f || py < -20.f ||
            px < -20.f || px > this->window->getSize().x + 20.f) {
            this->bossProjectiles.erase(this->bossProjectiles.begin() + i);
            i--; continue;
        }
        if (this->bossProjectiles[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            if (!isInvincible) health -= 5;
            this->bossProjectiles.erase(this->bossProjectiles.begin() + i);
            i--;
        }
    }

    // ------- Minions -------
    sf::Vector2f playerPos = player.getPosition();
    float safeRadius = 150.f;

    for (size_t i = 0; i < this->bossMinions.size(); i++) {
        this->bossMinions[i].teleportTimer += 1.f * speedMult;
        this->bossMinions[i].shootTimer += 1.f * speedMult;

        if (this->bossMinions[i].teleportTimer >= this->bossMinions[i].teleportTimerMax) {
            this->bossMinions[i].teleportTimer = 0.f;
            float tx, ty;
            int attempts = 0;
            do {
                tx = 30.f + rand() % (int)(this->window->getSize().x - 60.f);
                ty = 30.f + rand() % (int)(this->window->getSize().y / 2);
                attempts++;
            } while (attempts < 20 &&
                std::sqrt((tx - playerPos.x) * (tx - playerPos.x) +
                    (ty - playerPos.y) * (ty - playerPos.y)) < safeRadius);
            this->bossMinions[i].shape.setPosition(tx, ty);
        }

        if (this->bossMinions[i].shootTimer >= this->bossMinions[i].shootTimerMax) {
            this->bossMinions[i].shootTimer = 0.f;
            sf::Vector2f mPos = this->bossMinions[i].shape.getPosition();
            sf::Vector2f dir = playerPos - mPos;
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0) dir /= len;
            BossProj bullet;
            bullet.shape.setRadius(6.f);
            bullet.shape.setFillColor(sf::Color(255, 0, 255));
            bullet.shape.setPosition(mPos);
            bullet.velocity = dir * minionBulletSpd;
            bullet.hp = 1;
            this->minionBullets.push_back(bullet);
        }

        // Player body collision
        if (this->bossMinions[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            if (!isInvincible) health -= 10;
            this->bossMinions.erase(this->bossMinions.begin() + i);
            i--;
        }
    }

    // ------- Minion bullets -------
    for (size_t i = 0; i < this->minionBullets.size(); i++) {
        this->minionBullets[i].shape.move(
            this->minionBullets[i].velocity.x * speedMult,
            this->minionBullets[i].velocity.y * speedMult);
        float px = this->minionBullets[i].shape.getPosition().x;
        float py = this->minionBullets[i].shape.getPosition().y;
        if (py > this->window->getSize().y + 20.f || py < -20.f ||
            px < -20.f || px > this->window->getSize().x + 20.f) {
            this->minionBullets.erase(this->minionBullets.begin() + i);
            i--; continue;
        }
        if (this->minionBullets[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            if (!isInvincible) health -= 8;
            this->minionBullets.erase(this->minionBullets.begin() + i);
            i--;
        }
    }

    // ------- Boss power-up drops (every ~300 frames) -------
    static float puDropTimer = 0.f;
    puDropTimer += 1.f;
    if (puDropTimer > 300.f) {
        puDropTimer = 0.f;
        BossPowerUp p;
        p.type = rand() % 4;
        p.shape.setRadius(12.f);
        float bx = this->bossData.shape.getPosition().x + rand() % (int)this->bossData.shape.getSize().x;
        p.shape.setPosition(bx, this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y);
        switch (p.type) {
        case 0: p.shape.setFillColor(sf::Color::Green);   break; // health
        case 1: p.shape.setFillColor(sf::Color::Yellow);  break; // invincibility
        case 2: p.shape.setFillColor(sf::Color::Red);     break; // death ray
        case 3: p.shape.setFillColor(sf::Color::Cyan);    break; // fast fire
        }
        this->bossPowerUps.push_back(p);
    }

    // Move + collect boss power-ups
    for (size_t i = 0; i < this->bossPowerUps.size(); i++) {
        this->bossPowerUps[i].shape.move(0.f, 1.5f);
        if (this->bossPowerUps[i].shape.getPosition().y > this->window->getSize().y) {
            this->bossPowerUps.erase(this->bossPowerUps.begin() + i);
            i--; continue;
        }
        if (this->bossPowerUps[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            switch (this->bossPowerUps[i].type) {
            case 0: health = std::min(health + 10, 50); break;
            case 1: invincibilityTimer = 600.f; break;
            case 2: deathRayTimer = 600.f; break;
            case 3: fastFireTimer = 600.f; laserFireTimerMax = 5.f; break;
            }
            this->bossPowerUps.erase(this->bossPowerUps.begin() + i);
            i--;
        }
    }

    // Death
    if (this->bossData.hp <= 0) {
        points += 1000;
        this->bossActive = false;
        isBossStage = false;
        this->bossMinions.clear();
        this->bossProjectiles.clear();
        this->minionBullets.clear();
        this->bossPowerUps.clear();
        puDropTimer = 0.f;
    }
}

void Boss::render(sf::RenderTarget& target)
{
    if (!this->bossActive) return;

    target.draw(this->bossData.shape);
    for (auto& p : this->bossProjectiles) target.draw(p.shape);
    for (auto& m : this->bossMinions)     target.draw(m.shape);
    for (auto& b : this->minionBullets)   target.draw(b.shape);
    for (auto& p : this->bossPowerUps)    target.draw(p.shape);

    if (this->bossData.state == 1 || this->bossData.state == 2)
        target.draw(this->bossBeam);

    sf::RectangleShape hpBarBg(sf::Vector2f(this->window->getSize().x - 100.f, 20.f));
    hpBarBg.setPosition(50.f, 20.f);
    hpBarBg.setFillColor(sf::Color(100, 100, 100));

    sf::RectangleShape hpBar(sf::Vector2f(
        (this->bossData.hp / (float)this->bossData.maxHp) * (this->window->getSize().x - 100.f), 20.f));
    hpBar.setPosition(50.f, 20.f);
    hpBar.setFillColor(sf::Color::Red);

    target.draw(hpBarBg);
    target.draw(hpBar);
}