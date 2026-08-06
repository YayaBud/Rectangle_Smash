// game_render.cpp all render functions
#include "game.h"

// One drop shadow, one face. The old version drew the same string 9-10 times
// (4-way glow + per-pixel extrusion), which is both the single biggest source of
// draw calls in the HUD and the reason every label looked like a default neon
// preset. Two draws reads sharper and costs 1/5th.
static void draw3DText(sf::RenderTarget& target, sf::Text text) {
    const sf::Color face = text.getFillColor();
    const sf::Vector2f basePos = text.getPosition();

    text.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)(face.a * 0.65f)));
    text.setPosition(basePos.x + 2.f, basePos.y + 2.f);
    target.draw(text);

    text.setFillColor(face);
    text.setPosition(basePos);
    target.draw(text);
}

// ─────────────────────────────────────────────
//  BATCHING HELPERS
//  Everything below appends triangles to a shared sf::VertexArray so a whole
//  entity list collapses into a single draw() call.
// ─────────────────────────────────────────────
namespace {
    // Appends the quad a-b-c-d (wound in order) as two triangles.
    inline void pushQuad(sf::VertexArray& va,
        sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
        const sf::Color& col)
    {
        va.append(sf::Vertex(a, col));
        va.append(sf::Vertex(b, col));
        va.append(sf::Vertex(c, col));
        va.append(sf::Vertex(a, col));
        va.append(sf::Vertex(c, col));
        va.append(sf::Vertex(d, col));
    }

    // Axis-aligned rect (particles, stars-as-squares).
    inline void pushRect(sf::VertexArray& va, sf::Vector2f pos, sf::Vector2f size, const sf::Color& col)
    {
        pushQuad(va, pos,
            sf::Vector2f(pos.x + size.x, pos.y),
            sf::Vector2f(pos.x + size.x, pos.y + size.y),
            sf::Vector2f(pos.x, pos.y + size.y), col);
    }

    // Rect carried through a shape's own transform — handles rotation/origin,
    // so a rotated laser batches identically to how SFML would have drawn it.
    inline void pushShapeRect(sf::VertexArray& va, const sf::Transform& xf,
        sf::Vector2f size, const sf::Color& col)
    {
        pushQuad(va,
            xf.transformPoint(0.f, 0.f),
            xf.transformPoint(size.x, 0.f),
            xf.transformPoint(size.x, size.y),
            xf.transformPoint(0.f, size.y), col);
    }

    // Triangle fan around a centre, emitted as discrete triangles.
    // 10 segments is indistinguishable from SFML's 30-point circle at bullet size.
    inline void pushDisc(sf::VertexArray& va, sf::Vector2f centre, float radius,
        const sf::Color& col, int segments = 10)
    {
        const float TAU = 6.2831853f;
        sf::Vector2f prev(centre.x + radius, centre.y);
        for (int i = 1; i <= segments; i++) {
            float a = TAU * (float)i / (float)segments;
            sf::Vector2f next(centre.x + std::cos(a) * radius, centre.y + std::sin(a) * radius);
            va.append(sf::Vertex(centre, col));
            va.append(sf::Vertex(prev, col));
            va.append(sf::Vertex(next, col));
            prev = next;
        }
    }

    // Hollow ring — used by shockwaves and graze pulses.
    inline void pushRing(sf::VertexArray& va, sf::Vector2f centre, float radius,
        float thickness, const sf::Color& col, int segments = 40)
    {
        const float TAU = 6.2831853f;
        float inner = std::max(0.f, radius - thickness);
        for (int i = 0; i < segments; i++) {
            float a0 = TAU * (float)i / (float)segments;
            float a1 = TAU * (float)(i + 1) / (float)segments;
            sf::Vector2f o0(centre.x + std::cos(a0) * radius, centre.y + std::sin(a0) * radius);
            sf::Vector2f o1(centre.x + std::cos(a1) * radius, centre.y + std::sin(a1) * radius);
            sf::Vector2f i0(centre.x + std::cos(a0) * inner, centre.y + std::sin(a0) * inner);
            sf::Vector2f i1(centre.x + std::cos(a1) * inner, centre.y + std::sin(a1) * inner);
            pushQuad(va, i0, o0, o1, i1, col);
        }
    }
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

