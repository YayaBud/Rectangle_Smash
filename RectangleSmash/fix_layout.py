import re
with open('game_render.cpp', 'r') as f:
    text = f.read()

# Fix Brackets
text = text.replace('"[ SELECTED ]"', '"- SELECTED -"')
text = text.replace('"DASH [SPACE]"', '"DASH - SPACE"')
text = text.replace(' [or Auto]', ' - or Auto')
text = text.replace(' [direction = WASD]', ' - direction WASD')
text = text.replace(' [screen clear x3]', ' - screen clear x3')
text = text.replace(' [10s invincibility]', ' - 10s invincibility')
text = text.replace(' [wide laser, 10s]', ' - wide laser, 10s')
text = text.replace(' [fast shots, 10s]', ' - fast shots, 10s')
text = text.replace(' [all enemies slow]', ' - all enemies slow')
text = text.replace(' [spend in Shop]', ' - spend in Shop')
text = text.replace(' [5, 15, 20+]', ' - 5, 15, 20+')
text = text.replace('BOSS 2 [SURT]', 'BOSS 2 - SURT')

# Fix Menu Layout
text = text.replace('drawPanel(target, sf::Vector2f(cx.x, 460.f), sf::Vector2f(480.f, 440.f)', 'drawPanel(target, sf::Vector2f(cx.x, 485.f), sf::Vector2f(500.f, 500.f)')
text = text.replace('drawMenuItem(PlayText, 258.f);', 'drawMenuItem(PlayText, 288.f);')
text = text.replace('drawMenuItem(ShipSelectText, 323.f);', 'drawMenuItem(ShipSelectText, 353.f);')
text = text.replace('drawMenuItem(ShopText, 388.f);', 'drawMenuItem(ShopText, 418.f);')
text = text.replace('drawMenuItem(BlackMarketText, 453.f);', 'drawMenuItem(BlackMarketText, 483.f);')
text = text.replace('drawMenuItem(SettingsText, 518.f);', 'drawMenuItem(SettingsText, 548.f);')
text = text.replace('drawMenuItem(SkinsText, 583.f);', 'drawMenuItem(SkinsText, 613.f);')
text = text.replace('drawMenuItem(QuitText, 648.f);', 'drawMenuItem(QuitText, 678.f);')

with open('game_render.cpp', 'w') as f:
    f.write(text)
