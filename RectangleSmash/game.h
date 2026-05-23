#pragma once

#include <iostream>
#include <vector>
#include <ctime>
#include <sstream>
#include <cmath>
#include <functional>
#include <string>
#include <fstream>
#include <algorithm>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

#include "boss.h"
#include "boss2.h"

// ─────────────────────────────────────────────
//  debugLog — inline so every .cpp gets its own
//  copy; no linker "unresolved external" error.
// ─────────────────────────────────────────────
inline void debugLog(const std::string& msg)
{
    std::cout << "[DEBUG] " << msg << "\n";
}

// ─────────────────────────────────────────────
//  GAME STATES
// ─────────────────────────────────────────────
enum class GameState {
    MENU,
    DIFFICULTY_SELECT,
    SHIP_SELECT,
    PLAYING,
    PAUSED,
    GAME_OVER,
    INTRO,
    HOW_TO_PLAY,
    BOSS_ARRIVAL,
    SHOP,
    BLACK_MARKET,
    PERK_SELECT,
    SETTINGS,
    SKINS
};

// ─────────────────────────────────────────────
//  CONSTANTS
// ─────────────────────────────────────────────
constexpr int MAX_HIGH_SCORES = 5;
constexpr int MAX_LASER_SOUNDS = 8;

// Explosion spritesheet constants
constexpr int   EXPL_FRAMES = 8;
constexpr int   EXPL_FRAME_W = 64;
constexpr int   EXPL_FRAME_H = 64;

// ─────────────────────────────────────────────
//  SHARED DATA STRUCTURES
// ─────────────────────────────────────────────

struct Laser {
    sf::RectangleShape shape;
    sf::Vector2f       velocity;
    int                pierceCount = 0;
};

struct EnemyBullet {
    sf::CircleShape shape;
    sf::Vector2f    velocity;
};

struct Particle {
    sf::RectangleShape shape;
    sf::Vector2f       velocity;
    float              lifetime = 0.f;
    float              maxLifetime = 1.f;
};

struct Star {
    sf::CircleShape shape;
    float           speed = 1.f;
};

struct PowerUp {
    sf::CircleShape shape;
    int type = 0;
    // 0=Invincibility 1=DeathRay 2=CircularBlast 3=FireRate 4=SlowTime 5=Health
};

struct SpawnWarning {
    float xPos = 0.f;
    float width = 50.f;
    int   enemyType = 0;
    float timer = 0.f;
    float timerMax = 60.f;
};

struct ScorePopup {
    sf::Text     text;
    sf::Vector2f velocity;
    float        lifetime = 0.f;
    float        maxLifetime = 52.f;
};

struct CurrencyDrop {
    sf::CircleShape shape;
    sf::Vector2f    velocity;
    int             amount = 1;
    float           lifetime = 0.f;
    float           maxLifetime = 300.f;
    float           timer = 0.f;   // general-purpose tick timer
};

struct DashGhost {
    sf::ConvexShape shape;
    sf::Sprite      sprite;
    float           alpha = 255.f;
    float           lifetime = 0.f;
    float           maxLifetime = 10.f;
};

struct ExplosionAnim {
    sf::Sprite sprite;
    int        frame = 0;
    float      timer = 0.f;
    float      timerMax = 4.f;
    float      frameTick = 0.f;   // accumulator for frame advancement
    float      x = 0.f;
    float      y = 0.f;
    float      scale = 1.f;
    bool       done = false;
};

struct Asteroid {
    sf::CircleShape shape;
    sf::Vector2f    velocity;
    float           lifetime = 0.f;
    float           maxLifetime = 300.f;
    int             hp = 3;
    float           rotationSpeed = 1.f;
};

struct BlackHole {
    sf::CircleShape eventHorizon;
    sf::CircleShape core;
    float           gravityRadius = 200.f;
    float           pullStrength = 1.f;
    float           lifetime = 0.f;
    float           maxLifetime = 600.f;
};

struct SpaceMine {
    sf::CircleShape shape;
    float           timer = 300.f;
    bool            detonated = false;
};

