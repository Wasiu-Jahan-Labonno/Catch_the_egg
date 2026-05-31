/*
 * JUNGLE EGG CHASE
 * Build: g++ catchTheEgg.cpp -o jungle_egg_chase.exe -lfreeglut -lglu32 -lopengl32
 *
 * Controls
 * --------
 *  Main Menu  : ENTER = start | H = help | ESC = quit
 *  Playing    : LEFT/RIGHT arrows = move basket | ESC = pause
 *  Paused     : ESC or P = resume | H = help | M = main menu | Q = quit
 *  Help       : BACKSPACE = back
 *  Game Over  : ENTER = play again | M = main menu | ESC = quit
 */

#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

// ─── Window ──────────────────────────────────────────────────────────────────
const int WINDOW_WIDTH  = 1000;
const int WINDOW_HEIGHT = 600;

// ─── World constants ─────────────────────────────────────────────────────────
const float HORIZON_Y = 0.1f;
const float PLAYER_Y  = -0.8f;

// ─── Game states ─────────────────────────────────────────────────────────────
enum GameState { MENU, PLAYING, PAUSED, HELP, GAMEOVER };
GameState currentState    = MENU;
GameState helpReturnState = MENU;   // where BACKSPACE goes from Help

// ─── Item types ──────────────────────────────────────────────────────────────
enum ItemType { NORMAL_EGG, GOLDEN_EGG, ROTTEN_EGG, POWERUP_BIG_NET, POWERUP_SPEED };

// ─── Structs ─────────────────────────────────────────────────────────────────
struct Tree       { float x, y, speed; bool isLeft; };
struct FallingItem { float x, y, targetX, speed, size; ItemType type; bool active; };
struct Cloud      { float x, y, speed, size; };

// ─── Game variables ──────────────────────────────────────────────────────────
float playerX    = 0.0f;
float playerSpeed = 0.025f;
float netWidth   = 0.25f;
float dinoX      = 0.0f, dinoTargetX = 0.0f;
int   dinoTimer  = 0;
int   score      = 0;
int   highScore  = 0;
int   lives      = 3;
int   bigNetTimer = 0, speedTimer = 0;
bool  keys[256]  = { false };

// Animated title pulse (frame counter)
int   menuFrame  = 0;

std::vector<Tree>        jungleTrees;
std::vector<FallingItem> activeItems;
std::vector<Cloud>       skyClouds;

// ─── Forward declarations ────────────────────────────────────────────────────
void init();
void resetGame();
void updateGame();
void spawnTree(bool leftSide);
void spawnItem();

// Drawing helpers
void drawEllipse(float cx, float cy, float rx, float ry, int segs = 20);
void drawCircle(float cx, float cy, float r, int segs);
void drawEllipseOutline(float cx, float cy, float rx, float ry, int segs = 20);
void drawFilledRect(float x1, float y1, float x2, float y2);
void drawText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18);
void drawTextLarge(const char* text, float x, float y);

// Scene draw functions
void drawEnvironment();
void drawJungleTrees();
void drawDinosaur();
void drawEggShape(float x, float y, float size, float r, float g, float b);
void drawFallingItems();
void drawPlayerWithNet();
void drawHUD();

// Screen draw functions
void drawMenuScreen();
void drawPauseScreen();
void drawHelpScreen();
void drawGameOverScreen();
void drawDimOverlay(float alpha);

// GLUT callbacks
void display();
void timerCB(int v);
void keyboardDown(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void specialDown(int key, int x, int y);
void specialUp(int key, int x, int y);

// ═════════════════════════════════════════════════════════════════════════════
// Init / Reset
// ═════════════════════════════════════════════════════════════════════════════

void init() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    srand((unsigned)time(NULL));

    skyClouds.push_back({-0.6f, 0.6f,  0.001f,  0.15f});
    skyClouds.push_back({ 0.2f, 0.45f, 0.0007f, 0.20f});
    skyClouds.push_back({ 0.7f, 0.7f,  0.0013f, 0.12f});

    for (int i = 0; i < 6; i++) {
        float startY = HORIZON_Y - (i * 0.18f);
        jungleTrees.push_back({ -0.4f - (rand() % 40 / 100.0f), startY, 0.008f, true  });
        jungleTrees.push_back({  0.4f + (rand() % 40 / 100.0f), startY, 0.008f, false });
    }
}

void resetGame() {
    score        = 0;
    lives        = 3;
    playerX      = 0.0f;
    playerSpeed  = 0.025f;
    netWidth     = 0.25f;
    bigNetTimer  = 0;
    speedTimer   = 0;
    dinoX        = 0.0f;
    dinoTargetX  = 0.0f;
    dinoTimer    = 0;
    activeItems.clear();
    currentState = PLAYING;
}

// ═════════════════════════════════════════════════════════════════════════════
// Drawing Primitives
// ═════════════════════════════════════════════════════════════════════════════

void drawEllipse(float cx, float cy, float rx, float ry, int segs) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segs; i++) {
        float theta = 2.0f * 3.14159265f * i / segs;
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

void drawCircle(float cx, float cy, float r, int segs) {
    drawEllipse(cx, cy, r, r, segs);
}

void drawEllipseOutline(float cx, float cy, float rx, float ry, int segs) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        float theta = 2.0f * 3.14159265f * i / segs;
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

// Draw a filled screen-coordinate quad using OpenGL coords (-1..1)
void drawFilledRect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
        glVertex2f(x1, y1); glVertex2f(x2, y1);
        glVertex2f(x2, y2); glVertex2f(x1, y2);
    glEnd();
}

