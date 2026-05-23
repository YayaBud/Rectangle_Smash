// boss2.cpp  �  Boss 2 "Surt" : 6 evolving stages using surt1-6.png
#include "boss2.h"
#include "game.h"
#include <algorithm>
#include <iostream>
#include <sstream>

// per-stage max HP
const int Boss2::stageMaxHp[6] = { 200, 300, 400, 500, 600, 800 };

// per-stage speed
static float stageSpeed[6] = { 1.2f, 1.8f, 2.2f, 3.0f, 3.5f, 4.5f };

Boss2::Boss2()
    : window(nullptr), difficulty(0), active(false), stageTransitioned(false),
    hp(0), maxHp(0), stage(1), revived(false),
    stageFlashTimer(0.f), stageAnnounceTimer(0.f),
    speed(2.0f), attackTimer(0.f), beamTimer(0.f), radialTimer(0.f), homingTimer(0.f),
    beamDamageTimer(0.f), beamAlpha(0.f), beamActive(false), beamActiveTimer(0.f),
    puDropTimer(0.f), rageMode(false)
{
    // ...
}

void Boss2::initBoss2(sf::RenderWindow* win, int diff)
{
    window = win;
    difficulty = diff;
    active = false;
    stageTransitioned = false;
    stage = 1;
    dying = false;

    stage = 1;
    revived = false;
    rageMode = false;
    stageFlashTimer = 0.f;
    stageAnnounceTimer = 0.f;
    attackTimer = beamTimer = radialTimer = homingTimer = beamDamageTimer = 0.f;
    beamAlpha = 0.f;
    beamActive = false;
    beamActiveTimer = 0.f;
    puDropTimer = 0.f;
    projectiles.clear();
    minions.clear();
    powerUps.clear();
    moveDir = sf::Vector2f(1.f, 0.f);

    // Collision box
    body.setSize(sf::Vector2f(200.f, 200.f));
    body.setFillColor(sf::Color::Transparent);
    body.setOutlineColor(sf::Color::Transparent);
    body.setPosition(win->getSize().x / 2.f - 100.f, -220.f);

    // Load all 6 stage textures
    const char* paths[6] = {
        "assests/enemy/boss2/surt1.png",
        "assests/enemy/boss2/surt2.png",
        "assests/enemy/boss2/surt3.png",
        "assests/enemy/boss2/surt4.png",
        "assests/enemy/boss2/surt5.png",
        "assests/enemy/boss2/surt6.png",
    };
    for (int i = 0; i < 6; i++) {
        bool ok = stageTex[i].loadFromFile(paths[i]);
        stageTex[i].setSmooth(false);
        debugLog(std::string(ok ? "Boss2 loaded " : "Boss2 FAILED ") + paths[i]);
    }

    // Minion texture
    bool mok = minionTex.loadFromFile("assests/ship1.png");
    minionTex.setSmooth(false);
    debugLog(std::string(mok ? "Boss2 minion tex loaded" : "Boss2 minion tex FAILED"));

    // Missile texture
    bool misOk = missileTex.loadFromFile("assests/enemy/missile00.png");
    missileTex.setSmooth(false);
    debugLog(std::string(misOk ? "Boss2 missile tex loaded" : "Boss2 missile tex FAILED"));

    // Beam shape
    beam.setSize(sf::Vector2f(260.f, (float)win->getSize().y));
    beam.setFillColor(sf::Color(255, 60, 0, 0));

    loadStage(1);

    debugLog("Boss2 initialized | difficulty=" + std::to_string(diff));
}