struct HomingMissile {
    sf::CircleShape shape;
    sf::Vector2f    velocity;
    float           lifetime = 0.f;
    float           maxLifetime = 200.f;
};

struct Drone {
    sf::CircleShape shape;
    float           orbitAngle = 0.f;
    int             shootTimer = 0;
    int             shootTimerMax = 60;
};

struct DemoLaser {
    sf::RectangleShape shape;
    sf::Vector2f       vel;      // named vel to match usage in .cpp files
};

struct Perk {
    std::string name;
    std::string desc;
    int         id = 0;
    sf::Color   color = sf::Color::White;
};

struct EnemyData {
    sf::RectangleShape shape;
    sf::Sprite         sprite;
    sf::Color          baseColor = sf::Color::White;
    int                enemyType = 0;
    int                hp = 1;
    int                maxHp = 1;
    int                flashTimer = 0;
    float              zigzagTimer = 0.f;
    float              zigzagDir = 1.f;
    float              shootTimer = 0.f;
    float              shootTimerMax = 120.f;
    bool               hasShield = false;
    int                shieldHp = 0;
    bool               stopAndShoot = false;
    float              stopXDir = 1.f;
    bool               isGhost = false;
    float              ghostAlpha = 255.f;
};

// ─────────────────────────────────────────────
//  GAME CLASS
// ─────────────────────────────────────────────
class game
{
private:

    // ── Window ───────────────────────────────
    sf::RenderWindow* window;
    sf::VideoMode     dimensions;
    sf::Event         ev;

    GameState state;
    int       difficulty;   // 0=Easy 1=Normal 2=Hard

    // ── Fonts ────────────────────────────────
    sf::Font font;

    // ── HUD / Menu texts ─────────────────────
    sf::Text UiText;
    sf::Text GameOverText;
    sf::Text FinalScoreText;
    sf::Text PlayText;
    sf::Text QuitText;
    sf::Text PausedText;
    sf::Text DiffEasyText;
    sf::Text DiffNormalText;
    sf::Text DiffHardText;
    sf::Text comboText;
    sf::Text killBannerText;
    sf::Text waveBannerText;
    sf::Text highScoreEntryTexts[MAX_HIGH_SCORES];
    sf::Text highScoreTitleText;
    sf::Text SettingsTitleText;
    sf::Text skinsTitleText;

    // Menu item texts
    sf::Text ShipSelectText;
    sf::Text ShopText;
    sf::Text BlackMarketText;
    sf::Text SettingsText;
    sf::Text SkinsText;
    sf::Text BackText;

    // Settings control texts
    sf::Text MoveSpeedSettingText;
    sf::Text MoveSpeedMinus;
    sf::Text MoveSpeedValText;
    sf::Text MoveSpeedPlus;
    sf::Text FireSpeedSettingText;
    sf::Text FireSpeedMinus;
    sf::Text FireSpeedValText;
    sf::Text FireSpeedPlus;
    sf::Text FollowMouseSettingText;
    sf::Text AutoFireSettingText;
    sf::Text OverheatSettingText;

    // ── Mouse position ────────────────────────
    sf::Vector2i mousePoswindow;
    sf::Vector2f mousePosView;

    // ── Core game logic ───────────────────────
    unsigned points;
    int      health;
    int      maxHealth;
    bool     EndGame;
    bool     playerDeathSequence = false;
    float    playerDeathTimer = 0.f;
    int      currentWave;
    int      comboCount;
    float    timeScale;

    // High scores
    int highScores[MAX_HIGH_SCORES];
    int highScoreWaves[MAX_HIGH_SCORES];

    // ── Player stats / shop upgrades ─────────
    int   weaponLevel;
    int   totalGears;
    int   upgWeaponLevel;
    int   upgMoveSpeed;
    int   upgFireRate;
    int   upgDashCooldown;
    int   upgMaxHealth;
    int   selectedShipClass;
    int   highestWaveReached;
    float basePlayerMoveSpeed;
    float baseLaserFireTimerMax;
    bool  followMouse;
    bool  overheatEnabled;
    int   selectedSkin;

