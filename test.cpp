#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

// --- Window ---
const int WINDOW_WIDTH  = 1000;
const int WINDOW_HEIGHT = 600;

// --- World constants ---
const float HORIZON_Y = 0.1f;
const float PLAYER_Y  = -0.8f;

// --- Game states ---
enum GameState { MENU, PLAYING, PAUSED, HELP, GAMEOVER };
GameState currentState    = MENU;
GameState helpReturnState = MENU;

// --- Item types ---
enum ItemType { NORMAL_EGG, GOLDEN_EGG, ROTTEN_EGG, POWERUP_BIG_NET, POWERUP_SPEED };

// --- Structs ---
struct Tree        { float x, y, speed; bool isLeft; };
struct FallingItem { float x, y, targetX, speed, size; ItemType type; bool active; };
struct Cloud       { float x, y, speed, size; };

// --- Game variables ---
float playerX     = 0.0f;
float playerSpeed = 0.025f;
float netWidth    = 0.25f;
float dinoX       = 0.0f, dinoTargetX = 0.0f;
int   dinoTimer   = 0;
int   score       = 0;
int   highScore   = 0;
int   lives       = 5;       // EASY MODE: Start with 5 lives
int   bigNetTimer = 0, speedTimer = 0;
bool  keys[256]   = { false };
int   menuFrame   = 0;

std::vector<Tree>        jungleTrees;
std::vector<FallingItem> activeItems;
std::vector<Cloud>       skyClouds;

// --- Forward declarations ---
void init();
void resetGame();
void updateGame();
void spawnTree(bool leftSide);
void spawnItem();
void drawEllipse(float cx, float cy, float rx, float ry, int segs = 20);
void drawCircle(float cx, float cy, float r, int segs);
void drawEllipseOutline(float cx, float cy, float rx, float ry, int segs = 20);
void drawFilledRect(float x1, float y1, float x2, float y2);
void drawText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18);
void drawTextLarge(const char* text, float x, float y);
void drawEnvironment();
void drawJungleTrees();
void drawDinosaur();
void drawEggShape(float x, float y, float size, float r, float g, float b);
void drawFallingItems();
void drawPlayerWithNet();
void drawHUD();
void drawMenuScreen();
void drawPauseScreen();
void drawHelpScreen();
void drawGameOverScreen();
void drawDimOverlay(float alpha);
void display();
void timerCB(int v);
void keyboardDown(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void specialDown(int key, int x, int y);
void specialUp(int key, int x, int y);

// --- Initialization ---
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
    lives        = 5; // EASY MODE
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

// --- Logic functions ---
void spawnTree(bool leftSide) {
    Tree t;
    t.isLeft = leftSide;
    t.y      = HORIZON_Y;
    t.speed  = 0.008f;
    t.x      = leftSide ? (-0.3f - (rand() % 20 / 100.0f)) : (0.3f + (rand() % 20 / 100.0f));
    jungleTrees.push_back(t);
}

void spawnItem() {
    FallingItem item;
    item.x       = dinoX;
    item.y       = HORIZON_Y + 0.05f;
    // EASY MODE: Slower speed
    item.speed   = 0.009f + (score * 0.0001f);
    item.active  = true;
    item.size    = 0.01f;
    item.targetX = dinoX * 2.2f;

    int roll = rand() % 100;
    if (roll < 65)       item.type = NORMAL_EGG;
    else if (roll < 80)  item.type = ROTTEN_EGG;
    else if (roll < 90)  item.type = GOLDEN_EGG;
    else if (roll < 96)  item.type = POWERUP_BIG_NET;
    else                 item.type = POWERUP_SPEED;

    activeItems.push_back(item);
}

void updateGame() {
    menuFrame++;
    if (bigNetTimer > 0) { bigNetTimer--; netWidth = 0.45f; } else netWidth = 0.25f;
    if (speedTimer  > 0) { speedTimer--;  playerSpeed = 0.045f; } else playerSpeed = 0.025f;

    if (keys[GLUT_KEY_LEFT])  playerX -= playerSpeed;
    if (keys[GLUT_KEY_RIGHT]) playerX += playerSpeed;
    playerX = std::max(-0.9f, std::min(0.9f, playerX));

    if (--dinoTimer <= 0) {
        dinoTargetX = ((rand() % 120) - 60) / 100.0f;
        dinoTimer   = 60 + rand() % 60;
    }
    dinoX += (dinoTargetX - dinoX) * 0.04f;

    for (auto& c : skyClouds) { c.x -= c.speed; if (c.x < -1.3f) c.x = 1.3f; }

    for (int i = 0; i < (int)jungleTrees.size(); i++) {
        jungleTrees[i].y     -= jungleTrees[i].speed;
        jungleTrees[i].speed += 0.0001f;
        if (jungleTrees[i].y < -1.1f) {
            bool side = jungleTrees[i].isLeft;
            jungleTrees.erase(jungleTrees.begin() + i);
            spawnTree(side); i--;
        }
    }

    for (int i = 0; i < (int)activeItems.size(); i++) {
        auto& item = activeItems[i];
        if (!item.active) continue;

        float seg = std::max(0.0f, std::min(1.0f, (item.y - HORIZON_Y) / (PLAYER_Y - HORIZON_Y)));
        item.y -= item.speed;
        item.x = dinoX + (seg * (item.targetX - dinoX));
        item.size = 0.015f + (seg * 0.065f);

        // EASY MODE: Larger vertical hitbox
        if (item.y <= PLAYER_Y + 0.1f && item.y >= PLAYER_Y - 0.1f) {
            float halfNet = netWidth / 2.0f;
            if (item.x >= playerX - halfNet && item.x <= playerX + halfNet) {
                item.active = false;
                switch (item.type) {
                    case NORMAL_EGG: score += 10; break;
                    case GOLDEN_EGG: score += 35; break;
                    case ROTTEN_EGG: score = std::max(0, score - 25); lives--; break;
                    case POWERUP_BIG_NET: bigNetTimer = 400; break;
                    case POWERUP_SPEED:   speedTimer  = 400; break;
                }
                activeItems.erase(activeItems.begin() + i); i--;
                if (lives <= 0) { highScore = std::max(highScore, score); currentState = GAMEOVER; }
                continue;
            }
        }
        if (item.y < -1.1f) {
            if (item.type == NORMAL_EGG) lives--; // Dropping Gold doesn't lose life
            if (lives <= 0) { highScore = std::max(highScore, score); currentState = GAMEOVER; }
            activeItems.erase(activeItems.begin() + i); i--;
        }
    }
    if (rand() % 100 < 3 && activeItems.size() < 4) spawnItem();
}

// --- Keep Drawing functions from your original code (re-added for completeness) ---

void drawEllipse(float cx, float cy, float rx, float ry, int segs) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segs; i++) {
        float theta = 2.0f * 3.14159265f * i / segs;
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

void drawCircle(float cx, float cy, float r, int segs) { drawEllipse(cx, cy, r, r, segs); }
void drawEllipseOutline(float cx, float cy, float rx, float ry, int segs) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        float theta = 2.0f * 3.14159265f * i / segs;
        glVertex2f(cx + rx * cosf(theta), cy + ry * sinf(theta));
    }
    glEnd();
}

