// CATCH THE EGGS

#include <GL/freeglut.h>
#include <windows.h>

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
const float HORIZON_Y = 0.15f;
const float PLAYER_Y  = -0.80f;

// ─── Game states ─────────────────────────────────────────────────────────────
enum GameState { MENU, PLAYING, PAUSED, HELP, GAMEOVER };
GameState currentState    = MENU;
GameState helpReturnState = MENU;

// ─── Item types ──────────────────────────────────────────────────────────────
enum ItemType {
    NORMAL_EGG,
    GOLDEN_EGG,
    BLUE_EGG,
    POOP,
    POWERUP_BIG_NET,
    POWERUP_SLOW,
    POWERUP_TIME
};

// ─── Structs ─────────────────────────────────────────────────────────────────
struct Tree        { float x, y, speed; bool isLeft; };
struct FallingItem {
    float x, y, spawnX, targetX, speed, size;
    ItemType type;
    bool active;
};
struct Cloud       { float x, y, speed, size; };

// ─── Game variables ──────────────────────────────────────────────────────────
float playerX      = 0.0f;
float playerSpeed  = 0.025f;
float netWidth     = 0.28f;
float chickenX     = 0.0f, chickenTargetX = 0.0f;
int   chickenTimer = 0;
float chickenWingAngle = 0.0f;   // for wing flap animation
float chickenWingDir   = 1.0f;
int   score        = 0;
int   highScore    = 0;
int   lives        = 3;
int   gameTimeLeft = 120 * 60;
int   bigNetTimer  = 0;
int   slowTimer    = 0;
float baseEggSpeed = 0.0036f;
bool  keys[512]    = { false };
int   menuFrame    = 0;
int   mouseX_raw   = WINDOW_WIDTH / 2;

std::vector<Tree>        jungleTrees;
std::vector<FallingItem> activeItems;
std::vector<Cloud>       skyClouds;

// ═════════════════════════════════════════════════════════════════════════════
//  COORDINATE HELPERS
//  NDC [-1,1] ↔ pixel [0, WINDOW_WIDTH/HEIGHT]
// ═════════════════════════════════════════════════════════════════════════════
static inline int ndcToPixelX(float x) {
    return (int)((x + 1.0f) * 0.5f * WINDOW_WIDTH);
}
static inline int ndcToPixelY(float y) {
    return (int)((y + 1.0f) * 0.5f * WINDOW_HEIGHT);
}
static inline float pixelToNdcX(int px) {
    return (float)px / WINDOW_WIDTH * 2.0f - 1.0f;
}
static inline float pixelToNdcY(int py) {
    return (float)py / WINDOW_HEIGHT * 2.0f - 1.0f;
}

// ═════════════════════════════════════════════════════════════════════════════
//  BRESENHAM'S LINE ALGORITHM
//  Plots a line between two NDC points using integer Bresenham in pixel space,
//  then submits each pixel as a GL_POINTS primitive (sub-pixel accurate).
// ═════════════════════════════════════════════════════════════════════════════
void bresenhamLine(float x0n, float y0n, float x1n, float y1n) {
    int x0 = ndcToPixelX(x0n);
    int y0 = ndcToPixelY(y0n);
    int x1 = ndcToPixelX(x1n);
    int y1 = ndcToPixelY(y1n);

    int dx =  abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    glBegin(GL_POINTS);
    while (true) {
        glVertex2f(pixelToNdcX(x0), pixelToNdcY(y0));
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    glEnd();
}

// Convenience: draw a line-strip polygon outline using Bresenham
void bresenhamRect(float x1, float y1, float x2, float y2) {
    bresenhamLine(x1, y1, x2, y1);
    bresenhamLine(x2, y1, x2, y2);
    bresenhamLine(x2, y2, x1, y2);
    bresenhamLine(x1, y2, x1, y1);
}

// ═════════════════════════════════════════════════════════════════════════════
//  MIDPOINT CIRCLE ALGORITHM
//  Plots the 8-way symmetric pixel points of a circle in NDC.
//  Used both standalone and as the basis for filled circles and ellipses.
// ═════════════════════════════════════════════════════════════════════════════
void midpointCircleOutline(float cxn, float cyn, float rn) {
    // Convert radius to pixels (use average of x/y pixel densities)
    int r  = (int)(rn * WINDOW_WIDTH * 0.5f);
    int cx = ndcToPixelX(cxn);
    int cy = ndcToPixelY(cyn);
    if (r <= 0) return;

    int x = 0, y = r;
    int d = 1 - r;

    // Lambda for plotting 8 symmetric points
    auto plot8 = [&](int px, int py) {
        glVertex2f(pixelToNdcX(cx + px), pixelToNdcY(cy + py));
        glVertex2f(pixelToNdcX(cx - px), pixelToNdcY(cy + py));
        glVertex2f(pixelToNdcX(cx + px), pixelToNdcY(cy - py));
        glVertex2f(pixelToNdcX(cx - px), pixelToNdcY(cy - py));
        glVertex2f(pixelToNdcX(cx + py), pixelToNdcY(cy + px));
        glVertex2f(pixelToNdcX(cx - py), pixelToNdcY(cy + px));
        glVertex2f(pixelToNdcX(cx + py), pixelToNdcY(cy - px));
        glVertex2f(pixelToNdcX(cx - py), pixelToNdcY(cy - px));
    };

    glBegin(GL_POINTS);
    plot8(x, y);
    while (x < y) {
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
        plot8(x, y);
    }
    glEnd();
}

// Filled circle via horizontal scanlines derived from midpoint algorithm
void midpointCircleFilled(float cxn, float cyn, float rn) {
    int r  = (int)(rn * WINDOW_WIDTH * 0.5f);
    int cx = ndcToPixelX(cxn);
    int cy = ndcToPixelY(cyn);
    if (r <= 0) return;

    int x = 0, y = r;
    int d = 1 - r;

    auto hline = [&](int px1, int px2, int py) {
        glVertex2f(pixelToNdcX(px1), pixelToNdcY(py));
        glVertex2f(pixelToNdcX(px2), pixelToNdcY(py));
    };

    glBegin(GL_LINES);
    while (x <= y) {
        hline(cx - y, cx + y, cy + x);
        hline(cx - y, cx + y, cy - x);
        hline(cx - x, cx + x, cy + y);
        hline(cx - x, cx + x, cy - y);
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
    glEnd();
}

// ═════════════════════════════════════════════════════════════════════════════
//  MIDPOINT ELLIPSE ALGORITHM  (used for egg bodies)
//  The classical midpoint / Bresenham ellipse in integer pixel space.
// ═════════════════════════════════════════════════════════════════════════════
void midpointEllipseFilled(float cxn, float cyn, float rxn, float ryn) {
    int cx = ndcToPixelX(cxn);
    int cy = ndcToPixelY(cyn);
    int rx = (int)(rxn * WINDOW_WIDTH  * 0.5f);
    int ry = (int)(ryn * WINDOW_HEIGHT * 0.5f);
    if (rx <= 0 || ry <= 0) return;

    long long rx2 = (long long)rx * rx;
    long long ry2 = (long long)ry * ry;
    long long x = 0, y = ry;

    auto hline = [&](long long lx, long long ly) {
        glVertex2f(pixelToNdcX((int)(cx - lx)), pixelToNdcY((int)(cy + ly)));
        glVertex2f(pixelToNdcX((int)(cx + lx)), pixelToNdcY((int)(cy + ly)));
        glVertex2f(pixelToNdcX((int)(cx - lx)), pixelToNdcY((int)(cy - ly)));
        glVertex2f(pixelToNdcX((int)(cx + lx)), pixelToNdcY((int)(cy - ly)));
    };

    glBegin(GL_LINES);

    // Region 1
    long long d1 = ry2 - rx2 * ry + rx2 / 4;
    while (2 * ry2 * x < 2 * rx2 * y) {
        hline(x, y);
        x++;
        if (d1 < 0) {
            d1 += 2 * ry2 * x + ry2;
        } else {
            y--;
            d1 += 2 * ry2 * x - 2 * rx2 * y + ry2;
        }
    }

    // Region 2
    long long d2 = ry2 * (x + 1) * (x + 1) - 1 +
                   rx2 * (y - 1) * (y - 1) - rx2 * ry2;
    // (alternatively: re-derive from current x,y)
    d2 = (long long)(ry2 * (x + 0.5f) * (x + 0.5f) +
                     rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        hline(x, y);
        y--;
        if (d2 > 0) {
            d2 -= 2 * rx2 * y + rx2;
        } else {
            x++;
            d2 += 2 * ry2 * x - 2 * rx2 * y + rx2;
        }
    }

    glEnd();
}

void midpointEllipseOutline(float cxn, float cyn, float rxn, float ryn) {
    int cx = ndcToPixelX(cxn);
    int cy = ndcToPixelY(cyn);
    int rx = (int)(rxn * WINDOW_WIDTH  * 0.5f);
    int ry = (int)(ryn * WINDOW_HEIGHT * 0.5f);
    if (rx <= 0 || ry <= 0) return;

    long long rx2 = (long long)rx * rx;
    long long ry2 = (long long)ry * ry;
    long long x = 0, y = ry;

    auto plot4 = [&](long long px, long long py) {
        glVertex2f(pixelToNdcX((int)(cx + px)), pixelToNdcY((int)(cy + py)));
        glVertex2f(pixelToNdcX((int)(cx - px)), pixelToNdcY((int)(cy + py)));
        glVertex2f(pixelToNdcX((int)(cx + px)), pixelToNdcY((int)(cy - py)));
        glVertex2f(pixelToNdcX((int)(cx - px)), pixelToNdcY((int)(cy - py)));
    };

    glBegin(GL_POINTS);

    // Region 1
    long long d1 = ry2 - rx2 * ry + rx2 / 4;
    plot4(x, y);
    while (2 * ry2 * x < 2 * rx2 * y) {
        x++;
        if (d1 < 0) { d1 += 2 * ry2 * x + ry2; }
        else         { y--; d1 += 2 * ry2 * x - 2 * rx2 * y + ry2; }
        plot4(x, y);
    }
    // Region 2
    long long d2 = (long long)(ry2 * (x + 0.5f) * (x + 0.5f) +
                                rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        plot4(x, y);
        y--;
        if (d2 > 0) { d2 -= 2 * rx2 * y + rx2; }
        else         { x++; d2 += 2 * ry2 * x - 2 * rx2 * y + rx2; }
    }

    glEnd();
}

// ═════════════════════════════════════════════════════════════════════════════
//  LEGACY POLYGON / QUAD HELPERS  (kept for filled shapes — no algorithmic alt)
// ═════════════════════════════════════════════════════════════════════════════
void drawFilledRect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
        glVertex2f(x1, y1); glVertex2f(x2, y1);
        glVertex2f(x2, y2); glVertex2f(x1, y2);
    glEnd();
}

// GL ellipse polygon — used where smooth shading/transparency matters
void drawEllipsePoly(float cx, float cy, float rx, float ry, int segs = 20) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segs; i++) {
        float theta = 2.0f * 3.14159265f * i / segs;
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

// ─── Text helpers ─────────────────────────────────────────────────────────────
void drawText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++)
        glutBitmapCharacter(font, *c);
}
void drawTextLarge(const char* text, float x, float y) {
    drawText(text, x, y, GLUT_BITMAP_TIMES_ROMAN_24);
}

