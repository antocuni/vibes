A fully-functional Snake game was developed using only HTML, CSS, and vanilla JavaScript, showcasing how modern browser APIs like [`<canvas>`](https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API) and `localStorage` can deliver a polished, responsive gaming experience without any frameworks. The game features smooth canvas rendering on a 20x20 grid, custom keyboard controls, and visual enhancements such as rounded segments and glowing effects, all fully client-side. The implementation emphasizes clean collision detection, game speed scaling for a natural difficulty curve, and persistent high scores for replayability.

**Key Features:**
- Canvas-rendered board with visual polish (glowing food, rounded snake)
- Configurable, smooth tick-based game loop using `requestAnimationFrame`
- Persistent high score via `localStorage`
- Fast, responsive controls with anti-reversal logic
- No dependencies or build steps; play immediately in any browser