void drawText(const char* text, float x, float y, void* font) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++)
        glutBitmapCharacter(font, *c);
}

void drawTextLarge(const char* text, float x, float y) {
    drawText(text, x, y, GLUT_BITMAP_TIMES_ROMAN_24);
}

// ═════════════════════════════════════════════════════════════════════════════
// Scene Rendering
// ═════════════════════════════════════════════════════════════════════════════

void drawEnvironment() {
    // Sky gradient
    glBegin(GL_QUADS);
        glColor3f(0.0f, 0.4f, 0.6f);  glVertex2f(-1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glColor3f(0.8f, 0.5f, 0.3f);  glVertex2f(1.0f, HORIZON_Y); glVertex2f(-1.0f, HORIZON_Y);
    glEnd();

    // Mountain silhouettes
    glColor3f(0.2f, 0.25f, 0.3f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-1.0f, HORIZON_Y); glVertex2f(-0.6f, 0.4f);  glVertex2f(-0.2f, HORIZON_Y);
        glVertex2f(-0.4f, HORIZON_Y); glVertex2f( 0.1f, 0.5f);  glVertex2f( 0.6f, HORIZON_Y);
        glVertex2f( 0.3f, HORIZON_Y); glVertex2f( 0.75f,0.35f); glVertex2f( 1.2f, HORIZON_Y);
    glEnd();

    // Clouds
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    for (auto& c : skyClouds) {
        drawCircle(c.x,                    c.y,                   c.size,        12);
        drawCircle(c.x + c.size * 0.5f,   c.y + c.size * 0.2f,  c.size * 0.8f, 12);
        drawCircle(c.x - c.size * 0.5f,   c.y + c.size * 0.1f,  c.size * 0.7f, 12);
    }

    // Ground
    glBegin(GL_QUADS);
        glColor3f(0.2f, 0.4f, 0.15f);  glVertex2f(-1.0f, HORIZON_Y); glVertex2f(1.0f, HORIZON_Y);
        glColor3f(0.1f, 0.25f, 0.08f); glVertex2f(1.0f, -1.0f);      glVertex2f(-1.0f, -1.0f);
    glEnd();

    // Perspective path lines
    glColor3f(0.15f, 0.3f, 0.1f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        glVertex2f(0.0f, HORIZON_Y); glVertex2f(-1.0f, -1.0f);
        glVertex2f(0.0f, HORIZON_Y); glVertex2f( 1.0f, -1.0f);
    glEnd();
}

void drawJungleTrees() {
    for (const auto& t : jungleTrees) {
        float tFactor = (HORIZON_Y - t.y) / (HORIZON_Y - (-1.0f));
        tFactor = std::max(0.0f, std::min(1.0f, tFactor));
        float scale   = 0.02f + (tFactor * 0.18f);
        float currentX = t.x + (t.isLeft ? -(tFactor * 0.4f) : (tFactor * 0.4f));

        // Trunk
        glColor3f(0.35f, 0.2f, 0.05f);
        glBegin(GL_QUADS);
            glVertex2f(currentX - scale*0.15f, t.y);
            glVertex2f(currentX + scale*0.15f, t.y);
            glVertex2f(currentX + scale*0.15f, t.y + scale*1.2f);
            glVertex2f(currentX - scale*0.15f, t.y + scale*1.2f);
        glEnd();
        // Lower foliage
        glColor3f(0.05f, 0.35f, 0.1f);
        glBegin(GL_TRIANGLES);
            glVertex2f(currentX - scale,  t.y + scale*0.8f);
            glVertex2f(currentX + scale,  t.y + scale*0.8f);
            glVertex2f(currentX,          t.y + scale*2.2f);
        glEnd();
        // Upper foliage
        glColor3f(0.08f, 0.45f, 0.12f);
        glBegin(GL_TRIANGLES);
            glVertex2f(currentX - scale*0.75f, t.y + scale*1.3f);
            glVertex2f(currentX + scale*0.75f, t.y + scale*1.3f);
            glVertex2f(currentX,               t.y + scale*2.7f);
        glEnd();
    }
}

void drawDinosaur() {
    float dinoY = HORIZON_Y + 0.05f;
    float s = 0.08f;
    glColor3f(0.1f, 0.4f, 0.3f);
    glBegin(GL_POLYGON);
        glVertex2f(dinoX - s,        dinoY);
        glVertex2f(dinoX + s,        dinoY);
        glVertex2f(dinoX + s*0.5f,   dinoY + s*1.3f);
        glVertex2f(dinoX - s*0.5f,   dinoY + s*1.3f);
    glEnd();
    glColor3f(0.8f, 0.3f, 0.1f);
    glBegin(GL_TRIANGLES);
        glVertex2f(dinoX - s*0.2f, dinoY + s*1.3f);
        glVertex2f(dinoX + s*0.2f, dinoY + s*1.3f);
        glVertex2f(dinoX,          dinoY + s*1.7f);
    glEnd();
}

void drawEggShape(float x, float y, float size, float r, float g, float b) {
    float rx = size * 0.72f, ry = size;
    glColor3f(r, g, b);
    drawEllipse(x, y, rx, ry, 18);
    glColor4f(1.0f, 1.0f, 1.0f, 0.35f);
    drawEllipse(x + rx*0.22f, y + ry*0.28f, rx*0.28f, ry*0.2f, 14);
    glColor4f(r*0.65f, g*0.65f, b*0.65f, 0.8f);
    glLineWidth(1.2f);
    drawEllipseOutline(x, y, rx, ry, 18);
}

void drawFallingItems() {
    for (const auto& item : activeItems) {
        if (!item.active) continue;
        switch (item.type) {
            case NORMAL_EGG:
                drawEggShape(item.x, item.y, item.size, 0.97f, 0.93f, 0.84f);
                break;
            case GOLDEN_EGG:
                drawEggShape(item.x, item.y, item.size, 1.0f, 0.82f, 0.0f);
                glColor3f(1.0f, 1.0f, 0.5f);
                for (int i = 0; i < 4; i++) {
                    float angle = i * 3.14159f / 2.0f;
                    drawCircle(item.x + cosf(angle)*(item.size+0.012f),
                               item.y + sinf(angle)*(item.size+0.012f), 0.006f, 6);
                }
                break;
            case ROTTEN_EGG:
                drawEggShape(item.x, item.y, item.size, 0.32f, 0.42f, 0.18f);
                glColor4f(0.15f, 0.2f, 0.08f, 0.7f);
                drawCircle(item.x - item.size*0.2f, item.y + item.size*0.15f, item.size*0.18f, 8);
                drawCircle(item.x + item.size*0.25f,item.y - item.size*0.2f,  item.size*0.14f, 8);
                break;
            case POWERUP_BIG_NET:
                glColor3f(0.9f, 0.2f, 0.2f);
                glBegin(GL_POLYGON);
                    glVertex2f(item.x,            item.y + item.size*1.3f);
                    glVertex2f(item.x + item.size, item.y);
                    glVertex2f(item.x,            item.y - item.size*1.3f);
                    glVertex2f(item.x - item.size, item.y);
                glEnd();
                glColor3f(1.0f, 0.5f, 0.5f); glLineWidth(1.5f);
                glBegin(GL_LINE_LOOP);
                    glVertex2f(item.x,            item.y + item.size*1.3f);
                    glVertex2f(item.x + item.size, item.y);
                    glVertex2f(item.x,            item.y - item.size*1.3f);
                    glVertex2f(item.x - item.size, item.y);
                glEnd();
                break;
            case POWERUP_SPEED:
                glColor3f(0.1f, 0.85f, 0.85f);
                glBegin(GL_POLYGON);
                    glVertex2f(item.x,            item.y + item.size*1.3f);
                    glVertex2f(item.x + item.size, item.y);
                    glVertex2f(item.x,            item.y - item.size*1.3f);
                    glVertex2f(item.x - item.size, item.y);
                glEnd();
                glColor3f(0.6f, 1.0f, 1.0f); glLineWidth(1.5f);
                glBegin(GL_LINE_LOOP);
                    glVertex2f(item.x,            item.y + item.size*1.3f);
                    glVertex2f(item.x + item.size, item.y);
                    glVertex2f(item.x,            item.y - item.size*1.3f);
                    glVertex2f(item.x - item.size, item.y);
                glEnd();
                break;
        }
    }
}

void drawPlayerWithNet() {
    // Body
    if (speedTimer > 0) glColor3f(0.9f, 0.8f, 0.2f);
    else                glColor3f(0.2f, 0.5f, 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(playerX - 0.05f, PLAYER_Y);
        glVertex2f(playerX + 0.05f, PLAYER_Y);
        glVertex2f(playerX + 0.04f, PLAYER_Y + 0.15f);
        glVertex2f(playerX - 0.04f, PLAYER_Y + 0.15f);
    glEnd();

    float halfNet = netWidth / 2.0f;
    float bx   = playerX - halfNet;
    float bTop = PLAYER_Y + 0.12f;
    float bBot = PLAYER_Y - 0.04f;

    // Basket body
    glColor3f(0.52f, 0.26f, 0.07f);
    glBegin(GL_QUADS);
        glVertex2f(bx,            bBot); glVertex2f(bx + netWidth, bBot);
        glVertex2f(bx + netWidth, bTop); glVertex2f(bx,            bTop);
    glEnd();
    // Weave lines
    glColor3f(0.38f, 0.18f, 0.04f); glLineWidth(1.0f);
    for (int i = 1; i < 4; i++) {
        float ly = bBot + (bTop - bBot) * i / 4.0f;
        glBegin(GL_LINES); glVertex2f(bx, ly); glVertex2f(bx + netWidth, ly); glEnd();
    }
    for (int i = 1; i < 8; i++) {
        float lx = bx + netWidth * i / 8.0f;
        glBegin(GL_LINES); glVertex2f(lx, bBot); glVertex2f(lx, bTop); glEnd();
    }
    // Rim
    if (bigNetTimer > 0) glColor3f(0.95f, 0.25f, 0.25f);
    else                  glColor3f(0.70f, 0.40f, 0.10f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
        glVertex2f(bx - 0.01f,            bTop);
        glVertex2f(bx + netWidth + 0.01f, bTop);
    glEnd();
    // Handle arc
    glColor3f(0.55f, 0.28f, 0.07f); glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 16; i++) {
        float t2 = float(i) / 16.0f;
        glVertex2f(bx + netWidth * t2, bTop + 0.055f * sinf(3.14159f * t2));
    }
    glEnd();
    // Wide-net glow
    if (bigNetTimer > 0) {
        glColor4f(0.9f, 0.2f, 0.2f, 0.18f);
        drawFilledRect(bx - 0.01f, bBot, bx + netWidth + 0.01f, bTop + 0.01f);
    }
}

void drawHUD() {
    char buf[64];
    // Score
    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "Score: %d", score);
    drawText(buf, -0.95f, 0.9f);
    // High score
    glColor3f(0.9f, 0.8f, 0.3f);
    sprintf(buf, "Best: %d", highScore);
    drawText(buf, -0.95f, 0.82f, GLUT_BITMAP_HELVETICA_12);
    // Lives as hearts
    glColor3f(1.0f, 0.3f, 0.3f);
    sprintf(buf, "Lives: %d", lives);
    drawText(buf, 0.65f, 0.9f);
    // Active power-ups
    char powerupStr[80] = "";
    if      (bigNetTimer > 0 && speedTimer > 0) sprintf(powerupStr, "[WIDE NET + SPEED]");
    else if (bigNetTimer > 0)                   sprintf(powerupStr, "[WIDE NET ACTIVE]");
    else if (speedTimer  > 0)                   sprintf(powerupStr, "[SPEED BOOST ACTIVE]");
    if (strlen(powerupStr) > 0) {
        glColor3f(0.9f, 0.8f, 0.2f);
        drawText(powerupStr, -0.18f, 0.9f);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Overlay & Screen Helpers
// ═════════════════════════════════════════════════════════════════════════════

void drawDimOverlay(float alpha) {
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    drawFilledRect(-1.0f, -1.0f, 1.0f, 1.0f);
}

// Draw a semi-transparent rounded panel (just a plain rect for simplicity)
void drawPanel(float cx, float cy, float hw, float hh,
               float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    drawFilledRect(cx - hw, cy - hh, cx + hw, cy + hh);
    // thin border
    glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(cx - hw, cy - hh); glVertex2f(cx + hw, cy - hh);
        glVertex2f(cx + hw, cy + hh); glVertex2f(cx - hw, cy + hh);
    glEnd();
}

// Draw a centred string (approximate – each HELVETICA_18 char ~10px wide in
// a 1000-px window that maps to 2 units, so 1 char ≈ 0.02 units)
// Just a convenience – caller can fine-tune x manually.

// ─── Small egg icon used on menu / help screens ───────────────────────────
void drawSmallEgg(float x, float y, float sz,
                  float r, float g, float b) {
    glColor3f(r, g, b);
    drawEllipse(x, y, sz * 0.72f, sz, 18);
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f);
    drawEllipse(x + sz*0.18f, y + sz*0.25f, sz*0.26f, sz*0.18f, 10);
    glColor4f(r*0.6f, g*0.6f, b*0.6f, 0.9f);
    glLineWidth(1.0f);
    drawEllipseOutline(x, y, sz * 0.72f, sz, 18);
}

// Small coloured diamond (power-up icon)
void drawSmallDiamond(float x, float y, float sz, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
        glVertex2f(x,      y + sz);
        glVertex2f(x + sz, y);
        glVertex2f(x,      y - sz);
        glVertex2f(x - sz, y);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,      y + sz);
        glVertex2f(x + sz, y);
        glVertex2f(x,      y - sz);
        glVertex2f(x - sz, y);
    glEnd();
}

// ═════════════════════════════════════════════════════════════════════════════
// MAIN MENU
// ═════════════════════════════════════════════════════════════════════════════

void drawMenuScreen() {
    // ── Background: draw live scene (clouds still move, trees scroll) ──
    drawEnvironment();
    drawJungleTrees();

    // ── Semi-transparent centre panel ──
    drawPanel(0.0f, 0.05f, 0.72f, 0.88f, 0.04f, 0.12f, 0.04f, 0.82f);

    // ── Title ──
    // Pulsing colour
    float pulse = 0.75f + 0.25f * sinf(menuFrame * 0.05f);
    glColor3f(0.3f * pulse, 1.0f * pulse, 0.3f * pulse);
    drawTextLarge("JUNGLE EGG CHASE", -0.42f, 0.72f);

    // Subtitle / tagline
    glColor3f(0.75f, 0.88f, 0.65f);
    drawText("A prehistoric egg-catching adventure!", -0.38f, 0.60f, GLUT_BITMAP_HELVETICA_12);

    // ── Divider ──
    glColor4f(0.5f, 0.8f, 0.5f, 0.4f);
    glLineWidth(1.0f);
    glBegin(GL_LINES); glVertex2f(-0.65f, 0.54f); glVertex2f(0.65f, 0.54f); glEnd();

    // ── Animated egg preview row ──
    float eggBob = sinf(menuFrame * 0.06f) * 0.025f;
    // Normal
    drawSmallEgg(-0.50f, 0.38f + eggBob, 0.055f, 0.97f, 0.93f, 0.84f);
    glColor3f(0.85f, 0.85f, 0.75f);
    drawText("+10", -0.565f, 0.29f, GLUT_BITMAP_HELVETICA_12);
    // Golden
    drawSmallEgg(-0.15f, 0.38f + eggBob * 1.3f, 0.055f, 1.0f, 0.82f, 0.0f);
    glColor3f(1.0f, 0.9f, 0.4f);
    drawText("+35", -0.215f, 0.29f, GLUT_BITMAP_HELVETICA_12);
    // Rotten
    drawSmallEgg( 0.20f, 0.38f + eggBob * 0.8f, 0.055f, 0.32f, 0.42f, 0.18f);
    glColor3f(0.6f, 0.75f, 0.4f);
    drawText("-25 & life", 0.135f, 0.29f, GLUT_BITMAP_HELVETICA_12);
    // Big net power-up diamond
    drawSmallDiamond(0.55f, 0.38f + eggBob, 0.04f, 0.9f, 0.2f, 0.2f);
    glColor3f(0.9f, 0.5f, 0.5f);
    drawText("PWR", 0.515f, 0.29f, GLUT_BITMAP_HELVETICA_12);

    // ── Divider ──
    glColor4f(0.5f, 0.8f, 0.5f, 0.4f);
    glBegin(GL_LINES); glVertex2f(-0.65f, 0.24f); glVertex2f(0.65f, 0.24f); glEnd();

    // ── Menu items ──
    // ENTER – Play
    glColor3f(0.25f, 1.0f, 0.45f);
    drawTextLarge("[ ENTER ]  Start Game", -0.30f, 0.13f);

    // H – Help
    glColor3f(0.55f, 0.85f, 1.0f);
    drawTextLarge("[ H ]      How To Play", -0.30f, -0.03f);

    // ESC – Quit
    glColor3f(1.0f, 0.45f, 0.45f);
    drawTextLarge("[ ESC ]    Quit Game",  -0.30f, -0.19f);

    // ── Divider ──
    glColor4f(0.5f, 0.8f, 0.5f, 0.4f);
    glBegin(GL_LINES); glVertex2f(-0.65f, -0.28f); glVertex2f(0.65f, -0.28f); glEnd();

    // ── High score ──
    if (highScore > 0) {
        char buf[48];
        sprintf(buf, "High Score: %d", highScore);
        glColor3f(1.0f, 0.82f, 0.2f);
        drawText(buf, -0.18f, -0.38f);
    } else {
        glColor3f(0.55f, 0.7f, 0.55f);
        drawText("No high score yet – be the first!", -0.33f, -0.38f, GLUT_BITMAP_HELVETICA_12);
    }

    // ── Footer hint ──
    glColor3f(0.4f, 0.55f, 0.4f);
    drawText("Catch eggs dropped by the dinosaur at the horizon.",
             -0.48f, -0.56f, GLUT_BITMAP_HELVETICA_12);
    drawText("Don't drop normal/golden eggs – you'll lose a life!",
             -0.47f, -0.65f, GLUT_BITMAP_HELVETICA_12);
}

// ═════════════════════════════════════════════════════════════════════════════
// HELP SCREEN
// ═════════════════════════════════════════════════════════════════════════════

void drawHelpScreen() {
    drawEnvironment();
    drawDimOverlay(0.78f);

    // Main panel
    drawPanel(0.0f, 0.0f, 0.82f, 0.92f, 0.04f, 0.10f, 0.16f, 0.90f);

    // Title
    glColor3f(0.4f, 0.9f, 1.0f);
    drawTextLarge("HOW TO PLAY", -0.20f, 0.80f);
    glColor4f(0.4f, 0.8f, 1.0f, 0.4f);
    glBegin(GL_LINES); glVertex2f(-0.75f, 0.73f); glVertex2f(0.75f, 0.73f); glEnd();

    // ── Controls section ──────────────────────────────────────────────────
    glColor3f(1.0f, 0.85f, 0.3f);
    drawText("CONTROLS", -0.72f, 0.64f);

    const char* controls[][2] = {
        { "LEFT / RIGHT Arrows", "Move the basket" },
        { "ESC  (while playing)", "Pause the game"  },
        { "ESC  (while paused)",  "Resume"          },
        { "P   (while paused)",   "Resume"          },
        { "H   (while paused)",   "Open this Help"  },
        { "M   (while paused)",   "Return to menu"  },
        { "Q   (while paused)",   "Quit game"       },
    };
    int nCtrl = sizeof(controls) / sizeof(controls[0]);
    for (int i = 0; i < nCtrl; i++) {
        float y = 0.54f - i * 0.10f;
        glColor3f(0.7f, 0.95f, 0.7f);
        drawText(controls[i][0], -0.72f, y, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.55f, 0.75f, 0.55f);
        drawText(controls[i][1],  0.10f, y, GLUT_BITMAP_HELVETICA_12);
    }

    // Divider
    glColor4f(0.4f, 0.8f, 1.0f, 0.3f);
    glBegin(GL_LINES); glVertex2f(-0.75f, -0.18f); glVertex2f(0.75f, -0.18f); glEnd();

    // ── Egg / item guide ──────────────────────────────────────────────────
    glColor3f(1.0f, 0.85f, 0.3f);
    drawText("EGG & ITEM GUIDE", -0.72f, -0.26f);

    struct EggInfo {
        float r, g, b;          // colour
        bool  isDiamond;        // true = draw diamond icon, false = egg icon
        float dr, dg, db;      // diamond colour (if diamond)
        const char* name;
        const char* effect;
    };
    EggInfo eggs[] = {
        { 0.97f, 0.93f, 0.84f, false, 0,0,0, "White Egg",    "+10 points – catch these!"               },
        { 1.00f, 0.82f, 0.00f, false, 0,0,0, "Golden Egg",   "+35 points – rare and valuable"          },
        { 0.32f, 0.42f, 0.18f, false, 0,0,0, "Rotten Egg",   "-25 points AND lose 1 life – AVOID!"     },
        { 0,0,0,                true, 0.9f,0.2f,0.2f, "Red Diamond",  "Wide Net power-up – bigger basket (~4 s)"},
        { 0,0,0,                true, 0.1f,0.85f,0.85f,"Cyan Diamond","Speed Boost – move faster (~4 s)"       },
    };
    int nEggs = sizeof(eggs) / sizeof(eggs[0]);
    for (int i = 0; i < nEggs; i++) {
        float y = -0.37f - i * 0.10f;
        if (eggs[i].isDiamond)
            drawSmallDiamond(-0.68f, y + 0.025f, 0.032f, eggs[i].dr, eggs[i].dg, eggs[i].db);
        else
            drawSmallEgg(-0.68f, y + 0.025f, 0.042f, eggs[i].r, eggs[i].g, eggs[i].b);

        glColor3f(0.9f, 0.9f, 0.9f);
        drawText(eggs[i].name,   -0.60f, y, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.60f, 0.78f, 0.60f);
        drawText(eggs[i].effect, -0.10f, y, GLUT_BITMAP_HELVETICA_12);
    }

    // ── Tip ──────────────────────────────────────────────────────────────
    glColor4f(0.4f, 0.8f, 1.0f, 0.3f);
    glBegin(GL_LINES); glVertex2f(-0.75f, -0.86f); glVertex2f(0.75f, -0.86f); glEnd();
    glColor3f(0.5f, 0.7f, 0.5f);
    drawText("TIP: eggs grow larger as they approach – time your moves early!",
             -0.70f, -0.90f, GLUT_BITMAP_HELVETICA_12);

    // ── Back button ──────────────────────────────────────────────────────
    glColor3f(0.9f, 0.65f, 0.2f);
    drawText("[ BACKSPACE ]  Go Back", -0.22f, -0.78f);
}

// ═════════════════════════════════════════════════════════════════════════════
// PAUSE SCREEN
// ═════════════════════════════════════════════════════════════════════════════

void drawPauseScreen() {
    // Game scene is already drawn underneath; just add overlay + panel
    drawDimOverlay(0.60f);
    drawPanel(0.0f, 0.10f, 0.45f, 0.68f, 0.05f, 0.08f, 0.05f, 0.88f);

    glColor3f(1.0f, 0.85f, 0.2f);
    drawTextLarge("PAUSED", -0.13f, 0.67f);

    glColor4f(1.0f, 0.85f, 0.2f, 0.35f);
    glBegin(GL_LINES); glVertex2f(-0.42f, 0.60f); glVertex2f(0.42f, 0.60f); glEnd();

    struct MenuItem { const char* key; const char* label; float r, g, b; };
    MenuItem items[] = {
        { "[ ESC ] or [ P ]", "Resume",        0.3f,  1.0f,  0.4f  },
        { "[ H ]",            "How To Play",   0.4f,  0.85f, 1.0f  },
        { "[ M ]",            "Main Menu",     0.9f,  0.75f, 0.3f  },
        { "[ Q ]",            "Quit Game",     1.0f,  0.4f,  0.4f  },
    };
    int n = sizeof(items) / sizeof(items[0]);
    for (int i = 0; i < n; i++) {
        float y = 0.46f - i * 0.19f;
        glColor3f(items[i].r, items[i].g, items[i].b);
        drawText(items[i].key,   -0.38f, y);
        glColor3f(items[i].r * 0.75f, items[i].g * 0.75f, items[i].b * 0.75f);
        drawText(items[i].label,  0.05f, y);
    }

    // Current score reminder
    glColor4f(1.0f, 0.85f, 0.2f, 0.35f);
    glBegin(GL_LINES); glVertex2f(-0.42f, -0.46f); glVertex2f(0.42f, -0.46f); glEnd();
    char buf[40];
    sprintf(buf, "Score so far: %d", score);
    glColor3f(0.75f, 0.75f, 0.75f);
    drawText(buf, -0.22f, -0.53f, GLUT_BITMAP_HELVETICA_12);
}

// ═════════════════════════════════════════════════════════════════════════════
// GAME OVER SCREEN
// ═════════════════════════════════════════════════════════════════════════════

void drawGameOverScreen() {
    drawDimOverlay(0.72f);
    drawPanel(0.0f, 0.10f, 0.55f, 0.70f, 0.15f, 0.02f, 0.02f, 0.88f);

    glColor3f(1.0f, 0.25f, 0.25f);
    drawTextLarge("GAME OVER", -0.20f, 0.68f);
    glColor3f(0.65f, 0.15f, 0.15f);
    drawText("The dinosaur outran you!", -0.28f, 0.57f, GLUT_BITMAP_HELVETICA_12);

    glColor4f(1.0f, 0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES); glVertex2f(-0.50f, 0.50f); glVertex2f(0.50f, 0.50f); glEnd();

    // Scores
    char buf[48];
    sprintf(buf, "Your Score:  %d", score);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(buf, -0.28f, 0.38f);

    bool newBest = (score >= highScore && score > 0);
    if (newBest) {
        glColor3f(1.0f, 0.85f, 0.1f);
        drawText("NEW HIGH SCORE!", -0.22f, 0.27f);
    } else {
        sprintf(buf, "High Score:  %d", highScore);
        glColor3f(0.85f, 0.72f, 0.3f);
        drawText(buf, -0.28f, 0.27f);
    }

    glColor4f(1.0f, 0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES); glVertex2f(-0.50f, 0.17f); glVertex2f(0.50f, 0.17f); glEnd();

    // Options
    struct MenuItem { const char* key; const char* label; float r, g, b; };
    MenuItem items[] = {
        { "[ ENTER ]", "Play Again",  0.3f, 1.0f, 0.4f  },
        { "[ M ]",     "Main Menu",   0.9f, 0.75f,0.3f  },
        { "[ ESC ]",   "Quit Game",   1.0f, 0.4f, 0.4f  },
    };
    int n = sizeof(items) / sizeof(items[0]);
    for (int i = 0; i < n; i++) {
        float y = 0.04f - i * 0.18f;
        glColor3f(items[i].r, items[i].g, items[i].b);
        drawText(items[i].key,  -0.38f, y);
        glColor3f(items[i].r*0.75f, items[i].g*0.75f, items[i].b*0.75f);
        drawText(items[i].label, 0.02f, y);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// GLUT display callback
// ═════════════════════════════════════════════════════════════════════════════

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (currentState == MENU) {
        drawMenuScreen();
    } else if (currentState == HELP) {
        // Help can be opened from menu or paused – draw the right thing behind it
        if (helpReturnState == MENU) {
            drawMenuScreen();
        } else {
            // Show the game scene frozen behind it
            drawEnvironment();
            drawJungleTrees();
            drawDinosaur();
            drawFallingItems();
            drawPlayerWithNet();
            drawHUD();
        }
        drawHelpScreen();
    } else if (currentState == PLAYING) {
        drawEnvironment();
        drawJungleTrees();
        drawDinosaur();
        drawFallingItems();
        drawPlayerWithNet();
        drawHUD();
    } else if (currentState == PAUSED) {
        drawEnvironment();
        drawJungleTrees();
        drawDinosaur();
        drawFallingItems();
        drawPlayerWithNet();
        drawHUD();
        drawPauseScreen();
    } else if (currentState == GAMEOVER) {
        drawEnvironment();
        drawJungleTrees();
        drawDinosaur();
        drawFallingItems();
        drawPlayerWithNet();
        drawHUD();
        drawGameOverScreen();
    }

    glutSwapBuffers();
}

// ═════════════════════════════════════════════════════════════════════════════
// Game Logic
// ═════════════════════════════════════════════════════════════════════════════

void spawnTree(bool leftSide) {
    Tree t;
    t.isLeft = leftSide;
    t.y      = HORIZON_Y;
    t.speed  = 0.008f;
    t.x      = leftSide ? (-0.3f - (rand() % 20 / 100.0f))
                        : ( 0.3f + (rand() % 20 / 100.0f));
    jungleTrees.push_back(t);
}

void spawnItem() {
    FallingItem item;
    item.x       = dinoX;
    item.y       = HORIZON_Y + 0.05f;
    item.speed   = 0.012f + (score * 0.0003f);
    item.active  = true;
    item.size    = 0.01f;
    item.targetX = dinoX * 2.5f;

    int roll = rand() % 100;
    if      (roll < 55) item.type = NORMAL_EGG;
    else if (roll < 75) item.type = ROTTEN_EGG;
    else if (roll < 88) item.type = GOLDEN_EGG;
    else if (roll < 94) item.type = POWERUP_BIG_NET;
    else                item.type = POWERUP_SPEED;

    activeItems.push_back(item);
}

void updateGame() {
    menuFrame++;  // shared frame counter (also drives menu animation)

    // Power-up timers
    if (bigNetTimer > 0) { bigNetTimer--; netWidth = 0.45f; } else netWidth    = 0.25f;
    if (speedTimer  > 0) { speedTimer--;  playerSpeed = 0.045f; } else playerSpeed = 0.025f;

    // Player movement
    if (keys[GLUT_KEY_LEFT])  playerX -= playerSpeed;
    if (keys[GLUT_KEY_RIGHT]) playerX += playerSpeed;
    playerX = std::max(-0.9f, std::min(0.9f, playerX));

    // Dinosaur AI
    if (--dinoTimer <= 0) {
        dinoTargetX = ((rand() % 160) - 80) / 100.0f;
        dinoTimer   = 30 + rand() % 50;
    }
    dinoX += (dinoTargetX - dinoX) * 0.06f;

    // Clouds (run even on menu/paused for ambience)
    for (auto& c : skyClouds) { c.x -= c.speed; if (c.x < -1.3f) c.x = 1.3f; }

    // Trees
    for (int i = 0; i < (int)jungleTrees.size(); i++) {
        jungleTrees[i].y     -= jungleTrees[i].speed;
        jungleTrees[i].speed += 0.00015f;
        if (jungleTrees[i].y < -1.1f) {
            bool side = jungleTrees[i].isLeft;
            jungleTrees.erase(jungleTrees.begin() + i);
            spawnTree(side);
            i--;
        }
    }

    // Falling items
    for (int i = 0; i < (int)activeItems.size(); i++) {
        auto& item = activeItems[i];
        if (!item.active) continue;

        float seg = (item.y - HORIZON_Y) / (PLAYER_Y - HORIZON_Y);
        seg = std::max(0.0f, std::min(1.0f, seg));

        item.y    -= item.speed;
        item.x     = dinoX + (seg * (item.targetX - dinoX));
        item.size  = 0.015f + (seg * 0.065f);

        // Catch check
        if (item.y <= PLAYER_Y + 0.05f && item.y >= PLAYER_Y - 0.05f) {
            float halfNet = netWidth / 2.0f;
            if (item.x >= playerX - halfNet && item.x <= playerX + halfNet) {
                item.active = false;
                switch (item.type) {
                    case NORMAL_EGG:      score += 10; break;
                    case GOLDEN_EGG:      score += 35; break;
                    case ROTTEN_EGG:
                        score = std::max(0, score - 25);
                        lives--;
                        break;
                    case POWERUP_BIG_NET: bigNetTimer = 250; break;
                    case POWERUP_SPEED:   speedTimer  = 250; break;
                }
                activeItems.erase(activeItems.begin() + i);
                i--;
                if (lives <= 0) {
                    highScore = std::max(highScore, score);
                    currentState = GAMEOVER;
                }
                continue;
            }
        }

        // Missed
        if (item.y < -1.1f) {
            if (item.type == NORMAL_EGG || item.type == GOLDEN_EGG) {
                lives--;
                if (lives <= 0) {
                    highScore = std::max(highScore, score);
                    currentState = GAMEOVER;
                }
            }
            activeItems.erase(activeItems.begin() + i);
            i--;
        }
    }

    if (rand() % 100 < 3 && activeItems.size() < 5) spawnItem();
}

// Lightweight update for non-playing states (just keeps clouds/trees animated)
void updateAmbient() {
    menuFrame++;
    for (auto& c : skyClouds) { c.x -= c.speed; if (c.x < -1.3f) c.x = 1.3f; }
    for (int i = 0; i < (int)jungleTrees.size(); i++) {
        jungleTrees[i].y -= jungleTrees[i].speed;
        jungleTrees[i].speed += 0.00015f;
        if (jungleTrees[i].y < -1.1f) {
            bool side = jungleTrees[i].isLeft;
            jungleTrees.erase(jungleTrees.begin() + i);
            spawnTree(side);
            i--;
        }
    }
}

void timerCB(int v) {
    if (currentState == PLAYING) {
        updateGame();
    } else {
        // Keep background animated on menu / paused / help / gameover
        updateAmbient();
    }
    glutPostRedisplay();
    glutTimerFunc(16, timerCB, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
// Input
// ═════════════════════════════════════════════════════════════════════════════

void keyboardDown(unsigned char key, int x, int y) {
    switch (currentState) {

    case MENU:
        if (key == 13) { // ENTER
            resetGame();
        } else if (key == 'h' || key == 'H') {
            helpReturnState = MENU;
            currentState    = HELP;
        } else if (key == 27) { // ESC
            exit(0);
        }
        break;

    case PLAYING:
        if (key == 27) { // ESC → pause
            currentState = PAUSED;
        }
        break;

    case PAUSED:
        if (key == 27 || key == 'p' || key == 'P') { // ESC or P → resume
            currentState = PLAYING;
        } else if (key == 'h' || key == 'H') {
            helpReturnState = PAUSED;
            currentState    = HELP;
        } else if (key == 'm' || key == 'M') {
            highScore = std::max(highScore, score);
            currentState = MENU;
        } else if (key == 'q' || key == 'Q') {
            exit(0);
        }
        break;

    case HELP:
        if (key == 8) { // BACKSPACE
            currentState = helpReturnState;
        }
        break;

    case GAMEOVER:
        if (key == 13) { // ENTER → play again
            resetGame();
        } else if (key == 'm' || key == 'M') {
            currentState = MENU;
        } else if (key == 27) { // ESC
            exit(0);
        }
        break;
    }
}

void keyboardUp(unsigned char key, int x, int y) { /* nothing needed */ }

void specialDown(int key, int x, int y) { keys[key] = true;  }
void specialUp  (int key, int x, int y) { keys[key] = false; }

// ═════════════════════════════════════════════════════════════════════════════
// Main
// ═════════════════════════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Jungle Egg Chase");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(0, timerCB, 0);

    glutMainLoop();
    return 0;
}