void drawFilledRect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS); glVertex2f(x1, y1); glVertex2f(x2, y1); glVertex2f(x2, y2); glVertex2f(x1, y2); glEnd();
}

void drawText(const char* text, float x, float y, void* font) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) glutBitmapCharacter(font, *c);
}

void drawTextLarge(const char* text, float x, float y) { drawText(text, x, y, GLUT_BITMAP_TIMES_ROMAN_24); }

void drawEnvironment() {
    glBegin(GL_QUADS);
        glColor3f(0.0f, 0.4f, 0.6f); glVertex2f(-1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
        glColor3f(0.8f, 0.5f, 0.3f); glVertex2f(1.0f, HORIZON_Y); glVertex2f(-1.0f, HORIZON_Y);
    glEnd();
    glColor3f(0.2f, 0.25f, 0.3f);
    glBegin(GL_TRIANGLES);
        glVertex2f(-1.0f, HORIZON_Y); glVertex2f(-0.6f, 0.4f); glVertex2f(-0.2f, HORIZON_Y);
        glVertex2f(-0.4f, HORIZON_Y); glVertex2f(0.1f, 0.5f); glVertex2f(0.6f, HORIZON_Y);
        glVertex2f(0.3f, HORIZON_Y); glVertex2f(0.75f, 0.35f); glVertex2f(1.2f, HORIZON_Y);
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    for (auto& c : skyClouds) {
        drawCircle(c.x, c.y, c.size, 12);
        drawCircle(c.x + c.size * 0.5f, c.y + c.size * 0.2f, c.size * 0.8f, 12);
        drawCircle(c.x - c.size * 0.5f, c.y + c.size * 0.1f, c.size * 0.7f, 12);
    }
    glBegin(GL_QUADS);
        glColor3f(0.2f, 0.4f, 0.15f); glVertex2f(-1.0f, HORIZON_Y); glVertex2f(1.0f, HORIZON_Y);
        glColor3f(0.1f, 0.25f, 0.08f); glVertex2f(1.0f, -1.0f); glVertex2f(-1.0f, -1.0f);
    glEnd();
}

void drawJungleTrees() {
    for (const auto& t : jungleTrees) {
        float tFactor = std::max(0.0f, std::min(1.0f, (HORIZON_Y - t.y) / (HORIZON_Y - (-1.0f))));
        float scale = 0.02f + (tFactor * 0.18f);
        float currentX = t.x + (t.isLeft ? -(tFactor * 0.4f) : (tFactor * 0.4f));
        glColor3f(0.35f, 0.2f, 0.05f);
        drawFilledRect(currentX - scale*0.15f, t.y, currentX + scale*0.15f, t.y + scale*1.2f);
        glColor3f(0.05f, 0.35f, 0.1f);
        glBegin(GL_TRIANGLES); glVertex2f(currentX-scale, t.y+scale*0.8f); glVertex2f(currentX+scale, t.y+scale*0.8f); glVertex2f(currentX, t.y+scale*2.2f); glEnd();
    }
}

void drawDinosaur() {
    float dinoY = HORIZON_Y + 0.05f, s = 0.08f;
    glColor3f(0.1f, 0.4f, 0.3f);
    drawFilledRect(dinoX - s, dinoY, dinoX + s, dinoY + s*1.3f);
    glColor3f(0.8f, 0.3f, 0.1f);
    glBegin(GL_TRIANGLES); glVertex2f(dinoX-s*0.2f, dinoY+s*1.3f); glVertex2f(dinoX+s*0.2f, dinoY+s*1.3f); glVertex2f(dinoX, dinoY+s*1.7f); glEnd();
}

void drawEggShape(float x, float y, float size, float r, float g, float b) {
    drawEllipse(x, y, size * 0.72f, size, 18);
}

void drawFallingItems() {
    for (const auto& it : activeItems) {
        if (!it.active) continue;
        switch (it.type) {
            case NORMAL_EGG: drawEggShape(it.x, it.y, it.size, 0.97f, 0.93f, 0.84f); break;
            case GOLDEN_EGG: drawEggShape(it.x, it.y, it.size, 1.0f, 0.82f, 0.0f); break;
            case ROTTEN_EGG: drawEggShape(it.x, it.y, it.size, 0.32f, 0.42f, 0.18f); break;
            case POWERUP_BIG_NET: glColor3f(0.9f, 0.2f, 0.2f); drawCircle(it.x, it.y, it.size, 8); break;
            case POWERUP_SPEED:   glColor3f(0.1f, 0.8f, 0.8f); drawCircle(it.x, it.y, it.size, 8); break;
        }
    }
}

void drawPlayerWithNet() {
    if (speedTimer > 0) glColor3f(0.9f, 0.8f, 0.2f); else glColor3f(0.2f, 0.5f, 0.8f);
    drawFilledRect(playerX - 0.05f, PLAYER_Y, playerX + 0.05f, PLAYER_Y + 0.15f);
    float halfNet = netWidth / 2.0f;
    glColor3f(0.5f, 0.3f, 0.1f);
    drawFilledRect(playerX - halfNet, PLAYER_Y - 0.04f, playerX + halfNet, PLAYER_Y + 0.12f);
}

void drawHUD() {
    char buf[64];
    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "Score: %d", score); drawText(buf, -0.95f, 0.9f);
    sprintf(buf, "Lives: %d", lives); drawText(buf, 0.75f, 0.9f);
}

