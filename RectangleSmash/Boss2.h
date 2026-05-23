#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <functional>

struct Boss2Proj {
    sf::CircleShape shape;
    sf::Vector2f    velocity;
    bool            homing;
    bool            isMissile = false;
    sf::Sprite      sprite;
};

struct Boss2Minion {
    sf::CircleShape shape;
    sf::Sprite      sprite;
    sf::Vector2f    velocity;
    float           teleportTimer;
    float           teleportTimerMax;
    float           shootTimer;
    float           shootTimerMax;
    int             hp;
};

class Boss2
{
public:
    Boss2();
    ~Boss2() {}

    void initBoss2(sf::RenderWindow* win, int difficulty);
    void update(int& health, sf::ConvexShape& player,
        unsigned& points, bool& isBoss2Stage,
        bool isInvincible, bool isSlowed,
        float& invincibilityTimer, float& deathRayTimer,
        float& fastFireTimer, float& laserFireTimerMax,
        float& screenShakeOut, int maxPlayerHp,
        std::function<void(int)> playerDamageFn = nullptr);
    void render(sf::RenderTarget& target);
    void renderStageAnnouncement(sf::RenderTarget& target, sf::Font& font);

    void takeDamage(int dmg);
    bool isActive()  const { return active; }
    bool isBeamActive() const { return beamActive; }
    void setActive(bool v) { active = v; }
    sf::FloatRect getBounds() const { return body.getGlobalBounds(); }
    int  getHp()     const { return hp; }
    int  getMaxHp()  const { return maxHp; }
    float getHpRatio() const { return (maxHp > 0) ? (float)hp / maxHp : 0.f; }
    bool isDying() const { return dying; }
    int  getStage()  const { return stage; }
    bool hasStageTransitioned() const { return stageTransitioned; }
    void clearStageTransition() { stageTransitioned = false; }
    std::vector<Boss2Minion>& getMinions() { return minions; }
    std::vector<Boss2Proj>& getProjectiles() { return projectiles; }

private:
    void loadStage(int s);
    void fireRadial(int count, float speed, sf::Color color);
    void fireBeam();
    void spawnMinions(int n);

    sf::RenderWindow* window;
    int  difficulty;
    bool active;
    bool stageTransitioned;
    bool dying = false;

    // shape / sprite
    sf::RectangleShape body;        // 200x200 collision box
    sf::Texture        stageTex[6];
    sf::Sprite         bodySprite;

    sf::Texture        minionTex;
    sf::Texture        missileTex;

    // hp
    int hp, maxHp;
    static const int stageMaxHp[6];

    // stage
    int  stage;           // 1-6
    bool revived;         // stage 6 revive used
    float stageFlashTimer;
    float stageAnnounceTimer;

    // movement
    sf::Vector2f moveDir;
    float        speed;

    // attack timers
    float attackTimer;
    float beamTimer;
    float radialTimer;
    float homingTimer;
    float beamDamageTimer; // cooldown between beam damage ticks (A1 fix)

    // beam visuals
    sf::RectangleShape beam;
    float beamAlpha;
    bool  beamActive;
    float beamActiveTimer;

    // projectiles + minions
    std::vector<Boss2Proj>   projectiles;
    std::vector<Boss2Minion> minions;

    // power-up drops
    struct B2PowerUp { sf::CircleShape shape; int type; };
    std::vector<B2PowerUp> powerUps;
    float puDropTimer;

    // phase 3 rage (stage 6 revive rage)
    bool rageMode;
};