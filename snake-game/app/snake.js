// Snake Game - Pure JavaScript
(function () {
    "use strict";

    const canvas = document.getElementById("game");
    const ctx = canvas.getContext("2d");
    const scoreEl = document.getElementById("score");
    const highScoreEl = document.getElementById("high-score");
    const startOverlay = document.getElementById("start-overlay");
    const gameoverOverlay = document.getElementById("gameover-overlay");

    // --- Configuration ---
    const GRID = 20; // cell size in pixels
    const COLS = canvas.width / GRID;
    const ROWS = canvas.height / GRID;
    const INITIAL_SPEED = 150; // ms per tick
    const SPEED_INCREMENT = 2; // ms faster per food eaten
    const MIN_SPEED = 60;

    // Colors
    const COLOR_BG = "#16213e";
    const COLOR_GRID = "#1a1a3e";
    const COLOR_SNAKE_HEAD = "#00ff88";
    const COLOR_SNAKE_BODY = "#00cc6a";
    const COLOR_FOOD = "#ff4466";
    const COLOR_FOOD_GLOW = "rgba(255, 68, 102, 0.3)";

    // --- State ---
    let snake, direction, nextDirection, food, score, highScore, speed;
    let running, paused, gameOver, animFrameId, lastTick;

    // Load high score from localStorage
    highScore = parseInt(localStorage.getItem("snake-high-score")) || 0;
    highScoreEl.textContent = highScore;

    // Size the overlay to match the canvas
    function sizeOverlay(overlay) {
        overlay.style.width = canvas.width + "px";
        overlay.style.height = canvas.height + "px";
        overlay.style.left = canvas.offsetLeft + "px";
        overlay.style.top = canvas.offsetTop + "px";
    }
    sizeOverlay(startOverlay);
    sizeOverlay(gameoverOverlay);

    // --- Initialization ---
    function init() {
        snake = [
            { x: Math.floor(COLS / 2), y: Math.floor(ROWS / 2) },
        ];
        direction = { x: 1, y: 0 };
        nextDirection = { x: 1, y: 0 };
        score = 0;
        speed = INITIAL_SPEED;
        scoreEl.textContent = 0;
        gameOver = false;
        paused = false;
        placeFood();
    }

    function placeFood() {
        let pos;
        do {
            pos = {
                x: Math.floor(Math.random() * COLS),
                y: Math.floor(Math.random() * ROWS),
            };
        } while (snake.some(seg => seg.x === pos.x && seg.y === pos.y));
        food = pos;
    }

    // --- Game loop ---
    function start() {
        init();
        running = true;
        startOverlay.classList.add("hidden");
        gameoverOverlay.classList.add("hidden");
        lastTick = performance.now();
        loop(lastTick);
    }

    function loop(timestamp) {
        animFrameId = requestAnimationFrame(loop);

        if (paused || gameOver) return;

        if (timestamp - lastTick >= speed) {
            lastTick = timestamp;
            update();
            if (gameOver) {
                handleGameOver();
                return;
            }
        }
        draw();
    }

    function update() {
        // Apply queued direction
        direction = nextDirection;

        // Calculate new head position
        const head = {
            x: snake[0].x + direction.x,
            y: snake[0].y + direction.y,
        };

        // Wall collision
        if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
            gameOver = true;
            return;
        }

        // Self collision
        if (snake.some(seg => seg.x === head.x && seg.y === head.y)) {
            gameOver = true;
            return;
        }

        snake.unshift(head);

        // Food collision
        if (head.x === food.x && head.y === food.y) {
            score++;
            scoreEl.textContent = score;
            speed = Math.max(MIN_SPEED, INITIAL_SPEED - score * SPEED_INCREMENT);
            placeFood();
        } else {
            snake.pop();
        }
    }

    function handleGameOver() {
        cancelAnimationFrame(animFrameId);
        running = false;

        if (score > highScore) {
            highScore = score;
            highScoreEl.textContent = highScore;
            localStorage.setItem("snake-high-score", highScore);
        }

        draw(); // final frame
        gameoverOverlay.classList.remove("hidden");
        sizeOverlay(gameoverOverlay);
    }

    // --- Drawing ---
    function draw() {
        // Background
        ctx.fillStyle = COLOR_BG;
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        // Grid lines
        ctx.strokeStyle = COLOR_GRID;
        ctx.lineWidth = 0.5;
        for (let x = 0; x <= canvas.width; x += GRID) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, canvas.height);
            ctx.stroke();
        }
        for (let y = 0; y <= canvas.height; y += GRID) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(canvas.width, y);
            ctx.stroke();
        }

        // Food glow
        ctx.fillStyle = COLOR_FOOD_GLOW;
        ctx.beginPath();
        ctx.arc(
            food.x * GRID + GRID / 2,
            food.y * GRID + GRID / 2,
            GRID * 0.8,
            0,
            Math.PI * 2
        );
        ctx.fill();

        // Food
        ctx.fillStyle = COLOR_FOOD;
        ctx.beginPath();
        ctx.arc(
            food.x * GRID + GRID / 2,
            food.y * GRID + GRID / 2,
            GRID * 0.4,
            0,
            Math.PI * 2
        );
        ctx.fill();

        // Snake
        snake.forEach((seg, i) => {
            const isHead = i === 0;
            ctx.fillStyle = isHead ? COLOR_SNAKE_HEAD : COLOR_SNAKE_BODY;
            const padding = isHead ? 1 : 2;
            const radius = isHead ? 4 : 3;

            roundRect(
                ctx,
                seg.x * GRID + padding,
                seg.y * GRID + padding,
                GRID - padding * 2,
                GRID - padding * 2,
                radius
            );
            ctx.fill();
        });
    }

    function roundRect(ctx, x, y, w, h, r) {
        ctx.beginPath();
        ctx.moveTo(x + r, y);
        ctx.lineTo(x + w - r, y);
        ctx.quadraticCurveTo(x + w, y, x + w, y + r);
        ctx.lineTo(x + w, y + h - r);
        ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
        ctx.lineTo(x + r, y + h);
        ctx.quadraticCurveTo(x, y + h, x, y + h - r);
        ctx.lineTo(x, y + r);
        ctx.quadraticCurveTo(x, y, x + r, y);
        ctx.closePath();
    }

    // --- Input handling ---
    const KEY_MAP = {
        ArrowUp:    { x:  0, y: -1 },
        ArrowDown:  { x:  0, y:  1 },
        ArrowLeft:  { x: -1, y:  0 },
        ArrowRight: { x:  1, y:  0 },
        w:          { x:  0, y: -1 },
        s:          { x:  0, y:  1 },
        a:          { x: -1, y:  0 },
        d:          { x:  1, y:  0 },
        W:          { x:  0, y: -1 },
        S:          { x:  0, y:  1 },
        A:          { x: -1, y:  0 },
        D:          { x:  1, y:  0 },
    };

    document.addEventListener("keydown", function (e) {
        // Start / restart
        if (!running) {
            if (e.key === " " || e.key === "Enter" || KEY_MAP[e.key]) {
                e.preventDefault();
                start();
                return;
            }
        }

        // Pause
        if (e.key === "p" || e.key === "P") {
            if (running && !gameOver) {
                paused = !paused;
            }
            return;
        }

        // Direction
        const dir = KEY_MAP[e.key];
        if (dir) {
            e.preventDefault();
            // Prevent 180-degree turn
            if (dir.x + direction.x !== 0 || dir.y + direction.y !== 0) {
                nextDirection = dir;
            }
        }
    });

    // --- Initial draw ---
    init();
    draw();
})();
