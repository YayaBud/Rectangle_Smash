// game_render.cpp all render functions
#include "game.h"

static void draw3DText(sf::RenderTarget& target, sf::Text text) {
    sf::Color originalColor = text.getFillColor();
    sf::Vector2f basePos = text.getPosition();

    // Glow (Draw 4 times around)
    sf::Color glowColor = originalColor;
    glowColor.a = 50;
    text.setFillColor(glowColor);
    text.setPosition(basePos.x + 1, basePos.y + 1); target.draw(text);
    text.setPosition(basePos.x - 1, basePos.y - 1); target.draw(text);
    text.setPosition(basePos.x + 1, basePos.y - 1); target.draw(text);
    text.setPosition(basePos.x - 1, basePos.y + 1); target.draw(text);

    // 3D Extrusion
    sf::Color shadowColor(0, originalColor.g / 2, originalColor.b / 2, 255);
    if (originalColor == sf::Color::White || originalColor == sf::Color::Transparent) {
        shadowColor = sf::Color(100, 100, 100);
    }
    text.setFillColor(shadowColor);
    int offset = std::max(1, (int)(text.getCharacterSize() / 15));
    for(int i = offset; i > 0; --i) {
        text.setPosition(basePos.x + i, basePos.y + i);
        target.draw(text);
    }

    // Top Face
    text.setFillColor(originalColor);
    text.setPosition(basePos);
    target.draw(text);
}


namespace {
	float gRenderFrame = 0.f;

	float pulse(float speed, float low = 0.f, float high = 1.f)
	{
		float t = (std::sin(gRenderFrame * speed) + 1.f) * 0.5f;
		return low + (high - low) * t;
	}

	void centerOrigin(sf::Text& text)
	{
		sf::FloatRect b = text.getLocalBounds();
		text.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
	}

	void drawGlowText(sf::RenderTarget& target, sf::Text text, sf::Color fill, sf::Color glow, float glowScale = 1.08f)
	{
		sf::Text halo = text;
		halo.setFillColor(glow);
		float offset = (glowScale - 1.f) * 30.f; // convert scale factor to pixel offset roughly
		if (offset < 1.f) offset = 2.f;

		// Draw 4 directional offsets for a clean glow/outline
		halo.setPosition(text.getPosition() + sf::Vector2f(-offset, 0.f)); target.draw(halo);
		halo.setPosition(text.getPosition() + sf::Vector2f(offset, 0.f)); target.draw(halo);
		halo.setPosition(text.getPosition() + sf::Vector2f(0.f, -offset)); target.draw(halo);
		halo.setPosition(text.getPosition() + sf::Vector2f(0.f, offset)); target.draw(halo);

		text.setFillColor(fill);
		draw3DText(target, text);
	}

	void drawPanel(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, sf::Color edge, sf::Uint8 fillAlpha = 90)
	{
		sf::RectangleShape shadow(size + sf::Vector2f(14.f, 14.f));
		shadow.setOrigin((size.x + 14.f) / 2.f, (size.y + 14.f) / 2.f);
		shadow.setPosition(pos + sf::Vector2f(7.f, 9.f));
		shadow.setFillColor(sf::Color(0, 0, 0, 125));
		target.draw(shadow);

		sf::RectangleShape panel(size);
		panel.setOrigin(size.x / 2.f, size.y / 2.f);
		panel.setPosition(pos);
		panel.setFillColor(sf::Color(10, 18, 38, fillAlpha));
		panel.setOutlineColor(edge);
		panel.setOutlineThickness(2.f);
		target.draw(panel);

		sf::RectangleShape top(sf::Vector2f(size.x - 18.f, 3.f));
		top.setOrigin((size.x - 18.f) / 2.f, 1.5f);
		top.setPosition(pos.x, pos.y - size.y / 2.f + 10.f);
		top.setFillColor(sf::Color(edge.r, edge.g, edge.b, 180));
		target.draw(top);
	}

	void drawBar(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, float ratio, sf::Color fill)
	{
		ratio = std::max(0.f, std::min(ratio, 1.f));
		sf::RectangleShape bg(size);
		bg.setPosition(pos);
		bg.setFillColor(sf::Color(18, 22, 35, 210));
		bg.setOutlineColor(sf::Color(110, 130, 160, 130));
		bg.setOutlineThickness(1.f);
		target.draw(bg);

		sf::RectangleShape glow(sf::Vector2f(size.x * ratio, size.y));
		glow.setPosition(pos);
		glow.setFillColor(sf::Color(fill.r, fill.g, fill.b, 65));
		glow.setScale(1.f, 1.8f);
		target.draw(glow);

		sf::RectangleShape fg(sf::Vector2f(size.x * ratio, size.y));
		fg.setPosition(pos);
		fg.setFillColor(fill);
		target.draw(fg);
	}
}

void game::renderDemoBackground(sf::RenderTarget& target)
{
	for (auto& l : demoLasers) {
		target.draw(l.shape);
	}
	target.draw(demoShipSprite);
}

void game::renderBackground(sf::RenderTarget& target)
{
	gRenderFrame += 1.f;

	sf::RectangleShape bg(sf::Vector2f((float)window->getSize().x, (float)window->getSize().y));
	bg.setFillColor(sf::Color(3, 6, 20));
	target.draw(bg);

	for (auto& star : stars) {
		sf::CircleShape stretched = star.shape;
		if (hyperspaceProgress > 0.01f) {
			float stretch = 1.f + hyperspaceProgress * 15.f;
			stretched.setScale(1.f, stretch);
		}
		target.draw(stretched);
	}
}

void game::renderMenu(sf::RenderTarget& target)
{
	renderDemoBackground(target);

	sf::Vector2f cx((float)window->getSize().x / 2.f, 0.f);
	drawPanel(target, sf::Vector2f(cx.x, 485.f), sf::Vector2f(500.f, 500.f), sf::Color(0, 210, 255, 115), 42);

	sf::Text title;
	title.setFont(font); title.setCharacterSize(78);
	title.setFillColor(sf::Color::Cyan); title.setString("RECTANGLE SMASH");
	centerOrigin(title);
	title.setPosition((float)window->getSize().x / 2.f, 145.f + pulse(0.025f, -6.f, 6.f));
	title.setScale(1.f + pulse(0.018f, 0.f, 0.03f), 1.f + pulse(0.018f, 0.f, 0.03f));
	drawGlowText(target, title, sf::Color(225, 255, 255), sf::Color(0, 210, 255, 95), 1.06f);

	auto drawMenuItem = [&](sf::Text& t, float y) {
		centerOrigin(t);
		t.setPosition((float)window->getSize().x / 2.f, y);
		draw3DText(target, t);
		};
	drawMenuItem(PlayText, 288.f);
	drawMenuItem(ShipSelectText, 353.f);
	drawMenuItem(ShopText, 418.f);
	drawMenuItem(BlackMarketText, 483.f);
	drawMenuItem(SettingsText, 548.f);
	drawMenuItem(SkinsText, 613.f);
	drawMenuItem(QuitText, 678.f);

	// Settings summary removed from menu � belongs on settings screen only

	sf::Text bestWave;
	bestWave.setFont(font); bestWave.setCharacterSize(20);
	bestWave.setFillColor(sf::Color(255, 220, 0));
	bestWave.setString("BEST WAVE: " + std::to_string(highestWaveReached));
	centerOrigin(bestWave);
	bestWave.setPosition((float)window->getSize().x / 2.f, 210.f);
	target.draw(bestWave);
}

