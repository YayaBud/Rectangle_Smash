import re
with open('game_init.cpp', 'r') as f:
    text = f.read()
text = text.replace('fonts/PressStart2P.ttf', 'fonts/Sabo-Regular.otf')
def scale_size(match):
    size = int(match.group(1))
    new_size = int(size * 0.85)
    return f'setCharacterSize({new_size})'
text = re.sub(r'setCharacterSize\((\d+)\)', scale_size, text)
with open('game_init.cpp', 'w') as f:
    f.write(text)
