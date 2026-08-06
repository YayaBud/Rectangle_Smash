// game_init.cpp  —  constructor, all init* functions, audio, high scores, difficulty
#include "game.h"

//  PROCEDURAL AUDIO HELPERS
//
//  The originals were a linear-decay sine and a linear-decay white noise burst.
//  That is why nothing sounded like the thing it was attached to: real impacts
//  decay exponentially, have a transient at the front, and carry low-frequency
//  body. Everything below is built from four ingredients — an exponential
//  envelope, a one-pole lowpass, a soft clipper, and a sub-oscillator.
namespace {
	const int   SR = 44100;
	const float TAU = 6.2831853f;

	inline float frand() { return (rand() % 65536 - 32768) / 32768.f; }

	// Exponential decay. `curve` higher = snappier.
	inline float decay(float p, float curve) { return std::exp(-curve * p); }

	// Short fade-in kills the DC click at sample 0.
	inline float attack(int i, int atkSamples) {
		return atkSamples <= 0 ? 1.f : std::min(1.f, (float)i / (float)atkSamples);
	}

	// Soft clip — adds harmonics and stops loud sounds from crackling.
	inline float softClip(float v) {
		return std::tanh(v * 1.4f);
	}

	inline sf::Int16 toSample(float v) {
		v = std::max(-1.f, std::min(1.f, v));
		return (sf::Int16)(v * 32000.f);
	}

	inline void loadMono(sf::SoundBuffer& buf, std::vector<sf::Int16>& data) {
		buf.loadFromSamples(data.data(), data.size(), 1, SR);
	}

	// One-pole lowpass. `a` in (0,1]; smaller = darker. Turns hissy white noise
	// into something with weight behind it.
	struct LowPass {
		float a, z = 0.f;
		explicit LowPass(float alpha) : a(alpha) {}
		float operator()(float x) { z += a * (x - z); return z; }
	};
}

static void makeTone(sf::SoundBuffer& buf, float freqHz, float durSec,
	float volScale = 1.f, bool sweep = false, float freqEnd = 0.f)
{
	int n = (int)(SR * durSec);
	std::vector<sf::Int16> data(n);
	float phase = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		float freq = sweep ? (freqHz + (freqEnd - freqHz) * p) : freqHz;
		// Integrating phase instead of sin(2*pi*f*t) keeps a swept tone
		// continuous — the old form phase-jumps and buzzes as f changes.
		phase += TAU * freq / SR;
		float env = decay(p, 4.f) * attack(i, 64);
		// A little second harmonic so it reads as an instrument, not a test tone.
		float s = std::sin(phase) + 0.25f * std::sin(phase * 2.f);
		data[i] = toSample(env * volScale * 0.8f * softClip(s));
	}
	loadMono(buf, data);
}

static void makeNoise(sf::SoundBuffer& buf, float durSec, float volScale = 1.f)
{
	int n = (int)(SR * durSec);
	std::vector<sf::Int16> data(n);
	LowPass lp(0.14f);
	float subPhase = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		// Body: filtered noise, fast decay
		float body = lp(frand()) * 3.2f * decay(p, 5.f);
		// Sub: pitch dropping out from under it — this is the "boom"
		float subFreq = 110.f * (1.f - p * 0.72f);
		subPhase += TAU * subFreq / SR;
		float sub = std::sin(subPhase) * decay(p, 3.2f) * 0.7f;
		// Transient crack in the first few ms
		float crack = (p < 0.012f) ? frand() * (1.f - p / 0.012f) * 0.8f : 0.f;
		data[i] = toSample(softClip((body + sub + crack) * volScale) * attack(i, 24));
	}
	loadMono(buf, data);
}

// Sharp, dry hit. Replaces an 80 Hz sine, which was a hum with no transient.
static void makeImpact(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 0.09f);
	std::vector<sf::Int16> data(n);
	LowPass lp(0.35f);
	float phase = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		// Pitch drops 260 -> 70 Hz across 90ms: the classic "thock"
		float freq = 260.f * std::exp(-3.4f * p) + 70.f;
		phase += TAU * freq / SR;
		float tonal = std::sin(phase) * decay(p, 7.f);
		float click = lp(frand()) * decay(p, 26.f) * 2.4f;
		data[i] = toSample(softClip((tonal + click) * 0.95f) * attack(i, 12));
	}
	loadMono(buf, data);
}

// Rising major triad. Reads as "you gained something" instead of a bleep.
static void makePickup(sf::SoundBuffer& buf)
{
	const float notes[3] = { 523.25f, 659.25f, 783.99f };  // C5 E5 G5
	int noteLen = (int)(SR * 0.055f);
	int n = noteLen * 3 + (int)(SR * 0.10f);              // tail past the last note
	std::vector<sf::Int16> data(n, 0);

	for (int k = 0; k < 3; k++) {
		float phase = 0.f;
		int start = k * noteLen;
		for (int i = start; i < n; i++) {
			float p = (float)(i - start) / (float)(n - start);
			phase += TAU * notes[k] / SR;
			// Triangle-ish: fundamental + soft odd harmonic
			float s = std::sin(phase) + 0.16f * std::sin(phase * 3.f);
			float v = s * decay(p, 6.f) * 0.34f * attack(i - start, 48);
			data[i] = toSample((float)data[i] / 32000.f + v);
		}
	}
	loadMono(buf, data);
}

// Air moving past you: filtered noise with a bandpass-ish sweep.
static void makeWhoosh(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 0.22f);
	std::vector<sf::Int16> data(n);
	LowPass lp(0.3f);
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		// Filter opens then closes — that arc is what sells it as a movement,
		// not a static hiss.
		lp.a = 0.05f + 0.55f * std::sin(3.14159f * p);
		float env = std::sin(3.14159f * std::pow(p, 0.7f));
		data[i] = toSample(softClip(lp(frand()) * 2.6f * env * 0.8f));
	}
	loadMono(buf, data);
}

// Big, slow, low. The bomb is the loudest thing in the game and needs to
// occupy a different frequency range from every other effect or it just reads
// as "another explosion".
static void makeBoom(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 1.1f);
	std::vector<sf::Int16> data(n);
	LowPass rumble(0.045f);
	LowPass crackLp(0.5f);
	float subPhase = 0.f, subPhase2 = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;

		float f1 = 130.f * std::exp(-3.0f * p) + 26.f;
		float f2 = f1 * 1.5f;
		subPhase += TAU * f1 / SR;
		subPhase2 += TAU * f2 / SR;

		float sub = (std::sin(subPhase) * 0.9f + std::sin(subPhase2) * 0.35f) * decay(p, 2.4f);
		float body = rumble(frand()) * 5.0f * decay(p, 3.0f);
		float crack = (p < 0.02f) ? crackLp(frand()) * (1.f - p / 0.02f) * 3.0f : 0.f;

		data[i] = toSample(softClip((sub + body + crack) * 1.15f) * attack(i, 16));
	}
	loadMono(buf, data);
}

