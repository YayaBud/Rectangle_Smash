#include "boss.h"
#include <cmath>
#include <algorithm>

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

    // Boss sprite
    textureLoaded = this->bossTexture.loadFromFile("assests/enemy/boss1.png");
    this->bossTexture.setSmooth(true);
    this->bossSprite.setTexture(this->bossTexture, true);
    sf::Vector2u ts = this->bossTexture.getSize();
    if (ts.x > 0 && ts.y > 0) {
        this->bossSprite.setOrigin((float)ts.x / 2.f, (float)ts.y / 2.f);
        float sc = 200.f / (float)std::max(ts.x, ts.y);
        this->bossSprite.setScale(sc, sc);
    }

    // Minion sprite texture
    minionTexLoaded = this->minionTexture.loadFromFile("assests/ship1.png");
    this->minionTexture.setSmooth(false);

    // Missile texture
    missileTexLoaded = this->missileTex.loadFromFile("assests/enemy/missile00.png");
    this->missileTex.setSmooth(false);

    // Phase / respawn state
    phase = 1;
    phase2Triggered = false;
    phase3Triggered = false;
    minionRespawnTimer = 0.f;
    minionRespawnInterval = 480.f;
}

// Helper: build one minion with sprite, position, and timers
static void buildMinion(BossMinion& m, float cx, float cy,
    float teleportInterval, float minionShootInterval, int minionHp,
    sf::Texture& tex, bool texLoaded)
{
    m.shape.setRadius(8.f);
    m.shape.setOrigin(8.f, 8.f);
    m.shape.setFillColor(sf::Color(255, 80, 200, 180)); // visible magenta fallback
    m.shape.setPosition(cx, cy);

    if (texLoaded) {
        sf::Vector2u mts = tex.getSize();
        if (mts.x > 0 && mts.y > 0) {
            m.sprite.setTexture(tex);
            m.sprite.setOrigin(mts.x / 2.f, mts.y / 2.f);
            float msc = 28.f / (float)std::max(mts.x, mts.y);
            m.sprite.setScale(msc, -msc);   // Y-flip so ship faces down
            m.sprite.setColor(sf::Color(255, 80, 200));
            m.sprite.setPosition(cx, cy);
        }
    }

    m.teleportTimer = 0.f;
    m.teleportTimerMax = teleportInterval;
    m.shootTimer = 0.f;
    m.shootTimerMax = minionShootInterval;
    m.hp = minionHp;
}

void Boss::takeDamage(int dmg)
{
    this->bossData.hp -= dmg;
}