void game::renderHighScores(sf::RenderTarget& target)
{
	draw3DText(target, highScoreTitleText);
	for (int i = 0; i < MAX_HIGH_SCORES; i++) draw3DText(target, highScoreEntryTexts[i]);
}

void game::renderShipSelect(sf::RenderTarget& target)
{
	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;

	// Background panel
	drawPanel(target, sf::Vector2f(cx, cy + 30.f), sf::Vector2f(1100.f, 520.f), sf::Color(0, 210, 255, 110), 45);

	sf::Text title;
	title.setFont(font); title.setCharacterSize(72); title.setFillColor(sf::Color::Cyan);
	title.setString("SELECT SHIP");
	centerOrigin(title);
	title.setPosition(cx, 130.f);
	drawGlowText(target, title, sf::Color(240, 255, 255), sf::Color(0, 210, 255, 80), 1.06f);

	const char* names[] = { "THE TANK", "SPEEDSTER", "THE SNIPER" };
	const char* descs[] = {
		"HIGH HP\nSLOW MOVEMENT\nWIDE SHOTS",
		"LOW HP\nFAST MOVEMENT\nQUICK DASH",
		"MEDIUM HP\nPIERCING LASERS\nSLOW FIRE"
	};
	sf::Color cardColors[] = { sf::Color::Yellow, sf::Color::Cyan, sf::Color(180, 100, 255) };

	float cardW = 300.f, cardH = 380.f;
	float spacing = 350.f;

	for (int i = 0; i < 3; i++) {
		float bx = cx - spacing + i * spacing;
		bool selected = (selectedShipClass == i);
		bool hovered = sf::FloatRect(bx - cardW / 2.f, cy - cardH / 2.f + 30.f, cardW, cardH).contains(mousePosView);

		// Drop shadow
		sf::RectangleShape shadow(sf::Vector2f(cardW + 10.f, cardH + 10.f));
		shadow.setOrigin((cardW + 10.f) / 2.f, (cardH + 10.f) / 2.f);
		shadow.setPosition(bx + 6.f, cy + 36.f);
		shadow.setFillColor(sf::Color(0, 0, 0, 100));
		target.draw(shadow);

		sf::RectangleShape panel(sf::Vector2f(cardW, cardH));
		panel.setOrigin(cardW / 2.f, cardH / 2.f);
		panel.setPosition(bx, cy + 30.f);
		panel.setFillColor(sf::Color(8, 16, 40, selected ? 220 : 170));
		panel.setOutlineColor(selected ? cardColors[i] : sf::Color(60, 80, 120, 180));
		panel.setOutlineThickness(selected ? 4.f : 1.5f);
		if (hovered && !selected) panel.setOutlineColor(sf::Color(cardColors[i].r, cardColors[i].g, cardColors[i].b, 130));
		target.draw(panel);

		// Top accent bar
		sf::RectangleShape bar(sf::Vector2f(cardW - 20.f, 3.f));
		bar.setOrigin((cardW - 20.f) / 2.f, 1.5f);
		bar.setPosition(bx, cy + 30.f - cardH / 2.f + 10.f);
		bar.setFillColor(sf::Color(cardColors[i].r, cardColors[i].g, cardColors[i].b, selected ? 220 : 80));
		target.draw(bar);

		// Ship name
		sf::Text tName;
		tName.setFont(font); tName.setCharacterSize(26);
		tName.setFillColor(selected ? cardColors[i] : sf::Color(200, 220, 255));
		tName.setString(names[i]);
		centerOrigin(tName);
		tName.setPosition(bx, cy + 30.f - cardH / 2.f + 55.f);
		draw3DText(target, tName);

		// Separator
		sf::RectangleShape sep2(sf::Vector2f(cardW - 40.f, 1.f));
		sep2.setOrigin((cardW - 40.f) / 2.f, 0.5f);
		sep2.setPosition(bx, cy + 30.f - cardH / 2.f + 90.f);
		sep2.setFillColor(sf::Color(cardColors[i].r, cardColors[i].g, cardColors[i].b, 60));
		target.draw(sep2);

		// Description
		sf::Text tDesc;
		tDesc.setFont(font); tDesc.setCharacterSize(16);
		tDesc.setFillColor(sf::Color(160, 190, 230));
		tDesc.setString(descs[i]);
		tDesc.setLineSpacing(1.5f);
		sf::FloatRect db = tDesc.getLocalBounds();
		tDesc.setOrigin(db.left + db.width / 2.f, db.top);
		tDesc.setPosition(bx, cy + 30.f - cardH / 2.f + 110.f);
		draw3DText(target, tDesc);

		// Selected indicator
		if (selected) {
			sf::Text sel;
			sel.setFont(font); sel.setCharacterSize(18);
			sel.setFillColor(cardColors[i]);
			sel.setString("- SELECTED -");
			centerOrigin(sel);
			sel.setPosition(bx, cy + 30.f + cardH / 2.f - 35.f);
			target.draw(sel);
		}
	}

	// Back button
	sf::RectangleShape backBtn(sf::Vector2f(220.f, 60.f));
	backBtn.setOrigin(110.f, 30.f);
	backBtn.setPosition(cx, cy + cardH / 2.f + 90.f);
	bool backHover = backBtn.getGlobalBounds().contains(mousePosView);
	backBtn.setFillColor(sf::Color(8, 20, 50, 200));
	backBtn.setOutlineColor(backHover ? sf::Color::Cyan : sf::Color(60, 80, 120));
	backBtn.setOutlineThickness(2.f);
	target.draw(backBtn);
	sf::Text backT;
	backT.setFont(font); backT.setCharacterSize(28);
	backT.setFillColor(backHover ? sf::Color::Cyan : sf::Color::White);
	backT.setString("BACK");
	centerOrigin(backT);
	backT.setPosition(cx, cy + cardH / 2.f + 90.f);
	target.draw(backT);
}

