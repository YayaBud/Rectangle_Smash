#include <iostream>
#include "game.h"

int main()
{
    std::srand(static_cast<unsigned>(time(NULL)));

    game Game;

    // Fixed-timestep simulation, decoupled from rendering.
    // Every gameplay timer in this project counts in whole frames (t -= 1.f),
    // so the simulation must advance at a constant rate or the game literally
    // runs slower when the frame takes longer. Render as often as the display
    // allows; step the simulation exactly SIM_HZ times per second.
    const float SIM_HZ = 120.f;
    const sf::Time STEP = sf::seconds(1.f / SIM_HZ);

    sf::Clock clock;
    sf::Time accumulator = sf::Time::Zero;

    while (Game.running())
    {
        accumulator += clock.restart();

        // ponytail: cap catch-up at 5 steps. A hitch (alt-tab, disk stall) would
        // otherwise queue seconds of simulation and the game fast-forwards.
        // Dropping the excess time is the lesser evil.
        const sf::Time maxCatchUp = STEP * 5.f;
        if (accumulator > maxCatchUp) accumulator = maxCatchUp;

        while (accumulator >= STEP && Game.running())
        {
            Game.update();
            accumulator -= STEP;
        }

        if (!Game.running()) break;
        Game.render();
    }

    return 0;
}