// Two-note bell, perfect fifth. Cuts through combat without being aggressive —
// it is good news, not a warning.
static void makeChime(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 0.85f);
	std::vector<sf::Int16> data(n, 0);
	const float roots[2] = { 784.f, 1174.7f };   // G5, D6
	const int   offsets[2] = { 0, (int)(SR * 0.09f) };

	for (int k = 0; k < 2; k++) {
		float ph = 0.f, ph2 = 0.f;
		for (int i = offsets[k]; i < n; i++) {
			float p = (float)(i - offsets[k]) / (float)(n - offsets[k]);
			ph += TAU * roots[k] / SR;
			ph2 += TAU * roots[k] * 2.76f / SR;      // inharmonic partial = bell
			float v = (std::sin(ph) * 0.75f + std::sin(ph2) * 0.18f)
				* decay(p, 4.2f) * 0.4f * attack(i - offsets[k], 96);
			data[i] = toSample((float)data[i] / 32000.f + v);
		}
	}
	loadMono(buf, data);
}

// Charge-up sweep that lands on a low impact — the "unleash" sound.
static void makeUltFire(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 0.8f);
	int impactAt = (int)(SR * 0.34f);
	std::vector<sf::Int16> data(n);
	LowPass lp(0.3f);
	float ph = 0.f, subPh = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		float v = 0.f;

		if (i < impactAt) {
			// Rising sweep, gets brighter and louder as it climbs
			float rp = (float)i / impactAt;
			float freq = 180.f + 1500.f * rp * rp;
			ph += TAU * freq / SR;
			v += (std::sin(ph) * 0.5f + frand() * 0.18f * rp) * (0.2f + 0.8f * rp);
		}
		else {
			// Impact + tail
			float ip = (float)(i - impactAt) / (float)(n - impactAt);
			float subFreq = 190.f * std::exp(-3.6f * ip) + 44.f;
			subPh += TAU * subFreq / SR;
			v += std::sin(subPh) * decay(ip, 3.0f) * 1.1f;
			v += lp(frand()) * decay(ip, 6.0f) * 2.2f;
		}
		data[i] = toSample(softClip(v) * attack(i, 32) * (1.f - 0.15f * p));
	}
	loadMono(buf, data);
}

// Sustained beam. Detuned saw-ish stack with vibrato and a sizzle layer on top,
// held flat through the middle so it can play under a long attack without
// sounding like a one-shot that ran out.
static void makeBeam(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 1.5f);
	std::vector<sf::Int16> data(n);
	LowPass sizzle(0.55f);
	float p1 = 0.f, p2 = 0.f, p3 = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		float t = (float)i / SR;

		// Slow vibrato so the tone breathes instead of sitting dead still
		float vib = 1.f + 0.012f * std::sin(TAU * 5.5f * t);
		float f = 116.f * vib;

		p1 += TAU * f / SR;
		p2 += TAU * f * 2.01f / SR;   // slight detune = beating = thickness
		p3 += TAU * f * 3.02f / SR;

		float core = std::sin(p1) * 0.6f + std::sin(p2) * 0.3f + std::sin(p3) * 0.18f;
		float air = sizzle(frand()) * 0.5f;

		// Fast attack, long plateau, short release
		float env = std::min(1.f, p / 0.04f) * std::min(1.f, (1.f - p) / 0.12f);

		data[i] = toSample(softClip((core + air) * 0.85f) * env);
	}
	loadMono(buf, data);
}

// 22ms tick. Deliberately tiny and high — grazing fires many times per second
// and anything longer turns into a drone.
static void makeTick(sf::SoundBuffer& buf)
{
	int n = (int)(SR * 0.022f);
	std::vector<sf::Int16> data(n);
	float ph = 0.f;
	for (int i = 0; i < n; i++) {
		float p = (float)i / n;
		ph += TAU * 1650.f / SR;
		data[i] = toSample(std::sin(ph) * decay(p, 12.f) * 0.55f * attack(i, 8));
	}
	loadMono(buf, data);
}

static void makeBgMusic(sf::SoundBuffer& buf)
{
	const int sr = 44100;
	float dur = 3.f;
	int n = (int)(sr * dur);
	std::vector<sf::Int16> data(n);
	float notes[] = { 220.f, 277.18f, 329.63f, 440.f };
	int noteLen = n / 4;
	for (int i = 0; i < n; i++) {
		float t = (float)i / sr;
		int ni = std::min(i / noteLen, 3);
		float lp = (float)(i % noteLen) / noteLen;
		float env = (lp < 0.1f) ? lp / 0.1f : (lp > 0.8f ? (1.f - lp) / 0.2f : 1.f);
		data[i] = (sf::Int16)(0.15f * env * 32767.f * std::sin(2.f * 3.14159f * notes[ni] * t));
	}
	buf.loadFromSamples(data.data(), data.size(), 1, sr);
}

static void logTextureLoad(const char* label, const char* path, bool loaded, const sf::Texture& texture)
{
	sf::Vector2u size = texture.getSize();
	debugLog(std::string(loaded ? "Loaded " : "FAILED ") + label + ": " + path +
		" (" + std::to_string(size.x) + "x" + std::to_string(size.y) + ")");
}

static void logSoundBufferLoad(const char* label, const char* path, bool loaded, const sf::SoundBuffer& buffer)
{
	debugLog(std::string(loaded ? "Loaded " : "FAILED ") + label + ": " + path +
		" (" + std::to_string(buffer.getSampleCount()) + " samples)");
}

