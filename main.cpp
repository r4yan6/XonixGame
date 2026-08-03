#include <SFML/Graphics.hpp>
#include <cmath>
#include "cmath"
#include <string>
#include <time.h>
#include <iostream>
#include <fstream>
using namespace sf;
using namespace std;

const int M = 25;
const int N = 40;
const int topOffset = 50; // Reserved top bar for HUD (Score, Time, Moves, Power-ups)

int grid[M][N] = {0};
int ts = 18; // tile size

// Scoring & power-up globals
int playerscore = 0;
int powerupinventory = 0;
int powerupsawarded = 0;
int bonuscount = 0;
float freezetimer = 0.0f;

// Scoreboard globals
const int maxscores = 5;
const char* scorefile = "scores.txt";
int topscorearr[maxscores];
float toptimearr[maxscores];

int getnextpowerupthreshold(int awarded) {
    if (awarded == 0) return 50;
    if (awarded == 1) return 70;
    return 70 + (awarded - 1) * 30;
}

void loadscores() {
    for (int i = 0; i < maxscores; i++) {
        topscorearr[i] = 0;
        toptimearr[i] = 0.0f;
    }
    ifstream infile(scorefile);
    if (infile.is_open()) {
        for (int i = 0; i < maxscores; i++) {
            if (!(infile >> topscorearr[i] >> toptimearr[i]))
                break;
        }
        infile.close();
    }
}

void savescores() {
    ofstream outfile(scorefile);
    if (outfile.is_open()) {
        for (int i = 0; i < maxscores; i++) {
            outfile << topscorearr[i] << " " << toptimearr[i] << endl;
        }
        outfile.close();
    }
}

void updatescoreboard(int currentscore, float currenttime) {
    loadscores();
    int pos = -1;
    for (int i = 0; i < maxscores; i++) {
        if (currentscore > topscorearr[i]) {
            pos = i;
            break;
        }
    }
    if (pos == -1) return;
    for (int i = maxscores - 1; i > pos; i--) {
        topscorearr[i] = topscorearr[i - 1];
        toptimearr[i] = toptimearr[i - 1];
    }
    topscorearr[pos] = currentscore;
    toptimearr[pos] = currenttime;
    savescores();
}

void resizeEnemyArrays(int* &enemyX, int* &enemyY, float* &enemyAngle,
                        int* &enemyDX, int* &enemyDY,
                        int enemyCount, int &enemyCapacity, int newCapacity) {
    int* newX = new int[newCapacity];
    int* newY = new int[newCapacity];
    float* newAngle = new float[newCapacity];
    int* newDX = new int[newCapacity];
    int* newDY = new int[newCapacity];

    for (int i = 0; i < enemyCount; i++) {
        newX[i] = enemyX[i];
        newY[i] = enemyY[i];
        newAngle[i] = enemyAngle[i];
        newDX[i] = enemyDX[i];
        newDY[i] = enemyDY[i];
    }

    delete[] enemyX;
    delete[] enemyY;
    delete[] enemyAngle;
    delete[] enemyDX;
    delete[] enemyDY;

    enemyX = newX;
    enemyY = newY;
    enemyAngle = newAngle;
    enemyDX = newDX;
    enemyDY = newDY;

    enemyCapacity = newCapacity;
}

void drop(int y, int x) {
  if (y < 0 || y >= M || x < 0 || x >= N)
    return;

  if (grid[y][x] != 0)
    return;

  grid[y][x] = -1;

  if (y - 1 >= 0 && grid[y - 1][x] == 0)
    drop(y - 1, x);

  if (y + 1 < M && grid[y + 1][x] == 0)
    drop(y + 1, x);

  if (x - 1 >= 0 && grid[y][x - 1] == 0)
    drop(y, x - 1);

  if (x + 1 < N && grid[y][x + 1] == 0)
    drop(y, x + 1);
}