    // Black Market persistent upgrades
    int blkStartingExtraHealth;
    int blkPermanentSpeedBoost;
    int blkDamageMultiplier;
    int blkStartingGears;

    // ── Difficulty multipliers ────────────────
    float diffEnemySpeedMult;
    float diffBossDamageMult;
    float diffPowerUpDropRate;
    float diffEnemyHpBonus;

    // ── Wave logic ────────────────────────────
    int   enemiesKilledInWave;
    int   enemiesNeededInWave;
    int   totalEnemiesSpawnedInWave;
    bool  inBreather;
    float breatherTimer;
    float breatherTimerMax;
    float waveBannerTimerMax;
    float waveEnemySpeedBonus;

    // ── Enemy spawning ────────────────────────
    float enemiesSpawnTimer;
    float enemiesSpawnTimerMax;
    int   maxEnemies;
    bool  mouseHeld;

    // ── Fire rate ─────────────────────────────
    float laserFireTimer;
    float laserFireTimerMax;

    // ── Firing / input flags ──────────────────
    bool autoFire;
    bool isFiring;
    bool rightMouseHeld;

    // ── Combo ─────────────────────────────────
    float comboTimer;
    float comboTimerMax;

    // ── Heat (overheat) ───────────────────────
    float heatLevel;
    float heatMax;
    bool  isOverheated;
    float overheatTimer;
    float overheatTimerMax;
    bool  overheatWasActive;

    // ── Shield ───────────────────────────────
    bool  shieldActive;
    float shieldRechargeTimer;
    float shieldRechargeMax;

    // ── Dash ──────────────────────────────────
    bool         isDashing;
    float        dashCooldown;
    float        dashCooldownMax;
    float        dashTimer;
    float        dashTimerMax;
    float        parryWindowTimer;
    bool         isParrying;
    sf::Vector2f dashVelocity;

    // ── Secondary weapon energy ───────────────
    float secondaryEnergy;
    float secondaryEnergyMax;

    // ── Power-up timers ───────────────────────
    float invincibilityTimer;
    float deathRayTimer;
    float fastFireTimer;
    float slowTimeTimer;

    // ── Screen effects ────────────────────────
    float screenShake;
    float screenShakeIntensity;
    float hitStopTimer;
    float hyperspaceProgress;

    // ── Kill / wave banners ───────────────────
    float killBannerTimer;
    float killBannerTimerMax;
    float waveBannerTimer;

    // ── Intro / How-To-Play ───────────────────
    int   howToPage;
    float introTimer;
    float introTextAlpha;
    float introMoonY;
    float introTargetY;
    float introStartY;

    // ── Player damage flash ───────────────────
    float playerDamageFlashTimer;

    // ── State transition fade ─────────────────
    float     transitionAlpha;
    bool      transitionFadingOut;
    bool      transitionFadingIn;
    GameState transitionTarget;

    // ── Graze mechanic ────────────────────────
    int   grazeCount;
    float grazeTimer;
    float grazeMultiplier;
    float grazeFlashTimer;

    // ── Bomb ability ──────────────────────────
    int   bombCount;
    float bombCooldownTimer;
    float bombCooldownMax;

    // ── Companion drones ─────────────────────
    int droneLevel;

    // ── Combo announcer ───────────────────────
    float comboAnnouncerTimer;
    float comboAnnouncerScale;

    // ── Ship ultimate ─────────────────────────
    float ultimateCharge;
    float ultimateChargeMax;
    float ultimateActiveTimer;
    bool  ultimateActive;

    // ── Endless loop ──────────────────────────
    int   loopCount;
    bool  loopModeAvailable;
    float loopDifficultyMult;

    // ── Elite enemies ─────────────────────────
    bool nextWaveHasElite;
    int  eliteKillCount;

    // ── Perk synergies ────────────────────────
    bool hasPerkSynergy;
    int  evolvedWeaponId;

    // ── Custom crosshair sprites ──────────────
    sf::RectangleShape cursorVert;
    sf::RectangleShape cursorHoriz;

    // ── Sounds ───────────────────────────────
    sf::SoundBuffer laserBuf;
    sf::Sound       laserSounds[MAX_LASER_SOUNDS];
    int             currentLaserSoundIndex;