//  INIT VARIABLE
void game::initvariable()
{
	debugLog("Initializing game variables");
	this->window = nullptr;
	this->state = GameState::INTRO;  // start with intro splash
	this->difficulty = 1;

	this->points = 0;
	this->EndGame = false;
	this->health = 10;
	this->maxHealth = 10;
	this->weaponLevel = 1;
	this->mouseHeld = false;
	this->isFiring = false;
	this->totalGears = 0;
	this->upgWeaponLevel = 1;
	this->upgMoveSpeed = 0;
	this->upgFireRate = 0;
	this->upgDashCooldown = 0;
	this->upgMaxHealth = 0;
	this->selectedShipClass = 0; // Default to Tank
	this->highestWaveReached = 0;
	this->secondaryEnergy = 100.f;
	this->secondaryEnergyMax = 100.f;

	// Perks Pool
	perkPool.clear();
	perkPool.push_back({ "BOUNCY LASERS", "LASERS REFLECT OFF EDGES", 0, sf::Color::Cyan });
	perkPool.push_back({ "GLASS CANNON", "+100% DMG, HP SET TO 1", 1, sf::Color::Red });
	perkPool.push_back({ "VAMPIRISM", "5% CHANCE HEAL ON KILL", 2, sf::Color::Magenta });
	perkPool.push_back({ "TOXIC EXHAUST", "DASH TRAIL POISONS FOES", 3, sf::Color::Green });
	perkPool.push_back({ "SEEKING SHARDS", "ENEMIES SPLIT ON DEATH", 4, sf::Color::Yellow });

	this->blkStartingExtraHealth = 0;
	this->blkPermanentSpeedBoost = 0;
	this->blkDamageMultiplier = 0;
	this->blkStartingGears = 0;

	this->loadSaveData();

	// FIX: always boot with WASD movement and manual fire � never mouse-follow
	this->autoFire = false;
	this->followMouse = false;  // explicit reset here (also set below, belt-and-suspenders)

	this->currentWave = 1;
	this->enemiesKilledInWave = 0;
	this->enemiesNeededInWave = 6;
	this->totalEnemiesSpawnedInWave = 0;
	this->inBreather = false;
	this->breatherTimer = 0.f;
	this->breatherTimerMax = 150.f;
	this->waveBannerTimer = 0.f;
	this->waveBannerTimerMax = 150.f;
	this->waveEnemySpeedBonus = 0.f;
	this->bossWaveDefeated = false;

	this->comboCount = 0;
	this->comboTimer = 0.f;
	this->comboTimerMax = 120.f;

	this->heatLevel = 0.f;
	this->heatMax = 100.f;
	this->isOverheated = false;
	this->overheatTimer = 0.f;
	this->overheatTimerMax = 120.f;

	this->shieldActive = true;
	this->shieldRechargeTimer = 0.f;
	this->shieldRechargeMax = 720.f;

	this->isDashing = false;
	this->dashTimer = 0.f;
	this->dashTimerMax = 12.f;
	this->dashCooldown = 0.f;
	this->dashCooldownMax = 70.f; // Much shorter cooldown for snappy dodging
	this->dashVelocity = sf::Vector2f(0.f, 0.f);

	this->laserFireTimerMax = 11.f; // Faster base fire rate
	this->laserFireTimer = this->laserFireTimerMax;
	this->baseLaserFireTimerMax = 11.f;

	this->invincibilityTimer = 0.f;
	this->deathRayTimer = 0.f;
	this->fastFireTimer = 0.f;
	this->slowTimeTimer = 0.f;

	this->diffEnemySpeedMult = 1.f;
	this->diffBossDamageMult = 1.f;
	this->diffPowerUpDropRate = 35.f;
	this->diffEnemyHpBonus = 0.f;

	this->basePlayerMoveSpeed = 5.f;
	this->followMouse = false;   // FIX: default = WASD
	this->autoFire = false;   // FIX: default = manual L-click
	this->overheatEnabled = true;
	this->rightMouseHeld = false;
	this->selectedSkin = 0;
	this->bossLastState = 0;
	this->overheatWasActive = false;
	this->screenShake = 0.f;
	this->screenShakeIntensity = 1.f;
	this->hitStopTimer = 0.f;
	this->timeScale = 1.0f;
	this->hyperspaceProgress = 0.f;

	this->isBossStage = false;
	this->killBannerTimer = 0.f;
	this->killBannerTimerMax = 90.f;

	// Boss 2
	this->isBoss2Stage = false;
	this->boss2Defeated = false;
	this->boss2DeathSequence = false;
	this->boss2DeathTimer = 0.f;
	this->boss2LastBeamState = false;

	// Intro
	this->introTimer = 0.f;
	this->introTextAlpha = 0.f;
	this->introMoonY = 0.f;
	this->introTargetY = 0.f;
	this->introStartY = 0.f;
	this->howToPage = 0;

	this->parryWindowTimer = 0.f;
	this->isParrying = false;

	// D1: damage flash
	this->playerDamageFlashTimer = 0.f;

	// Boss arrival cinematic
	this->bossArrivalPending = false;
	this->bossArrivalTimer = 0.f;
	this->bossArrivalWave = 0;

	// Explosion animation
	this->bossDeathTimer = 0.f;
	this->bossDeathSequence = false;

	// STATE TRANSITION FADE
	this->transitionAlpha = 0.f;
	this->transitionFadingOut = false;
	this->transitionFadingIn = false;
	this->transitionTarget = GameState::MENU;

	// GRAZE MECHANIC
	this->grazeCount = 0;
	this->grazeTimer = 0.f;
	this->grazeMultiplier = 1.f;
	this->grazeFlashTimer = 0.f;

	// BOMB ABILITY
	this->bombCount = 3;
	this->bombCooldownTimer = 0.f;
	this->bombCooldownMax = 120.f;   // 1s lockout so a double-tap can't dump two

	// COMPANION DRONES
	this->droneLevel = 0;

	// COMBO ANNOUNCER
	this->comboAnnouncerTimer = 0.f;
	this->comboAnnouncerScale = 1.f;

	// SHIP ULTIMATE
	// 1000 was unreachable — nothing ever added charge, and at the rates below
	// it would have taken most of a run. 300 lands roughly one ultimate per wave
	// for a player who grazes.
	this->ultimateCharge = 0.f;
	this->ultimateChargeMax = 300.f;
	this->ultimateActiveTimer = 0.f;
	this->ultimateActiveMax = 360.f;   // 3s at 120 Hz
	this->ultimateActive = false;
	this->ultimateReadyAnnounced = false;
	this->ultimateFlashTimer = 0.f;

	// ENDLESS LOOP
	this->loopCount = 0;
	this->loopModeAvailable = false;
	this->loopDifficultyMult = 1.f;

	// ELITE ENEMIES
	this->nextWaveHasElite = false;
	this->eliteKillCount = 0;

	// PERK SYNERGIES
	this->hasPerkSynergy = false;
	this->evolvedWeaponId = -1;
}

//  INIT WINDOW
void game::initwindow()
{
	this->dimensions.height = 1080;
	this->dimensions.width = 1920;
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	this->window = new sf::RenderWindow(this->dimensions, "Rectangle Smash", sf::Style::Default, settings);
	// Pacing lives in main()'s fixed-timestep loop. A framerate limit here would
	// fight it (SFML's limiter is a sleep, which jitters); vsync just blocks on
	// the display and lets the accumulator do the rest.
	this->window->setVerticalSyncEnabled(true);
	this->window->setKeyRepeatEnabled(false);
	this->window->setMouseCursorVisible(false); // D5: hide system cursor; we draw our own crosshair
	debugLog("Created window 1920x1080, vsync on, 120 Hz fixed simulation");
}