// --- Screen states ---
void drawMenuScreen() {
    drawEnvironment(); drawJungleTrees();
    glColor4f(0,0,0,0.7f); drawFilledRect(-1,-1,1,1);
    glColor3f(0,1,0); drawTextLarge("JUNGLE EGG CHASE", -0.25f, 0.3f);
    glColor3f(1,1,1); drawText("Press ENTER to Start", -0.15f, 0.1f);
}

void drawPauseScreen() { 
    glColor4f(0,0,0,0.5f); drawFilledRect(-1,-1,1,1);
    glColor3f(1,1,1); drawTextLarge("PAUSED", -0.1f, 0.1f);
}

void drawHelpScreen() {
    glColor4f(0,0,0,0.8f); drawFilledRect(-1,-1,1,1);
    glColor3f(1,1,1); drawTextLarge("HOW TO PLAY", -0.15f, 0.3f);
    drawText("Catch eggs from Dino. Avoid rotten ones.", -0.3f, 0.1f);
}

void drawGameOverScreen() {
    glColor4f(0,0,0,0.8f); drawFilledRect(-1,-1,1,1);
    glColor3f(1,0,0); drawTextLarge("GAME OVER", -0.15f, 0.3f);
    glColor3f(1,1,1); char b[32]; sprintf(b, "Score: %d", score); drawText(b, -0.1f, 0.1f);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (currentState == MENU) drawMenuScreen();
    else if (currentState == PLAYING) { drawEnvironment(); drawJungleTrees(); drawDinosaur(); drawFallingItems(); drawPlayerWithNet(); drawHUD(); }
    else if (currentState == PAUSED) { drawEnvironment(); drawFallingItems(); drawPauseScreen(); }
    else if (currentState == HELP) drawHelpScreen();
    else if (currentState == GAMEOVER) drawGameOverScreen();
    glutSwapBuffers();
}

void timerCB(int v) {
    if (currentState == PLAYING) updateGame();
    glutPostRedisplay();
    glutTimerFunc(16, timerCB, 0);
}

void keyboardDown(unsigned char key, int x, int y) {
    if (key == 13) { if(currentState == MENU || currentState == GAMEOVER) resetGame(); }
    if (key == 27) { if(currentState == PLAYING) currentState = PAUSED; else if(currentState == PAUSED) currentState = PLAYING; else exit(0); }
}

void specialDown(int key, int x, int y) { keys[key] = true; }
void specialUp(int key, int x, int y) { keys[key] = false; }

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Jungle Egg Chase");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardDown);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutTimerFunc(0, timerCB, 0);
    glutMainLoop();
    return 0;
}