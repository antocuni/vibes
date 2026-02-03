# Snake Game

A classic Snake game built with pure HTML, CSS, and vanilla JavaScript - no frameworks or dependencies. Fully playable on both desktop and mobile devices.

## What was explored

Building a complete, polished browser game using only vanilla web technologies. The goal was to create a clean, playable Snake implementation with smooth visuals and responsive controls.

## Implementation

- **Canvas-based rendering** - uses HTML5 `<canvas>` for the game board with a 20x20 grid
- **Game loop** - `requestAnimationFrame` with configurable tick rate that increases as the score goes up
- **Collision detection** - wall boundaries and self-collision end the game
- **High score persistence** - stores the best score in `localStorage`
- **Visual polish** - rounded snake segments, glowing food, subtle grid lines, dark theme
- **Keyboard controls** - arrow keys or WASD for movement, P to pause, Space/Enter to start/restart
- **Mobile support** - swipe gestures on the canvas for direction, on-screen D-pad with pause button, tap to start/restart
- **Responsive layout** - canvas scales to fit mobile screens; D-pad and mobile hints shown automatically on touch devices
- **Anti-reversal** - prevents the snake from doing a 180-degree turn into itself

## Key findings

- A simple tick-based game loop on top of `requestAnimationFrame` keeps rendering smooth while controlling game speed independently
- Starting at 150ms per tick and decreasing by 2ms per food eaten (minimum 60ms) gives a good difficulty curve
- Drawing rounded rectangles for the snake body and a glowing circle for food makes the game look polished with minimal code

## Usage

Open `index.html` in any modern browser. No build step or server required.

```bash
# Or use a simple local server
python3 -m http.server 8000 --directory snake-game
# Then open http://localhost:8000
```

Desktop controls:
- **Arrow keys** or **WASD** - move the snake
- **P** - pause/unpause
- **Space** or **Enter** - start or restart after game over

Mobile controls:
- **Swipe** on the game canvas to change direction
- **D-pad** buttons below the canvas for directional input
- **Tap** the canvas to start, restart, or pause
- **PAUSE** button in the center of the D-pad to pause/unpause