//  INIT PLAYER + ALL TEXTURES
void game::initPlayer()
{
	// Invisible collision triangle (bounds still used for hit detection)
	this->player.setPointCount(3);
	this->player.setPoint(0, sf::Vector2f(0.f, -30.f));
	this->player.setPoint(1, sf::Vector2f(-20.f, 20.f));
	this->player.setPoint(2, sf::Vector2f(20.f, 20.f));
	this->player.setFillColor(sf::Color::Transparent);
	this->player.setOutlineColor(sf::Color::Transparent);
	this->player.setPosition(this->window->getSize().x / 2.f, this->window->getSize().y - 100.f);

	// Player sprite — blue spiked ship (faces up naturally, correct orientation)
	const char* playerPath = "assests/spiked ship 3. small.blue_.PNG";
	bool playerLoaded = this->playerTexture.loadFromFile(playerPath);
	logTextureLoad("player texture", playerPath, playerLoaded, this->playerTexture);
	if (!playerLoaded)
		std::cout << "Failed to load player texture\n";
	this->playerTexture.setSmooth(true);
	this->playerSprite.setTexture(this->playerTexture, true);
	sf::Vector2u pts = this->playerTexture.getSize();
	this->playerSprite.setOrigin((float)pts.x / 2.f, (float)pts.y / 2.f);
	float initScl = 60.f / (float)std::max(pts.x, pts.y);
	this->playerSprite.setScale(initScl, initScl);
	this->playerSprite.setPosition(this->player.getPosition());

	// Demo ship
	this->demoShipSprite.setTexture(this->playerTexture, true);
	this->demoShipSprite.setOrigin((float)pts.x / 2.f, (float)pts.y / 2.f);
	this->demoShipSprite.setScale(initScl * 0.8f, initScl * 0.8f); // Slightly smaller
	this->demoShipSprite.setColor(sf::Color(100, 255, 255, 150)); // Ghostly blue
	this->demoShipPos = sf::Vector2f(-100.f, 300.f);
	this->demoShipVel = sf::Vector2f(2.f, 0.f);
	this->demoShootTimer = 0.f;

	// Enemy textures (pixel art — no smoothing)
	const char* enemyPaths[4] = {
		"assests/enemy/enemy_ (1).png",   // Type 0: Zigzagger
		"assests/enemy/enemy_ (2).png",   // Type 1: Shooter
		"assests/enemy/enemy_ (3).png",   // Type 2: Homing
		"assests/enemy/enemy_ (4).png"    // Type 3: Swarm
	};
	for (int i = 0; i < 4; i++) {
		std::string label = "enemy texture " + std::to_string(i);
		bool loaded = this->enemyTextures[i].loadFromFile(enemyPaths[i]);
		logTextureLoad(label.c_str(), enemyPaths[i], loaded, this->enemyTextures[i]);
		if (!loaded)
			std::cout << "Failed to load enemy texture " << i << "\n";
		this->enemyTextures[i].setSmooth(false);
	}

	// Boss texture (loaded here for hp-bar reference; Boss also loads its own)
	const char* bossPath = "assests/enemy/boss1.png";
	bool bossLoaded = this->bossTexture.loadFromFile(bossPath);
	logTextureLoad("boss texture", bossPath, bossLoaded, this->bossTexture);
	if (!bossLoaded)
		std::cout << "Failed to load boss texture\n";
	this->bossTexture.setSmooth(false);

	// Minion texture
	const char* minionPath = "assests/ship1.png";
	bool minionLoaded = this->minionTexture.loadFromFile(minionPath);
	logTextureLoad("minion texture", minionPath, minionLoaded, this->minionTexture);
	if (!minionLoaded)
		std::cout << "Failed to load minion texture\n";
	this->minionTexture.setSmooth(false);

	// Shield ring (still drawn as an sf::CircleShape overlay)
	// skin textures
	const char* skinPaths[6] = {
		"assests/spiked ship 3. small.blue_.PNG",
		"assests/spiked ship 3. small.green_.PNG",
		"assests/spiked ship 3. small.PNG",
		"assests/ship1.png",
		"assests/ship3.png",
		"assests/ship5.png"
	};
	for (int i = 0; i < 6; i++) {
		std::string label = "skin " + std::to_string(i);
		bool loaded = this->skinTextures[i].loadFromFile(skinPaths[i]);
		logTextureLoad(label.c_str(), skinPaths[i], loaded, this->skinTextures[i]);
		if (!loaded)
			std::cout << "Failed to load skin " << i << "\n";
		this->skinTextures[i].setSmooth(true);
		sf::Vector2u sz = this->skinTextures[i].getSize();
		this->skinPreviews[i].setTexture(this->skinTextures[i], true);
		this->skinPreviews[i].setOrigin(sz.x / 2.f, sz.y / 2.f);
		float previewScale = 120.f / (float)std::max(sz.x, sz.y);
		this->skinPreviews[i].setScale(previewScale, previewScale);
	}

	float cx = this->window->getSize().x / 2.f;
	this->skinPreviews[0].setPosition(cx - 250.f, 350.f);
	this->skinPreviews[1].setPosition(cx, 350.f);
	this->skinPreviews[2].setPosition(cx + 250.f, 350.f);
	this->skinPreviews[3].setPosition(cx - 250.f, 550.f);
	this->skinPreviews[4].setPosition(cx, 550.f);
	this->skinPreviews[5].setPosition(cx + 250.f, 550.f);

	this->shieldShape.setRadius(45.f);
	this->shieldShape.setOrigin(45.f, 45.f);
	this->shieldShape.setFillColor(sf::Color(0, 200, 255, 50));
	this->shieldShape.setOutlineColor(sf::Color(0, 200, 255, 180));
	this->shieldShape.setOutlineThickness(2.f);

	// Explosion spritesheet (384x48 = 8 frames x 48px)
	const char* explPath = "assests/Explosion/explosion.png";
	bool explLoaded = this->explosionTex.loadFromFile(explPath);
	this->explosionTex.setSmooth(false);
	debugLog(std::string(explLoaded ? "Loaded explosion sheet: " : "FAILED explosion sheet: ") + explPath);

	// Moon sprite for intro
	const char* moonPath = "assests/intro/pixel_moon.png";
	bool moonLoaded = this->moonTexture.loadFromFile(moonPath);
	this->moonTexture.setSmooth(false);
	if (moonLoaded) {
		sf::Vector2u ms = this->moonTexture.getSize();
		this->moonSprite.setTexture(this->moonTexture, true);
		this->moonSprite.setOrigin(ms.x / 2.f, ms.y / 2.f);
		float mscl = 1800.f / (float)std::max(ms.x, ms.y);
		this->moonSprite.setScale(mscl, mscl);
		// FIX: always start below screen and ease up to partially visible bottom edge
		float winH = (float)this->window->getSize().y;
		this->introStartY = winH + 1200.f;   // start well off-screen
		this->introMoonY = this->introStartY;
		this->introTargetY = winH + 350.f;     // adjusted for tighter title layout
		this->moonSprite.setPosition((float)this->window->getSize().x / 2.f, this->introMoonY);
	}
	debugLog(std::string(moonLoaded ? "Loaded moon: " : "FAILED moon: ") + moonPath);

	debugLog("Player, enemy, boss, minion, skin, shield, explosion, and moon visuals initialized");

	// D5: custom crosshair shapes (thin cyan cross)
	cursorVert.setSize(sf::Vector2f(2.f, 22.f));
	cursorVert.setOrigin(1.f, 11.f);
	cursorVert.setFillColor(sf::Color(0, 255, 200, 210));
	cursorHoriz.setSize(sf::Vector2f(22.f, 2.f));
	cursorHoriz.setOrigin(11.f, 1.f);
	cursorHoriz.setFillColor(sf::Color(0, 255, 200, 210));
}

//  INIT BACKGROUND STARS
void game::initBackground()
{
	for (int i = 0; i < 120; ++i) {
		Star star;
		star.shape.setRadius((float)(rand() % 2 + 1));
		star.shape.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)(rand() % 150 + 50)));
		star.shape.setPosition((float)(rand() % this->window->getSize().x),
			(float)(rand() % this->window->getSize().y));
		star.speed = (rand() % 50 + 10) / 10.f;
		this->stars.push_back(star);
	}
	debugLog("Initialized background stars: " + std::to_string(stars.size()));
}