    sf::SoundBuffer bossBeamBuf;        // was bossBeamSoundBuffer
    sf::Sound       bossBeamSound;

    sf::SoundBuffer bossLazerBuf2;      // was bossLazerSoundBuffer2
    sf::Sound       bossLazerSound2;

    sf::SoundBuffer dashBuf;            // was dashSoundBuffer
    sf::Sound       dashSound;

    sf::SoundBuffer overheatBuf;
    sf::Sound       overheatSound;

    sf::SoundBuffer explosionBuf;
    sf::Sound       explosionSound;

    sf::SoundBuffer powerUpBuf;
    sf::Sound       powerUpSound;

    sf::SoundBuffer hitBuf;
    sf::Sound       hitSound;

    sf::SoundBuffer waveCompleteBuf;
    sf::Sound       waveCompleteSound;

    sf::SoundBuffer destructionBuf;
    sf::Sound       destructionSound;

    sf::Music bgMusic;
    sf::Music bossBgMusic;

    // ── Boss 1 ────────────────────────────────
    bool  isBossStage;
    Boss  bossEntity;
    bool  bossWaveDefeated;
    bool  bossArrivalPending;
    float bossArrivalTimer;
    int   bossArrivalWave;
    bool  bossDeathSequence;
    float bossDeathTimer;
    int   bossLastState;

    // ── Boss 2 (Surt) ─────────────────────────
    bool  isBoss2Stage;
    Boss2 boss2Entity;
    bool  boss2Defeated;
    bool  boss2DeathSequence;
    float boss2DeathTimer;
    bool  boss2LastBeamState;

    // ── Textures ──────────────────────────────
    sf::Texture playerTexture;
    sf::Texture enemyTextures[4];
    sf::Texture bossTexture;
    sf::Texture minionTexture;
    sf::Texture skinTextures[6];
    sf::Texture explosionTex;
    sf::Texture moonTexture;

    // ── Sprites ───────────────────────────────
    sf::Sprite  playerSprite;
    sf::Sprite  skinPreviews[6];
    sf::Sprite  moonSprite;
    sf::Sprite  demoShipSprite;

    // ── Shield overlay ────────────────────────
    sf::CircleShape shieldShape;

    // ── Demo background ───────────────────────
    sf::Vector2f demoShipPos;
    sf::Vector2f demoShipVel;
    float        demoShootTimer;

    // ── Perks ────────────────────────────────
    std::vector<Perk> perkPool;
    int               perkChoices[3];
    std::vector<int>  activePerkIds;

    // ── Game objects ──────────────────────────
    sf::ConvexShape            player;
    std::vector<EnemyData>     enemies;
    std::vector<Laser>         lasers;
    std::vector<EnemyBullet>   enemyBullets;
    std::vector<Particle>      particles;
    std::vector<Star>          stars;
    std::vector<PowerUp>       powerUps;
    std::vector<SpawnWarning>  spawnWarnings;
    std::vector<ScorePopup>    scorePopups;
    std::vector<CurrencyDrop>  drops;
    std::vector<DashGhost>     dashTrail;
    std::vector<ExplosionAnim> activeExplosions;
    std::vector<Asteroid>      asteroids;
    std::vector<BlackHole>     blackHoles;
    std::vector<SpaceMine>     mines;
    std::vector<HomingMissile> missiles;
    std::vector<Drone>         drones;
    std::vector<DemoLaser>     demoLasers;

    // ── Init functions ────────────────────────
    void initvariable();
    void initwindow();
    void initEnemies();
    void initfonts();
    void initText();
    void initMenuTexts();
    void initGameOverText();
    void initSettingsTexts();
    void initPlayer();
    void initBackground();
    void initAudio();
    void initHighScores();
    void loadHighScore();
    void loadSaveData();
    void saveGameData();
    void applyDifficultySettings(int diff);

    // ── Helper ────────────────────────────────
    // Maps fire-rate timer value → 1-5 display level
    static inline int fireSpeedLevelFromTimer(float t)
    {
        if (t <= 5.f)  return 5;
        if (t <= 9.f)  return 4;
        if (t <= 13.f) return 3;
        if (t <= 17.f) return 2;
        return 1;
    }