	// ── PALETTE ──────────────────────────────────────────────────────
	// Four hues with fixed jobs, instead of every element picking its own
	// saturated neon. Colour carries meaning here: warm = yours, red = threat,
	// slate = inert chrome.
	const sf::Color COL_INK(10, 13, 20);       // panel fill
	const sf::Color COL_CHROME(126, 142, 163); // labels, rules, inert UI
	const sf::Color COL_MINE(232, 178, 76);    // your resources: ult, bombs
	const sf::Color COL_READY(74, 200, 180);   // available / cooled down
	const sf::Color COL_DANGER(226, 82, 74);   // heat, damage, threat

	void drawPanel(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, sf::Color edge, sf::Uint8 fillAlpha = 90)
	{
		sf::RectangleShape panel(size);
		panel.setOrigin(size.x / 2.f, size.y / 2.f);
		panel.setPosition(pos);
		panel.setFillColor(sf::Color(COL_INK.r, COL_INK.g, COL_INK.b, (sf::Uint8)std::min(235, fillAlpha + 120)));
		panel.setOutlineColor(sf::Color(edge.r, edge.g, edge.b, 70));
		panel.setOutlineThickness(1.f);
		target.draw(panel);

		// A single hairline at the top edge instead of a glowing bevel. Reads as
		// a designed frame rather than a default "sci-fi panel" preset.
		sf::RectangleShape rule(sf::Vector2f(size.x, 2.f));
		rule.setOrigin(size.x / 2.f, 1.f);
		rule.setPosition(pos.x, pos.y - size.y / 2.f);
		rule.setFillColor(sf::Color(edge.r, edge.g, edge.b, 200));
		target.draw(rule);
	}

