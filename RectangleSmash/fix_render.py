import re
with open('game_render.cpp', 'r') as f:
    text = f.read()

# Add the draw3DText helper function at the top
helper = '''#include "game.h"

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
'''
text = text.replace('#include "game.h"', helper)

# Replace target.draw(textVar) with draw3DText(target, textVar)
# For specific text variables
text_vars = [
    'text', 'highScoreTitleText', 'highScoreEntryTexts[i]', 'gearsText', 'BackText',
    'UiText', 'waveBannerText', 'tName', 'tDesc', 'tCost', 'tLvl', 'nameT', 'detT',
    'lbl', 'val', 'hudTitle', 'heatLabel', 'dashLabel', 'pLabel', 'shieldLabel', 'secLabel',
    'title', 'prompt', 'sub', 'warn', 'sp.text', 'GameOverText', 'FinalScoreText',
    'SettingsTitleText', 'skinsTitleText', 'PlayText', 'ShipSelectText', 'ShopText',
    'BlackMarketText', 'SettingsText', 'QuitText', 'SkinsText', 'DiffEasyText', 'DiffNormalText',
    'DiffHardText', 'comboText', 'killBannerText', 'PausedText', 'MoveSpeedSettingText',
    'MoveSpeedMinus', 'MoveSpeedValText', 'MoveSpeedPlus', 'FireSpeedSettingText',
    'FireSpeedMinus', 'FireSpeedValText', 'FireSpeedPlus', 'FollowMouseSettingText',
    'AutoFireSettingText', 'OverheatSettingText', 't'
]
for var in text_vars:
    # We need to escape brackets for regex
    esc_var = re.escape(var)
    text = re.sub(fr'target\.draw\({esc_var}\);', f'draw3DText(target, {var});', text)
    # also handle window->draw since there are some of those
    text = re.sub(fr'window->draw\({esc_var}\);', f'draw3DText(*window, {var});', text)

with open('game_render.cpp', 'w') as f:
    f.write(text)