// ═════════════════════════════════════════════════════════════════════════════
//  EGG SHAPE  — drawn with the midpoint ellipse algorithm
// ═════════════════════════════════════════════════════════════════════════════
void drawEggShape(float x, float y, float size, float r, float g, float b) {
    float rx = size * 0.68f;
    float ry = size;

    // Body fill (midpoint ellipse)
    glColor3f(r, g, b);
    midpointEllipseFilled(x, y, rx, ry);

    // Shine highlight (small filled ellipse)
    glColor4f(1.0f, 1.0f, 1.0f, 0.45f);
    midpointEllipseFilled(x + rx * 0.20f, y + ry * 0.28f, rx * 0.26f, ry * 0.20f);

    // Outline (midpoint ellipse outline)
    glColor3f(r * 0.55f, g * 0.55f, b * 0.55f);
    midpointEllipseOutline(x, y, rx, ry);
}

// ═════════════════════════════════════════════════════════════════════════════
//  POOP
// ═════════════════════════════════════════════════════════════════════════════
void drawPoop(float x, float y, float size) {
    glColor3f(0.40f, 0.24f, 0.05f);
    midpointCircleFilled(x,             y,            size * 0.60f);
    midpointCircleFilled(x - size*0.20f,y + size*0.50f,size * 0.46f);
    midpointCircleFilled(x + size*0.10f,y + size*0.90f,size * 0.34f);
    midpointCircleFilled(x,             y + size*1.25f, size * 0.22f);

    // Stink wavy lines (Bresenham segments)
    glColor4f(0.55f, 0.40f, 0.10f, 0.55f);
    float offsets[3] = { -size*0.30f, 0.0f, size*0.30f };
    for (int i = 0; i < 3; i++) {
        float ox = offsets[i];
        bresenhamLine(x+ox,           y+size*1.55f, x+ox+size*0.10f, y+size*1.80f);
        bresenhamLine(x+ox+size*0.10f,y+size*1.80f, x+ox-size*0.10f, y+size*2.05f);
        bresenhamLine(x+ox-size*0.10f,y+size*2.05f, x+ox,            y+size*2.25f);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  CHICKEN  — drawn with 2D transformations (glTranslatef / glRotatef / glScalef)
//  Each body part is modelled in its own local coordinate space and
//  transformed into world space.  The wing uses a rotation driven by
//  chickenWingAngle to produce a natural flapping animation.
// ═════════════════════════════════════════════════════════════════════════════
void drawChicken() {
    float cx = chickenX;
    float cy = HORIZON_Y + 0.02f;   // feet rest on bamboo

    // ── 2D Transformation matrix context ─────────────────────────────────────
    // All parts push their own matrix so transforms don't bleed.

    // ── LEGS ──────────────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx, cy, 0.0f);
    glColor3f(0.85f, 0.68f, 0.15f);
    // Left leg
    bresenhamLine(-0.015f, 0.014f, -0.020f, 0.0f);
    // Right leg
    bresenhamLine( 0.015f, 0.014f,  0.020f, 0.0f);
    // Toe lines
    bresenhamLine(-0.020f, 0.0f, -0.035f,  0.003f);
    bresenhamLine(-0.020f, 0.0f, -0.020f, -0.008f);
    bresenhamLine( 0.020f, 0.0f,  0.035f,  0.003f);
    bresenhamLine( 0.020f, 0.0f,  0.020f, -0.008f);
    glPopMatrix();

    // ── BODY ──────────────────────────────────────────────────────────────────
    // Scale is applied to draw a plump oval body
    glPushMatrix();
    glTranslatef(cx, cy + 0.077f, 0.0f);
    glColor3f(0.88f, 0.58f, 0.20f);
    // filled body ellipse  (local rx=0.075, ry=0.090)
    midpointEllipseFilled(0.0f, 0.0f, 0.075f, 0.090f);
    // darker belly shade
    glColor3f(0.72f, 0.45f, 0.12f);
    midpointEllipseFilled(-0.010f, -0.020f, 0.045f, 0.050f);
    glPopMatrix();

    // ── WING (animated rotation around shoulder pivot) ─────────────────────
    glPushMatrix();
    // Shoulder pivot is on the right side of the body, mid-height
    glTranslatef(cx + 0.030f, cy + 0.077f, 0.0f);
    glRotatef(chickenWingAngle, 0.0f, 0.0f, 1.0f);   // ← 2D rotation
    glColor3f(0.68f, 0.38f, 0.08f);
    // Wing is a scaled ellipse in local space
    glScalef(1.0f, 0.7f, 1.0f);                       // ← 2D scale
    midpointEllipseFilled(0.018f, 0.0f, 0.042f, 0.062f);
    // Wing tip feather highlight
    glColor3f(0.82f, 0.52f, 0.18f);
    midpointEllipseFilled(0.018f, -0.025f, 0.028f, 0.028f);
    glPopMatrix();

    // ── TAIL FEATHERS ─────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx - 0.060f, cy + 0.077f, 0.0f);
    glRotatef(25.0f, 0.0f, 0.0f, 1.0f);               // ← 2D rotation
    glColor3f(0.55f, 0.30f, 0.05f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 0.0f,   0.0f);
        glVertex2f(-0.040f, 0.010f);
        glVertex2f(-0.025f, 0.065f);
    glEnd();
    glColor3f(0.72f, 0.42f, 0.10f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 0.0f,   0.0f);
        glVertex2f(-0.055f, 0.000f);
        glVertex2f(-0.038f, 0.050f);
    glEnd();
    glColor3f(0.85f, 0.55f, 0.15f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 0.0f,  0.0f);
        glVertex2f(-0.030f,0.005f);
        glVertex2f(-0.015f,0.075f);
    glEnd();
    glPopMatrix();

    // ── NECK ──────────────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx + 0.010f, cy + 0.155f, 0.0f);
    glColor3f(0.88f, 0.60f, 0.22f);
    midpointEllipseFilled(0.0f, 0.0f, 0.028f, 0.040f);
    glPopMatrix();

    // ── HEAD ──────────────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx + 0.015f, cy + 0.214f, 0.0f);
    glColor3f(0.90f, 0.62f, 0.22f);
    midpointCircleFilled(0.0f, 0.0f, 0.040f);
    // Cheek blush
    glColor4f(0.95f, 0.40f, 0.40f, 0.35f);
    midpointCircleFilled(0.018f, -0.008f, 0.016f);
    glPopMatrix();

    // ── COMB (red ridge on top of head) ───────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx + 0.012f, cy + 0.250f, 0.0f);
    glColor3f(0.92f, 0.12f, 0.12f);
    // Three bumps along the comb, each scaled differently
    midpointCircleFilled(-0.010f, 0.0f, 0.013f);
    midpointCircleFilled( 0.003f, 0.006f, 0.011f);
    midpointCircleFilled( 0.016f, 0.001f, 0.010f);
    glPopMatrix();

    // ── WATTLE ────────────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx + 0.028f, cy + 0.182f, 0.0f);
    glColor3f(0.88f, 0.10f, 0.10f);
    midpointEllipseFilled(0.0f, 0.0f, 0.012f, 0.018f);
    glPopMatrix();

    // ── BEAK ──────────────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx + 0.015f, cy + 0.214f, 0.0f);
    glColor3f(0.95f, 0.78f, 0.10f);
    glBegin(GL_TRIANGLES);
        glVertex2f( 0.036f,  0.006f);
        glVertex2f( 0.036f, -0.006f);
        glVertex2f( 0.060f,  0.000f);
    glEnd();
    // Beak outline using Bresenham
    glColor3f(0.75f, 0.58f, 0.05f);
    bresenhamLine( 0.036f,  0.006f, 0.060f, 0.0f);
    bresenhamLine( 0.036f, -0.006f, 0.060f, 0.0f);
    glPopMatrix();

    // ── EYE ───────────────────────────────────────────────────────────────────
    glPushMatrix();
    glTranslatef(cx + 0.015f, cy + 0.214f, 0.0f);
    glColor3f(0.05f, 0.05f, 0.05f);
    midpointCircleFilled(0.022f, 0.010f, 0.009f);
    glColor3f(1.0f, 1.0f, 1.0f);
    midpointCircleFilled(0.026f, 0.014f, 0.004f);
    glPopMatrix();
}

// ═════════════════════════════════════════════════════════════════════════════
//  FALLING ITEMS
// ═════════════════════════════════════════════════════════════════════════════
void drawFallingItems() {
    for (const auto& item : activeItems) {
        if (!item.active) continue;
        switch (item.type) {
        case NORMAL_EGG:
            drawEggShape(item.x, item.y, item.size, 0.97f, 0.95f, 0.87f);
            break;
        case GOLDEN_EGG:
            drawEggShape(item.x, item.y, item.size, 1.0f, 0.82f, 0.0f);
            glColor3f(1.0f, 1.0f, 0.5f);
            for (int i = 0; i < 4; i++) {
                float angle = i * 3.14159f / 2.0f + menuFrame * 0.05f;
                midpointCircleFilled(
                    item.x + cosf(angle) * (item.size + 0.013f),
                    item.y + sinf(angle) * (item.size + 0.013f),
                    0.006f);
            }
            break;
        case BLUE_EGG:
            drawEggShape(item.x, item.y, item.size, 0.25f, 0.55f, 0.95f);
            break;
        case POOP:
            drawPoop(item.x, item.y, item.size * 0.80f);
            break;
        case POWERUP_BIG_NET: {
            glColor3f(0.90f, 0.18f, 0.18f);
            glBegin(GL_POLYGON);
                glVertex2f(item.x,             item.y + item.size*1.2f);
                glVertex2f(item.x + item.size, item.y);
                glVertex2f(item.x,             item.y - item.size*1.2f);
                glVertex2f(item.x - item.size, item.y);
            glEnd();
            glColor3f(1.0f, 0.55f, 0.55f);
            bresenhamLine(item.x,             item.y + item.size*1.2f, item.x + item.size, item.y);
            bresenhamLine(item.x + item.size, item.y,                  item.x,             item.y - item.size*1.2f);
            bresenhamLine(item.x,             item.y - item.size*1.2f, item.x - item.size, item.y);
            bresenhamLine(item.x - item.size, item.y,                  item.x,             item.y + item.size*1.2f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText("W", item.x - 0.018f, item.y - 0.018f, GLUT_BITMAP_HELVETICA_12);
            break;
        }
        case POWERUP_SLOW: {
            glColor3f(0.10f, 0.82f, 0.82f);
            glBegin(GL_POLYGON);
                glVertex2f(item.x,             item.y + item.size*1.2f);
                glVertex2f(item.x + item.size, item.y);
                glVertex2f(item.x,             item.y - item.size*1.2f);
                glVertex2f(item.x - item.size, item.y);
            glEnd();
            glColor3f(0.65f, 1.0f, 1.0f);
            bresenhamLine(item.x,             item.y + item.size*1.2f, item.x + item.size, item.y);
            bresenhamLine(item.x + item.size, item.y,                  item.x,             item.y - item.size*1.2f);
            bresenhamLine(item.x,             item.y - item.size*1.2f, item.x - item.size, item.y);
            bresenhamLine(item.x - item.size, item.y,                  item.x,             item.y + item.size*1.2f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText("S", item.x - 0.012f, item.y - 0.018f, GLUT_BITMAP_HELVETICA_12);
            break;
        }
        case POWERUP_TIME: {
            glColor3f(0.15f, 0.80f, 0.25f);
            glBegin(GL_POLYGON);
                glVertex2f(item.x,             item.y + item.size*1.2f);
                glVertex2f(item.x + item.size, item.y);
                glVertex2f(item.x,             item.y - item.size*1.2f);
                glVertex2f(item.x - item.size, item.y);
            glEnd();
            glColor3f(0.60f, 1.0f, 0.60f);
            bresenhamLine(item.x,             item.y + item.size*1.2f, item.x + item.size, item.y);
            bresenhamLine(item.x + item.size, item.y,                  item.x,             item.y - item.size*1.2f);
            bresenhamLine(item.x,             item.y - item.size*1.2f, item.x - item.size, item.y);
            bresenhamLine(item.x - item.size, item.y,                  item.x,             item.y + item.size*1.2f);
            glColor3f(1.0f, 1.0f, 1.0f);
            drawText("+T", item.x - 0.024f, item.y - 0.018f, GLUT_BITMAP_HELVETICA_12);
            break;
        }
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  ENVIRONMENT
// ═════════════════════════════════════════════════════════════════════════════
void drawEnvironment() {
    // Sky gradient
    glBegin(GL_QUADS);
        glColor3f(0.40f, 0.72f, 0.95f); glVertex2f(-1.0f, 1.0f);  glVertex2f(1.0f, 1.0f);
        glColor3f(0.75f, 0.90f, 1.00f); glVertex2f(1.0f, HORIZON_Y); glVertex2f(-1.0f, HORIZON_Y);
    glEnd();

    // Distant hills
    glColor3f(0.45f, 0.65f, 0.35f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-1.0f, HORIZON_Y); glVertex2f(-0.55f, 0.40f); glVertex2f(-0.10f, HORIZON_Y);
        glVertex2f(-0.35f,HORIZON_Y); glVertex2f( 0.10f, 0.50f); glVertex2f( 0.55f, HORIZON_Y);
        glVertex2f( 0.30f,HORIZON_Y); glVertex2f( 0.72f, 0.38f); glVertex2f( 1.10f, HORIZON_Y);
    glEnd();

    // Clouds using midpoint circle algorithm
    glColor4f(1.0f, 1.0f, 1.0f, 0.90f);
    for (auto& c : skyClouds) {
        midpointCircleFilled(c.x,                   c.y,                  c.size       );
        midpointCircleFilled(c.x + c.size*0.55f,    c.y + c.size*0.22f,   c.size*0.75f );
        midpointCircleFilled(c.x - c.size*0.55f,    c.y + c.size*0.12f,   c.size*0.70f );
        midpointCircleFilled(c.x + c.size*0.20f,    c.y + c.size*0.35f,   c.size*0.55f );
    }

    // Ground gradient
    glBegin(GL_QUADS);
        glColor3f(0.30f, 0.60f, 0.20f); glVertex2f(-1.0f, HORIZON_Y); glVertex2f(1.0f, HORIZON_Y);
        glColor3f(0.15f, 0.38f, 0.08f); glVertex2f(1.0f, -1.0f);      glVertex2f(-1.0f, -1.0f);
    glEnd();

    // Bamboo perch — Bresenham line
    glColor3f(0.60f, 0.45f, 0.15f);
    bresenhamLine(-0.90f, HORIZON_Y + 0.02f, 0.90f, HORIZON_Y + 0.02f);

    // ── Road ──────────────────────────────────────────────────────────────────
    const float roadTopL = -0.30f, roadTopR = 0.30f;
    const float roadBotL = -1.00f, roadBotR = 1.00f;

    glColor3f(0.42f, 0.30f, 0.14f);
    glBegin(GL_QUADS);
        glVertex2f(roadTopL, HORIZON_Y); glVertex2f(roadTopR, HORIZON_Y);
        glVertex2f(roadBotR, -1.0f);     glVertex2f(roadBotL, -1.0f);
    glEnd();

    glColor3f(0.50f, 0.36f, 0.18f);
    glBegin(GL_QUADS);
        glVertex2f(-0.06f, HORIZON_Y); glVertex2f( 0.06f, HORIZON_Y);
        glVertex2f( 0.20f, -1.0f);    glVertex2f(-0.20f, -1.0f);
    glEnd();

    // Road edge lines — Bresenham
    glColor3f(0.28f, 0.18f, 0.06f);
    bresenhamLine(roadTopL, HORIZON_Y, roadBotL, -1.0f);
    bresenhamLine(roadTopR, HORIZON_Y, roadBotR, -1.0f);
}

// ═════════════════════════════════════════════════════════════════════════════
//  JUNGLE TREES
// ═════════════════════════════════════════════════════════════════════════════
void drawJungleTrees() {
    const float roadTopEdge = 0.30f;
    const float roadBotEdge = 1.00f;
    const float margin      = 0.10f;

    for (const auto& t : jungleTrees) {
        float tFactor = (HORIZON_Y - t.y) / (HORIZON_Y - (-1.0f));
        tFactor = std::max(0.0f, std::min(1.0f, tFactor));
        float scale = 0.025f + tFactor * 0.20f;
        float roadEdgeX = roadTopEdge + tFactor * (roadBotEdge - roadTopEdge);
        float cx = t.isLeft ? -(roadEdgeX + margin) : (roadEdgeX + margin);

        // Trunk using bresenham rectangle fill
        glColor3f(0.42f, 0.24f, 0.06f);
        drawFilledRect(cx - scale*0.12f, t.y, cx + scale*0.12f, t.y + scale*1.3f);
        // Trunk outline
        glColor3f(0.28f, 0.14f, 0.03f);
        bresenhamRect(cx - scale*0.12f, t.y, cx + scale*0.12f, t.y + scale*1.3f);

        // Foliage layers (filled triangles)
        glColor3f(0.08f, 0.42f, 0.12f);
        glBegin(GL_TRIANGLES);
            glVertex2f(cx - scale,       t.y + scale*0.7f);
            glVertex2f(cx + scale,       t.y + scale*0.7f);
            glVertex2f(cx,               t.y + scale*2.1f);
        glEnd();
        // Outline
        glColor3f(0.04f, 0.28f, 0.06f);
        bresenhamLine(cx - scale, t.y + scale*0.7f, cx + scale, t.y + scale*0.7f);
        bresenhamLine(cx + scale, t.y + scale*0.7f, cx,         t.y + scale*2.1f);
        bresenhamLine(cx,         t.y + scale*2.1f, cx - scale, t.y + scale*0.7f);

        glColor3f(0.12f, 0.55f, 0.16f);
        glBegin(GL_TRIANGLES);
            glVertex2f(cx - scale*0.78f, t.y + scale*1.2f);
            glVertex2f(cx + scale*0.78f, t.y + scale*1.2f);
            glVertex2f(cx,               t.y + scale*2.6f);
        glEnd();

        glColor3f(0.18f, 0.68f, 0.22f);
        glBegin(GL_TRIANGLES);
            glVertex2f(cx - scale*0.50f, t.y + scale*1.7f);
            glVertex2f(cx + scale*0.50f, t.y + scale*1.7f);
            glVertex2f(cx,               t.y + scale*3.0f);
        glEnd();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  PLAYER BASKET
// ═════════════════════════════════════════════════════════════════════════════
void drawPlayerWithNet() {
    // Body / handle — 2D translate to player position
    glPushMatrix();
    glTranslatef(playerX, PLAYER_Y, 0.0f);

    if (slowTimer > 0) glColor3f(0.20f, 0.80f, 0.80f);
    else               glColor3f(0.25f, 0.52f, 0.85f);
    drawFilledRect(-0.045f, 0.0f, 0.045f, 0.14f);

    float halfNet = netWidth / 2.0f;
    float bx   = -halfNet;
    float bTop = 0.11f;
    float bBot = -0.05f;

    // Basket body
    glColor3f(0.55f, 0.30f, 0.08f);
    drawFilledRect(bx, bBot, bx + netWidth, bTop);

    // Basket weave lines — Bresenham
    glColor3f(0.40f, 0.20f, 0.04f);
    for (int i = 1; i < 4; i++) {
        float ly = bBot + (bTop - bBot) * i / 4.0f;
        bresenhamLine(bx, ly, bx + netWidth, ly);
    }
    for (int i = 1; i < 8; i++) {
        float lx = bx + netWidth * i / 8.0f;
        bresenhamLine(lx, bBot, lx, bTop);
    }
    // Basket outline
    glColor3f(0.30f, 0.14f, 0.03f);
    bresenhamRect(bx, bBot, bx + netWidth, bTop);

    // Rim — Bresenham
    if (bigNetTimer > 0) glColor3f(0.95f, 0.22f, 0.22f);
    else                  glColor3f(0.72f, 0.42f, 0.10f);
    bresenhamLine(bx - 0.01f, bTop, bx + netWidth + 0.01f, bTop);
    bresenhamLine(bx - 0.01f, bTop + 0.003f, bx + netWidth + 0.01f, bTop + 0.003f);
    bresenhamLine(bx - 0.01f, bTop - 0.003f, bx + netWidth + 0.01f, bTop - 0.003f);

    // Arc handle — Bresenham segments approximating a semicircle
    glColor3f(0.58f, 0.30f, 0.08f);
    const int arcSegs = 16;
    for (int i = 0; i < arcSegs; i++) {
        float t0 = float(i)     / arcSegs;
        float t1 = float(i + 1) / arcSegs;
        float ax0 = bx + netWidth * t0;
        float ay0 = bTop + 0.060f * sinf(3.14159f * t0);
        float ax1 = bx + netWidth * t1;
        float ay1 = bTop + 0.060f * sinf(3.14159f * t1);
        bresenhamLine(ax0, ay0, ax1, ay1);
    }

    // Power-up glow tints
    if (bigNetTimer > 0) {
        glColor4f(0.90f, 0.18f, 0.18f, 0.16f);
        drawFilledRect(bx - 0.01f, bBot, bx + netWidth + 0.01f, bTop + 0.01f);
    }
    if (slowTimer > 0) {
        glColor4f(0.10f, 0.80f, 0.80f, 0.12f);
        drawFilledRect(bx - 0.01f, bBot, bx + netWidth + 0.01f, bTop + 0.01f);
    }

    glPopMatrix();
}

// ═════════════════════════════════════════════════════════════════════════════
//  HUD
// ═════════════════════════════════════════════════════════════════════════════
void drawHUD() {
    char buf[80];

    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "Score: %d", score);
    drawText(buf, -0.97f, 0.90f);

    glColor3f(1.0f, 0.82f, 0.20f);
    sprintf(buf, "Best: %d", highScore);
    drawText(buf, -0.97f, 0.82f, GLUT_BITMAP_HELVETICA_12);

    glColor3f(1.0f, 0.30f, 0.30f);
    sprintf(buf, "Lives: %d", lives);
    drawText(buf, 0.65f, 0.90f);

    int secs = gameTimeLeft / 60;
    int mins = secs / 60;
    secs %= 60;
    glColor3f(0.30f, 0.95f, 0.50f);
    sprintf(buf, "Time: %d:%02d", mins, secs);
    drawText(buf, 0.60f, 0.82f, GLUT_BITMAP_HELVETICA_12);

    char powerupStr[120] = "";
    if      (bigNetTimer > 0 && slowTimer > 0) sprintf(powerupStr, "[WIDE BASKET + SLOW]");
    else if (bigNetTimer > 0)                   sprintf(powerupStr, "[WIDE BASKET ACTIVE]");
    else if (slowTimer   > 0)                   sprintf(powerupStr, "[SLOW EGGS ACTIVE]");
    if (strlen(powerupStr) > 0) {
        glColor3f(0.95f, 0.85f, 0.20f);
        drawText(powerupStr, -0.20f, 0.90f);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  OVERLAY HELPERS
// ═════════════════════════════════════════════════════════════════════════════
void drawDimOverlay(float alpha) {
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    drawFilledRect(-1.0f, -1.0f, 1.0f, 1.0f);
}

void drawPanel(float cx, float cy, float hw, float hh,
               float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    drawFilledRect(cx - hw, cy - hh, cx + hw, cy + hh);
    glColor4f(1.0f, 1.0f, 1.0f, 0.22f);
    bresenhamRect(cx - hw, cy - hh, cx + hw, cy + hh);
}

void drawSmallEgg(float x, float y, float sz, float r, float g, float b) {
    glColor3f(r, g, b);
    midpointEllipseFilled(x, y, sz * 0.70f, sz);
    glColor4f(1.0f, 1.0f, 1.0f, 0.40f);
    midpointEllipseFilled(x + sz*0.18f, y + sz*0.25f, sz*0.24f, sz*0.18f);
    glColor3f(r*0.60f, g*0.60f, b*0.60f);
    midpointEllipseOutline(x, y, sz * 0.70f, sz);
}

void drawSmallDiamond(float x, float y, float sz, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POLYGON);
        glVertex2f(x,      y + sz); glVertex2f(x + sz, y);
        glVertex2f(x,      y - sz); glVertex2f(x - sz, y);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    bresenhamLine(x,      y + sz, x + sz, y);
    bresenhamLine(x + sz, y,      x,      y - sz);
    bresenhamLine(x,      y - sz, x - sz, y);
    bresenhamLine(x - sz, y,      x,      y + sz);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SCREENS
// ═════════════════════════════════════════════════════════════════════════════
void drawMenuScreen() {
    drawEnvironment();
    drawJungleTrees();
    drawChicken();

    drawPanel(0.0f, 0.05f, 0.74f, 0.88f, 0.04f, 0.10f, 0.04f, 0.84f);

    float pulse = 0.75f + 0.25f * sinf(menuFrame * 0.05f);
    glColor3f(0.30f * pulse, 1.0f * pulse, 0.30f * pulse);
    drawTextLarge("CATCH THE EGGS", -0.36f, 0.72f);

    glColor3f(0.78f, 0.90f, 0.65f);
    drawText("A chicken egg-catching adventure!", -0.34f, 0.61f, GLUT_BITMAP_HELVETICA_12);

    glColor4f(0.5f, 0.8f, 0.5f, 0.4f);
    bresenhamLine(-0.67f, 0.55f, 0.67f, 0.55f);

    float eggBob = sinf(menuFrame * 0.06f) * 0.022f;
    float iconY  = 0.39f + eggBob;
    float labelY = 0.29f;
    float spacing = 0.30f;
    float startX  = -0.55f;

    drawSmallEgg(startX,             iconY, 0.050f, 0.97f, 0.95f, 0.87f);
    glColor3f(0.88f, 0.88f, 0.80f);
    drawText("+1", startX - 0.030f, labelY, GLUT_BITMAP_HELVETICA_12);

    drawSmallEgg(startX + spacing,   iconY + eggBob*0.3f, 0.050f, 0.25f, 0.55f, 0.95f);
    glColor3f(0.55f, 0.75f, 1.00f);
    drawText("+5", startX + spacing - 0.030f, labelY, GLUT_BITMAP_HELVETICA_12);

    drawSmallEgg(startX + spacing*2, iconY + eggBob*0.8f, 0.050f, 1.0f, 0.82f, 0.0f);
    glColor3f(1.0f, 0.90f, 0.40f);
    drawText("+10", startX + spacing*2 - 0.040f, labelY, GLUT_BITMAP_HELVETICA_12);

    glColor3f(0.40f, 0.24f, 0.05f);
    midpointCircleFilled(startX + spacing*3, iconY, 0.038f);
    midpointCircleFilled(startX + spacing*3 - 0.010f, iconY + 0.050f, 0.028f);
    glColor3f(0.65f, 0.45f, 0.20f);
    drawText("-10", startX + spacing*3 - 0.042f, labelY, GLUT_BITMAP_HELVETICA_12);

    glColor4f(0.5f, 0.8f, 0.5f, 0.4f);
    bresenhamLine(-0.67f, 0.24f, 0.67f, 0.24f);

    glColor3f(0.25f, 1.0f, 0.45f);
    drawTextLarge("[ ENTER ]  Start Game", -0.32f, 0.14f);
    glColor3f(0.55f, 0.85f, 1.00f);
    drawTextLarge("[ H ]      How To Play", -0.32f, -0.02f);
    glColor3f(1.00f, 0.45f, 0.45f);
    drawTextLarge("[ ESC ]    Quit Game",   -0.32f, -0.18f);

    glColor4f(0.5f, 0.8f, 0.5f, 0.4f);
    bresenhamLine(-0.67f, -0.27f, 0.67f, -0.27f);

    if (highScore > 0) {
        char buf[48];
        sprintf(buf, "High Score: %d", highScore);
        glColor3f(1.0f, 0.82f, 0.20f);
        drawText(buf, -0.20f, -0.38f);
    } else {
        glColor3f(0.55f, 0.72f, 0.55f);
        drawText("No high score yet - be the first!", -0.34f, -0.38f, GLUT_BITMAP_HELVETICA_12);
    }

    glColor3f(0.42f, 0.58f, 0.42f);
    drawText("Catch eggs dropped by the chicken.  Avoid the poop!",
             -0.50f, -0.56f, GLUT_BITMAP_HELVETICA_12);
    drawText("Catch power-up blocks for special abilities.",
             -0.38f, -0.65f, GLUT_BITMAP_HELVETICA_12);
}

void drawHelpScreen() {
    drawEnvironment();
    drawDimOverlay(0.78f);
    drawPanel(0.0f, 0.0f, 0.84f, 0.93f, 0.04f, 0.10f, 0.18f, 0.92f);

    glColor3f(0.40f, 0.92f, 1.0f);
    drawTextLarge("HOW TO PLAY", -0.22f, 0.80f);
    glColor4f(0.4f, 0.8f, 1.0f, 0.4f);
    bresenhamLine(-0.77f, 0.73f, 0.77f, 0.73f);

    glColor3f(1.0f, 0.86f, 0.30f);
    drawText("CONTROLS", -0.74f, 0.64f);

    const char* controls[][2] = {
        { "LEFT / RIGHT Arrows",  "Move the basket left/right"    },
        { "Mouse movement",       "Move the basket (alternative)" },
        { "ESC  (while playing)", "Pause the game"                },
        { "ESC or P (paused)",    "Resume"                        },
        { "H   (paused / menu)",  "Open this Help screen"         },
        { "M   (paused)",         "Return to main menu"           },
        { "Q   (paused)",         "Quit the game"                 },
    };
    int nCtrl = (int)(sizeof(controls) / sizeof(controls[0]));
    for (int i = 0; i < nCtrl; i++) {
        float y = 0.54f - i * 0.09f;
        glColor3f(0.70f, 0.96f, 0.70f);
        drawText(controls[i][0], -0.74f, y, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.55f, 0.76f, 0.55f);
        drawText(controls[i][1],  0.08f, y, GLUT_BITMAP_HELVETICA_12);
    }

    glColor4f(0.4f, 0.8f, 1.0f, 0.3f);
    bresenhamLine(-0.77f, -0.12f, 0.77f, -0.12f);

    glColor3f(1.0f, 0.86f, 0.30f);
    drawText("EGG & ITEM GUIDE", -0.74f, -0.20f);

    struct EggInfo {
        float r, g, b;
        bool  isDiamond;
        float dr, dg, db;
        const char* name;
        const char* effect;
    };
    EggInfo items[] = {
        { 0.97f,0.95f,0.87f, false,0,0,0,         "White Egg",     "+1 point"                         },
        { 0.25f,0.55f,0.95f, false,0,0,0,         "Blue Egg",      "+5 points"                        },
        { 1.00f,0.82f,0.00f, false,0,0,0,         "Golden Egg",    "+10 points - rare and valuable!"  },
        { 0.40f,0.24f,0.05f, false,0,0,0,         "Poop",          "-10 points - AVOID!"              },
        { 0,0,0,              true, 0.90f,0.18f,0.18f,"Red Block  [W]","Wide basket for ~4 seconds"   },
        { 0,0,0,              true, 0.10f,0.82f,0.82f,"Cyan Block [S]","Slow falling eggs ~4 sec"     },
        { 0,0,0,              true, 0.15f,0.80f,0.25f,"Green Block[+T]","Extra 15 seconds of time"    },
    };
    int nItems = (int)(sizeof(items) / sizeof(items[0]));
    for (int i = 0; i < nItems; i++) {
        float y = -0.31f - i * 0.085f;
        if (i == 3) {
            glColor3f(0.40f, 0.24f, 0.05f);
            midpointCircleFilled(-0.68f, y + 0.020f, 0.028f);
            midpointCircleFilled(-0.692f,y + 0.048f, 0.020f);
        } else if (items[i].isDiamond) {
            drawSmallDiamond(-0.68f, y + 0.020f, 0.028f, items[i].dr, items[i].dg, items[i].db);
        } else {
            drawSmallEgg(-0.68f, y + 0.022f, 0.040f, items[i].r, items[i].g, items[i].b);
        }
        glColor3f(0.92f, 0.92f, 0.92f);
        drawText(items[i].name,   -0.60f, y, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.62f, 0.80f, 0.62f);
        drawText(items[i].effect, -0.08f, y, GLUT_BITMAP_HELVETICA_12);
    }

    glColor4f(0.4f, 0.8f, 1.0f, 0.3f);
    bresenhamLine(-0.77f, -0.93f, 0.77f, -0.93f);
    glColor3f(0.50f, 0.72f, 0.50f);
    drawText("TIP: eggs grow larger as they fall - position your basket early!",
             -0.72f, -0.87f, GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.92f, 0.66f, 0.22f);
    drawText("[ BACKSPACE ]  Go Back", -0.24f, -0.79f);
}

void drawPauseScreen() {
    drawDimOverlay(0.60f);
    drawPanel(0.0f, 0.10f, 0.46f, 0.70f, 0.05f, 0.08f, 0.05f, 0.90f);

    glColor3f(1.0f, 0.86f, 0.20f);
    drawTextLarge("PAUSED", -0.14f, 0.68f);

    glColor4f(1.0f, 0.86f, 0.20f, 0.35f);
    bresenhamLine(-0.43f, 0.61f, 0.43f, 0.61f);

    struct MI { const char* key; const char* label; float r, g, b; };
    MI mitems[] = {
        { "[ ESC ] or [ P ]", "Resume",      0.30f,1.0f, 0.40f },
        { "[ H ]",            "How To Play", 0.40f,0.86f,1.0f  },
        { "[ M ]",            "Main Menu",   0.92f,0.76f,0.30f },
        { "[ Q ]",            "Quit Game",   1.0f, 0.40f,0.40f },
    };
    int n = (int)(sizeof(mitems) / sizeof(mitems[0]));
    for (int i = 0; i < n; i++) {
        float y = 0.47f - i * 0.19f;
        glColor3f(mitems[i].r, mitems[i].g, mitems[i].b);
        drawText(mitems[i].key,  -0.39f, y);
        glColor3f(mitems[i].r * 0.75f, mitems[i].g * 0.75f, mitems[i].b * 0.75f);
        drawText(mitems[i].label, 0.05f, y);
    }

    glColor4f(1.0f, 0.86f, 0.20f, 0.35f);
    bresenhamLine(-0.43f, -0.47f, 0.43f, -0.47f);
    char buf[40];
    sprintf(buf, "Score so far: %d", score);
    glColor3f(0.75f, 0.75f, 0.75f);
    drawText(buf, -0.24f, -0.54f, GLUT_BITMAP_HELVETICA_12);
}

void drawGameOverScreen() {
    drawDimOverlay(0.72f);
    drawPanel(0.0f, 0.10f, 0.56f, 0.72f, 0.14f, 0.02f, 0.02f, 0.90f);

    glColor3f(1.0f, 0.25f, 0.25f);
    drawTextLarge("GAME OVER", -0.22f, 0.69f);

    glColor3f(0.65f, 0.15f, 0.15f);
    if (gameTimeLeft <= 0 && lives > 0)
        drawText("Time's up!", -0.14f, 0.58f, GLUT_BITMAP_HELVETICA_12);
    else
        drawText("You ran out of lives!", -0.24f, 0.58f, GLUT_BITMAP_HELVETICA_12);

    glColor4f(1.0f, 0.30f, 0.30f, 0.30f);
    bresenhamLine(-0.52f, 0.51f, 0.52f, 0.51f);

    char buf[48];
    sprintf(buf, "Your Score:  %d", score);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(buf, -0.30f, 0.40f);

    if (score > highScore && score > 0) {
        glColor3f(1.0f, 0.86f, 0.10f);
        drawText("NEW HIGH SCORE!", -0.24f, 0.29f);
    } else {
        sprintf(buf, "High Score:  %d", highScore);
        glColor3f(0.86f, 0.72f, 0.30f);
        drawText(buf, -0.30f, 0.29f);
    }

    glColor4f(1.0f, 0.30f, 0.30f, 0.30f);
    bresenhamLine(-0.52f, 0.18f, 0.52f, 0.18f);

    struct MI { const char* key; const char* label; float r, g, b; };
    MI mitems[] = {
        { "[ ENTER ]", "Play Again", 0.30f,1.0f, 0.40f },
        { "[ M ]",     "Main Menu",  0.92f,0.76f,0.30f },
        { "[ ESC ]",   "Quit Game",  1.0f, 0.40f,0.40f },
    };
    int n = (int)(sizeof(mitems) / sizeof(mitems[0]));
    for (int i = 0; i < n; i++) {
        float y = 0.05f - i * 0.18f;
        glColor3f(mitems[i].r, mitems[i].g, mitems[i].b);
        drawText(mitems[i].key,  -0.40f, y);
        glColor3f(mitems[i].r*0.75f,mitems[i].g*0.75f,mitems[i].b*0.75f);
        drawText(mitems[i].label, 0.02f, y);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  GLUT DISPLAY CALLBACK
// ═════════════════════════════════════════════════════════════════════════════
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (currentState == MENU) {
        drawMenuScreen();
    } else if (currentState == HELP) {
        if (helpReturnState == MENU) drawMenuScreen();
        else {
            drawEnvironment(); drawJungleTrees(); drawChicken();
            drawFallingItems(); drawPlayerWithNet(); drawHUD();
        }
        drawHelpScreen();
    } else if (currentState == PLAYING) {
        drawEnvironment(); drawJungleTrees(); drawChicken();
        drawFallingItems(); drawPlayerWithNet(); drawHUD();
    } else if (currentState == PAUSED) {
        drawEnvironment(); drawJungleTrees(); drawChicken();
        drawFallingItems(); drawPlayerWithNet(); drawHUD();
        drawPauseScreen();
    } else if (currentState == GAMEOVER) {
        drawEnvironment(); drawJungleTrees(); drawChicken();
        drawFallingItems(); drawPlayerWithNet(); drawHUD();
        drawGameOverScreen();
    }

    glutSwapBuffers();
}

// ═════════════════════════════════════════════════════════════════════════════
//  INIT / RESET
// ═════════════════════════════════════════════════════════════════════════════
void init() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    srand((unsigned)time(NULL));

    skyClouds.push_back({ -0.6f, 0.65f, 0.0010f, 0.15f });
    skyClouds.push_back({  0.2f, 0.50f, 0.0007f, 0.20f });
    skyClouds.push_back({  0.7f, 0.72f, 0.0013f, 0.12f });

    for (int i = 0; i < 8; i++) {
        float startY = HORIZON_Y - (i * 0.22f);
        jungleTrees.push_back({ -0.68f, startY, 0.009f, true  });
        jungleTrees.push_back({  0.68f, startY, 0.009f, false });
    }
}

void resetGame() {
    score          = 0;
    lives          = 3;
    gameTimeLeft   = 120 * 60;
    playerX        = 0.0f;
    playerSpeed    = 0.025f;
    netWidth       = 0.28f;
    bigNetTimer    = 0;
    slowTimer      = 0;
    baseEggSpeed   = 0.0036f;
    chickenX       = 0.0f;
    chickenTargetX = 0.0f;
    chickenTimer   = 0;
    chickenWingAngle = 0.0f;
    activeItems.clear();
    currentState   = PLAYING;
}

// ═════════════════════════════════════════════════════════════════════════════
//  GAME LOGIC
// ═════════════════════════════════════════════════════════════════════════════
void spawnTree(bool leftSide) {
    Tree t;
    t.isLeft = leftSide;
    t.y      = HORIZON_Y;
    t.speed  = 0.009f;
    t.x      = leftSide ? -0.68f : 0.68f;
    jungleTrees.push_back(t);
}

void spawnItem() {
    FallingItem item;
    item.spawnX  = chickenX;
    item.x       = chickenX;
    item.y       = HORIZON_Y + 0.05f;
    item.active  = true;
    item.size    = 0.012f;

    float speedMult = (slowTimer > 0) ? 0.45f : 1.0f;
    item.speed   = (baseEggSpeed + (score * 0.00010f)) * speedMult;

    float spread = ((rand() % 41) - 20) / 100.0f;
    item.targetX = std::max(-0.88f, std::min(0.88f, item.spawnX + spread));

    int roll = rand() % 100;
    if      (roll < 45) item.type = NORMAL_EGG;
    else if (roll < 65) item.type = BLUE_EGG;
    else if (roll < 78) item.type = GOLDEN_EGG;
    else if (roll < 88) item.type = POOP;
    else if (roll < 93) item.type = POWERUP_BIG_NET;
    else if (roll < 97) item.type = POWERUP_SLOW;
    else                item.type = POWERUP_TIME;

    activeItems.push_back(item);
}

void updateGame() {
    menuFrame++;

    // Wing flap animation
    chickenWingAngle += 3.5f * chickenWingDir;
    if (chickenWingAngle >  22.0f) chickenWingDir = -1.0f;
    if (chickenWingAngle < -22.0f) chickenWingDir =  1.0f;

    // Power-up timers
    if (bigNetTimer > 0) { bigNetTimer--; netWidth = 0.46f; } else netWidth      = 0.28f;
    if (slowTimer   > 0) { slowTimer--;   baseEggSpeed = 0.0018f; } else baseEggSpeed = 0.0036f;

    // Keyboard
    if (keys[GLUT_KEY_LEFT  + 256]) playerX -= playerSpeed;
    if (keys[GLUT_KEY_RIGHT + 256]) playerX += playerSpeed;

    // Mouse smooth follow
    {
        float mx   = (float)mouseX_raw / WINDOW_WIDTH * 2.0f - 1.0f;
        float diff = mx - playerX;
        if (fabsf(diff) > 0.005f) playerX += diff * 0.18f;
    }
    playerX = std::max(-0.90f, std::min(0.90f, playerX));

    gameTimeLeft--;
    if (gameTimeLeft <= 0) {
        gameTimeLeft = 0;
        highScore = std::max(highScore, score);
        currentState = GAMEOVER;
        return;
    }

    // Chicken movement
    if (--chickenTimer <= 0) {
        chickenTargetX = ((rand() % 45) - 22) / 100.0f;
        chickenTimer   = 30 + rand() % 50;
    }
    chickenX += (chickenTargetX - chickenX) * 0.055f;
    chickenX  = std::max(-0.22f, std::min(0.22f, chickenX));

    // Clouds
    for (auto& c : skyClouds) { c.x -= c.speed; if (c.x < -1.3f) c.x = 1.3f; }

    // Trees
    for (int i = 0; i < (int)jungleTrees.size(); i++) {
        jungleTrees[i].y     -= jungleTrees[i].speed;
        jungleTrees[i].speed += 0.00012f;
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

        item.y   -= item.speed;
        item.x    = item.spawnX + (seg * (item.targetX - item.spawnX));
        item.size = 0.012f + (seg * 0.058f);

        if (item.y <= PLAYER_Y + 0.06f && item.y >= PLAYER_Y - 0.06f) {
            float halfNet = netWidth / 2.0f;
            if (item.x >= playerX - halfNet && item.x <= playerX + halfNet) {
                item.active = false;
                switch (item.type) {
                    case NORMAL_EGG:      score += 1;  break;
                    case BLUE_EGG:        score += 5;  break;
                    case GOLDEN_EGG:      score += 10; break;
                    case POOP:            score = std::max(0, score - 10); break;
                    case POWERUP_BIG_NET: bigNetTimer = 240; break;
                    case POWERUP_SLOW:    slowTimer   = 240; break;
                    case POWERUP_TIME:    gameTimeLeft = std::min(gameTimeLeft + 15*60, 120*60); break;
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

        if (item.y < -1.1f) {
            if (item.type == NORMAL_EGG || item.type == BLUE_EGG || item.type == GOLDEN_EGG) {
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

    if (rand() % 100 < 3 && (int)activeItems.size() < 5) spawnItem();
}

void updateAmbient() {
    menuFrame++;
    chickenWingAngle += 2.5f * chickenWingDir;
    if (chickenWingAngle >  20.0f) chickenWingDir = -1.0f;
    if (chickenWingAngle < -20.0f) chickenWingDir =  1.0f;

    for (auto& c : skyClouds) { c.x -= c.speed; if (c.x < -1.3f) c.x = 1.3f; }
    if (--chickenTimer <= 0) {
        chickenTargetX = ((rand() % 160) - 80) / 100.0f;
        chickenTimer   = 30 + rand() % 50;
    }
    chickenX += (chickenTargetX - chickenX) * 0.055f;
    for (int i = 0; i < (int)jungleTrees.size(); i++) {
        jungleTrees[i].y -= jungleTrees[i].speed;
        jungleTrees[i].speed += 0.00012f;
        if (jungleTrees[i].y < -1.1f) {
            bool side = jungleTrees[i].isLeft;
            jungleTrees.erase(jungleTrees.begin() + i);
            spawnTree(side);
            i--;
        }
    }
}

void timerCB(int v) {
    if (currentState == PLAYING) updateGame();
    else                          updateAmbient();
    glutPostRedisplay();
    glutTimerFunc(16, timerCB, 0);
}

// ═════════════════════════════════════════════════════════════════════════════
//  INPUT
// ═════════════════════════════════════════════════════════════════════════════
void keyboardDown(unsigned char key, int x, int y) {
    switch (currentState) {
    case MENU:
        if (key == 13)               resetGame();
        else if (key=='h'||key=='H') { helpReturnState=MENU; currentState=HELP; }
        else if (key==27)            exit(0);
        break;
    case PLAYING:
        if (key==27) currentState=PAUSED;
        break;
    case PAUSED:
        if (key==27||key=='p'||key=='P')  currentState=PLAYING;
        else if (key=='h'||key=='H')      { helpReturnState=PAUSED; currentState=HELP; }
        else if (key=='m'||key=='M')      { highScore=std::max(highScore,score); currentState=MENU; }
        else if (key=='q'||key=='Q')      exit(0);
        break;
    case HELP:
        if (key==8) currentState=helpReturnState;
        break;
    case GAMEOVER:
        if (key==13)               resetGame();
        else if (key=='m'||key=='M') currentState=MENU;
        else if (key==27)            exit(0);
        break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {}

void specialDown(int key, int x, int y) {
    if (key < 256) keys[key + 256] = true;
}
void specialUp(int key, int x, int y) {
    if (key < 256) keys[key + 256] = false;
}

void mouseMotion(int x, int y)        { mouseX_raw = x; }
void mousePassiveMotion(int x, int y) { mouseX_raw = x; }

// ═════════════════════════════════════════════════════════════════════════════
//  MAIN
// ═════════════════════════════════════════════════════════════════════════════
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Catch The Eggs - Windows Edition");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mousePassiveMotion);
    glutTimerFunc(0, timerCB, 0);

    glutMainLoop();
    return 0;
}