	void drawBar(sf::RenderTarget& target, sf::Vector2f pos, sf::Vector2f size, float ratio, sf::Color fill)
	{
		ratio = std::max(0.f, std::min(ratio, 1.f));

		sf::RectangleShape bg(size);
		bg.setPosition(pos);
		bg.setFillColor(sf::Color(0, 0, 0, 170));
		target.draw(bg);

		if (ratio > 0.f) {
			sf::RectangleShape fg(sf::Vector2f(size.x * ratio, size.y));
			fg.setPosition(pos);
			fg.setFillColor(fill);
			target.draw(fg);

			// Bright leading edge — gives the bar a direction of travel, and
			// makes small changes legible without an animated glow.
			sf::RectangleShape tip(sf::Vector2f(2.f, size.y));
			tip.setPosition(pos.x + size.x * ratio - 2.f, pos.y);
			tip.setFillColor(sf::Color(255, 255, 255, 190));
			target.draw(tip);
		}

		sf::RectangleShape frame(size);
		frame.setPosition(pos);
		frame.setFillColor(sf::Color::Transparent);
		frame.setOutlineColor(sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 90));
		frame.setOutlineThickness(1.f);
		target.draw(frame);
	}

	// Discrete charges (bombs). Pips beat a number for anything under ~6:
	// countable at a glance, no reading required.
	void drawPips(sf::RenderTarget& target, sf::Vector2f pos, int count, int maxCount, sf::Color fill)
	{
		const float w = 16.f, h = 8.f, gap = 5.f;
		for (int i = 0; i < maxCount; i++) {
			sf::RectangleShape pip(sf::Vector2f(w, h));
			pip.setPosition(pos.x + i * (w + gap), pos.y);
			if (i < count) pip.setFillColor(fill);
			else {
				pip.setFillColor(sf::Color(0, 0, 0, 150));
				pip.setOutlineColor(sf::Color(fill.r, fill.g, fill.b, 80));
				pip.setOutlineThickness(1.f);
			}
			target.draw(pip);
		}
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
	bg.setFillColor(sf::Color(6, 8, 14));
	target.draw(bg);

	// All stars in one draw call. During hyperspace they stretch into streaks,
	// which is a rect rather than a disc — cheaper and reads better than a
	// scaled circle.
	batch.setPrimitiveType(sf::Triangles);
	batch.clear();
	const float streak = hyperspaceProgress > 0.01f ? (1.f + hyperspaceProgress * 15.f) : 0.f;
	for (auto& star : stars) {
		sf::Vector2f p = star.shape.getPosition();
		float r = star.shape.getRadius();
		sf::Color c = star.shape.getFillColor();
		if (streak > 0.f)
			pushRect(batch, sf::Vector2f(p.x, p.y), sf::Vector2f(r * 2.f, r * 2.f * streak), c);
		else
			pushDisc(batch, sf::Vector2f(p.x + r, p.y + r), r, c, 6);
	}
	if (batch.getVertexCount()) target.draw(batch);
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

	// Four stacked translucent circles used to sit under the ship at all times —
	// that permanent glow blob is most of what made the ship read as generic.
	// Now the ship is just the ship, and rings appear only to say something.
	batch.setPrimitiveType(sf::Triangles);
	batch.clear();

	// Engine bloom, small and tied to thrust
	pushDisc(batch, p, 13.f + pulse(0.22f, 0.f, 3.f), sf::Color(90, 170, 255, 40), 14);

	// Graze band: shows the exact radius that pays out, but only while you are
	// actually earning it. Teaches the mechanic by being visible at the moment
	// it fires.
	if (grazeFlashTimer > 0.f) {
		float t = grazeFlashTimer / 10.f;
		pushRing(batch, p, 62.f, 2.5f, sf::Color(COL_MINE.r, COL_MINE.g, COL_MINE.b, (sf::Uint8)(150 * t)), 44);
	}

	// Dash i-frames: you are untouchable, so say so plainly.
	if (isDashing || invincibilityTimer > 0.f)
		pushRing(batch, p, 30.f, 3.f, sf::Color(120, 220, 255, (sf::Uint8)pulse(0.3f, 120.f, 220.f)), 32);

	// Ultimate running
	if (ultimateActive) {
		float t = ultimateActiveTimer / ultimateActiveMax;
		pushRing(batch, p, 40.f + (1.f - t) * 10.f, 4.f,
			sf::Color(255, 236, 190, (sf::Uint8)(200 * t)), 36);
	}

	// Ultimate charged and unspent — a small persistent tell on the ship itself,
	// so you do not have to watch the HUD to know it is available.
	if (!ultimateActive && ultimateCharge >= ultimateChargeMax)
		pushRing(batch, p, 34.f, 2.f, sf::Color(COL_MINE.r, COL_MINE.g, COL_MINE.b, (sf::Uint8)pulse(0.1f, 60.f, 170.f)), 30);

	if (batch.getVertexCount()) target.draw(batch);

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
	// Glow pass + core pass, both batched: 2 draw calls total instead of 3 per laser.
	batch.setPrimitiveType(sf::Triangles);
	batch.clear();
	for (auto& l : lasers) {
		sf::Color core = l.shape.getFillColor();
		sf::Color glowCol = (core == sf::Color::White)
			? sf::Color(70, 110, 255, 90)
			: sf::Color(core.r, core.g, core.b, 80);

		// Widen around the shape's own local axes so rotation still lines up.
		sf::Vector2f size = l.shape.getSize();
		sf::Transform xf = l.shape.getTransform();
		sf::Transform glowXf = xf;
		glowXf.translate(-5.f, -6.f);
		pushShapeRect(batch, glowXf, size + sf::Vector2f(10.f, 12.f), glowCol);
	}
	if (batch.getVertexCount()) target.draw(batch);

	batch.clear();
	for (auto& l : lasers)
		pushShapeRect(batch, l.shape.getTransform(), l.shape.getSize(), l.shape.getFillColor());
	if (batch.getVertexCount()) target.draw(batch);
}

void game::renderEnemyBullets(sf::RenderTarget& target)
{
	batch.setPrimitiveType(sf::Triangles);
	batch.clear();
	for (auto& b : enemyBullets) {
		float r = b.shape.getRadius();
		sf::Vector2f c = b.shape.getPosition() + sf::Vector2f(r, r);
		pushDisc(batch, c, r * 2.1f, sf::Color(255, 80, 255, 55), 10);
		pushDisc(batch, c, r, b.shape.getFillColor(), 10);
	}
	if (batch.getVertexCount()) target.draw(batch);
}

void game::renderParticles(sf::RenderTarget& target)
{
	batch.setPrimitiveType(sf::Triangles);
	batch.clear();
	for (auto& p : particles) {
		sf::Vector2f pos = p.shape.getPosition();
		sf::Vector2f size = p.shape.getSize();
		sf::Color c = p.shape.getFillColor();

		// Halo, then core — same look as the old two-draw version, one call for all.
		sf::Vector2f haloSize = size * 1.8f;
		pushRect(batch, pos - (haloSize - size) / 2.f, haloSize,
			sf::Color(c.r, c.g, c.b, (sf::Uint8)(c.a / 3)));
		pushRect(batch, pos, size, c);
	}
	if (batch.getVertexCount()) target.draw(batch);
}