void resetGame(int* &enemyX, int* &enemyY, int* &enemyDX, int* &enemyDY, float* &enemyAngle, int &enemyCount, int &enemyCapacity, int& moves_count, bool& isBuilding, float& gameTime, int& speedBoostCount, int currentMode) {
  for (int i = 0; i < M; i++) {
    for (int j = 0; j < N; j++) {
      if (i == 0 || j == 0 || i == M - 1 || j == N - 1)
        grid[i][j] = 1;
      else
        grid[i][j] = 0;
    }
  }
  if (currentMode == 4) { // MODE_CONTINUOUS
    enemyCount = 2;
  }
  for (int i = 0; i < enemyCount; i++) {
    enemyX[i] = 300 + (i % 2 == 0 ? 60 : -60);
    enemyY[i] = 220 + (i % 2 == 0 ? 40 : -40);
    enemyAngle[i] = 0.0f;
    enemyDX[i] = 4 - rand() % 8;
    if (enemyDX[i] == 0) enemyDX[i] = 2;
    enemyDY[i] = 4 - rand() % 8;
    if (enemyDY[i] == 0) enemyDY[i] = 2;
  }
  enemyCapacity = 8;
  moves_count = 0;
  isBuilding = true;
  gameTime = 0.0f;
  speedBoostCount = 0;

  // Reset scoring & power-up state
  playerscore = 0;
  bonuscount = 0;
  powerupinventory = 0;
  powerupsawarded = 0;
  freezetimer = 0.0f;
}

void moveZigZag(int&x, int&y, int&dx, int&dy, float gameTime){
  // Guard rail: Cap speed magnitude to prevent large single-frame leaps
  int maxSpeed = 6;
  int currentDx = (dx > maxSpeed) ? maxSpeed : ((dx < -maxSpeed) ? -maxSpeed : dx);
  int currentDy = (dy > maxSpeed) ? maxSpeed : ((dy < -maxSpeed) ? -maxSpeed : dy);

  int amplitude = 4;
  float freq = 8.0f;
  x += currentDx;
  y += currentDy + int(sin(freq * gameTime) * amplitude);
}

void moveLinear(int&x, int&y, int&dx, int&dy, float gameTime){
  x += dx;
  y += dy;
}

void moveCircle(int &x, int &y, float &angle){
  int centreX = N*ts / 2;
  int centreY = M*ts / 2;
  float radius = 150.0f;
  float twoPI = M_PI * 2.0f;
  float rate = twoPI / 270.0f;

  angle += rate;
  if (angle >= twoPI){
    angle -= twoPI;
  }

  x = centreX + int(cos(angle) * radius);
  y = centreY + int(sin(angle) * radius);
}