//  INIT FONTS
void game::initfonts()
{
	const char* fontPath = "fonts/Sabo-Regular.otf";
	bool loaded = this->font.loadFromFile(fontPath);
	debugLog(std::string(loaded ? "Loaded font: " : "FAILED font: ") + fontPath);
	if (!loaded)
		std::cout << "Failed to load font\n";
}

//  INIT TEXT
void game::initText()
{
	this->UiText.setFont(this->font);
	this->UiText.setCharacterSize(22);
	this->UiText.setFillColor(sf::Color::White);
	this->UiText.setString("");
	this->UiText.setPosition(10.f, 10.f);
}

//  INIT GAME OVER TEXT
void game::initGameOverText()
{
	this->GameOverText.setFont(this->font);
	this->GameOverText.setCharacterSize(68);
	this->GameOverText.setFillColor(sf::Color(255, 90, 90));
	this->GameOverText.setOutlineColor(sf::Color::Black);
	this->GameOverText.setOutlineThickness(3.f);
	this->GameOverText.setString("GAME OVER");
	sf::FloatRect tb = this->GameOverText.getLocalBounds();
	this->GameOverText.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
	this->GameOverText.setPosition(this->window->getSize().x / 2.f, this->window->getSize().y / 2.f - 130.f);

	this->FinalScoreText.setFont(this->font);
	this->FinalScoreText.setCharacterSize(31);
	this->FinalScoreText.setFillColor(sf::Color::White);
	this->FinalScoreText.setOutlineColor(sf::Color::Black);
	this->FinalScoreText.setOutlineThickness(2.f);
	this->FinalScoreText.setString("Score: 0  |  Wave: 0");
	sf::FloatRect fb = this->FinalScoreText.getLocalBounds();
	this->FinalScoreText.setOrigin(fb.left + fb.width / 2.f, fb.top + fb.height / 2.f);
	this->FinalScoreText.setPosition(this->window->getSize().x / 2.f, this->window->getSize().y / 2.f - 30.f);
}

//  INIT MENU TEXTS
void game::initMenuTexts()
{
	float cx = (float)this->window->getSize().x / 2.f;

	this->PlayText.setFont(this->font);
	this->PlayText.setCharacterSize(35);
	this->PlayText.setFillColor(sf::Color::White);
	this->PlayText.setString("PLAY");

	this->ShipSelectText.setFont(this->font);
	this->ShipSelectText.setCharacterSize(30);
	this->ShipSelectText.setFillColor(sf::Color::White);
	this->ShipSelectText.setString("SHIP SELECT");

	this->ShopText.setFont(this->font);
	this->ShopText.setCharacterSize(30);
	this->ShopText.setFillColor(sf::Color::White);
	this->ShopText.setString("UPGRADES");

	this->SettingsText.setFont(this->font);
	this->SettingsText.setCharacterSize(30);
	this->SettingsText.setFillColor(sf::Color::White);
	this->SettingsText.setString("SETTINGS");

	this->QuitText.setFont(this->font);
	this->QuitText.setCharacterSize(30);
	this->QuitText.setFillColor(sf::Color::White);
	this->QuitText.setString("QUIT");

	this->SkinsText.setFont(this->font);
	this->SkinsText.setCharacterSize(30);
	this->SkinsText.setFillColor(sf::Color::White);
	this->SkinsText.setString("SKINS");

	this->BlackMarketText.setFont(this->font);
	this->BlackMarketText.setCharacterSize(30);
	this->BlackMarketText.setFillColor(sf::Color::White);
	this->BlackMarketText.setString("BLACK MARKET");

	auto centerText = [&](sf::Text& t, float y) {
		sf::FloatRect bounds = t.getLocalBounds();
		t.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
		t.setPosition(cx, y);
		};
	centerText(this->PlayText, 258.f);
	centerText(this->ShipSelectText, 323.f);
	centerText(this->ShopText, 388.f);
	centerText(this->BlackMarketText, 453.f);
	centerText(this->SettingsText, 518.f);
	centerText(this->SkinsText, 583.f);
	centerText(this->QuitText, 648.f);

	this->skinsTitleText.setFont(this->font);
	this->skinsTitleText.setCharacterSize(66);
	this->skinsTitleText.setFillColor(sf::Color::Cyan);
	this->skinsTitleText.setString("SELECT SHIP");
	sf::FloatRect stb = this->skinsTitleText.getLocalBounds();
	this->skinsTitleText.setOrigin(stb.left + stb.width / 2.f, stb.top + stb.height / 2.f);
	this->skinsTitleText.setPosition(cx, 120.f);

	// BUG1/2 FIX: positions must match the rowHit rectangles in updateDifficultySelect.
	// rowYs[i] = cy +/- offset where cy = window->getSize().y/2 = 540.
	// rowYs = { cy-100, cy+50, cy+200 } = { 440, 590, 740 }.
	// We center each label on its row so getGlobalBounds() aligns with the hitbox.
	auto centerDiffText = [&](sf::Text& t, float rowCY) {
		sf::FloatRect b = t.getLocalBounds();
		t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
		t.setPosition(cx, rowCY);
		};

	this->DiffEasyText.setFont(this->font);
	this->DiffEasyText.setCharacterSize(47);
	this->DiffEasyText.setFillColor(sf::Color::White);
	this->DiffEasyText.setString("EASY    (15 HP, slow)");
	centerDiffText(this->DiffEasyText, (float)this->window->getSize().y / 2.f - 100.f);

	this->DiffNormalText.setFont(this->font);
	this->DiffNormalText.setCharacterSize(47);
	this->DiffNormalText.setFillColor(sf::Color::White);
	this->DiffNormalText.setString("NORMAL  (10 HP, standard)");
	centerDiffText(this->DiffNormalText, (float)this->window->getSize().y / 2.f + 50.f);

	this->DiffHardText.setFont(this->font);
	this->DiffHardText.setCharacterSize(47);
	this->DiffHardText.setFillColor(sf::Color::White);
	this->DiffHardText.setString("HARD    (5 HP, brutal)");
	centerDiffText(this->DiffHardText, (float)this->window->getSize().y / 2.f + 200.f);

	this->SettingsTitleText.setFont(this->font);
	this->SettingsTitleText.setCharacterSize(66);
	this->SettingsTitleText.setFillColor(sf::Color::Cyan);
	this->SettingsTitleText.setString("SETTINGS");
	sf::FloatRect setb = this->SettingsTitleText.getLocalBounds();
	this->SettingsTitleText.setOrigin(setb.left + setb.width / 2.f, setb.top + setb.height / 2.f);
	this->SettingsTitleText.setPosition(cx, 120.f);

	this->highScoreTitleText.setFont(this->font);
	this->highScoreTitleText.setCharacterSize(30);
	this->highScoreTitleText.setFillColor(sf::Color::Yellow);
	this->highScoreTitleText.setString("TOP SCORES");
	this->highScoreTitleText.setPosition(cx + 420.f, 260.f);

	for (int i = 0; i < MAX_HIGH_SCORES; i++) {
		highScoreEntryTexts[i].setFont(this->font);
		highScoreEntryTexts[i].setCharacterSize(22);
		highScoreEntryTexts[i].setFillColor(sf::Color::White);
		highScoreEntryTexts[i].setPosition(cx + 390.f, 310.f + i * 36.f);
	}

	this->comboText.setFont(this->font);
	this->comboText.setCharacterSize(33);
	this->comboText.setFillColor(sf::Color::Yellow);
	this->comboText.setPosition(20.f, 80.f);

	this->waveBannerText.setFont(this->font);
	this->waveBannerText.setCharacterSize(66);
	this->waveBannerText.setFillColor(sf::Color::Cyan);
	this->waveBannerText.setString("WAVE 1");
	sf::FloatRect wb = this->waveBannerText.getLocalBounds();
	this->waveBannerText.setOrigin(wb.left + wb.width / 2.f, wb.top + wb.height / 2.f);
	this->waveBannerText.setPosition((float)this->window->getSize().x / 2.f, 200.f);

	this->killBannerText.setFont(this->font);
	this->killBannerText.setCharacterSize(57);
	this->killBannerText.setFillColor(sf::Color::Yellow);
	this->killBannerText.setPosition((float)this->window->getSize().x / 2.f, 300.f);
}