void game::renderShop(sf::RenderTarget& target)
{
	sf::Text title;
	title.setFont(font); title.setCharacterSize(70); title.setFillColor(sf::Color::Cyan);
	title.setString("UPGRADES");
	centerOrigin(title);
	title.setPosition((float)window->getSize().x / 2.f, 100.f);
	drawGlowText(target, title, sf::Color(240, 255, 255), sf::Color(0, 210, 255, 80), 1.06f);

	sf::Text gearsText;
	gearsText.setFont(font); gearsText.setCharacterSize(30);
	gearsText.setFillColor(sf::Color::Yellow);
	gearsText.setString("GEARS: " + std::to_string(totalGears));
	centerOrigin(gearsText);
	gearsText.setPosition((float)window->getSize().x / 2.f, 170.f);
	draw3DText(target, gearsText);

	float cx = (float)window->getSize().x / 2.f;
	float startY = 250.f;
	const char* names[] = { "MULTI-SHOT", "MOVE SPEED", "FIRE RATE", "DASH COOLDOWN", "MAX HEALTH" };
	int* upgrades[] = { &upgWeaponLevel, &upgMoveSpeed, &upgFireRate, &upgDashCooldown, &upgMaxHealth };
	int baseCost[] = { 100, 50, 60, 50, 80 };

	for (int i = 0; i < 5; i++) {
		float y = startY + i * 70.f;

		sf::Text tName;
		tName.setFont(font); tName.setCharacterSize(24);
		tName.setFillColor(sf::Color::White);
		tName.setString(names[i]);
		tName.setOrigin(0.f, tName.getLocalBounds().top + tName.getLocalBounds().height / 2.f);
		tName.setPosition(cx - 300.f, y);
		draw3DText(target, tName);

		// Draw blocks for level
		for (int lvl = 0; lvl < 5; lvl++) {
			sf::RectangleShape block(sf::Vector2f(20.f, 20.f));
			block.setPosition(cx - 50.f + lvl * 25.f, y - 10.f);
			block.setFillColor(lvl < *upgrades[i] ? sf::Color::Cyan : sf::Color(30, 30, 50));
			block.setOutlineColor(sf::Color::White);
			block.setOutlineThickness(1.f);
			target.draw(block);
		}

		// Buy button
		sf::RectangleShape btn(sf::Vector2f(150.f, 40.f));
		btn.setOrigin(75.f, 20.f);
		btn.setPosition(cx + 175.f, y);
		btn.setFillColor(sf::Color(0, 150, 200, 150));
		btn.setOutlineColor(sf::Color::Cyan);
		btn.setOutlineThickness(2.f);
		target.draw(btn);

		sf::Text tCost;
		tCost.setFont(font); tCost.setCharacterSize(16);
		if (*upgrades[i] >= 5) {
			tCost.setString("MAXED");
			tCost.setFillColor(sf::Color::Green);
		}
		else {
			int cost = baseCost[i] * (*upgrades[i] + 1);
			tCost.setString(std::to_string(cost) + " G");
			tCost.setFillColor(totalGears >= cost ? sf::Color::Yellow : sf::Color::Red);
		}
		centerOrigin(tCost);
		tCost.setPosition(cx + 175.f, y);
		draw3DText(target, tCost);
	}

	draw3DText(target, BackText);
}

void game::renderDifficultySelect(sf::RenderTarget& target)
{
	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;

	// Background panel — centered on the rows (cy-100 to cy+200, center = cy+50, height ~450)
	drawPanel(target, sf::Vector2f(cx, cy + 50.f), sf::Vector2f(960.f, 460.f), sf::Color(0, 200, 255, 100), 42);

	sf::Text title;
	title.setFont(font); title.setCharacterSize(70); title.setFillColor(sf::Color::Cyan);
	title.setString("SELECT DIFFICULTY");
	centerOrigin(title);
	title.setPosition(cx, 130.f);
	drawGlowText(target, title, sf::Color(240, 255, 255), sf::Color(0, 210, 255, 80), 1.06f);

	// Difficulty options as styled rows
	struct DiffOption { sf::Text& t; sf::Color accent; std::string detail; };
	const char* details[] = { "15 HP  |  SLOW ENEMIES  |  MORE DROPS", "10 HP  |  STANDARD  |  BALANCED", "5 HP  |  BRUTAL ENEMIES  |  FEW DROPS" };
	sf::Color accents[] = { sf::Color(80, 255, 120), sf::Color(255, 220, 60), sf::Color(255, 80, 80) };
	sf::Text* texts[] = { &DiffEasyText, &DiffNormalText, &DiffHardText };

	float rowY = cy - 100.f;
	for (int i = 0; i < 3; i++) {
		bool hov = texts[i]->getGlobalBounds().contains(mousePosView);

		sf::RectangleShape row(sf::Vector2f(840.f, 90.f));
		row.setOrigin(420.f, 45.f);
		row.setPosition(cx, rowY);
		row.setFillColor(sf::Color(accents[i].r, accents[i].g, accents[i].b, hov ? 30 : 12));
		row.setOutlineColor(sf::Color(accents[i].r, accents[i].g, accents[i].b, hov ? 200 : 70));
		row.setOutlineThickness(hov ? 2.5f : 1.f);
		target.draw(row);

		// Left accent bar
		sf::RectangleShape bar(sf::Vector2f(4.f, 70.f));
		bar.setOrigin(2.f, 35.f);
		bar.setPosition(cx - 420.f + 10.f, rowY);
		bar.setFillColor(sf::Color(accents[i].r, accents[i].g, accents[i].b, hov ? 255 : 120));
		target.draw(bar);

		// Name
		sf::Text nameT = *texts[i];
		nameT.setCharacterSize(hov ? 46 : 40);
		nameT.setFillColor(accents[i]);
		sf::FloatRect nb = nameT.getLocalBounds();
		nameT.setOrigin(0.f, nb.top + nb.height / 2.f);
		nameT.setPosition(cx - 390.f, rowY - 12.f);
		draw3DText(target, nameT);

		// Detail
		sf::Text detT;
		detT.setFont(font); detT.setCharacterSize(16);
		detT.setFillColor(sf::Color(180, 200, 230, hov ? 255 : 180));
		detT.setString(details[i]);
		sf::FloatRect db = detT.getLocalBounds();
		detT.setOrigin(0.f, db.top);
		detT.setPosition(cx - 390.f, rowY + 14.f);
		draw3DText(target, detT);

		rowY += 150.f;
	}

	// Back button — below panel bottom (cy+50+230=cy+280)
	sf::RectangleShape backBtn(sf::Vector2f(220.f, 60.f));
	backBtn.setOrigin(110.f, 30.f);
	backBtn.setPosition(cx, cy + 310.f);
	bool backHov = backBtn.getGlobalBounds().contains(mousePosView);
	backBtn.setFillColor(sf::Color(8, 20, 50, 200));
	backBtn.setOutlineColor(backHov ? sf::Color::Cyan : sf::Color(60, 80, 120));
	backBtn.setOutlineThickness(2.f);
	target.draw(backBtn);
	sf::Text backT;
	backT.setFont(font); backT.setCharacterSize(28);
	backT.setFillColor(backHov ? sf::Color::Cyan : sf::Color::White);
	backT.setString("BACK");
	centerOrigin(backT);
	backT.setPosition(cx, cy + 310.f);
	target.draw(backT);
}