void Boss2::loadStage(int s)
{
    stage = s;
    int idx = s - 1;
    maxHp = stageMaxHp[idx];
    hp = maxHp;
    speed = stageSpeed[idx] * (rageMode ? 1.6f : 1.f);

    sf::Vector2u ts = stageTex[idx].getSize();
    bodySprite.setTexture(stageTex[idx], true);
    bodySprite.setOrigin(ts.x / 2.f, ts.y / 2.f);
    bodySprite.setScale(200.f / ts.x, 200.f / ts.y);
    bodySprite.setColor(sf::Color::White);

    stageFlashTimer = 40.f;
    stageAnnounceTimer = 200.f;

    // AGRO FIX: pre-charge timers so the first attack fires ~0.5s after slide-in,
    // not after the full interval.  Values mirror the diffMult=1.2 (Normal) worst case.
    float diffMult = (difficulty == 0) ? 1.8f : (difficulty == 1) ? 1.2f : 0.6f;
    float rage = rageMode ? 2.0f : 1.f;
    float radialInterval = std::max(80.f, 160.f / (s * rage)) * diffMult;
    float beamInterval = std::max(160.f, 400.f / (s * rage)) * diffMult;
    float homingInterval = std::max(100.f, 240.f / (s * rage)) * diffMult;
    radialTimer = radialInterval * 0.85f;  // first radial burst ~15% of interval after spawn
    beamTimer = beamInterval * 0.70f;
    homingTimer = homingInterval * 0.70f;
    attackTimer = (200.f / rage) * 0.80f;  // minion spawn timer
    beamDamageTimer = 0.f;
    beamActive = false;
    beamActiveTimer = 0.f;
    beamAlpha = 0.f;

    debugLog("Boss2 stage " + std::to_string(s) + " loaded | hp=" + std::to_string(maxHp));
}

void Boss2::takeDamage(int dmg)
{
    hp -= dmg;
    if (hp < 0) hp = 0;
}

// ---- helpers ----

void Boss2::fireRadial(int count, float spd, sf::Color color)
{
    float cx = body.getPosition().x + 100.f;
    float cy = body.getPosition().y + 100.f;
    for (int i = 0; i < count; i++) {
        float angle = (float)i / count * 2.f * 3.14159f;
        Boss2Proj p;
        p.shape.setRadius(7.f);
        p.shape.setFillColor(sf::Color::Transparent); // Hide circle
        p.shape.setPosition(cx, cy);
        p.velocity = sf::Vector2f(std::cos(angle) * spd, std::sin(angle) * spd);
        p.homing = false;

        p.isMissile = true;
        p.sprite.setTexture(missileTex);
        sf::Vector2u msz = missileTex.getSize();
        p.sprite.setOrigin(msz.x / 2.f, msz.y / 2.f);
        float sc = 20.f / (float)std::max(msz.x, msz.y);
        p.sprite.setScale(sc, sc);
        p.sprite.setRotation(angle * 180.f / 3.14159f + 90.f);
        p.sprite.setColor(color);

        projectiles.push_back(p);
    }
}

void Boss2::spawnMinions(int n)
{
    sf::Vector2u mts = minionTex.getSize();
    bool texOk = (mts.x > 0 && mts.y > 0);
    for (int i = 0; i < n; i++) {
        Boss2Minion m;
        m.shape.setRadius(8.f);   // RING FIX: was 15 — matched a 30x30 invisible hitbox much larger than the ~14px sprite
        // SKIN FIX: keep a dim visible fill as fallback if texture is missing
        m.shape.setFillColor(sf::Color(255, 100, 50, 160));
        m.shape.setOrigin(8.f, 8.f); // center the circle shape for cleaner positioning
        float cx = body.getPosition().x + (i % 2 == 0 ? -50.f : 210.f + (i / 2) * 50.f);
        float cy = body.getPosition().y + 110.f;
        m.shape.setPosition(cx, cy);
        m.teleportTimer = 0.f;
        m.teleportTimerMax = 240.f;
        m.shootTimer = (float)(rand() % 90);
        m.shootTimerMax = 90.f;
        m.hp = 3;
        if (texOk) {
            m.sprite.setTexture(minionTex);
            m.sprite.setOrigin(mts.x / 2.f, mts.y / 2.f);
            float msc = 30.f / (float)std::max(mts.x, mts.y);
            m.sprite.setScale(msc, -msc);   // Y-flip so ship faces down
            m.sprite.setColor(sf::Color(255, 100, 50));
            // SKIN FIX: sprite center = circle center (origin already set to center above)
            m.sprite.setPosition(cx, cy);
        }
        minions.push_back(m);
    }
}

