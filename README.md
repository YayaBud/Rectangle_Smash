# Rectangle Smash

Rectangle Smash (formerly known as "2nd_try") is a 2D desktop game developed in C++ using the Simple and Fast Multimedia Library (SFML).

## Overview
Rectangle Smash is a fast-paced interactive game where players must click on spawning colored shapes directly to earn points while maintaining their health. The game evaluates reaction times and tests how well players can manage their screen without missing targets.

## Features
- **C++ and SFML:** Built purely in C++ utilizing SFML for window management, graphics, and text rendering.
- **Dynamic Entities:** Enemies and targets spawn dynamically with varying sizes and colors.
- **Game State Management:** Includes a full game loop handling initialization, updates, rendering, health tracking, score points, and a built-in "Game Over" state.
- **Custom Fonts & UI:** Incorporates custom font files to render health, points, and end-game information to the player in real-time.

## Prerequisites
- **Visual Studio** (Recommended: 2022 or newer)
- **C++ Build Tools** installed in Visual Studio.
- **SFML Library** (Included/linked in the project environment dependencies).

## Getting Started
1. Clone or download the repository.
2. Navigate to the project root directory.
3. Open `Rectangle Smash.sln` using Visual Studio.
4. Ensure your configuration is set exactly to your system requirements (e.g., `x64` and `Debug` or `Release`).
5. Build and Run the project (`F5` or `Ctrl + F5`).
6. Left-click the colored squares to gain points!

## Controls
- **Left Mouse Click:** Click on the colored targets to score points.
- **ESC Key:** Exit the game at any time (especially useful during the _Game Over_ screen).

## Project Structure
- `Rectangle Smash.sln`: Main solution file.
- `Rectangle Smash/Rectangle Smash.cpp`: Entry point containing the main game loop.
- `Rectangle Smash/game.cpp` & `game.h`: Core game logic, window initialization, entity spawning, update mechanics, and render processing.
- `External/SFML`: Contains SFML include headers and precompiled binaries required to run the game natively.