void Boss::update(int& health, sf::ConvexShape& player,
    unsigned& points, bool& isBossStage, bool isInvincible, bool isSlowed,
    float& invincibilityTimer, float& deathRayTimer, float& fastFireTimer,
    float& laserFireTimerMax, float& screenShakeOut)
{
    if (!this->bossActive && isBossStage) {
        this->bossActive = true;
        this->bossIsDying = false;
        this->bossData.shape.setPosition(this->window->getSize().x / 2.f - 100.f, -200.f);
        this->bossData.hp = this->bossData.maxHp;
        this->bossData.state = 0;
        this->bossData.attackTimer = 0.f;
        this->bossData.stateTimer = 0.f;
        this->bossProjectiles.clear();
        this->bossMinions.clear();
        this->minionBullets.clear();
        this->bossPowerUps.clear();
        phase = 1;
        phase2Triggered = false;
        phase3Triggered = false;
        minionRespawnTimer = 0.f;
        minionRespawnInterval = 480.f;
    }

    if (!bossActive) return;
    if (bossIsDying) return;

    float speedMult = isSlowed ? 0.5f : 1.0f;

    if (phaseFlashTimer > 0.f) phaseFlashTimer -= 1.f * speedMult;
    if (phaseAnnounceTimer > 0.f) phaseAnnounceTimer -= 1.f * speedMult;

    // ── Phase thresholds (HP-based, difficulty-independent) ─────────────────
    float hpRatio = (float)this->bossData.hp / (float)this->bossData.maxHp;

    if (!phase2Triggered && hpRatio <= 0.5f) {
        phase2Triggered = true;
        phase = 2;
        screenShakeOut = 25.f;
        phaseFlashTimer = 60.f;
        phaseAnnounceTimer = 200.f;
        // Clear beam state so it can re-trigger immediately
        this->bossData.state = 0;
        this->bossData.stateTimer = 0.f;
        this->bossData.attackTimer = 0.f;
        minionRespawnInterval = 300.f; // minions respawn faster in phase 2
        minionRespawnTimer = minionRespawnInterval; // trigger a spawn right away
    }

    if (!phase3Triggered && hpRatio <= 0.25f) {
        phase3Triggered = true;
        phase = 3;
        screenShakeOut = 35.f;
        phaseFlashTimer = 60.f;
        phaseAnnounceTimer = 200.f;
        this->bossData.state = 0;
        this->bossData.stateTimer = 0.f;
        this->bossData.attackTimer = 0.f;
        minionRespawnInterval = 180.f; // even faster in phase 3
        minionRespawnTimer = minionRespawnInterval;
    }

    // ── Per-phase params ─────────────────────────────────────────────────────
    // Attack interval: phase 1 is calm, phase 2 is medium, phase 3 is frantic
    // Easy/Normal/Hard scale the base but phases drive the escalation
    float phaseAttackMult = (phase == 1) ? 1.0f : (phase == 2) ? 0.55f : 0.30f;
    float attackInterval = ((difficulty == 0) ? 240.f : (difficulty == 1) ? 200.f : 150.f) * phaseAttackMult;

    float beamWarnTime = (difficulty == 0) ? 80.f : (difficulty == 1) ? 55.f : 35.f;
    float beamActiveTime = (difficulty == 0) ? 30.f : (difficulty == 1) ? 40.f : 55.f;

    // Phase 3: beam triggers more often
    if (phase == 3) { beamWarnTime *= 0.7f; beamActiveTime *= 1.3f; }

    float bossSpeed = ((difficulty == 0) ? 1.2f : (difficulty == 1) ? 2.f : 3.f)
        * (phase == 1 ? 1.0f : phase == 2 ? 1.5f : 2.2f);

    float radialSpeed = (difficulty == 0) ? 2.f : (difficulty == 1) ? 3.5f : 5.f;
    if (phase == 2) radialSpeed *= 1.3f;
    if (phase == 3) radialSpeed *= 1.7f;

    int radialCount = (difficulty == 0) ? 8 : (difficulty == 1) ? 16 : 24;
    if (phase == 2) radialCount = (int)(radialCount * 1.5f);
    if (phase == 3) radialCount = radialCount * 2;

    float teleportInterval = (difficulty == 0) ? 300.f : (difficulty == 1) ? 220.f : 140.f;
    float minionShootInterval = (difficulty == 0) ? 120.f : (difficulty == 1) ? 90.f : 60.f;
    float minionBulletSpd = (difficulty == 0) ? 2.5f : (difficulty == 1) ? 4.f : 6.f;
    int   minionHp = (difficulty == 0) ? 2 : (difficulty == 1) ? 3 : 4;

    // Phase 3: minions shoot faster and move faster
    if (phase >= 3) { minionShootInterval *= 0.6f; minionBulletSpd *= 1.4f; }

    // ── Slide-in ─────────────────────────────────────────────────────────────
    if (this->bossData.shape.getPosition().y < 50.f) {
        this->bossData.shape.move(0.f, 1.5f * speedMult);
        return;
    }

    // ── Horizontal movement ──────────────────────────────────────────────────
    if (this->bossData.state != 2) {
        this->bossData.shape.move(this->bossData.moveDir.x * bossSpeed * speedMult, 0.f);
        if (this->bossData.shape.getPosition().x < 50.f)
            this->bossData.moveDir.x = 1.f;
        if (this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x > this->window->getSize().x - 50.f)
            this->bossData.moveDir.x = -1.f;
    }

    this->bossData.attackTimer += 1.f * speedMult;
    this->bossData.stateTimer += 1.f * speedMult;
    minionRespawnTimer += 1.f * speedMult;

    // ── STATE 0: Attack ───────────────────────────────────────────────────────
    if (this->bossData.state == 0) {
        if (this->bossData.attackTimer > attackInterval) {
            this->bossData.attackTimer = 0.f;

            // Radial burst (always)
            float cx = this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x / 2.f;
            float cy = this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y / 2.f;
            for (int i = 0; i < radialCount; i++) {
                BossProj ball;
                ball.shape.setRadius(8.f);
                sf::Color projCol = (phase == 1) ? sf::Color(255, 140, 0)
                    : (phase == 2) ? sf::Color(255, 60, 0)
                    : sf::Color(255, 20, 20);
                ball.shape.setFillColor(projCol);
                ball.shape.setPosition(cx, cy);
                float angle = (i / (float)radialCount) * 2.f * 3.14159f;
                ball.velocity = sf::Vector2f(std::cos(angle) * radialSpeed, std::sin(angle) * radialSpeed);
                ball.hp = 1;
                
                if (missileTexLoaded) {
                    ball.isMissile = true;
                    ball.sprite.setTexture(this->missileTex);
                    sf::Vector2u msz = this->missileTex.getSize();
                    ball.sprite.setOrigin(msz.x / 2.f, msz.y / 2.f);
                    float sc = 20.f / (float)std::max(msz.x, msz.y);
                    ball.sprite.setScale(sc, sc);
                    ball.sprite.setRotation(angle * 180.f / 3.14159f + 90.f);
                    ball.sprite.setColor(projCol);
                    ball.shape.setFillColor(sf::Color::Transparent);
                }

                this->bossProjectiles.push_back(ball);
            }

            // Phase 3: also fire aimed triple-shot at player
            if (phase == 3) {
                sf::Vector2f dir = player.getPosition() - sf::Vector2f(cx, cy);
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (len > 0.f) dir /= len;
                for (int s = -1; s <= 1; s++) {
                    float a = std::atan2(dir.y, dir.x) + s * 0.18f;
                    BossProj aimed;
                    aimed.shape.setRadius(7.f);
                    aimed.shape.setFillColor(sf::Color(200, 0, 255));
                    aimed.shape.setPosition(cx, cy);
                    aimed.velocity = sf::Vector2f(std::cos(a), std::sin(a)) * radialSpeed * 1.1f;
                    aimed.hp = 1;
                    
                    if (missileTexLoaded) {
                        aimed.isMissile = true;
                        aimed.sprite.setTexture(this->missileTex);
                        sf::Vector2u msz = this->missileTex.getSize();
                        aimed.sprite.setOrigin(msz.x / 2.f, msz.y / 2.f);
                        float sc = 20.f / (float)std::max(msz.x, msz.y);
                        aimed.sprite.setScale(sc, sc);
                        aimed.sprite.setRotation(a * 180.f / 3.14159f + 90.f);
                        aimed.sprite.setColor(sf::Color(200, 0, 255));
                        aimed.shape.setFillColor(sf::Color::Transparent);
                    }

                    this->bossProjectiles.push_back(aimed);
                }
            }

            // Transition to beam warning (more likely in higher phases)
            int beamChance = (phase == 1) ? 3 : (phase == 2) ? 2 : 1;
            if (rand() % beamChance == 0) {
                this->bossData.state = 1;
                this->bossData.stateTimer = 0.f;
            }
        }
    }
    // ── STATE 1: Beam warning ─────────────────────────────────────────────────
    else if (this->bossData.state == 1) {
        float cx = this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x / 2.f - 10.f;
        this->bossBeam.setPosition(cx, this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y);
        this->bossBeam.setFillColor(sf::Color(255, 100, 100, 100));
        if (this->bossData.stateTimer > beamWarnTime) {
            this->bossData.state = 2;
            this->bossData.stateTimer = 0.f;
        }
    }
    // ── STATE 2: Beam active ──────────────────────────────────────────────────
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

    // ── Minion respawn (separate timer, not tied to attack cycle) ─────────────
    if (minionRespawnTimer >= minionRespawnInterval) {
        minionRespawnTimer = 0.f;
        int cap = (phase >= 2) ? 3 : MAX_MINIONS; // phase 2+ allows 3 minions
        int toSpawn = cap - (int)this->bossMinions.size();
        for (int i = 0; i < toSpawn; i++) {
            BossMinion minion;
            float cx = this->bossData.shape.getPosition().x
                + (i == 0 ? -40.f : this->bossData.shape.getSize().x + 10.f + (i > 1 ? 50.f : 0.f));
            float cy = this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y / 2.f;
            buildMinion(minion, cx, cy, teleportInterval, minionShootInterval, minionHp,
                minionTexture, minionTexLoaded);
            this->bossMinions.push_back(minion);
        }
    }

    // ── Radial projectiles ────────────────────────────────────────────────────
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

    // ── Minions ───────────────────────────────────────────────────────────────
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
            this->bossMinions[i].sprite.setPosition(tx, ty);
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

        // Keep sprite in sync with shape
        this->bossMinions[i].sprite.setPosition(this->bossMinions[i].shape.getPosition());

        if (this->bossMinions[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            if (!isInvincible) health -= 10;
            this->bossMinions.erase(this->bossMinions.begin() + i);
            i--;
        }
    }

    // ── Minion bullets ────────────────────────────────────────────────────────
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

    // ── Power-up drops (every ~300 frames) ───────────────────────────────────
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
        case 0: p.shape.setFillColor(sf::Color::Green);   break;
        case 1: p.shape.setFillColor(sf::Color::Yellow);  break;
        case 2: p.shape.setFillColor(sf::Color::Red);     break;
        case 3: p.shape.setFillColor(sf::Color::Cyan);    break;
        }
        this->bossPowerUps.push_back(p);
    }

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

    // ── Death ─────────────────────────────────────────────────────────────────
    if (this->bossData.hp <= 0) {
        points += 1000;
        if (!this->bossIsDying) {
            this->bossIsDying = true;
            isBossStage = false;
            this->bossData.state = 0; // Turn off lasers/attacks
            this->bossMinions.clear();
            this->bossProjectiles.clear();
            this->minionBullets.clear();
            this->bossPowerUps.clear();
            puDropTimer = 0.f;
        }
    }
}