// ---- update ----

void Boss2::update(int& health, sf::ConvexShape& player,
    unsigned& points, bool& isBoss2Stage,
    bool isInvincible, bool isSlowed,
    float& invincibilityTimer, float& deathRayTimer,
    float& fastFireTimer, float& laserFireTimerMax,
    float& screenShakeOut, int maxPlayerHp,
    std::function<void(int)> playerDamageFn)
{
    if (!active && isBoss2Stage) {
        active = true;
        body.setPosition(window->getSize().x / 2.f - 100.f, -220.f);
        stage = 0;
        revived = false;
        rageMode = false;
        loadStage(1);
        projectiles.clear();
        minions.clear();
        powerUps.clear();
        beamActive = false;
        dying = false;
        debugLog("Boss2 activated");
    }
    if (!active) return;
    if (dying) return;

    float speedMult = isSlowed ? 0.5f : 1.f;
    float rage = rageMode ? 2.0f : 1.f;

    // Stage flash tint
    if (stageFlashTimer > 0.f) {
        stageFlashTimer -= 1.f;
        bodySprite.setColor((int)stageFlashTimer % 6 < 3 ? sf::Color::White : sf::Color(255, 80, 0));
    }
    else if (rageMode) {
        bodySprite.setColor(sf::Color(255, 80, 80));
    }

    if (stageAnnounceTimer > 0.f) stageAnnounceTimer -= 1.f;

    // ---- slide-in ----
    if (body.getPosition().y < 50.f) {
        body.move(0.f, 1.8f * speedMult);
        return;
    }

    // ---- horizontal drift ----
    if (!beamActive) {
        body.move(moveDir.x * speed * speedMult, 0.f);
        if (body.getPosition().x < 50.f)
            moveDir.x = 1.f;
        if (body.getPosition().x + 200.f > window->getSize().x - 50.f)
            moveDir.x = -1.f;
    }

    attackTimer += speedMult;
    radialTimer += speedMult;
    beamTimer += speedMult;
    homingTimer += speedMult;
    puDropTimer += 1.f;

    // ---- B1: difficulty-scaled attack cadences ----
    float diffMult = (difficulty == 0) ? 1.8f : (difficulty == 1) ? 1.2f : 0.6f;
    float radialInterval = std::max(80.f, 160.f / (stage * rage)) * diffMult;
    float beamInterval = std::max(160.f, 400.f / (stage * rage)) * diffMult;
    float homingInterval = std::max(100.f, 240.f / (stage * rage)) * diffMult;
    // FIX: fewer radial bullets on Easy/Normal — Hard unchanged
    int   radialCount = (difficulty == 0) ? (3 + stage) : (difficulty == 1) ? (4 + stage) : (5 + stage * 2);
    float projSpd = (1.4f + stage * 0.2f) * (difficulty == 0 ? 0.6f : difficulty == 1 ? 0.85f : 1.3f);

    // stage 1-2: radial bursts only
    if (radialTimer >= radialInterval) {
        radialTimer = 0.f;
        sf::Color c = rageMode ? sf::Color(255, 30, 30) : sf::Color(255, 140 + stage * 15, 0);
        fireRadial(radialCount, projSpd, c);
    }

    // stage 2+: spread (minion spawn)
    if (stage >= 2 && attackTimer >= 200.f / rage) {
        attackTimer = 0.f;
        if ((int)minions.size() < stage) spawnMinions(1);
    }

    // stage 3+: beam
    if (stage >= 3 && !beamActive && beamTimer >= beamInterval) {
        beamTimer = 0.f;
        beamActive = true;
        beamActiveTimer = 0.f;
        beamDamageTimer = 0.f; // A1: reset on each beam activation
        float cx = body.getPosition().x + 100.f;
        float cy = body.getPosition().y + 200.f;
        beam.setSize(sf::Vector2f(260.f, (float)window->getSize().y));
        beam.setPosition(cx - 130.f, cy);
        screenShakeOut = 15.f;
    }

    if (beamActive) {
        beamActiveTimer += speedMult;
        beamDamageTimer += speedMult; // A1: track time between damage ticks
        beamAlpha = std::min(255.f, beamAlpha + 18.f);
        float cx = body.getPosition().x + 100.f;
        float cy = body.getPosition().y + 200.f;
        beam.setPosition(cx - 130.f, cy);
        beam.setFillColor(sf::Color(255, 60, 0, (sf::Uint8)beamAlpha));

        // deal damage every 20 frames (first hit ~0.17s in, then repeating)
        // 60f threshold was longer than the beam itself at stage 3 (55f), so it never fired
        if (beamDamageTimer >= 20.f) {
            beamDamageTimer = 0.f;
            if (!isInvincible && player.getGlobalBounds().intersects(beam.getGlobalBounds())) {
                if (playerDamageFn) playerDamageFn(1);
                else health -= 1;
                invincibilityTimer = 10.f; // brief iframes so two simultaneous ticks can't stack
            }
        }

        float activeTime = 40.f + stage * 5.f;
        if (beamActiveTimer >= activeTime) {
            beamActive = false;
            beamAlpha = 0.f;
            beamDamageTimer = 0.f;
            beam.setFillColor(sf::Color(255, 60, 0, 0));
        }
    }

    // stage 5+: homing bullets
    if (stage >= 5) {
        if (homingTimer >= homingInterval) {
            homingTimer = 0.f;
            float cx = body.getPosition().x + 100.f;
            float cy = body.getPosition().y + 200.f;
            for (int i = 0; i < 3; i++) {
                Boss2Proj p;
                p.shape.setRadius(9.f);
                p.shape.setFillColor(sf::Color(200, 0, 255));
                p.shape.setPosition(cx + (i - 1) * 40.f, cy);
                sf::Vector2f dir = player.getPosition() - p.shape.getPosition();
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                if (len > 0.f) dir /= len;
                p.velocity = dir * (3.5f + stage * 0.3f);
                p.homing = true;
                projectiles.push_back(p);
            }
        }
    }

    // ---- power-up drops — FIX: every 140 frames (was 280) ----
    if (puDropTimer >= 140.f) {
        puDropTimer = 0.f;
        B2PowerUp pu;
        pu.type = rand() % 4;
        pu.shape.setRadius(12.f);
        pu.shape.setPosition(body.getPosition().x + rand() % 200, body.getPosition().y + 210.f);
        sf::Color puColors[] = { sf::Color::Green, sf::Color::Yellow, sf::Color::Red, sf::Color::Cyan };
        pu.shape.setFillColor(puColors[pu.type]);
        powerUps.push_back(pu);
    }
    for (size_t i = 0; i < powerUps.size(); i++) {
        powerUps[i].shape.move(0.f, 1.5f);
        if (powerUps[i].shape.getPosition().y > window->getSize().y) { powerUps.erase(powerUps.begin() + i); i--; continue; }
        if (powerUps[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            switch (powerUps[i].type) {
            case 0: health = std::min(health + 2, maxPlayerHp); break;
            case 1: invincibilityTimer = 600.f; break;
            case 2: deathRayTimer = 600.f; break;
            case 3: fastFireTimer = 600.f; laserFireTimerMax = 5.f; break;
            }
            powerUps.erase(powerUps.begin() + i); i--;
        }
    }

    // ---- projectile movement ----
    for (size_t i = 0; i < projectiles.size(); i++) {
        // Homing: nudge toward player
        if (projectiles[i].homing) {
            sf::Vector2f dir = player.getPosition() - projectiles[i].shape.getPosition();
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.f) dir /= len;
            projectiles[i].velocity += dir * 0.06f * speedMult;
            // cap speed
            float spd2 = std::sqrt(projectiles[i].velocity.x * projectiles[i].velocity.x +
                projectiles[i].velocity.y * projectiles[i].velocity.y);
            float maxSpd = 6.f;
            if (spd2 > maxSpd) projectiles[i].velocity *= maxSpd / spd2;
        }
        projectiles[i].shape.move(projectiles[i].velocity.x * speedMult,
            projectiles[i].velocity.y * speedMult);
        float px = projectiles[i].shape.getPosition().x;
        float py = projectiles[i].shape.getPosition().y;
        if (py > window->getSize().y + 20.f || py < -20.f || px < -20.f || px > window->getSize().x + 20.f) {
            projectiles.erase(projectiles.begin() + i); i--; continue;
        }
        if (projectiles[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            // A3: reduced damage + invincibility frames to prevent multi-hit in same frame
            if (!isInvincible) {
                if (playerDamageFn) playerDamageFn((stage >= 4) ? 2 : 1);
                else health -= (stage >= 4) ? 2 : 1;
                invincibilityTimer = 45.f;
            }
            projectiles.erase(projectiles.begin() + i); i--;
        }
    }

    // ---- minions ----
    for (size_t i = 0; i < minions.size(); i++) {
        minions[i].teleportTimer += speedMult;
        minions[i].shootTimer += speedMult;
        if (minions[i].teleportTimer >= minions[i].teleportTimerMax) {
            minions[i].teleportTimer = 0.f;
            float tx, ty; int att = 0;
            do {
                tx = 15.f + rand() % (int)(window->getSize().x - 30.f);
                ty = 15.f + rand() % (int)(window->getSize().y - 30.f);
                att++;
            } while (att < 20 &&
                std::sqrt((tx - player.getPosition().x) * (tx - player.getPosition().x) +
                    (ty - player.getPosition().y) * (ty - player.getPosition().y)) < 140.f);
            minions[i].shape.setPosition(tx, ty);
        }
        if (minions[i].shootTimer >= minions[i].shootTimerMax) {
            minions[i].shootTimer = 0.f;
            sf::Vector2f mpos = minions[i].shape.getPosition();
            sf::Vector2f dir = player.getPosition() - mpos;
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.f) dir /= len;
            Boss2Proj b;
            b.shape.setRadius(6.f);
            b.shape.setFillColor(sf::Color(255, 0, 200));
            b.shape.setPosition(mpos);
            b.velocity = dir * 4.5f;
            b.homing = false;
            projectiles.push_back(b);
        }
        // SKIN FIX: shape origin is now centered, so getPosition() == circle center
        minions[i].shape.setPosition(minions[i].shape.getPosition()); // no-op; teleport already set it
        minions[i].sprite.setPosition(minions[i].shape.getPosition());
        if (minions[i].shape.getGlobalBounds().intersects(player.getGlobalBounds())) {
            // A2: 8 damage was an instant kill on Normal (10 HP); reduced to 2 + iframes
            if (!isInvincible) {
                if (playerDamageFn) playerDamageFn(2);
                else health -= 2;
                invincibilityTimer = 45.f;
            }
            minions.erase(minions.begin() + i); i--;
        }
    }

    // ---- HP zero: stage transition or death ----
    if (hp <= 0) {
        // B2: stage-6 revive HP scales with difficulty
        if (stage == 6 && !revived) {
            revived = true;
            rageMode = true;
            hp = (difficulty == 0) ? 200 : (difficulty == 1) ? 350 : 500;
            maxHp = hp;
            speed = stageSpeed[5] * 1.4f;
            stageFlashTimer = 60.f;
            stageAnnounceTimer = 200.f;
            screenShakeOut = 35.f;
            stageTransitioned = true;
            // Red tint
            bodySprite.setColor(sf::Color(255, 50, 50));
            debugLog("Boss2 Stage 6 REVIVED - RAGE MODE");
            return;
        }

        if (stage < 6) {
            // transition to next stage
            int prevStage = stage;
            projectiles.clear();
            minions.clear();
            body.setPosition(window->getSize().x / 2.f - 100.f, 50.f);
            screenShakeOut = 25.f;
            stageTransitioned = true;
            loadStage(prevStage + 1);
            debugLog("Boss2 stage transition: " + std::to_string(prevStage) + " -> " + std::to_string(stage));
        }
        else {
            // fully dead
            points += 3000;
            dying = true;
            isBoss2Stage = false;
            projectiles.clear();
            minions.clear();
            powerUps.clear();
            debugLog("Boss2 DEFEATED");
        }
    }
}