//  INIT SETTINGS TEXTS
void game::initSettingsTexts()
{
	float cx = (float)this->window->getSize().x / 2.f;
	float labelX = cx - 170.f;
	float minusX = cx + 160.f;
	float valueX = cx + 230.f;
	float plusX = cx + 310.f;

	this->PausedText.setFont(this->font);
	this->PausedText.setCharacterSize(41);
	this->PausedText.setFillColor(sf::Color::Yellow);
	this->PausedText.setString("PAUSED\nESC: RESUME\nM: MENU");
	sf::FloatRect pb = this->PausedText.getLocalBounds();
	this->PausedText.setOrigin(pb.left + pb.width / 2.f, pb.top + pb.height / 2.f);
	this->PausedText.setPosition(cx, 170.f);

	this->MoveSpeedSettingText.setFont(this->font); this->MoveSpeedSettingText.setCharacterSize(38);
	this->MoveSpeedSettingText.setString("MOVE SPEED:");
	this->MoveSpeedSettingText.setOrigin(this->MoveSpeedSettingText.getLocalBounds().left + this->MoveSpeedSettingText.getLocalBounds().width, this->MoveSpeedSettingText.getLocalBounds().top + this->MoveSpeedSettingText.getLocalBounds().height / 2.f);
	this->MoveSpeedSettingText.setPosition(labelX, 300.f);

	this->MoveSpeedMinus.setFont(this->font); this->MoveSpeedMinus.setCharacterSize(38);
	this->MoveSpeedMinus.setFillColor(sf::Color::White); this->MoveSpeedMinus.setString("-");
	this->MoveSpeedMinus.setOrigin(this->MoveSpeedMinus.getLocalBounds().left + this->MoveSpeedMinus.getLocalBounds().width / 2.f, this->MoveSpeedMinus.getLocalBounds().top + this->MoveSpeedMinus.getLocalBounds().height / 2.f);
	this->MoveSpeedMinus.setPosition(minusX, 280.f);

	this->MoveSpeedValText.setFont(this->font); this->MoveSpeedValText.setCharacterSize(38);
	this->MoveSpeedValText.setFillColor(sf::Color::White); this->MoveSpeedValText.setString("3");
	this->MoveSpeedValText.setOrigin(this->MoveSpeedValText.getLocalBounds().left + this->MoveSpeedValText.getLocalBounds().width / 2.f, this->MoveSpeedValText.getLocalBounds().top + this->MoveSpeedValText.getLocalBounds().height / 2.f);
	this->MoveSpeedValText.setPosition(valueX, 280.f);

	this->MoveSpeedPlus.setFont(this->font); this->MoveSpeedPlus.setCharacterSize(38);
	this->MoveSpeedPlus.setFillColor(sf::Color::White); this->MoveSpeedPlus.setString("+");
	this->MoveSpeedPlus.setOrigin(this->MoveSpeedPlus.getLocalBounds().left + this->MoveSpeedPlus.getLocalBounds().width / 2.f, this->MoveSpeedPlus.getLocalBounds().top + this->MoveSpeedPlus.getLocalBounds().height / 2.f);
	this->MoveSpeedPlus.setPosition(plusX, 280.f);

	this->FireSpeedSettingText.setFont(this->font); this->FireSpeedSettingText.setCharacterSize(38);
	this->FireSpeedSettingText.setFillColor(sf::Color::White);
	this->FireSpeedSettingText.setString("FIRE SPEED:");
	this->FireSpeedSettingText.setOrigin(this->FireSpeedSettingText.getLocalBounds().left + this->FireSpeedSettingText.getLocalBounds().width, this->FireSpeedSettingText.getLocalBounds().top + this->FireSpeedSettingText.getLocalBounds().height / 2.f);
	this->FireSpeedSettingText.setPosition(labelX, 390.f);

	this->FireSpeedMinus.setFont(this->font); this->FireSpeedMinus.setCharacterSize(38);
	this->FireSpeedMinus.setFillColor(sf::Color::White); this->FireSpeedMinus.setString("-");
	this->FireSpeedMinus.setOrigin(this->FireSpeedMinus.getLocalBounds().left + this->FireSpeedMinus.getLocalBounds().width / 2.f, this->FireSpeedMinus.getLocalBounds().top + this->FireSpeedMinus.getLocalBounds().height / 2.f);
	this->FireSpeedMinus.setPosition(minusX, 370.f);

	this->FireSpeedValText.setFont(this->font); this->FireSpeedValText.setCharacterSize(38);
	this->FireSpeedValText.setFillColor(sf::Color::White); this->FireSpeedValText.setString(std::to_string(fireSpeedLevelFromTimer(this->baseLaserFireTimerMax)));
	this->FireSpeedValText.setOrigin(this->FireSpeedValText.getLocalBounds().left + this->FireSpeedValText.getLocalBounds().width / 2.f, this->FireSpeedValText.getLocalBounds().top + this->FireSpeedValText.getLocalBounds().height / 2.f);
	this->FireSpeedValText.setPosition(valueX, 370.f);

	this->FireSpeedPlus.setFont(this->font); this->FireSpeedPlus.setCharacterSize(38);
	this->FireSpeedPlus.setFillColor(sf::Color::White); this->FireSpeedPlus.setString("+");
	this->FireSpeedPlus.setOrigin(this->FireSpeedPlus.getLocalBounds().left + this->FireSpeedPlus.getLocalBounds().width / 2.f, this->FireSpeedPlus.getLocalBounds().top + this->FireSpeedPlus.getLocalBounds().height / 2.f);
	this->FireSpeedPlus.setPosition(plusX, 370.f);

	auto centerText = [&](sf::Text& t, float y) {
		sf::FloatRect bounds = t.getLocalBounds();
		t.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
		t.setPosition(window->getSize().x / 2.f, y);
		};

	this->FollowMouseSettingText.setFont(this->font); this->FollowMouseSettingText.setCharacterSize(31);
	this->FollowMouseSettingText.setFillColor(sf::Color::White); this->FollowMouseSettingText.setString("MOVEMENT: MOUSE"); centerText(this->FollowMouseSettingText, 480.f);

	this->AutoFireSettingText.setFont(this->font); this->AutoFireSettingText.setCharacterSize(31);
	this->AutoFireSettingText.setFillColor(sf::Color::White); this->AutoFireSettingText.setString("FIRE: MANUAL (L-CLICK)"); centerText(this->AutoFireSettingText, 555.f);

	this->OverheatSettingText.setFont(this->font); this->OverheatSettingText.setCharacterSize(31);
	this->OverheatSettingText.setFillColor(sf::Color::Green); this->OverheatSettingText.setString("OVERHEAT: ON"); centerText(this->OverheatSettingText, 630.f);

	this->BackText.setFont(this->font); this->BackText.setCharacterSize(41);
	this->BackText.setFillColor(sf::Color::White); this->BackText.setString("BACK");
	centerText(this->BackText, 760.f);
}

