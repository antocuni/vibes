// Snake Game - Pure JavaScript
(function () {
    "use strict";

    const canvas = document.getElementById("game");
    const ctx = canvas.getContext("2d");
    const scoreEl = document.getElementById("score");
    const highScoreEl = document.getElementById("high-score");
    const startOverlay = document.getElementById("start-overlay");
    const gameoverOverlay = document.getElementById("gameover-overlay");
    const startText = document.getElementById("start-text");
    const gameoverText = document.getElementById("gameover-text");

    // --- Configuration ---
    const LOGICAL_SIZE = 400; // logical canvas size
    const COLS = 20;
    const ROWS = 20;
    const GRID = LOGICAL_SIZE / COLS; // cell size in logical pixels
    const INITIAL_SPEED = 150; // ms per tick
    const SPEED_INCREMENT = 2; // ms faster per food eaten
    const MIN_SPEED = 60;
    const SWIPE_THRESHOLD = 20; // minimum px for a swipe

    // Colors
    const COLOR_BG = "#16213e";
    const COLOR_GRID = "#1a1a3e";
    const COLOR_SNAKE_HEAD = "#00ff88";
    const COLOR_SNAKE_BODY = "#00cc6a";
    const COLOR_FOOD = "#ff4466";
    const COLOR_FOOD_GLOW = "rgba(255, 68, 102, 0.3)";

    // --- Detect touch device ---
    const isTouchDevice = ("ontouchstart" in window) || (navigator.maxTouchPoints > 0);

    // Update overlay text for touch devices
    if (isTouchDevice) {
        startText.textContent = "Tap to start";
        gameoverText.textContent = "Tap to restart";
    }

    // --- State ---
    let snake, direction, nextDirection, food, score, highScore, speed;
    let running, paused, gameOver, animFrameId, lastTick;
    let canvasScale = 1;

    // Load high score from localStorage
    highScore = parseInt(localStorage.getItem("snake-high-score")) || 0;
    highScoreEl.textContent = highScore;

    // --- Responsive canvas sizing ---
    function resizeCanvas() {
        const maxWidth = Math.min(window.innerWidth - 24, 400);
        // Reserve space for header, score, controls, dpad
        const headerSpace = isTouchDevice ? 280 : 120;
        const maxHeight = window.innerHeight - headerSpace;
        const size = Math.min(maxWidth, maxHeight);
        const displaySize = Math.max(200, Math.floor(size));

        canvas.style.width = displaySize + "px";
        canvas.style.height = displaySize + "px";
        canvasScale = displaySize / LOGICAL_SIZE;

        sizeOverlay(startOverlay);
        sizeOverlay(gameoverOverlay);
    }

    // Size the overlay to match the canvas
    function sizeOverlay(overlay) {
        overlay.style.width = canvas.style.width;
        overlay.style.height = canvas.style.height;
        overlay.style.left = canvas.offsetLeft + "px";
        overlay.style.top = canvas.offsetTop + "px";
    }

    resizeCanvas();
    window.addEventListener("resize", resizeCanvas);

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

    // --- Direction helper ---
    function setDirection(dir) {
        if (dir.x + direction.x !== 0 || dir.y + direction.y !== 0) {
            nextDirection = dir;
        }
    }

    // --- Keyboard input ---
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
            setDirection(dir);
        }
    });

    // --- Touch / Swipe input on canvas ---
    let touchStartX = null;
    let touchStartY = null;
    let touchStartTime = 0;

    canvas.addEventListener("touchstart", function (e) {
        e.preventDefault();
        const touch = e.touches[0];
        touchStartX = touch.clientX;
        touchStartY = touch.clientY;
        touchStartTime = Date.now();
    }, { passive: false });

    canvas.addEventListener("touchmove", function (e) {
        e.preventDefault();
    }, { passive: false });

    canvas.addEventListener("touchend", function (e) {
        e.preventDefault();
        if (touchStartX === null) return;

        const touch = e.changedTouches[0];
        const dx = touch.clientX - touchStartX;
        const dy = touch.clientY - touchStartY;
        const elapsed = Date.now() - touchStartTime;

        touchStartX = null;
        touchStartY = null;

        const absDx = Math.abs(dx);
        const absDy = Math.abs(dy);

        // If the gesture is too small, treat as a tap
        if (absDx < SWIPE_THRESHOLD && absDy < SWIPE_THRESHOLD) {
            // Tap: start/restart if not running, toggle pause if running
            if (!running) {
                start();
            } else if (!gameOver) {
                paused = !paused;
            }
            return;
        }

        // Swipe detected - determine direction
        if (!running || gameOver || paused) return;

        if (absDx > absDy) {
            // Horizontal swipe
            setDirection(dx > 0 ? { x: 1, y: 0 } : { x: -1, y: 0 });
        } else {
            // Vertical swipe
            setDirection(dy > 0 ? { x: 0, y: 1 } : { x: 0, y: -1 });
        }
    }, { passive: false });

    // --- Overlay tap to start/restart ---
    startOverlay.addEventListener("click", function () {
        if (!running) start();
    });
    startOverlay.addEventListener("touchend", function (e) {
        e.preventDefault();
        if (!running) start();
    });

    gameoverOverlay.addEventListener("click", function () {
        if (!running) start();
    });
    gameoverOverlay.addEventListener("touchend", function (e) {
        e.preventDefault();
        if (!running) start();
    });

    // --- D-pad input ---
    const DPAD_MAP = {
        up:    { x:  0, y: -1 },
        down:  { x:  0, y:  1 },
        left:  { x: -1, y:  0 },
        right: { x:  1, y:  0 },
    };

    document.querySelectorAll(".dpad-btn").forEach(function (btn) {
        function handleDpad(e) {
            e.preventDefault();
            const dirName = btn.getAttribute("data-dir");
            const dir = DPAD_MAP[dirName];
            if (!dir) return;

            if (!running) {
                start();
                // Set direction after starting if it differs from default
                setDirection(dir);
                return;
            }

            if (paused || gameOver) return;
            setDirection(dir);
        }

        btn.addEventListener("touchstart", handleDpad, { passive: false });
        btn.addEventListener("mousedown", handleDpad);
    });

    // D-pad pause button
    const dpadPause = document.getElementById("dpad-pause");
    if (dpadPause) {
        function handlePause(e) {
            e.preventDefault();
            if (running && !gameOver) {
                paused = !paused;
            }
        }
        dpadPause.addEventListener("touchstart", handlePause, { passive: false });
        dpadPause.addEventListener("mousedown", handlePause);
    }

    // --- Prevent pull-to-refresh and zoom gestures on the whole page ---
    document.addEventListener("touchmove", function (e) {
        if (e.touches.length > 1) {
            e.preventDefault();
        }
    }, { passive: false });

    // --- Initial draw ---
    init();
    draw();
})();