int main() {
  srand(time(0));

  RenderWindow window(VideoMode(N * ts, M * ts + topOffset), "Xonix Game!");
  window.setFramerateLimit(60);

  Texture t1, t2, t3;
  t1.loadFromFile("images/tiles.png");
  t2.loadFromFile("images/gameover.png");
  t3.loadFromFile("images/enemy.png");

  Sprite sTile(t1), sGameover(t2), sEnemy(t3);
  sGameover.setPosition((N * ts - 300) / 2.0f, 150.0f);
  sEnemy.setOrigin(20, 20);

  // bgs
  Texture bg1, bg2, bg3, bg4;
  bg1.loadFromFile("assets/Bgs/main_menu.png");
  bg2.loadFromFile("assets/Bgs/easy.png");
  bg3.loadFromFile("assets/Bgs/medium.png");
  bg4.loadFromFile("assets/Bgs/hard.png");
  Sprite menu_bg(bg1), easy_bg(bg2), medium_bg(bg3), hard_bg(bg4);

  // Load font for menus and text
  Font font;
  font.loadFromFile("assets/font.ttf");

  // Reusable HUD text object (optimized for performance)
  Text moveText("", font, 22);
  moveText.setFillColor(Color::Yellow);
  moveText.setStyle(Text::Bold);
  moveText.setPosition(15, 12);

  // Scale backgrounds to window size (720 x (450 + topOffset))
  if (bg1.getSize().x > 0)
    menu_bg.setScale((float)(N * ts) / bg1.getSize().x, (float)(M * ts + topOffset) / bg1.getSize().y);
  if (bg2.getSize().x > 0)
    easy_bg.setScale((float)(N * ts) / bg2.getSize().x, (float)(M * ts + topOffset) / bg2.getSize().y);
  if (bg3.getSize().x > 0)
    medium_bg.setScale((float)(N * ts) / bg3.getSize().x, (float)(M * ts + topOffset) / bg3.getSize().y);
  if (bg4.getSize().x > 0)
    hard_bg.setScale((float)(N * ts) / bg4.getSize().x, (float)(M * ts + topOffset) / bg4.getSize().y);

  int enemyCount = 4;
  int enemyCapacity = 8;
  int moveCount = 0;
  bool isBuilding = true;
  float gameTime = 0.0f;
  int speedBoostCount = 0;

  // Separate arrays store each enemy's data.
  int* enemyX = new int [enemyCapacity];
  int* enemyY = new int [enemyCapacity];
  int* enemyDX = new int [enemyCapacity];
  int* enemyDY = new int [enemyCapacity];
  float* enemyAngle = new float [enemyCapacity];

  const int STATE_MENU = 0;
  const int STATE_DIFFICULTY = 1;
  const int STATE_PLAYING = 2;
  const int STATE_SCOREBOARD = 3;
  const int STATE_GAMEOVER = 4;

  const int MODE_EASY = 1;
  const int MODE_MEDIUM = 2;
  const int MODE_HARD = 3;
  const int MODE_CONTINUOUS = 4;

  int currentState = STATE_MENU;
  int currentMode = MODE_EASY;
  int selectedOption = 0;
  int selectedDifficultyOption = 0;

  resetGame(enemyX, enemyY, enemyDX, enemyDY, enemyAngle, enemyCount, enemyCapacity, moveCount, isBuilding, gameTime, speedBoostCount, currentMode);

  bool Game = true;
  int x = 10, y = 0, dx = 0, dy = 0;
  float timer = 0, delay = 0.07;
  Clock clock;

  while (window.isOpen()) {
    float time = clock.getElapsedTime().asSeconds();
    clock.restart();
    timer += time;

    Event e;
    while (window.pollEvent(e)) {
      if (e.type == Event::Closed)
        window.close();

      if (e.type == Event::KeyPressed) {
        if (currentState == STATE_MENU) {
          if (e.key.code == Keyboard::Up) {
            selectedOption--;
            if(selectedOption < 0)  selectedOption = 3;
          }
          if (e.key.code == Keyboard::Down) {
            selectedOption++;
            if(selectedOption > 3)  selectedOption = 0;
          }
          if (e.key.code == Keyboard::Return || e.key.code == Keyboard::Space) {
            if (selectedOption == 0) {
              // Start Game with current mode
              if (currentMode == MODE_EASY) enemyCount = 2;
              else if (currentMode == MODE_MEDIUM) enemyCount = 4;
              else if (currentMode == MODE_HARD) enemyCount = 6;
              else if (currentMode == MODE_CONTINUOUS) enemyCount = 2;
              else  enemyCount = 2;

              resetGame(enemyX, enemyY, enemyDX, enemyDY, enemyAngle, enemyCount, enemyCapacity, moveCount, isBuilding, gameTime, speedBoostCount, currentMode);
              x = 10; y = 0; dx = 0; dy = 0;
              Game = true;
              currentState = STATE_PLAYING;
            } else if (selectedOption == 1) {
              currentState = STATE_DIFFICULTY;
            } else if (selectedOption == 2) {
              currentState = STATE_SCOREBOARD;
            } else if (selectedOption == 3) {
              window.close(); //Exit Game
            }
          }
        } else if (currentState == STATE_DIFFICULTY) {
          if (e.key.code == Keyboard::Up) {
            selectedDifficultyOption--;
            if(selectedDifficultyOption < 0)  selectedDifficultyOption = 3;
          }
          if (e.key.code == Keyboard::Down) {
            selectedDifficultyOption++;
            if(selectedDifficultyOption > 3)  selectedDifficultyOption = 0;
          }
          if (e.key.code == Keyboard::Return || e.key.code == Keyboard::Space) {
            if (selectedDifficultyOption == 0) {
              currentMode = MODE_EASY;
              enemyCount = 2;
            } else if (selectedDifficultyOption == 1) {
              currentMode = MODE_MEDIUM;
              enemyCount = 4;
            } else if (selectedDifficultyOption == 2) {
              currentMode = MODE_HARD;
              enemyCount = 6;
            } else if (selectedDifficultyOption == 3) {
              currentMode = MODE_CONTINUOUS;
              enemyCount = 2;
            }
            resetGame(enemyX, enemyY, enemyDX, enemyDY, enemyAngle, enemyCount, enemyCapacity, moveCount, isBuilding, gameTime, speedBoostCount, currentMode);
            x = 10; y = 0; dx = 0; dy = 0;
            Game = true;
            currentState = STATE_PLAYING;
          }
          if (e.key.code == Keyboard::Escape) {
            currentState = STATE_MENU;
          }
        } else if (currentState == STATE_GAMEOVER) {
          if (e.key.code == Keyboard::Return || e.key.code == Keyboard::Space) {
            resetGame(enemyX, enemyY, enemyDX, enemyDY, enemyAngle, enemyCount, enemyCapacity, moveCount, isBuilding, gameTime, speedBoostCount, currentMode);
            x = 10; y = 0; dx = 0; dy = 0;
            Game = true;
            currentState = STATE_PLAYING;
          }
          if (e.key.code == Keyboard::Escape) {
            currentState = STATE_MENU;
          }
        } else if (currentState == STATE_SCOREBOARD) {
          if (e.key.code == Keyboard::Escape) {
            currentState = STATE_MENU;
          }
        } else if (currentState == STATE_PLAYING) {
          if (e.key.code == Keyboard::Escape) {
            currentState = STATE_MENU;
          }
          if (e.key.code == Keyboard::F) {
            if (powerupinventory > 0 && freezetimer <= 0.0f) {
              powerupinventory--;
              freezetimer = 3.0f;
            }
          }
        }
      }
    }

    if (currentState == STATE_MENU) {
      window.clear();
      window.draw(menu_bg);

      // Game Title
      Text titleText("XONIX GAME", font, 69);
      titleText.setFillColor(Color::Yellow);
      titleText.setStyle(Text::Bold);
      titleText.setPosition((N * ts - titleText.getGlobalBounds().width) / 2.0f, 25.0f);
      window.draw(titleText);

      // Subtitle
      Text subText("Capture the Board", font, 27);
      subText.setFillColor(Color(200, 200, 200));
      subText.setPosition((N * ts - subText.getGlobalBounds().width) / 2.0f, 100.0f);
      window.draw(subText);

      // Menu Options
      const char* options[4] = {
        "Start Game",
        "Select Level / Difficulty",
        "Scoreboard",
        "Exit Game"
      };

      for (int i = 0; i < 4; i++) {
        Text optionText(options[i], font, 36);
        if (i == selectedOption) {
          optionText.setFillColor(Color::Cyan);
          optionText.setStyle(Text::Bold);
        } else {
          optionText.setFillColor(Color::White);
        }
        optionText.setPosition((N * ts - optionText.getGlobalBounds().width) / 2.0f, 150.0f + i * 55.0f);
        window.draw(optionText);
      }

      // Hint Text
      Text hintText("Use UP / DOWN arrows to navigate, ENTER to select", font, 21);
      hintText.setFillColor(Color(180, 180, 180));
      hintText.setPosition((N * ts - hintText.getGlobalBounds().width) / 2.0f, 395.0f);
      window.draw(hintText);

      window.display();
      continue;
    }

    if (currentState == STATE_DIFFICULTY) {
      window.clear();
      window.draw(menu_bg);

      // Title
      Text titleText("SELECT DIFFICULTY", font, 48);
      titleText.setFillColor(Color::Yellow);
      titleText.setStyle(Text::Bold);
      titleText.setPosition((N * ts - titleText.getGlobalBounds().width) / 2.0f, 35.0f);
      window.draw(titleText);

      const char* diffOptions[4] = {
        "Easy Mode (2 Enemies)",
        "Medium Mode (4 Enemies)",
        "Hard Mode (6 Enemies)",
        "Continuous Mode (+2 Every 20s)"
      };

      for (int i = 0; i < 4; i++) {
        Text optionText(diffOptions[i], font, 32);
        if (i == selectedDifficultyOption) {
          optionText.setFillColor(Color::Cyan);
          optionText.setStyle(Text::Bold);
        } else {
          optionText.setFillColor(Color::White);
        }
        optionText.setPosition((N * ts - optionText.getGlobalBounds().width) / 2.0f, 130.0f + i * 55.0f);
        window.draw(optionText);
      }

      Text hintText("Use UP / DOWN to navigate, ENTER to choose, ESC for Menu", font, 18);
      hintText.setFillColor(Color(180, 180, 180));
      hintText.setPosition((N * ts - hintText.getGlobalBounds().width) / 2.0f, 395.0f);
      window.draw(hintText);

      window.display();
      continue;
    }

    if (currentState == STATE_SCOREBOARD) {
      window.clear();
      window.draw(menu_bg);

      Text sbtitle("TOP 5 SCORES", font, 48);
      sbtitle.setFillColor(Color::Yellow);
      sbtitle.setStyle(Text::Bold);
      sbtitle.setPosition((N * ts - sbtitle.getGlobalBounds().width) / 2.0f, 30.0f);
      window.draw(sbtitle);

      loadscores();

      for (int i = 0; i < maxscores; i++) {
        string line;
        if (topscorearr[i] == 0) {
          line = "--- Empty Slot ---";
        } else {
          line = "#" + to_string(i + 1) + "   Score: "
               + to_string(topscorearr[i])
               + "   Time: " + to_string((int)toptimearr[i]) + "s";
        }
        Text scoreline(line, font, 28);
        scoreline.setFillColor((i == 0) ? Color::Yellow : Color::White);
        scoreline.setPosition((N * ts - scoreline.getGlobalBounds().width) / 2.0f,
                              120.0f + i * 50.0f);
        window.draw(scoreline);
      }

      Text sbhint("Press ESC to return to Menu", font, 20);
      sbhint.setFillColor(Color(180, 180, 180));
      sbhint.setPosition((N * ts - sbhint.getGlobalBounds().width) / 2.0f, 400.0f);
      window.draw(sbhint);

      window.display();
      continue;
    }

    if (currentState == STATE_GAMEOVER) {
      window.clear();
      if (currentMode == MODE_EASY) window.draw(easy_bg);
      else if (currentMode == MODE_MEDIUM) window.draw(medium_bg);
      else if (currentMode == MODE_HARD) window.draw(hard_bg);
      else window.draw(easy_bg);

      for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
          if (grid[i][j] == 0) continue;
          if (grid[i][j] == 1) sTile.setTextureRect(IntRect(0, 0, ts, ts));
          if (grid[i][j] == 2) sTile.setTextureRect(IntRect(54, 0, ts, ts));
          sTile.setPosition(j * ts, i * ts + topOffset);
          window.draw(sTile);
        }
      }

      sGameover.setPosition((N * ts - sGameover.getGlobalBounds().width) / 2.0f, 55.0f + topOffset);
      window.draw(sGameover);
      float gobottom = 55.0f + topOffset + sGameover.getGlobalBounds().height;

      Text finalscore("Score: " + to_string(playerscore)
                    + "    Time: " + to_string((int)gameTime) + "s", font, 28);
      finalscore.setFillColor(Color::White);
      finalscore.setStyle(Text::Bold);
      finalscore.setPosition((N * ts - finalscore.getGlobalBounds().width) / 2.0f, gobottom + 15.0f);
      window.draw(finalscore);

      Text restartHint("Press ENTER to Restart", font, 32);
      restartHint.setFillColor(Color::Cyan);
      restartHint.setStyle(Text::Bold);
      restartHint.setPosition((N * ts - restartHint.getGlobalBounds().width) / 2.0f, gobottom + 60.0f);
      window.draw(restartHint);

      Text menuhint("Press ESC for Main Menu", font, 20);
      menuhint.setFillColor(Color(180, 180, 180));
      menuhint.setPosition((N * ts - menuhint.getGlobalBounds().width) / 2.0f, gobottom + 105.0f);
      window.draw(menuhint);

      window.display();
      continue;
    }

    if (currentState == STATE_PLAYING) {
      // Accumulate total active game duration in seconds
      gameTime += time;

      // Speed scaling: increase enemy movement speed by a fixed amount (+1 unit) every 20 seconds
      int current20sInterval = (int)gameTime / 20;
      if (current20sInterval > speedBoostCount) {
        speedBoostCount = current20sInterval;
        if(currentMode == MODE_CONTINUOUS){
          enemyCount+= 2;
          if(enemyCount > enemyCapacity){
            resizeEnemyArrays(enemyX, enemyY, enemyAngle, enemyDX, enemyDY, enemyCount, enemyCapacity, enemyCapacity*2);
          }
        // Initialize the new enemy's starting position and speed:
        for(int idx = enemyCount - 2; idx <= enemyCount - 1; idx++){
          enemyX[idx] = 300 + (rand() % 80 - 40);
          enemyY[idx] = 220 + (rand() % 60 - 30);
          enemyAngle[idx] = 0.0f;
          enemyDX[idx] = 4 - rand() % 8;
          if (enemyDX[idx] == 0) enemyDX[idx] = 2;
          enemyDY[idx] = 4 - rand() % 8;
          if (enemyDY[idx] == 0) enemyDY[idx] = 2;
        }
        }
        for (int i = 0; i < enemyCount; i++) {
          if (enemyDX[i] >= 0) enemyDX[i] = (enemyDX[i] < 6) ? enemyDX[i] + 1 : 6;
          else enemyDX[i] = (enemyDX[i] > -6) ? enemyDX[i] - 1 : -6;

          if (enemyDY[i] >= 0) enemyDY[i] = (enemyDY[i] < 6) ? enemyDY[i] + 1 : 6;
          else enemyDY[i] = (enemyDY[i] > -6) ? enemyDY[i] - 1 : -6;
        }
      }

      if (Keyboard::isKeyPressed(Keyboard::Left))  { dx = -1; dy = 0; }
      if (Keyboard::isKeyPressed(Keyboard::Right)) { dx = 1;  dy = 0; }
      if (Keyboard::isKeyPressed(Keyboard::Up))    { dx = 0;  dy = -1; }
      if (Keyboard::isKeyPressed(Keyboard::Down))  { dx = 0;  dy = 1; }

      if (timer > delay) {
        x += dx;
        y += dy;

        if (x < 0) x = 0;
        if (x > N - 1) x = N - 1;
        if (y < 0) y = 0;
        if (y > M - 1) y = M - 1;
        //checks if we collide with our trail
        if (grid[y][x] == 2) {
          Game = false;
          currentState = STATE_GAMEOVER;
          updatescoreboard(playerscore, gameTime);
        }
        if(grid[y][x] == 0){
          if(isBuilding)
            moveCount++;//increment move count
        }

        if (grid[y][x] == 0){
          grid[y][x] = 2;
          isBuilding = false;
        }

        timer = 0;
      }

      //continuos mode


      // Decrement freeze timer
      if (freezetimer > 0.0f) {
        freezetimer -= time;
        if (freezetimer < 0.0f) freezetimer = 0.0f;
      }

      // Move every enemy using the four arrays with guard rails against land overlap.
      // Enemies only move when NOT frozen.
      if (freezetimer <= 0.0f) {
        for (int i = 0; i < enemyCount; i++) {
          int prevX = enemyX[i];
          int prevY = enemyY[i];

          if ((int)gameTime >= 5) {
            if (i % 3 == 0)
              moveZigZag(enemyX[i], enemyY[i], enemyDX[i], enemyDY[i], gameTime);
            else if (i % 3 == 1)
              moveCircle(enemyX[i], enemyY[i], enemyAngle[i]);
            else
              moveLinear(enemyX[i], enemyY[i], enemyDX[i], enemyDY[i], gameTime);
          } else {
            moveLinear(enemyX[i], enemyY[i], enemyDX[i], enemyDY[i], gameTime);
          }

          // Clamp screen outer boundaries
          if (enemyX[i] < ts) { enemyX[i] = ts; enemyDX[i] = abs(enemyDX[i]); }
          if (enemyX[i] >= (N - 1) * ts) { enemyX[i] = (N - 1) * ts - 1; enemyDX[i] = -abs(enemyDX[i]); }
          if (enemyY[i] < ts) { enemyY[i] = ts; enemyDY[i] = abs(enemyDY[i]); }
          if (enemyY[i] >= (M - 1) * ts) { enemyY[i] = (M - 1) * ts - 1; enemyDY[i] = -abs(enemyDY[i]); }

          // Grid index safety checks
          int ex = enemyX[i] / ts;
          if (ex < 0) ex = 0; if (ex >= N) ex = N - 1;
          int ey = enemyY[i] / ts;
          if (ey < 0) ey = 0; if (ey >= M) ey = M - 1;

          // Guard Rail: If new position overlaps land tile (grid == 1), revert position & invert velocity
          if (grid[ey][ex] == 1) {
            enemyX[i] = prevX;
            enemyY[i] = prevY;
            enemyDX[i] = -enemyDX[i];
            enemyDY[i] = -enemyDY[i];
          }
        }
      }

      if (grid[y][x] == 1) {
        dx = 0;
        dy = 0;
        isBuilding = true;

        // Count empty cells before flood fill for scoring
        int emptybefore = 0;
        for (int i = 0; i < M; i++)
          for (int j = 0; j < N; j++)
            if (grid[i][j] == 0) emptybefore++;

        for (int i = 0; i < enemyCount; i++) {
          int ey = enemyY[i] / ts;
          int ex = enemyX[i] / ts;
          if (ey < 0) ey = 0; if (ey >= M) ey = M - 1;
          if (ex < 0) ex = 0; if (ex >= N) ex = N - 1;
          drop(ey, ex);
        }

        // Count enemy-reachable cells (marked -1)
        int enemyreachable = 0;
        for (int i = 0; i < M; i++)
          for (int j = 0; j < N; j++)
            if (grid[i][j] == -1) enemyreachable++;

        // Calculate captured tiles and apply scoring multipliers
        int capturedtiles = emptybefore - enemyreachable;
        if (capturedtiles > 0) {
          int multiplier = 1;
          int threshold = (bonuscount >= 3) ? 5 : 10;

          if (capturedtiles > threshold) {
            if (bonuscount >= 5)
              multiplier = 4;
            else
              multiplier = 2;
            bonuscount++;
          }

          playerscore += capturedtiles * multiplier;

          // Check if a power-up should be awarded
          int nextthreshold = getnextpowerupthreshold(powerupsawarded);
          if (playerscore >= nextthreshold) {
            powerupinventory++;
            powerupsawarded++;
          }
        }

        for (int i = 0; i < M; i++) {
          for (int j = 0; j < N; j++) {
            if (grid[i][j] == -1)
              grid[i][j] = 0;
            else
              grid[i][j] = 1;
          }
        }
      }
      //checks if enemy collides with our trail
      for (int i = 0; i < enemyCount; i++) {
        int ey = enemyY[i] / ts;
        int ex = enemyX[i] / ts;
        if (ey < 0) ey = 0; if (ey >= M) ey = M - 1;
        if (ex < 0) ex = 0; if (ex >= N) ex = N - 1;

        if (grid[ey][ex] == 2) {
          Game = false;
          currentState = STATE_GAMEOVER;
          updatescoreboard(playerscore, gameTime);
        }
      }

      // Draw the game background based on current mode
      window.clear();
      if (currentMode == MODE_EASY) window.draw(easy_bg);
      else if (currentMode == MODE_MEDIUM) window.draw(medium_bg);
      else if (currentMode == MODE_HARD) window.draw(hard_bg);
      else window.draw(easy_bg);

      // Draw top HUD bar background
      RectangleShape hudBar(Vector2f(N * ts, topOffset));
      hudBar.setFillColor(Color(12, 12, 28, 230));
      hudBar.setOutlineThickness(1.0f);
      hudBar.setOutlineColor(Color(60, 60, 100));
      hudBar.setPosition(0, 0);
      window.draw(hudBar);

      // Render Score
      Text scoretext("Score: " + to_string(playerscore), font, 20);
      scoretext.setFillColor(Color::White);
      scoretext.setStyle(Text::Bold);
      scoretext.setPosition(10, 12);
      window.draw(scoretext);

      // Render Move Count
      Text count("Moves: " + std::to_string(moveCount), font, 20);
      count.setFillColor(Color::Yellow);
      count.setStyle(Text::Bold);
      count.setPosition(140, 12);
      window.draw(count);

      // Render Power-up Inventory
      Text powertext("Powerups [F]: " + to_string(powerupinventory), font, 20);
      powertext.setFillColor(Color::Green);
      powertext.setStyle(Text::Bold);
      powertext.setPosition(270, 12);
      window.draw(powertext);

      // Render Freeze Timer (only when active)
      if (freezetimer > 0.0f) {
        Text freezetext("FROZEN: " + to_string((int)freezetimer + 1) + "s", font, 20);
        freezetext.setFillColor(Color::Cyan);
        freezetext.setStyle(Text::Bold);
        freezetext.setPosition(460, 12);
        window.draw(freezetext);
      }

      // Render Elapsed Game Time
      int seconds = (int)gameTime;
      int mins = seconds / 60;
      int secs = seconds % 60;
      std::string timeStr = "Time: " + (mins < 10 ? std::string("0") : std::string("")) + std::to_string(mins) + ":" + (secs < 10 ? std::string("0") : std::string("")) + std::to_string(secs);
      Text timeDisplay(timeStr, font, 20);
      timeDisplay.setFillColor(Color(200, 200, 200));
      timeDisplay.setStyle(Text::Bold);
      timeDisplay.setPosition(N * ts - timeDisplay.getGlobalBounds().width - 10, 12);
      window.draw(timeDisplay);

      for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
          if (grid[i][j] == 0)
            continue;

          if (grid[i][j] == 1)
            sTile.setTextureRect(IntRect(0, 0, ts, ts));

          if (grid[i][j] == 2)
            sTile.setTextureRect(IntRect(54, 0, ts, ts));

          sTile.setPosition(j * ts, i * ts + topOffset);
          window.draw(sTile);
        }
      }

      sTile.setTextureRect(IntRect(36, 0, ts, ts));
      sTile.setPosition(x * ts, y * ts + topOffset);
      window.draw(sTile);

      sEnemy.rotate(10);

      for (int i = 0; i < enemyCount; i++) {
        sEnemy.setPosition(enemyX[i], enemyY[i] + topOffset);
        window.draw(sEnemy);
      }

      window.display();
    }
  }

  delete[] enemyX;
  delete[] enemyY;
  delete[] enemyAngle;
  delete[] enemyDX;
  delete[] enemyDY;

  return 0;
}