//  INIT AUDIO
void game::initAudio()
{
	// real sounds from assets
	const char* laserPath = "assests/music/player_laser.ogg";
	bool laserLoaded = this->laserBuf.loadFromFile(laserPath);
	logSoundBufferLoad("laser sound", laserPath, laserLoaded, this->laserBuf);
	if (!laserLoaded)
		std::cout << "Failed to load player_laser.ogg\n";

	this->currentLaserSoundIndex = 0;
	for (int i = 0; i < MAX_LASER_SOUNDS; i++) {
		this->laserSounds[i].setBuffer(this->laserBuf);
		this->laserSounds[i].setVolume(45.f);
	}

	// boss beam — boss_lazerbeam.ogg has never existed in assests/music, so this
	// load always failed and the beam attack was silent. Synthesized instead.
	const char* bossBeamPath = "assests/music/boss_lazerbeam.ogg";
	bool bossBeamLoaded = this->bossBeamBuf.loadFromFile(bossBeamPath);
	if (!bossBeamLoaded) {
		makeBeam(this->bossBeamBuf);
		debugLog("Boss beam sound: generated (no boss_lazerbeam.ogg on disk)");
	}
	else logSoundBufferLoad("boss beam sound", bossBeamPath, bossBeamLoaded, this->bossBeamBuf);
	this->bossBeamSound.setBuffer(this->bossBeamBuf);
	this->bossBeamSound.setVolume(65.f);

	// overheat alarm — the file on disk is overheat.mp3; this asked for .ogg,
	// so the overheat warning never made a sound.
	const char* overheatPath = "assests/music/overheat.mp3";
	bool overheatLoaded = this->overheatBuf.loadFromFile(overheatPath);
	logSoundBufferLoad("overheat sound", overheatPath, overheatLoaded, this->overheatBuf);
	if (!overheatLoaded)
		std::cout << "Failed to load overheat.mp3\n";
	this->overheatSound.setBuffer(this->overheatBuf);
	this->overheatSound.setVolume(55.f);

	// generated sfx (no files for these)
	// Longer than the old 0.2s burst: an explosion that stops dead sounds like
	// a click. The tail is what gives it size.
	makeNoise(this->explosionBuf, 0.55f, 0.85f);
	this->explosionSound.setBuffer(this->explosionBuf);
	this->explosionSound.setVolume(38.f);
	debugLog("Generated explosion sound (" + std::to_string(explosionBuf.getSampleCount()) + " samples)");

	makePickup(this->powerUpBuf);
	this->powerUpSound.setBuffer(this->powerUpBuf);
	this->powerUpSound.setVolume(35.f);
	debugLog("Generated power-up sound (" + std::to_string(powerUpBuf.getSampleCount()) + " samples)");

	makeImpact(this->hitBuf);
	this->hitSound.setBuffer(this->hitBuf);
	this->hitSound.setVolume(50.f);
	debugLog("Generated hit sound (" + std::to_string(hitBuf.getSampleCount()) + " samples)");

	makeWhoosh(this->dashBuf);
	this->dashSound.setBuffer(this->dashBuf);
	this->dashSound.setVolume(34.f);
	debugLog("Generated dash sound (" + std::to_string(dashBuf.getSampleCount()) + " samples)");

	// ── Ability sounds ───────────────────────────────────────────────
	makeBoom(this->bombBuf);
	this->bombSound.setBuffer(this->bombBuf);
	this->bombSound.setVolume(80.f);

	makeChime(this->ultReadyBuf);
	this->ultReadySound.setBuffer(this->ultReadyBuf);
	this->ultReadySound.setVolume(50.f);

	makeUltFire(this->ultFireBuf);
	this->ultFireSound.setBuffer(this->ultFireBuf);
	this->ultFireSound.setVolume(70.f);

	makeTick(this->grazeBuf);
	this->currentGrazeSoundIndex = 0;
	for (int i = 0; i < MAX_GRAZE_SOUNDS; i++) {
		this->grazeSounds[i].setBuffer(this->grazeBuf);
		this->grazeSounds[i].setVolume(26.f);
	}
	debugLog("Generated ability sounds (bomb / ult ready / ult fire / graze)");

	// wave clear jingle
	{
		const int sr = 44100;
		float notes[] = { 523.25f, 659.25f, 783.99f };
		int noteLen = (int)(sr * 0.12f);
		int total = noteLen * 3;
		std::vector<sf::Int16> data(total);
		for (int i = 0; i < total; i++) {
			int ni = i / noteLen;
			float t = (float)i / sr;
			float lp = (float)(i % noteLen) / noteLen;
			float env = (lp < 0.1f) ? lp / 0.1f : (1.f - lp);
			data[i] = (sf::Int16)(env * 0.5f * 32767.f * std::sin(2.f * 3.14159f * notes[ni] * t));
		}
		this->waveCompleteBuf.loadFromSamples(data.data(), data.size(), 1, sr);
		this->waveCompleteSound.setBuffer(this->waveCompleteBuf);
		this->waveCompleteSound.setVolume(45.f);
		debugLog("Generated wave-complete sound (" + std::to_string(waveCompleteBuf.getSampleCount()) + " samples)");
	}

	// background music — stream directly from mp3
	const char* bgPath = "assests/music/background_sound.mp3";
	bool bgLoaded = this->bgMusic.openFromFile(bgPath);
	debugLog(std::string(bgLoaded ? "Loaded background music: " : "FAILED background music: ") + bgPath);
	if (!bgLoaded)
		std::cout << "Failed to load background_sound.mp3\n";
	this->bgMusic.setLoop(true);
	this->bgMusic.setVolume(18.f);
	this->bgMusic.play();
	// boss background music
	const char* bossMusicPath = "assests/music/boss_music.mp3";
	bool bossMusicLoaded = this->bossBgMusic.openFromFile(bossMusicPath);
	debugLog(std::string(bossMusicLoaded ? "Loaded boss music: " : "FAILED boss music (will use generated): ") + bossMusicPath);
	this->bossBgMusic.setLoop(true);
	this->bossBgMusic.setVolume(0.f);

	// destruction boss sound
	const char* destructPath = "assests/Explosion/destruction_boss.mp3";
	bool destructLoaded = this->destructionBuf.loadFromFile(destructPath);
	if (!destructLoaded) {
		// fallback: generate loud noise burst
		makeNoise(this->destructionBuf, 0.6f, 1.5f);
		debugLog("Boss destruction sound: generated fallback");
	}
	else debugLog("Loaded boss destruction sound");
	this->destructionSound.setBuffer(this->destructionBuf);
	this->destructionSound.setVolume(95.f);

	// boss laser wav
	const char* bossLazerPath = "assests/music/boss_lazer.wav";
	bool bossLazerLoaded = this->bossLazerBuf2.loadFromFile(bossLazerPath);
	if (!bossLazerLoaded) {
		makeTone(this->bossLazerBuf2, 120.f, 0.5f, 0.8f, true, 60.f);
		debugLog("Boss laser sound: generated fallback");
	}
	else debugLog("Loaded boss laser wav");
	this->bossLazerSound2.setBuffer(this->bossLazerBuf2);
	this->bossLazerSound2.setVolume(55.f);

	debugLog("Audio initialized and background music started");
}