    // ── Spawn / damage / persistence ─────────
    void spawnPowerUp(sf::Vector2f pos, bool forceHealth = false);
    void playerTakeDamage(int damage);
    void saveHighScore();
    void queueSpawnWarning(int enemyType);
    void spawnEnemyFromWarning(int enemyType, float xPos);
    void spawnEnemyGroup(int enemyType, float xPos, int count);
    void addScorePopup(sf::Vector2f pos, int pts);
    void triggerKillBanner(int combo);
    void gainPoints(int base, sf::Vector2f pos);
    void addExplosionParticles(sf::Vector2f pos, sf::Color color, int count);
    void spawnExplosion(float x, float y, float scale);
    void triggerBomb();
    void generatePerkChoices();

    // ── Update functions ──────────────────────
    void updateTransition();
    void updateMusic();
    void updateIntro();
    void updateHowToPlay();
    void updateBossArrival();
    void updateMenu();
    void updateShipSelect();
    void updateShop();
    void updateBlackMarket();
    void updatePerkSelect();
    void updateDifficultySelect();
    void updateSettings();
    void updateSkins();
    void updateWave();
    void updateCombo();
    void updateHeat();
    void updateShield();
    void updateDash();
    void updateSecondaryWeapons();
    void updatePowerUps();
    void updateDrops();
    void updateEnemies();
    void updateSpawnWarnings();
    void updateHazards();
    void updateBoss2();
    void updateExplosions();
    void updatePlayer();
    void updateBombs();
    void updateDrones();
    void updateLasers();
    void updateEnemyBullets();
    void updateParticles();
    void updateParticleTrails();
    void updateScorePopups();
    void updateBackground();
    void updateDemoBackground();
    void updateComboAnnouncer();
    void updateUltimate();
    void updateGraze();

    // ── Render functions ──────────────────────
    void renderTransitionFade(sf::RenderTarget& target);
    void renderIntro(sf::RenderTarget& target);
    void renderHowToPlay(sf::RenderTarget& target);
    void renderBossArrival(sf::RenderTarget& target);
    void renderMenu(sf::RenderTarget& target);
    void renderHighScores(sf::RenderTarget& target);
    void renderShipSelect(sf::RenderTarget& target);
    void renderShop(sf::RenderTarget& target);
    void renderBlackMarket(sf::RenderTarget& target);
    void renderPerkSelect(sf::RenderTarget& target);
    void renderDifficultySelect(sf::RenderTarget& target);
    void renderSettings(sf::RenderTarget& target);
    void renderSkins(sf::RenderTarget& target);
    void renderHazards(sf::RenderTarget& target);
    void renderSpawnWarnings(sf::RenderTarget& target);
    void renderPowerUps(sf::RenderTarget& target);
    void renderLasers(sf::RenderTarget& target);
    void renderSecondaryWeapons(sf::RenderTarget& target);
    void renderEnemyBullets(sf::RenderTarget& target);
    void renderParticles(sf::RenderTarget& target);
    void renderDashTrail(sf::RenderTarget& target);
    void renderPlayer(sf::RenderTarget& target);
    void renderHUD(sf::RenderTarget& target);
    void renderText(sf::RenderTarget& target);
    void renderScorePopups(sf::RenderTarget& target);
    void renderVignette(sf::RenderTarget& target);
    void renderExplosions(sf::RenderTarget& target);
    void renderBoss2(sf::RenderTarget& target);
    void renderEnemies(sf::RenderTarget& target);
    void renderBackground(sf::RenderTarget& target);
    void renderDemoBackground(sf::RenderTarget& target);

public:
    // ── Constructor / Destructor ──────────────
    game();
    virtual ~game();

    // ── State queries ─────────────────────────
    const bool running()    const;
    const bool getEndGame() const;

    // ── Public game loop functions ────────────
    void spawnEnemyWithoutOverlap();
    void pollevent();
    void updatemousePositions();
    void updateText();
    void update();
    void render();
};