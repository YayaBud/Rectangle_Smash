import re

with open('game_init.cpp', 'r') as f:
    init_text = f.read()

def scale_up(match):
    size = int(match.group(1))
    new_size = int(size * 1.12)
    return f'setCharacterSize({new_size})'

init_text = re.sub(r'setCharacterSize\((\d+)\)', scale_up, init_text)
with open('game_init.cpp', 'w') as f:
    f.write(init_text)

with open('game_render.cpp', 'r') as f:
    render_text = f.read()

replacements = {
    '(or Auto)': '[or Auto]',
    '(direction = WASD)': '[direction = WASD]',
    '(screen clear x3)': '[screen clear x3]',
    '(10s invincibility)': '[10s invincibility]',
    '(wide laser, 10s)': '[wide laser, 10s]',
    '(fast shots, 10s)': '[fast shots, 10s]',
    '(all enemies slow)': '[all enemies slow]',
    '(spend in Shop)': '[spend in Shop]',
    '(5, 15, 20+)': '[5, 15, 20+]',
    'BOSS 2 (SURT)': 'BOSS 2 [SURT]'
}

for old, new in replacements.items():
    render_text = render_text.replace(old, new)

with open('game_render.cpp', 'w') as f:
    f.write(render_text)