void game::renderSettings(sf::RenderTarget& target)
{
	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;

	// Single clean background panel
	drawPanel(target, sf::Vector2f(cx, cy + 30.f), sf::Vector2f(760.f, 580.f), sf::Color(0, 210, 255, 115), 48);

	drawGlowText(target, SettingsTitleText, sf::Color(235, 255, 255), sf::Color(0, 230, 255, 80), 1.06f);

	// Helper: draw label on left, minus/val/plus on right � no row box
	auto drawRow = [&](const std::string& label, sf::Text& minus, sf::Text& val, sf::Text& plus, float y) {
		sf::Text lbl;
		lbl.setFont(font); lbl.setCharacterSize(26);
		lbl.setFillColor(sf::Color(180, 220, 255));
		lbl.setString(label);
		sf::FloatRect lb = lbl.getLocalBounds();
		lbl.setOrigin(lb.left + lb.width / 2.f, lb.top + lb.height / 2.f);
		lbl.setPosition(cx - 140.f, y);
		draw3DText(target, lbl);

		// Reposition controls relative to cx
		minus.setPosition(cx + 155.f, y);
		val.setPosition(cx + 215.f, y);
		plus.setPosition(cx + 275.f, y);
		target.draw(minus);
		draw3DText(target, val);
		target.draw(plus);
		};

	drawRow("MOVE SPEED:", MoveSpeedMinus, MoveSpeedValText, MoveSpeedPlus, 345.f);
	drawRow("FIRE SPEED:", FireSpeedMinus, FireSpeedValText, FireSpeedPlus, 425.f);

	// Thin separator
	sf::RectangleShape sep(sf::Vector2f(660.f, 1.f));
	sep.setOrigin(330.f, 0.5f);
	sep.setPosition(cx, 480.f);
	sep.setFillColor(sf::Color(0, 180, 255, 55));
	target.draw(sep);

	// Toggle rows � just the text, no box
	auto drawToggle = [&](sf::Text& t, float y) {
		bool hov = t.getGlobalBounds().contains(mousePosView);
		t.setFillColor(hov ? sf::Color::Cyan : sf::Color(200, 220, 255));
		sf::FloatRect tb = t.getLocalBounds();
		t.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
		t.setPosition(cx, y);
		draw3DText(target, t);
		};
	drawToggle(FollowMouseSettingText, 530.f);
	drawToggle(AutoFireSettingText, 600.f);
	drawToggle(OverheatSettingText, 670.f);

	// Back button � centered, no box
	sf::FloatRect bb = BackText.getLocalBounds();
	BackText.setOrigin(bb.left + bb.width / 2.f, bb.top + bb.height / 2.f);
	BackText.setPosition(cx, 770.f);
	bool backHov = BackText.getGlobalBounds().contains(mousePosView);
	BackText.setFillColor(backHov ? sf::Color::Cyan : sf::Color::White);
	draw3DText(target, BackText);
}

void game::renderSecondaryWeapons(sf::RenderTarget& target)
{
	for (auto& m : missiles) target.draw(m.shape);
	for (auto& m : mines) target.draw(m.shape);
}

void game::renderText(sf::RenderTarget& target)
{
	sf::Text shadow = UiText;
	shadow.move(3.f, 4.f);
	shadow.setFillColor(sf::Color(0, 0, 0, 170));
	target.draw(shadow);
	draw3DText(target, UiText);
}

// ---- Enemies: draw sprite (already synced in update), then HP bar ----
void game::renderEnemies(sf::RenderTarget& target)
{
	for (auto& e : enemies) {
		sf::Uint8 alpha = (sf::Uint8)e.ghostAlpha;
		float hpRatio = (float)e.hp / (float)e.maxHp;
		float damagePulse = (e.flashTimer > 0) ? 1.f : 0.f;
		float auraScale = 0.42f + (1.f - hpRatio) * 0.08f;

		sf::CircleShape aura(e.shape.getSize().x * auraScale + pulse(0.12f, 0.f, 4.f));
		aura.setOrigin(aura.getRadius(), aura.getRadius());
		aura.setPosition(e.sprite.getPosition());
		aura.setFillColor(sf::Color(e.baseColor.r, e.baseColor.g, e.baseColor.b, e.flashTimer > 0 ? 110 : (alpha == 255 ? 30 : (sf::Uint8)(alpha / 5))));
		target.draw(aura);

		sf::Sprite ghostSprite = e.sprite;
		sf::Color c = ghostSprite.getColor();
		c.a = alpha;
		if (damagePulse > 0.f) {
			c.r = 255; c.g = 255; c.b = 255;
		}
		ghostSprite.setColor(c);
		ghostSprite.setScale(e.sprite.getScale().x * (e.flashTimer > 0 ? 1.04f : 1.f), e.sprite.getScale().y * (e.flashTimer > 0 ? 1.04f : 1.f));
		target.draw(ghostSprite);

		if (e.hasShield) {
			sf::CircleShape shield(e.shape.getSize().x * 0.64f + pulse(0.15f, 0.f, 6.f));
			shield.setOrigin(shield.getRadius(), shield.getRadius());
			shield.setPosition(e.sprite.getPosition());
			shield.setFillColor(sf::Color::Transparent);
			shield.setOutlineColor(sf::Color(0, 210, 255, 190));
			shield.setOutlineThickness(3.f);
			target.draw(shield);
		}
		if (e.flashTimer > 0) {
			sf::CircleShape burst(e.shape.getSize().x * 0.8f + pulse(0.12f, 0.f, 5.f));
			burst.setOrigin(burst.getRadius(), burst.getRadius());
			burst.setPosition(e.sprite.getPosition());
			burst.setFillColor(sf::Color(255, 255, 255, 20));
			target.draw(burst);
		}

		// HP bar below sprite
		sf::Color hpColor = hpRatio < 0.35f ? sf::Color(255, 70, 80) : hpRatio < 0.7f ? sf::Color(255, 210, 70) : sf::Color(60, 255, 130);
		drawBar(target, sf::Vector2f(e.shape.getPosition().x, e.shape.getPosition().y - 10.f), sf::Vector2f(e.shape.getSize().x, 5.f), hpRatio, hpColor);
		if (e.flashTimer > 0) {
			sf::CircleShape flash(e.shape.getSize().x * 0.72f);
			flash.setOrigin(flash.getRadius(), flash.getRadius());
			flash.setPosition(e.sprite.getPosition());
			flash.setFillColor(sf::Color(255, 255, 255, 28));
			target.draw(flash);
		}
	}
}

// ---- Player: layered glow then sprite ----
void game::renderPlayer(sf::RenderTarget& target)
{
	sf::Vector2f p = playerSprite.getPosition();
	float thrust = 14.f + pulse(0.22f, 0.f, 6.f);
	float auraBoost = (invincibilityTimer > 0.f ? 14.f : 0.f) + (deathRayTimer > 0.f ? 10.f : 0.f) + (fastFireTimer > 0.f ? 6.f : 0.f);

	sf::CircleShape outer(28.f + auraBoost + pulse(0.08f, 0.f, 4.f));
	outer.setOrigin(outer.getRadius(), outer.getRadius());
	outer.setPosition(p);
	outer.setFillColor(sf::Color(0, 180, 255, 20));
	target.draw(outer);

	sf::CircleShape flare(16.f + auraBoost * 0.4f);
	flare.setOrigin(flare.getRadius(), flare.getRadius());
	flare.setPosition(p);
	flare.setFillColor(sf::Color(255, 255, 255, 20));
	target.draw(flare);

	sf::CircleShape engine(thrust);
	engine.setOrigin(engine.getRadius(), engine.getRadius());
	engine.setPosition(p);
	engine.setFillColor(sf::Color(0, 230, 255, 65));
	target.draw(engine);

	sf::CircleShape core(10.f + pulse(0.18f, 0.f, 2.f));
	core.setOrigin(core.getRadius(), core.getRadius());
	core.setPosition(p);
	core.setFillColor(sf::Color(255, 255, 255, 28));
	target.draw(core);

	target.draw(shieldShape);
	target.draw(playerSprite);
}

// ---- Dash trail: sprite copies with fading alpha ----
void game::renderDashTrail(sf::RenderTarget& target)
{
	for (auto& g : dashTrail) {
		sf::Sprite glow = g.sprite;
		glow.setScale(g.sprite.getScale().x * 1.18f, g.sprite.getScale().y * 1.18f);
		target.draw(glow);
		target.draw(g.sprite);
	}
}