//  HIGH SCORES
void game::initHighScores()
{
	for (int i = 0; i < MAX_HIGH_SCORES; i++) { highScores[i] = 0; highScoreWaves[i] = 0; }
	loadHighScore();
}

void game::saveHighScore()
{
	std::ofstream file("highscores.txt");
	if (file.is_open()) {
		for (int i = 0; i < MAX_HIGH_SCORES; i++)
			file << highScores[i] << " " << highScoreWaves[i] << "\n";
		debugLog("Saved highscores.txt");
	}
	else {
		debugLog("FAILED to open highscores.txt for saving");
	}
}

void game::loadHighScore()
{
	std::ifstream file("highscores.txt");
	if (file.is_open()) {
		for (int i = 0; i < MAX_HIGH_SCORES; i++)
			file >> highScores[i] >> highScoreWaves[i];
		debugLog("Loaded highscores.txt");
	}
	else {
		debugLog("No highscores.txt found; using empty high score table");
	}
}

void game::saveGameData()
{
	std::ofstream file("savedata.txt");
	if (file.is_open()) {
		file << totalGears << "\n";
		file << upgWeaponLevel << "\n";
		file << upgMoveSpeed << "\n";
		file << upgFireRate << "\n";
		file << upgDashCooldown << "\n";
		file << upgMaxHealth << "\n";
		file << highestWaveReached << "\n";
		file << blkStartingExtraHealth << "\n";
		file << blkPermanentSpeedBoost << "\n";
		file << blkDamageMultiplier << "\n";
		file << blkStartingGears << "\n";
		debugLog("Saved savedata.txt");
	}
	else {
		debugLog("FAILED to open savedata.txt for saving");
	}
}

void game::loadSaveData()
{
	std::ifstream file("savedata.txt");
	if (file.is_open()) {
		file >> totalGears;
		file >> upgWeaponLevel;
		file >> upgMoveSpeed;
		file >> upgFireRate;
		file >> upgDashCooldown;
		file >> upgMaxHealth;
		if (!(file >> highestWaveReached)) highestWaveReached = 0;
		if (!(file >> blkStartingExtraHealth)) blkStartingExtraHealth = 0;
		if (!(file >> blkPermanentSpeedBoost)) blkPermanentSpeedBoost = 0;
		if (!(file >> blkDamageMultiplier)) blkDamageMultiplier = 0;
		if (!(file >> blkStartingGears)) blkStartingGears = 0;
		debugLog("Loaded savedata.txt");
	}
	else {
		debugLog("No savedata.txt found; starting fresh");
	}
}

//  DIFFICULTY
void game::applyDifficultySettings(int diff)
{
	switch (diff) {
	case 0: diffEnemySpeedMult = 0.45f; diffBossDamageMult = 2.5f;  diffPowerUpDropRate = 55.f; diffEnemyHpBonus = -2.f; break;
	case 1: diffEnemySpeedMult = 1.0f;  diffBossDamageMult = 1.0f;  diffPowerUpDropRate = 35.f; diffEnemyHpBonus = 0.f;  break;
	case 2: diffEnemySpeedMult = 1.5f;  diffBossDamageMult = 0.5f;  diffPowerUpDropRate = 5.f;  diffEnemyHpBonus = 2.f;  break;
	}

	// Apply Ship Class Base Stats
	if (selectedShipClass == 0) {
		// Tank
		maxHealth = 15;
		basePlayerMoveSpeed = 3.0f;
		baseLaserFireTimerMax = 15.f;
		dashCooldownMax = 70.f;
	}
	else if (selectedShipClass == 1) {
		// Speedster
		maxHealth = 5;
		basePlayerMoveSpeed = 7.0f;
		baseLaserFireTimerMax = 11.f;
		dashCooldownMax = 20.f;
	}
	else if (selectedShipClass == 2) {
		// Sniper
		maxHealth = 8;
		basePlayerMoveSpeed = 4.5f;
		baseLaserFireTimerMax = 25.f;
		dashCooldownMax = 70.f;
	}

	// Easy mode: bonus HP on top of ship base + faster dash recharge
	if (diff == 0) {
		maxHealth += 5;
		dashCooldownMax *= 0.6f;
		baseLaserFireTimerMax *= 0.8f; // slightly faster fire on easy
	}

	// Apply Shop Upgrades
	maxHealth += upgMaxHealth * 2;
	health = maxHealth;

	basePlayerMoveSpeed += upgMoveSpeed * 0.5f;
	baseLaserFireTimerMax -= upgFireRate * 2.f;
	dashCooldownMax -= upgDashCooldown * 5.f;
	weaponLevel = upgWeaponLevel;

	// Clamp values
	// BUG4 FIX: floor raised from 3 → 6.  At timer=3 with 7 shots the weapon fires
	// ~140 projectiles/second (continuous beam).  6 still feels very fast (~70/s)
	// but remains clearly discrete bursts.
	if (baseLaserFireTimerMax < 6.f) baseLaserFireTimerMax = 6.f;
	if (dashCooldownMax < 5.f) dashCooldownMax = 5.f;

	laserFireTimerMax = baseLaserFireTimerMax;
	laserFireTimer = laserFireTimerMax;

	debugLog("Applied difficulty & ship stats | class=" + std::to_string(selectedShipClass) +
		" health=" + std::to_string(health) +
		" speed=" + std::to_string(basePlayerMoveSpeed) +
		" fireRate=" + std::to_string(baseLaserFireTimerMax));
}

//  CONSTRUCTOR / DESTRUCTOR
game::game()
{
	debugLog("Game constructor started");
	this->initvariable();
	this->initwindow();
	this->initfonts();
	this->initText();
	this->initGameOverText();
	this->initMenuTexts();
	this->initSettingsTexts();
	this->initPlayer();
	this->initBackground();
	this->initAudio();
	this->initHighScores();
	this->bossEntity.initBoss(this->window, this->difficulty);
	this->boss2Entity.initBoss2(this->window, this->difficulty);
	debugLog("Game constructor finished");
}

game::~game() { delete this->window; }

const bool game::running()    const { return this->window->isOpen(); }
const bool game::getEndGame() const { return this->EndGame; }