// ---- render ----

void Boss2::render(sf::RenderTarget& target)
{
    if (!active) return;

    // Beam (behind sprite)
    if (beamActive)
        target.draw(beam);

    // Body sprite
    sf::Vector2f center(body.getPosition().x + 100.f, body.getPosition().y + 100.f);
    bodySprite.setPosition(center);
    target.draw(bodySprite);

    // Projectiles
    for (auto& p : projectiles) {
        if (p.isMissile) {
            p.sprite.setPosition(p.shape.getPosition().x + p.shape.getRadius(), p.shape.getPosition().y + p.shape.getRadius());
            target.draw(p.sprite);
        }
        else {
            sf::CircleShape glow(p.shape.getRadius() * 2.f);
            glow.setOrigin(glow.getRadius(), glow.getRadius());
            glow.setPosition(p.shape.getPosition().x + p.shape.getRadius(),
                p.shape.getPosition().y + p.shape.getRadius());
            sf::Color gc = p.shape.getFillColor();
            glow.setFillColor(sf::Color(gc.r, gc.g, gc.b, 50));
            target.draw(glow);
            target.draw(p.shape);
        }
    }

    // Minions — draw circle fallback first, then sprite on top (sprite is transparent if tex failed)
    for (auto& m : minions) {
        target.draw(m.shape);   // always visible (dim orange circle fallback)
        if (m.sprite.getTexture() != nullptr)
            target.draw(m.sprite);
    }

    // Power-ups
    for (auto& pu : powerUps) target.draw(pu.shape);

    // HP bar
    float hpRatio = (float)hp / (float)maxHp;
    sf::RectangleShape hpBg(sf::Vector2f((float)window->getSize().x - 100.f, 22.f));
    hpBg.setPosition(50.f, 50.f);
    hpBg.setFillColor(sf::Color(40, 0, 0));
    target.draw(hpBg);

    sf::Color barCol;
    if (hpRatio > 0.5f) barCol = sf::Color((sf::Uint8)(255 * (1.f - (hpRatio - 0.5f) * 2.f)), 255, 0);
    else                barCol = sf::Color(255, (sf::Uint8)(255 * hpRatio * 2.f), 0);
    if (rageMode) barCol = sf::Color(255, 30, 30);
    sf::RectangleShape hpFill(sf::Vector2f(hpRatio * (window->getSize().x - 100.f), 22.f));
    hpFill.setPosition(50.f, 50.f);
    hpFill.setFillColor(barCol);
    target.draw(hpFill);

    // Stage label on bar
    sf::Text stageLabel;
    // (rendered via renderStageAnnouncement which takes font)
}