void game::renderLasers(sf::RenderTarget& target)
{
	for (auto& l : lasers) {
		sf::Color coreColor = l.shape.getFillColor();

		// Outer glow
		sf::RectangleShape glow = l.shape;
		glow.setSize(l.shape.getSize() + sf::Vector2f(16.f, 20.f));
		glow.setOrigin(8.f, 10.f);
		if (coreColor == sf::Color::White) {
			glow.setFillColor(sf::Color(40, 40, 255, 80)); // Deep blue/purple glow
		}
		else {
			glow.setFillColor(sf::Color(coreColor.r, coreColor.g, coreColor.b, 70));
		}
		target.draw(glow);

		// Inner glow
		sf::RectangleShape innerGlow = l.shape;
		innerGlow.setSize(l.shape.getSize() + sf::Vector2f(6.f, 10.f));
		innerGlow.setOrigin(3.f, 5.f);
		if (coreColor == sf::Color::White) {
			innerGlow.setFillColor(sf::Color(100, 100, 255, 150)); // Brighter blue inner glow
		}
		else {
			innerGlow.setFillColor(sf::Color(coreColor.r, coreColor.g, coreColor.b, 140));
		}
		target.draw(innerGlow);

		target.draw(l.shape);
	}
}

void game::renderEnemyBullets(sf::RenderTarget& target)
{
	for (auto& b : enemyBullets) {
		sf::CircleShape glow(b.shape.getRadius() * 2.1f);
		glow.setOrigin(glow.getRadius(), glow.getRadius());
		glow.setPosition(b.shape.getPosition() + sf::Vector2f(b.shape.getRadius(), b.shape.getRadius()));
		glow.setFillColor(sf::Color(255, 80, 255, 60));
		target.draw(glow);
		target.draw(b.shape);
	}
}

void game::renderParticles(sf::RenderTarget& target)
{
	for (auto& p : particles) {
		sf::RectangleShape glow = p.shape;
		glow.setScale(1.8f, 1.8f);
		sf::Color c = p.shape.getFillColor();
		glow.setFillColor(sf::Color(c.r, c.g, c.b, (sf::Uint8)(c.a / 3)));
		target.draw(glow);
		target.draw(p.shape);
	}
}

void game::renderPowerUps(sf::RenderTarget& target)
{
	for (auto& p : powerUps) {
		sf::CircleShape glow(p.shape.getRadius() * 2.4f);
		glow.setOrigin(glow.getRadius(), glow.getRadius());
		glow.setPosition(p.shape.getPosition() + sf::Vector2f(p.shape.getRadius(), p.shape.getRadius()));
		sf::Color c = p.shape.getFillColor();
		glow.setFillColor(sf::Color(c.r, c.g, c.b, 70));
		target.draw(glow);
		target.draw(p.shape);
	}
	for (auto& d : drops) {
		sf::CircleShape glow(d.shape.getRadius() * 2.f);
		glow.setOrigin(glow.getRadius(), glow.getRadius());
		glow.setPosition(d.shape.getPosition() + sf::Vector2f(d.shape.getRadius(), d.shape.getRadius()));
		glow.setFillColor(sf::Color(255, 220, 0, 55));
		target.draw(glow);
		target.draw(d.shape);
	}
}

void game::renderSpawnWarnings(sf::RenderTarget& target)
{
	for (auto& sw : spawnWarnings) {
		float progress = sw.timer / sw.timerMax;
		float t = 1.f - progress; // goes 0 to 1
		sf::Uint8 alpha = (sf::Uint8)(200 * std::abs(std::sin(t * (10.f + t * 40.f))));

		sf::ConvexShape tri; tri.setPointCount(3);
		float w = sw.width;
		tri.setPoint(0, sf::Vector2f(0.f, 0.f));
		tri.setPoint(1, sf::Vector2f(w / 2.f, -20.f));
		tri.setPoint(2, sf::Vector2f(w, 0.f));
		tri.setPosition(sw.xPos, 5.f);
		tri.setFillColor(sf::Color(255, 80, 80, alpha));
		target.draw(tri);

		sf::RectangleShape line(sf::Vector2f(w, 4.f));
		line.setPosition(sw.xPos, 34.f + pulse(0.18f, -5.f, 5.f));
		line.setFillColor(sf::Color(255, 220, 80, alpha));
		target.draw(line);
	}
}

void game::renderScorePopups(sf::RenderTarget& target)
{
	for (auto& sp : scorePopups) draw3DText(target, sp.text);
}

void game::renderHUD(sf::RenderTarget& target)
{
	float hudX = (float)window->getSize().x - 330.f;
	float hudY = (float)window->getSize().y - 235.f;
	drawPanel(target, sf::Vector2f(hudX + 115.f, hudY + 78.f), sf::Vector2f(270.f, 240.f), sf::Color(0, 230, 255, 120), 55);

	sf::Text hudTitle; hudTitle.setFont(font); hudTitle.setCharacterSize(20);
	hudTitle.setFillColor(sf::Color(200, 245, 255));
	hudTitle.setString("STATUS");
	hudTitle.setPosition(hudX, hudY - 56.f);
	draw3DText(target, hudTitle);

	// Heat bar
	sf::Text heatLabel; heatLabel.setFont(font); heatLabel.setCharacterSize(18);
	heatLabel.setFillColor(isOverheated ? sf::Color::Red : sf::Color::White);
	heatLabel.setString(isOverheated ? "OVERHEAT!" : "HEAT");
	heatLabel.setPosition(hudX, hudY - 30.f); draw3DText(target, heatLabel);

	float heatRatio = heatLevel / heatMax;
	sf::Color heatColor = heatRatio > 0.7f ? sf::Color::Red : heatRatio > 0.4f ? sf::Color(255, 165, 0) : sf::Color(0, 200, 100);
	drawBar(target, sf::Vector2f(hudX, hudY - 10.f), sf::Vector2f(230.f, 13.f), heatRatio, heatColor);

	// Dash cooldown bar
	sf::Text dashLabel; dashLabel.setFont(font); dashLabel.setCharacterSize(18);
	dashLabel.setFillColor(dashCooldown <= 0.f ? sf::Color::Cyan : sf::Color(150, 150, 150));
	dashLabel.setString(dashCooldown <= 0.f ? "DASH - SPACE" : "DASH COOLING");
	dashLabel.setPosition(hudX, hudY + 15.f); draw3DText(target, dashLabel);

	float dashPct = 1.f - (dashCooldown / dashCooldownMax);
	drawBar(target, sf::Vector2f(hudX, hudY + 38.f), sf::Vector2f(230.f, 9.f), dashPct, sf::Color::Cyan);

	// Power-up timers
	float puY = hudY + 60.f;
	float puMaxTime = 600.f;
	struct PuInfo { float* timer; sf::Color color; const char* name; };
	PuInfo pus[] = {
		{ &invincibilityTimer, sf::Color::Yellow, "SHIELD" },
		{ &deathRayTimer,      sf::Color::Red,    "D-RAY"  },
		{ &fastFireTimer,      sf::Color::Cyan,   "RAPID"  },
		{ &slowTimeTimer,      sf::Color::Blue,   "SLOW"   },
	};
	for (int pi = 0; pi < 4; pi++) {
		if (*pus[pi].timer > 0.f) {
			drawBar(target, sf::Vector2f(hudX + pi * 58.f, puY), sf::Vector2f(48.f, 8.f), *pus[pi].timer / puMaxTime, pus[pi].color);
			sf::Text pLabel; pLabel.setFont(font); pLabel.setCharacterSize(14);
			pLabel.setFillColor(pus[pi].color); pLabel.setString(pus[pi].name);
			pLabel.setPosition(hudX + pi * 58.f, puY + 10.f); draw3DText(target, pLabel);
		}
	}

	// Shield status
	sf::Text shieldLabel; shieldLabel.setFont(font); shieldLabel.setCharacterSize(18);
	if (shieldActive) {
		shieldLabel.setFillColor(sf::Color(0, 200, 255)); shieldLabel.setString("SHIELD: READY");
	}
	else {
		float pct = shieldRechargeTimer / shieldRechargeMax;
		std::stringstream ss; ss << "SHIELD: " << (int)(pct * 100) << "%";
		shieldLabel.setString(ss.str()); shieldLabel.setFillColor(sf::Color(100, 100, 200));
	}
	shieldLabel.setPosition(hudX, puY + 35.f); draw3DText(target, shieldLabel);

	// Secondary Energy bar
	sf::Text secLabel; secLabel.setFont(font); secLabel.setCharacterSize(18);
	secLabel.setFillColor(secondaryEnergy >= 30.f ? sf::Color::Cyan : sf::Color(100, 100, 100));
	secLabel.setString("SECONDARY");
	secLabel.setPosition(hudX, puY + 65.f); draw3DText(target, secLabel);
	drawBar(target, sf::Vector2f(hudX, puY + 90.f), sf::Vector2f(230.f, 9.f), secondaryEnergy / secondaryEnergyMax, sf::Color(0, 150, 255));
}