void game::renderShockwaves(sf::RenderTarget& target)
{
	if (shockwaves.empty()) return;

	batch.setPrimitiveType(sf::Triangles);
	batch.clear();
	for (auto& w : shockwaves) {
		float t = std::min(1.f, w.lifetime / w.maxLifetime);
		sf::Uint8 a = (sf::Uint8)(220.f * (1.f - t) * (1.f - t));

		// Two rings: a bright leading edge and a wider soft wake behind it.
		pushRing(batch, w.pos, w.radius, w.thickness,
			sf::Color(w.color.r, w.color.g, w.color.b, a));
		pushRing(batch, w.pos, w.radius * 0.86f, w.thickness * 2.2f,
			sf::Color(w.color.r, w.color.g, w.color.b, (sf::Uint8)(a / 4)));
	}
	target.draw(batch);
}

void game::renderDrones(sf::RenderTarget& target)
{
	if (drones.empty()) return;

	batch.setPrimitiveType(sf::Triangles);
	batch.clear();
	for (auto& d : drones) {
		sf::Vector2f p = d.shape.getPosition();
		// Halo dims while the drone is on cooldown — readable status at a glance.
		float ready = d.shootTimer <= 0 ? 1.f : 0.35f;
		pushDisc(batch, p, 15.f, sf::Color(120, 235, 255, (sf::Uint8)(50 * ready)), 12);
		pushDisc(batch, p, 7.f, sf::Color(150, 245, 255), 12);
		pushDisc(batch, p, 3.f, sf::Color::White, 8);
	}
	target.draw(batch);
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
	// One column, one grid, consistent row height. The old HUD mixed four
	// unrelated bar styles and eight accent colours in 240 vertical pixels.
	const float BAR_W = 236.f;
	const float ROW = 34.f;
	const float hudX = (float)window->getSize().x - 284.f;
	// Content below runs 284px tall; the panel is sized to fit it with padding
	// and sits 16px off the bottom edge. The previous numbers were 20px short,
	// so the shield row and power-up strip fell off the screen.
	float y = (float)window->getSize().y - 304.f;

	drawPanel(target,
		sf::Vector2f(hudX + BAR_W / 2.f, y + 138.f),
		sf::Vector2f(BAR_W + 28.f, 300.f),
		COL_CHROME, 40);

	// Reused for every label: one Text object, restyled per row. Constructing a
	// fresh sf::Text per label per frame was pure waste.
	sf::Text label;
	label.setFont(font);
	label.setCharacterSize(15);
	label.setLetterSpacing(1.15f);

	auto row = [&](const char* text, sf::Color col, const char* rightText = nullptr) {
		label.setCharacterSize(15);
		label.setFillColor(col);
		label.setString(text);
		label.setPosition(hudX, y);
		draw3DText(target, label);

		if (rightText) {
			label.setString(rightText);
			sf::FloatRect b = label.getLocalBounds();
			label.setFillColor(sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 190));
			label.setPosition(hudX + BAR_W - b.width, y);
			draw3DText(target, label);
		}
		y += 18.f;
		};

	// ── ULTIMATE ──────────────────────────────────────────────────────
	{
		bool ready = ultimateCharge >= ultimateChargeMax;
		const char* name = selectedShipClass == 1 ? "BULWARK"
			: selectedShipClass == 2 ? "LANCE" : "OVERDRIVE";

		// No square brackets anywhere in the HUD: this pixel font has no glyph
		// for them and draws a filled box instead.
		if (ultimateActive) row(name, sf::Color(255, 236, 190), "ACTIVE");
		else if (ready)     row(name, COL_MINE, "READY - Q");
		else                row(name, sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 210), "Q");

		float ratio = ultimateActive ? (ultimateActiveTimer / ultimateActiveMax)
			: (ultimateCharge / ultimateChargeMax);
		// Flashes only while READY and unspent — a permanently animated bar is
		// noise, a bar that starts moving is a message.
		sf::Color fill = ultimateActive ? sf::Color(255, 236, 190)
			: ready ? sf::Color(COL_MINE.r, COL_MINE.g, COL_MINE.b,
				(sf::Uint8)(pulse(0.14f, 150.f, 255.f)))
			: sf::Color(COL_MINE.r, COL_MINE.g, COL_MINE.b, 200);
		drawBar(target, sf::Vector2f(hudX, y), sf::Vector2f(BAR_W, 12.f), ratio, fill);
		y += ROW;
	}

	// ── HEAT ──────────────────────────────────────────────────────────
	{
		float ratio = heatLevel / heatMax;
		row(isOverheated ? "OVERHEATED" : "HEAT",
			isOverheated ? COL_DANGER : sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 210));
		sf::Color fill = isOverheated ? COL_DANGER
			: ratio > 0.65f ? sf::Color(226, 140, 74)
			: COL_CHROME;
		drawBar(target, sf::Vector2f(hudX, y), sf::Vector2f(BAR_W, 8.f), ratio, fill);
		y += ROW - 4.f;
	}

	// ── DASH ──────────────────────────────────────────────────────────
	{
		bool ready = dashCooldown <= 0.f;
		row("DASH", ready ? COL_READY : sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 150),
			ready ? "SPACE" : "");
		drawBar(target, sf::Vector2f(hudX, y), sf::Vector2f(BAR_W, 8.f),
			1.f - (dashCooldown / dashCooldownMax),
			ready ? COL_READY : sf::Color(COL_READY.r, COL_READY.g, COL_READY.b, 110));
		y += ROW - 4.f;
	}

	// ── SECONDARY ─────────────────────────────────────────────────────
	{
		bool ready = secondaryEnergy >= 30.f;
		row("MISSILES", ready ? COL_READY : sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 150),
			ready ? "RMB" : "");
		drawBar(target, sf::Vector2f(hudX, y), sf::Vector2f(BAR_W, 8.f),
			secondaryEnergy / secondaryEnergyMax,
			ready ? COL_READY : sf::Color(COL_READY.r, COL_READY.g, COL_READY.b, 110));
		y += ROW - 4.f;
	}

	// ── BOMBS ─────────────────────────────────────────────────────────
	{
		bool ready = bombCount > 0 && bombCooldownTimer <= 0.f;
		row("BOMBS", ready ? COL_MINE : sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 150),
			ready ? "SHIFT" : "");
		drawPips(target, sf::Vector2f(hudX, y), bombCount, 3,
			ready ? COL_MINE : sf::Color(COL_MINE.r, COL_MINE.g, COL_MINE.b, 110));
		y += ROW - 8.f;
	}

	// ── SHIELD / GRAZE ────────────────────────────────────────────────
	{
		std::stringstream ss;
		if (shieldActive) ss << "SHIELD UP";
		else ss << "SHIELD " << (int)(shieldRechargeTimer / shieldRechargeMax * 100.f) << "%";

		std::stringstream gs;
		if (grazeCount > 0) gs << "GRAZE x" << grazeCount;

		label.setCharacterSize(15);
		label.setFillColor(shieldActive ? COL_READY : sf::Color(COL_CHROME.r, COL_CHROME.g, COL_CHROME.b, 170));
		label.setString(ss.str());
		label.setPosition(hudX, y);
		draw3DText(target, label);

		if (grazeCount > 0) {
			label.setString(gs.str());
			sf::FloatRect b = label.getLocalBounds();
			label.setFillColor(sf::Color(COL_MINE.r, COL_MINE.g, COL_MINE.b,
				grazeFlashTimer > 0.f ? 255 : 200));
			label.setPosition(hudX + BAR_W - b.width, y);
			draw3DText(target, label);
		}
		y += 24.f;
	}

	// ── ACTIVE POWER-UPS ──────────────────────────────────────────────
	// Only drawn when something is running, so the resting HUD stays quiet.
	{
		struct PuInfo { float* timer; sf::Color color; const char* name; };
		PuInfo pus[] = {
			{ &invincibilityTimer, COL_MINE,               "INVULN" },
			{ &deathRayTimer,      COL_DANGER,             "D-RAY"  },
			{ &fastFireTimer,      COL_READY,              "RAPID"  },
			{ &slowTimeTimer,      sf::Color(150,160,230), "SLOW"   },
		};
		const float puMaxTime = 600.f;
		float x = hudX;
		for (int pi = 0; pi < 4; pi++) {
			if (*pus[pi].timer <= 0.f) continue;
			drawBar(target, sf::Vector2f(x, y + 14.f), sf::Vector2f(52.f, 5.f),
				*pus[pi].timer / puMaxTime, pus[pi].color);
			label.setCharacterSize(12);
			label.setFillColor(pus[pi].color);
			label.setString(pus[pi].name);
			label.setPosition(x, y);
			draw3DText(target, label);
			x += 58.f;
		}
	}
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