void Boss2::renderStageAnnouncement(sf::RenderTarget& target, sf::Font& font)
{
    if (!active) return;

    // Bar stage label
    {
        sf::Text sl;
        sl.setFont(font); sl.setCharacterSize(16);
        sl.setFillColor(rageMode ? sf::Color(255, 80, 80) : sf::Color(255, 200, 0));
        std::string label = "SURT  STAGE " + std::to_string(stage);
        if (rageMode) label += "  [RAGE]";
        sl.setString(label);
        sl.setPosition(50.f, 74.f);
        target.draw(sl);
    }

    if (stageAnnounceTimer > 0.f) {
        float alpha = std::min(1.f, stageAnnounceTimer / 30.f) * 255.f;
        sf::Text at;
        at.setFont(font); at.setCharacterSize(80);
        std::string msg = rageMode ? "SURT RAGES!" : ("STAGE " + std::to_string(stage) + "!");
        at.setString(msg);
        sf::Color c = rageMode ? sf::Color(255, 30, 30, (sf::Uint8)alpha)
            : sf::Color(255, 120 + stage * 20, 0, (sf::Uint8)alpha);
        at.setFillColor(c);
        sf::FloatRect tb = at.getLocalBounds();
        at.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
        at.setPosition((float)window->getSize().x / 2.f, (float)window->getSize().y / 2.f - 120.f);
        target.draw(at);
    }
}