void game::renderSkins(sf::RenderTarget& target)
{
	drawGlowText(target, skinsTitleText, sf::Color(240, 255, 255), sf::Color(255, 50, 180, 80), 1.06f);
	for (int i = 0; i < 6; i++) {
		sf::CircleShape pad(72.f);
		pad.setOrigin(72.f, 72.f);
		pad.setPosition(skinPreviews[i].getPosition());
		pad.setFillColor(sf::Color(0, 220, 255, selectedSkin == i ? 55 : 25));
		pad.setOutlineColor(selectedSkin == i ? sf::Color(255, 230, 80, 190) : sf::Color(0, 220, 255, 90));
		pad.setOutlineThickness(selectedSkin == i ? 3.f : 1.f);
		target.draw(pad);
		target.draw(skinPreviews[i]);
	}
	draw3DText(target, BackText);
}

void game::renderVignette(sf::RenderTarget& target)
{
	if (health > 3) return; // Only show warning vignette when health is low
	sf::Uint8 baseAlpha = (sf::Uint8)(105 + 70 * std::sin(gRenderFrame * 0.08f));
	sf::Color edge = sf::Color(255, 0, 60, baseAlpha);
	float th = 95.f;
	sf::RectangleShape top(sf::Vector2f((float)window->getSize().x, th)); top.setFillColor(edge); target.draw(top);
	sf::RectangleShape bot(sf::Vector2f((float)window->getSize().x, th)); bot.setPosition(0.f, (float)window->getSize().y - th); bot.setFillColor(edge); target.draw(bot);
	sf::RectangleShape lft(sf::Vector2f(th, (float)window->getSize().y)); lft.setFillColor(edge); target.draw(lft);
	sf::RectangleShape rgt(sf::Vector2f(th, (float)window->getSize().y)); rgt.setPosition((float)window->getSize().x - th, 0.f); rgt.setFillColor(edge); target.draw(rgt);
}

// ---- INTRO SPLASH ----
void game::renderIntro(sf::RenderTarget& target)
{
	// Stars already drawn by renderBackground
	renderDemoBackground(target);

	// Moon
	if (moonTexture.getSize().x > 0)
		target.draw(moonSprite);

	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;

	// Title � positioned in upper 40% of screen, well above the moon
	{
		sf::Text title;
		title.setFont(font); title.setCharacterSize(80);
		sf::Uint8 alpha = (sf::Uint8)std::min(255.f, introTextAlpha);
		title.setFillColor(sf::Color(0, 240, 255, alpha));
		title.setString("RECTANGLE SMASH");
		centerOrigin(title);
		title.setPosition(cx, cy - 180.f);
		sf::Text shadow = title;
		shadow.setFillColor(sf::Color(0, 50, 100, (sf::Uint8)(alpha / 1.5f)));
		shadow.setPosition(cx + 4.f, cy - 176.f);
		target.draw(shadow);
		draw3DText(target, title);
	}

	// Blinking prompt after 120 frames
	if (introTimer > 120.f) {
		float blink = std::sin(introTimer * 0.03f);
		sf::Uint8 ba = (sf::Uint8)(std::max(0.f, std::min(1.f, blink + 0.75f)) * 255.f);
		sf::Text prompt;
		prompt.setFont(font); prompt.setCharacterSize(28);
		prompt.setFillColor(sf::Color(255, 230, 80, ba));
		prompt.setOutlineColor(sf::Color::Black);
		prompt.setOutlineThickness(2.f);
		prompt.setString("PRESS ANY KEY TO CONTINUE");
		centerOrigin(prompt);
		prompt.setPosition(cx, cy - 105.f);
		draw3DText(target, prompt);
	}

	// Scanline overlay
	for (int y = 0; y < (int)window->getSize().y; y += 4) {
		sf::RectangleShape line(sf::Vector2f((float)window->getSize().x, 1.f));
		line.setPosition(0.f, (float)y);
		line.setFillColor(sf::Color(0, 0, 0, 16));
		target.draw(line);
	}
}