void Boss::render(sf::RenderTarget& target)
{
    if (!this->bossActive) return;

    if (textureLoaded) {
        sf::Vector2f center(
            this->bossData.shape.getPosition().x + this->bossData.shape.getSize().x / 2.f,
            this->bossData.shape.getPosition().y + this->bossData.shape.getSize().y / 2.f);
        this->bossSprite.setPosition(center);
        
        if (phaseFlashTimer > 0.f) {
            this->bossSprite.setColor((int)phaseFlashTimer % 6 < 3 ? sf::Color::White : sf::Color(255, 80, 0));
        }
        else if (phase == 3)
            this->bossSprite.setColor(sf::Color(255, 80, 80));
        else if (phase == 2)
            this->bossSprite.setColor(sf::Color(255, 200, 80));
        else
            this->bossSprite.setColor(sf::Color::White);
        
        target.draw(this->bossSprite);
    }
    else {
        target.draw(this->bossData.shape);
    }

    for (auto& p : this->bossProjectiles) {
        if (p.isMissile) {
            p.sprite.setPosition(p.shape.getPosition().x + p.shape.getRadius(), p.shape.getPosition().y + p.shape.getRadius());
            target.draw(p.sprite);
        } else {
            target.draw(p.shape);
        }
    }

    // Minions: draw shape (fallback) then sprite on top
    for (auto& m : this->bossMinions) {
        target.draw(m.shape);
        if (m.sprite.getTexture() != nullptr)
            target.draw(m.sprite);
    }

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
    // HP bar colour matches phase
    hpBar.setFillColor(phase == 3 ? sf::Color(255, 30, 30) :
        phase == 2 ? sf::Color(255, 160, 0) :
        sf::Color::Red);

    target.draw(hpBarBg);
    target.draw(hpBar);
}