// ---- HOW-TO-PLAY ----
void game::renderHowToPlay(sf::RenderTarget& target)
{
	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;
	float wy = (float)window->getSize().y;

	// Dark overlay
	sf::RectangleShape bg(sf::Vector2f((float)window->getSize().x, wy));
	bg.setFillColor(sf::Color(0, 0, 15, 210));
	target.draw(bg);

	// Page title
	const char* titles[] = { "CONTROLS", "ENEMIES", "POWER-UPS", "BOSSES" };
	sf::Text title;
	title.setFont(font); title.setCharacterSize(54);
	title.setFillColor(sf::Color::Cyan);
	title.setString(titles[howToPage]);
	centerOrigin(title);
	title.setPosition(cx, cy - 230.f);
	draw3DText(target, title);

	// Page content
	struct Line { std::string txt; sf::Color col; };
	std::vector<Line> lines;
	if (howToPage == 0) {
		lines = {
			{"MOVE:   WASD  or  Arrow Keys",     sf::Color(200, 240, 255)},
			{"AIM:    Mouse",                     sf::Color(200, 240, 255)},
			{"FIRE:   Left Click  - or Auto",     sf::Color(200, 240, 255)},
			{"DASH:   SPACE  - direction WASD", sf::Color(200, 240, 255)},
			{"PAUSE:  ESC",                       sf::Color(200, 240, 255)},
			{"BOMB:   SHIFT  - screen clear x3",  sf::Color(255, 230, 80)},
		};
	}
	else if (howToPage == 1) {
		// FIX: colour each enemy entry in its own colour for readability
		lines = {
			{"YELLOW  -  Zigzagger: fast weaving",         sf::Color(255, 230, 50)},
			{"MAGENTA -  Shooter: fires aimed bursts",     sf::Color(255, 120, 255)},
			{"RED     -  Homing: tracks you slowly",       sf::Color(255, 80,  80)},
			{"GREEN   -  Swarm: weak, come in groups",     sf::Color(80,  255, 120)},
			{"CYAN    -  Dodger: evades cursor (wave 6+)", sf::Color(0,   230, 255)},
		};
	}
	else if (howToPage == 2) {
		// FIX: power-up page � single coherent colour, no multicolour mess
		lines = {
			{"YELLOW  -  Shield - 10s invincibility",     sf::Color(220, 230, 255)},
			{"RED     -  Death Ray - wide laser, 10s",    sf::Color(220, 230, 255)},
			{"CYAN    -  Rapid Fire - fast shots, 10s",   sf::Color(220, 230, 255)},
			{"BLUE    -  Slow Time - all enemies slow",   sf::Color(220, 230, 255)},
			{"GREEN   -  Health +1",                      sf::Color(220, 230, 255)},
			{"GOLD    -  Gear drop - spend in Shop",      sf::Color(255, 220, 60)},
		};
	}
	else {
		lines = {
			{"BOSS 1  -  Every 5 waves - 5, 15, 20+", sf::Color(255, 200, 60)},
			{"- 50% HP triggers phase 2, faster",      sf::Color(200, 220, 255)},
			{"- Laser beam + radial bursts",            sf::Color(200, 220, 255)},
			{"BOSS 2 - SURT  -  Wave 10",               sf::Color(255, 100, 80)},
			{"- 6 evolving stages",                     sf::Color(200, 220, 255)},
			{"- Stage 6 revives in RAGE MODE",          sf::Color(200, 220, 255)},
		};
	}

	float lineY = cy - 150.f;
	for (auto& l : lines) {
		sf::Text t;
		t.setFont(font); t.setCharacterSize(30);
		t.setFillColor(l.col);
		t.setOutlineColor(sf::Color(0, 0, 0, 170));
		t.setOutlineThickness(1.f);
		t.setString(l.txt);
		centerOrigin(t);
		t.setPosition(cx, lineY);
		draw3DText(target, t);
		lineY += 55.f;
	}

	// Page indicator
	sf::Text pg;
	pg.setFont(font); pg.setCharacterSize(22);
	pg.setFillColor(sf::Color(130, 150, 180));
	pg.setString(std::to_string(howToPage + 1) + " / 4");
	centerOrigin(pg);
	pg.setPosition(cx, wy - 160.f);
	target.draw(pg);

	// FIX: BACK / NEXT buttons � darker fill, thicker border, larger text for clarity
	auto drawBtn = [&](const std::string& label, float x, float y, sf::Color accent) {
		sf::RectangleShape btn(sf::Vector2f(200.f, 60.f));
		btn.setOrigin(100.f, 30.f);
		btn.setPosition(x, y);
		btn.setFillColor(sf::Color(8, 20, 50, 210));
		btn.setOutlineColor(accent);
		btn.setOutlineThickness(3.f);
		target.draw(btn);
		// Inner accent line at top
		sf::RectangleShape topLine(sf::Vector2f(186.f, 2.f));
		topLine.setOrigin(93.f, 1.f);
		topLine.setPosition(x, y - 29.f);
		topLine.setFillColor(sf::Color(accent.r, accent.g, accent.b, 160));
		target.draw(topLine);
		sf::Text bt; bt.setFont(font); bt.setCharacterSize(32);
		bt.setFillColor(sf::Color::White);
		bt.setString(label);
		centerOrigin(bt); bt.setPosition(x, y);
		target.draw(bt);
		};
	drawBtn("BACK", cx - 160.f, wy - 90.f, sf::Color(0, 180, 255));
	drawBtn(howToPage < 3 ? "NEXT" : "START!", cx + 160.f, wy - 90.f, sf::Color(0, 255, 160));
}

// ---- BOSS ARRIVAL CINEMATIC ----
void game::renderBossArrival(sf::RenderTarget& target)
{
	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;
	float progress = 1.f - (bossArrivalTimer / 300.f);

	// Red pulsing vignette
	sf::Uint8 vigAlpha = (sf::Uint8)(80 + 60 * std::sin(gRenderFrame * 0.12f));
	sf::RectangleShape top(sf::Vector2f((float)window->getSize().x, 120.f));
	top.setFillColor(sf::Color(180, 0, 0, vigAlpha)); target.draw(top);
	sf::RectangleShape bot(sf::Vector2f((float)window->getSize().x, 120.f));
	bot.setPosition(0.f, (float)window->getSize().y - 120.f);
	bot.setFillColor(sf::Color(180, 0, 0, vigAlpha)); target.draw(bot);
	sf::RectangleShape lft(sf::Vector2f(120.f, (float)window->getSize().y));
	lft.setFillColor(sf::Color(180, 0, 0, vigAlpha)); target.draw(lft);
	sf::RectangleShape rgt(sf::Vector2f(120.f, (float)window->getSize().y));
	rgt.setPosition((float)window->getSize().x - 120.f, 0.f);
	rgt.setFillColor(sf::Color(180, 0, 0, vigAlpha)); target.draw(rgt);

	drawPanel(target, sf::Vector2f(cx, cy - 15.f), sf::Vector2f(760.f, 320.f), sf::Color(255, 70, 70, 150), 55);

	// Main warning text
	sf::Text warn;
	warn.setFont(font); warn.setCharacterSize(80);
	std::string warnStr = (bossArrivalWave == 2) ? "! SURT AWAKENS !" : "! BOSS INCOMING !";
	warn.setString(warnStr);
	sf::Uint8 flickerA = (sf::Uint8)(200 + 55 * std::sin(gRenderFrame * 0.25f));
	warn.setFillColor(sf::Color(255, 30, 30, flickerA));
	warn.setOutlineColor(sf::Color::Black);
	warn.setOutlineThickness(2.f);
	centerOrigin(warn);
	warn.setPosition(cx, cy - 70.f);
	sf::Text warnHalo = warn;
	warnHalo.setFillColor(sf::Color(255, 0, 0, flickerA / 4));
	warnHalo.setScale(1.09f, 1.09f);
	target.draw(warnHalo);
	draw3DText(target, warn);

	// Sub-text
	sf::Text sub;
	sub.setFont(font); sub.setCharacterSize(38);
	sf::Uint8 subA = (sf::Uint8)(180 * std::abs(std::sin(gRenderFrame * 0.06f)));
	sub.setFillColor(sf::Color(255, 180, 60, subA));
	sub.setOutlineColor(sf::Color::Black);
	sub.setOutlineThickness(1.f);
	sub.setString("BRACE YOURSELF");
	centerOrigin(sub);
	sub.setPosition(cx, cy + 40.f);
	draw3DText(target, sub);

	// Countdown bar
	sf::RectangleShape barBg(sf::Vector2f(600.f, 16.f));
	barBg.setOrigin(300.f, 8.f);
	barBg.setPosition(cx, cy + 110.f);
	barBg.setFillColor(sf::Color(40, 0, 0));
	target.draw(barBg);

	sf::RectangleShape barFill(sf::Vector2f(600.f * progress, 16.f));
	barFill.setOrigin(300.f, 8.f);
	barFill.setPosition(cx, cy + 110.f);
	barFill.setFillColor(sf::Color(255, 30, 30));
	target.draw(barFill);

	// Wave banner
	if (waveBannerTimer > 0.f) {
		float alpha = std::min(waveBannerTimer / 20.f, 1.f) * 255.f;
		sf::Color wc = waveBannerText.getFillColor();
		wc.a = (sf::Uint8)alpha; waveBannerText.setFillColor(wc);
		draw3DText(target, waveBannerText);
	}
}

// ---- EXPLOSION ANIMATIONS ----
void game::renderExplosions(sf::RenderTarget& target)
{
	for (auto& ex : activeExplosions) {
		if (!ex.done) target.draw(ex.sprite);
	}
}

// ---- BOSS 2 RENDER ----
void game::renderBoss2(sf::RenderTarget& target)
{
	boss2Entity.render(target);
	boss2Entity.renderStageAnnouncement(target, font);
}

void game::renderHazards(sf::RenderTarget& target)
{
	for (auto& a : asteroids) target.draw(a.shape);
	for (auto& bh : blackHoles) {
		target.draw(bh.eventHorizon);
		target.draw(bh.core);
	}
}

void game::renderPerkSelect(sf::RenderTarget& target)
{
	sf::Text title;
	title.setFont(font); title.setCharacterSize(70); title.setFillColor(sf::Color::Cyan);
	title.setString("CHOOSE YOUR ASCENSION");
	centerOrigin(title);
	title.setPosition((float)window->getSize().x / 2.f, 100.f);
	drawGlowText(target, title, sf::Color::White, sf::Color(0, 255, 255, 100), 1.03f);

	float cx = (float)window->getSize().x / 2.f;
	float cy = (float)window->getSize().y / 2.f;

	for (int i = 0; i < 3; i++) {
		Perk& p = perkPool[perkChoices[i]];
		float bx = cx - 400.f + i * 400.f;

		sf::RectangleShape card(sf::Vector2f(360.f, 240.f));
		card.setOrigin(180.f, 120.f);
		card.setPosition(bx, cy);
		card.setFillColor(sf::Color(15, 25, 45, 230));
		bool h = card.getGlobalBounds().contains(mousePosView);
		card.setOutlineColor(h ? p.color : sf::Color(100, 100, 150));
		card.setOutlineThickness(h ? 5.f : 2.f);
		if (h) { card.setSize(sf::Vector2f(370.f, 250.f)); card.setOrigin(185.f, 125.f); }
		target.draw(card);

		sf::CircleShape accent(18.f);
		accent.setOrigin(18.f, 18.f);
		accent.setPosition(bx, cy - 85.f);
		accent.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, h ? 180 : 120));
		target.draw(accent);

		sf::Text tName;
		tName.setFont(font); tName.setCharacterSize(28);
		tName.setFillColor(p.color);
		tName.setString(p.name);
		centerOrigin(tName);
		tName.setPosition(bx, cy - 60.f);
		if (h) tName.scale(1.05f, 1.05f);
		draw3DText(target, tName);

		sf::Text tDesc;
		tDesc.setFont(font); tDesc.setCharacterSize(14);
		tDesc.setFillColor(sf::Color::White);
		tDesc.setString(p.desc);
		centerOrigin(tDesc);
		tDesc.setPosition(bx, cy + 20.f);
		if (h) tDesc.scale(1.05f, 1.05f);
		draw3DText(target, tDesc);
	}
}

void game::renderBlackMarket(sf::RenderTarget& target)
{
	sf::Text title;
	title.setFont(font); title.setCharacterSize(70); title.setFillColor(sf::Color(255, 160, 40));
	title.setString("BLACK MARKET");
	centerOrigin(title);
	title.setPosition((float)window->getSize().x / 2.f, 100.f);
	drawGlowText(target, title, sf::Color(255, 200, 100), sf::Color(255, 140, 0, 80), 1.06f);

	float bmcx = (float)window->getSize().x / 2.f;
	drawPanel(target, sf::Vector2f(bmcx, 420.f), sf::Vector2f(780.f, 420.f), sf::Color(255, 140, 0, 100), 42);

	sf::Text gearsText;
	gearsText.setFont(font); gearsText.setCharacterSize(30);
	gearsText.setFillColor(sf::Color::Yellow);
	gearsText.setString("GEARS: " + std::to_string(totalGears));
	centerOrigin(gearsText);
	gearsText.setPosition((float)window->getSize().x / 2.f, 170.f);
	draw3DText(target, gearsText);

	float cx = (float)window->getSize().x / 2.f;
	float startY = 250.f;
	const char* names[] = { "EXTRA HEALTH", "PERMANENT SPEED", "DMG MULTIPLIER", "STARTING GEARS" };
	int* upgrades[] = { &blkStartingExtraHealth, &blkPermanentSpeedBoost, &blkDamageMultiplier, &blkStartingGears };
	int baseCost[] = { 1000, 1500, 2000, 500 };

	for (int i = 0; i < 4; i++) {
		float y = startY + i * 80.f;

		sf::Text tName;
		tName.setFont(font); tName.setCharacterSize(24);
		tName.setFillColor(sf::Color::White);
		tName.setString(names[i]);
		tName.setOrigin(0.f, tName.getLocalBounds().top + tName.getLocalBounds().height / 2.f);
		tName.setPosition(cx - 400.f, y);
		draw3DText(target, tName);

		sf::Text tLvl;
		tLvl.setFont(font); tLvl.setCharacterSize(24);
		tLvl.setFillColor(sf::Color::Cyan);
		tLvl.setString("LVL " + std::to_string(*upgrades[i]));
		centerOrigin(tLvl);
		tLvl.setPosition(cx + 20.f, y);
		draw3DText(target, tLvl);

		// Buy button
		sf::RectangleShape btn(sf::Vector2f(180.f, 50.f));
		btn.setOrigin(90.f, 25.f);
		btn.setPosition(cx + 250.f, y);
		btn.setFillColor(sf::Color(100, 0, 0, 150));
		btn.setOutlineColor(sf::Color::Red);
		btn.setOutlineThickness(2.f);
		target.draw(btn);

		sf::Text tCost;
		tCost.setFont(font); tCost.setCharacterSize(16);
		int cost = baseCost[i] * (*upgrades[i] + 1);
		tCost.setString(std::to_string(cost) + " G");
		tCost.setFillColor(totalGears >= cost ? sf::Color::Yellow : sf::Color(150, 0, 0));
		centerOrigin(tCost);
		tCost.setPosition(cx + 250.f, y);
		draw3DText(target, tCost);
	}

	draw3DText(target, BackText);
}

void game::renderTransitionFade(sf::RenderTarget& target)
{
	if (transitionAlpha > 0.f) {
		sf::RectangleShape fadeRect(sf::Vector2f((float)window->getSize().x, (float)window->getSize().y));
		fadeRect.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)transitionAlpha));
		target.draw(fadeRect);
	}
}