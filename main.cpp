#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
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

// Player 2 scoring & power-up globals
int playerscore2 = 0;
int powerupinventory2 = 0;
int powerupsawarded2 = 0;
int bonuscount2 = 0;
float p1freezetimer = 0.0f;
float p2freezetimer = 0.0f;

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

  // Reset P2 scoring & power-up state
  playerscore2 = 0;
  bonuscount2 = 0;
  powerupinventory2 = 0;
  powerupsawarded2 = 0;
  p1freezetimer = 0.0f;
  p2freezetimer = 0.0f;
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
  Texture bg1, bg2, bg3;
  bg1.loadFromFile("assets/Bgs/main_menu.png");
  bg2.loadFromFile("assets/Bgs/easy.png");
  bg3.loadFromFile("assets/Bgs/medium.png");
  Sprite menu_bg(bg1), game_bg(bg2), freeze_bg(bg3);

  // Sound Buffers and Sounds (Bonus Feature)
  SoundBuffer freezeBuffer, bonusBuffer, gameOverBuffer;
  bool freezeSoundLoaded = freezeBuffer.loadFromFile("assets/freeze.wav");
  bool bonusSoundLoaded = bonusBuffer.loadFromFile("assets/bonus.wav");
  bool gameOverSoundLoaded = gameOverBuffer.loadFromFile("assets/gameOver.wav");

  Sound freezeSound, bonusSound, gameOverSound;
  if (freezeSoundLoaded) freezeSound.setBuffer(freezeBuffer);
  if (bonusSoundLoaded) bonusSound.setBuffer(bonusBuffer);
  if (gameOverSoundLoaded) gameOverSound.setBuffer(gameOverBuffer);

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
    game_bg.setScale((float)(N * ts) / bg2.getSize().x, (float)(M * ts + topOffset) / bg2.getSize().y);
  if (bg3.getSize().x > 0)
    freeze_bg.setScale((float)(N * ts) / bg3.getSize().x, (float)(M * ts + topOffset) / bg3.getSize().y);

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
  bool Game2 = true;
  bool twoPlayerMode = false;
  int x = 10, y = 0, dx = 0, dy = 0;
  int x2 = N - 11, y2 = M - 1, dx2 = 0, dy2 = 0;
  bool isBuilding2 = true;
  int moveCount2 = 0;
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
            if(selectedOption < 0)  selectedOption = 4;
          }
          if (e.key.code == Keyboard::Down) {
            selectedOption++;
            if(selectedOption > 4)  selectedOption = 0;
          }
          if (e.key.code == Keyboard::Return || e.key.code == Keyboard::Space) {
            if (selectedOption == 0 || selectedOption == 1) {
              // Start Game (1P or 2P)
              twoPlayerMode = (selectedOption == 1);
              if (currentMode == MODE_EASY) enemyCount = 2;
              else if (currentMode == MODE_MEDIUM) enemyCount = 4;
              else if (currentMode == MODE_HARD) enemyCount = 6;
              else if (currentMode == MODE_CONTINUOUS) enemyCount = 2;
              else  enemyCount = 2;

              resetGame(enemyX, enemyY, enemyDX, enemyDY, enemyAngle, enemyCount, enemyCapacity, moveCount, isBuilding, gameTime, speedBoostCount, currentMode);
              x = 10; y = 0; dx = 0; dy = 0;
              Game = true;
              if (twoPlayerMode) {
                x2 = N - 11; y2 = M - 1; dx2 = 0; dy2 = 0;
                Game2 = true; isBuilding2 = true; moveCount2 = 0;
              }
              currentState = STATE_PLAYING;
            } else if (selectedOption == 2) {
              currentState = STATE_DIFFICULTY;
            } else if (selectedOption == 3) {
              currentState = STATE_SCOREBOARD;
            } else if (selectedOption == 4) {
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
            if (twoPlayerMode) {
              x2 = N - 11; y2 = M - 1; dx2 = 0; dy2 = 0;
              Game2 = true; isBuilding2 = true; moveCount2 = 0;
            }
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
            if (twoPlayerMode) {
              x2 = N - 11; y2 = M - 1; dx2 = 0; dy2 = 0;
              Game2 = true; isBuilding2 = true; moveCount2 = 0;
            }
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
          if (e.key.code == Keyboard::F && Game) {
            if (powerupinventory > 0 && freezetimer <= 0.0f) {
              powerupinventory--;
              freezetimer = 3.0f;
              if (twoPlayerMode) p2freezetimer = 3.0f;
              if (freezeSoundLoaded) freezeSound.play();
            }
          }
          if (twoPlayerMode && e.key.code == Keyboard::Q && Game2) {
            if (powerupinventory2 > 0 && freezetimer <= 0.0f) {
              powerupinventory2--;
              freezetimer = 3.0f;
              p1freezetimer = 3.0f;
              if (freezeSoundLoaded) freezeSound.play();
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
      const char* options[5] = {
        "Start Game (1P)",
        "Two Player Mode",
        "Select Level / Difficulty",
        "Scoreboard",
        "Exit Game"
      };

      for (int i = 0; i < 5; i++) {
        Text optionText(options[i], font, 32);
        if (i == selectedOption) {
          optionText.setFillColor(Color::Cyan);
          optionText.setStyle(Text::Bold);
        } else {
          optionText.setFillColor(Color::White);
        }
        optionText.setPosition((N * ts - optionText.getGlobalBounds().width) / 2.0f, 150.0f + i * 45.0f);
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
      window.draw(game_bg);

      for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
          if (grid[i][j] == 0) continue;
          if (grid[i][j] == 1) sTile.setTextureRect(IntRect(0, 0, ts, ts));
          if (grid[i][j] == 2) sTile.setTextureRect(IntRect(54, 0, ts, ts));
          if (grid[i][j] == 3) sTile.setTextureRect(IntRect(18, 0, ts, ts));
          sTile.setPosition(j * ts, i * ts + topOffset);
          window.draw(sTile);
        }
      }

      sGameover.setPosition((N * ts - sGameover.getGlobalBounds().width) / 2.0f, 55.0f + topOffset);
      window.draw(sGameover);
      float gobottom = 55.0f + topOffset + sGameover.getGlobalBounds().height;

      if (twoPlayerMode) {
        string winnerStr;
        if (playerscore > playerscore2) winnerStr = "Player 1 Wins!";
        else if (playerscore2 > playerscore) winnerStr = "Player 2 Wins!";
        else winnerStr = "It's a Tie!";

        Text winnerText(winnerStr, font, 32);
        winnerText.setFillColor(Color::Yellow);
        winnerText.setStyle(Text::Bold);
        winnerText.setPosition((N * ts - winnerText.getGlobalBounds().width) / 2.0f, gobottom + 10.0f);
        window.draw(winnerText);

        Text p1final("P1 Score: " + to_string(playerscore), font, 22);
        p1final.setFillColor(Color(255, 100, 100));
        p1final.setPosition((N * ts - p1final.getGlobalBounds().width) / 2.0f, gobottom + 50.0f);
        window.draw(p1final);

        Text p2final("P2 Score: " + to_string(playerscore2), font, 22);
        p2final.setFillColor(Color(100, 150, 255));
        p2final.setPosition((N * ts - p2final.getGlobalBounds().width) / 2.0f, gobottom + 78.0f);
        window.draw(p2final);
      } else {
        Text finalscore("Score: " + to_string(playerscore)
                      + "    Time: " + to_string((int)gameTime) + "s", font, 28);
        finalscore.setFillColor(Color::White);
        finalscore.setStyle(Text::Bold);
        finalscore.setPosition((N * ts - finalscore.getGlobalBounds().width) / 2.0f, gobottom + 15.0f);
        window.draw(finalscore);
      }

      Text restartHint("Press ENTER to Restart", font, 32);
      restartHint.setFillColor(Color::Cyan);
      restartHint.setStyle(Text::Bold);
      restartHint.setPosition((N * ts - restartHint.getGlobalBounds().width) / 2.0f, gobottom + (twoPlayerMode ? 110.0f : 60.0f));
      window.draw(restartHint);

      Text menuhint("Press ESC for Main Menu", font, 20);
      menuhint.setFillColor(Color(180, 180, 180));
      menuhint.setPosition((N * ts - menuhint.getGlobalBounds().width) / 2.0f, gobottom + (twoPlayerMode ? 150.0f : 105.0f));
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

      // P1 input (blocked when frozen)
      if (Game && p1freezetimer <= 0.0f) {
        if (Keyboard::isKeyPressed(Keyboard::Left))  { dx = -1; dy = 0; }
        if (Keyboard::isKeyPressed(Keyboard::Right)) { dx = 1;  dy = 0; }
        if (Keyboard::isKeyPressed(Keyboard::Up))    { dx = 0;  dy = -1; }
        if (Keyboard::isKeyPressed(Keyboard::Down))  { dx = 0;  dy = 1; }
      }

      // P2 input (only in 2P mode, blocked when frozen)
      if (twoPlayerMode && Game2 && p2freezetimer <= 0.0f) {
        if (Keyboard::isKeyPressed(Keyboard::A)) { dx2 = -1; dy2 = 0; }
        if (Keyboard::isKeyPressed(Keyboard::D)) { dx2 = 1;  dy2 = 0; }
        if (Keyboard::isKeyPressed(Keyboard::W)) { dx2 = 0;  dy2 = -1; }
        if (Keyboard::isKeyPressed(Keyboard::S)) { dx2 = 0;  dy2 = 1; }
      }

      if (timer > delay) {
        // --- P1 Movement (blocked when frozen) ---
        if (Game && p1freezetimer <= 0.0f) {
          x += dx;
          y += dy;
          if (x < 0) x = 0;
          if (x > N - 1) x = N - 1;
          if (y < 0) y = 0;
          if (y > M - 1) y = M - 1;
        }

        // --- P2 Movement (blocked when frozen) ---
        if (twoPlayerMode && Game2 && p2freezetimer <= 0.0f) {
          x2 += dx2;
          y2 += dy2;
          if (x2 < 0) x2 = 0;
          if (x2 > N - 1) x2 = N - 1;
          if (y2 < 0) y2 = 0;
          if (y2 > M - 1) y2 = M - 1;
        }

        // --- P1 Trail Collision (own trail or P2 trail) ---
        if (Game && (grid[y][x] == 2 || grid[y][x] == 3)) {
          Game = false;
          updatescoreboard(playerscore, gameTime);
          if (!twoPlayerMode || !Game2) {
            currentState = STATE_GAMEOVER;
            if (gameOverSoundLoaded) gameOverSound.play();
          }
        }

        // --- P2 Trail Collision (own trail or P1 trail) ---
        if (twoPlayerMode && Game2 && (grid[y2][x2] == 3 || grid[y2][x2] == 2)) {
          Game2 = false;
          updatescoreboard(playerscore2, gameTime);
          if (!Game) {
            currentState = STATE_GAMEOVER;
            if (gameOverSoundLoaded) gameOverSound.play();
          }
        }

        // --- Player vs Player same-cell Collision ---
        if (twoPlayerMode && Game && Game2 && x == x2 && y == y2) {
          bool p1mid = !isBuilding;  // true if P1 is mid-trail
          bool p2mid = !isBuilding2; // true if P2 is mid-trail
          if (p1mid && p2mid) {
            // Both mid-build -> both die
            Game = false; Game2 = false;
            updatescoreboard(playerscore, gameTime);
            updatescoreboard(playerscore2, gameTime);
            currentState = STATE_GAMEOVER;
            if (gameOverSoundLoaded) gameOverSound.play();
          } else if (p1mid && !p2mid) {
            // P1 building, P2 not -> P1 dies
            Game = false;
            updatescoreboard(playerscore, gameTime);
            if (!Game2) {
              currentState = STATE_GAMEOVER;
              if (gameOverSoundLoaded) gameOverSound.play();
            }
          } else if (!p1mid && p2mid) {
            // P2 building, P1 not -> P2 dies
            Game2 = false;
            updatescoreboard(playerscore2, gameTime);
            if (!Game) {
              currentState = STATE_GAMEOVER;
              if (gameOverSoundLoaded) gameOverSound.play();
            }
          }
          // Both on land -> no death
        }

        // --- P1 Trail Laying ---
        if (Game && grid[y][x] == 0) {
          if (isBuilding) moveCount++;
          grid[y][x] = 2;
          isBuilding = false;
        }

        // --- P2 Trail Laying ---
        if (twoPlayerMode && Game2 && grid[y2][x2] == 0) {
          if (isBuilding2) moveCount2++;
          grid[y2][x2] = 3;
          isBuilding2 = false;
        }

        timer = 0;
      }

      //continuos mode


      // Decrement freeze timers
      if (freezetimer > 0.0f) {
        freezetimer -= time;
        if (freezetimer < 0.0f) freezetimer = 0.0f;
      }
      if (p1freezetimer > 0.0f) {
        p1freezetimer -= time;
        if (p1freezetimer < 0.0f) p1freezetimer = 0.0f;
      }
      if (p2freezetimer > 0.0f) {
        p2freezetimer -= time;
        if (p2freezetimer < 0.0f) p2freezetimer = 0.0f;
      }

      // Move every enemy using the four arrays with guard rails against land overlap.
      // Enemies only move when NOT frozen.
      if (freezetimer <= 0.0f) {
        for (int i = 0; i < enemyCount; i++) {
          int prevX = enemyX[i];
          int prevY = enemyY[i];

          if ((int)gameTime >= 30) {
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

      // --- P1 Territory Claiming ---
      if (Game && grid[y][x] == 1) {
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
          if (bonusSoundLoaded) bonusSound.play();
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
            else if (grid[i][j] == 3)
              ; // preserve P2 trail
            else
              grid[i][j] = 1;
          }
        }
      }

      // --- P2 Territory Claiming ---
      if (twoPlayerMode && Game2 && grid[y2][x2] == 1) {
        dx2 = 0;
        dy2 = 0;
        isBuilding2 = true;

        int emptybefore2 = 0;
        for (int i = 0; i < M; i++)
          for (int j = 0; j < N; j++)
            if (grid[i][j] == 0) emptybefore2++;

        for (int i = 0; i < enemyCount; i++) {
          int ey = enemyY[i] / ts;
          int ex = enemyX[i] / ts;
          if (ey < 0) ey = 0; if (ey >= M) ey = M - 1;
          if (ex < 0) ex = 0; if (ex >= N) ex = N - 1;
          drop(ey, ex);
        }

        int enemyreachable2 = 0;
        for (int i = 0; i < M; i++)
          for (int j = 0; j < N; j++)
            if (grid[i][j] == -1) enemyreachable2++;

        int capturedtiles2 = emptybefore2 - enemyreachable2;
        if (capturedtiles2 > 0) {
          if (bonusSoundLoaded) bonusSound.play();
          int multiplier2 = 1;
          int threshold2 = (bonuscount2 >= 3) ? 5 : 10;

          if (capturedtiles2 > threshold2) {
            if (bonuscount2 >= 5)
              multiplier2 = 4;
            else
              multiplier2 = 2;
            bonuscount2++;
          }

          playerscore2 += capturedtiles2 * multiplier2;

          int nextthreshold2 = getnextpowerupthreshold(powerupsawarded2);
          if (playerscore2 >= nextthreshold2) {
            powerupinventory2++;
            powerupsawarded2++;
          }
        }

        for (int i = 0; i < M; i++) {
          for (int j = 0; j < N; j++) {
            if (grid[i][j] == -1)
              grid[i][j] = 0;
            else if (grid[i][j] == 2)
              ; // preserve P1 trail
            else
              grid[i][j] = 1;
          }
        }
      }
      //checks if enemy collides with player trails
      for (int i = 0; i < enemyCount; i++) {
        int ey = enemyY[i] / ts;
        int ex = enemyX[i] / ts;
        if (ey < 0) ey = 0; if (ey >= M) ey = M - 1;
        if (ex < 0) ex = 0; if (ex >= N) ex = N - 1;

        if (grid[ey][ex] == 2 && Game) {
          Game = false;
          updatescoreboard(playerscore, gameTime);
          if (!twoPlayerMode || !Game2) {
            currentState = STATE_GAMEOVER;
            if (gameOverSoundLoaded) gameOverSound.play();
          }
        }
        if (grid[ey][ex] == 3 && twoPlayerMode && Game2) {
          Game2 = false;
          updatescoreboard(playerscore2, gameTime);
          if (!Game) {
            currentState = STATE_GAMEOVER;
            if (gameOverSoundLoaded) gameOverSound.play();
          }
        }
      }

      // Draw background (switch to freeze_bg image when power-up is active)
      window.clear();
      if (freezetimer > 0.0f) window.draw(freeze_bg);
      else window.draw(game_bg);

      // Draw top HUD bar background
      RectangleShape hudBar(Vector2f(N * ts, topOffset));
      hudBar.setFillColor(Color(12, 12, 28, 230));
      hudBar.setOutlineThickness(1.0f);
      hudBar.setOutlineColor(Color(60, 60, 100));
      hudBar.setPosition(0, 0);
      window.draw(hudBar);

      if (!twoPlayerMode) {
        // --- Single Player HUD ---
        Text scoretext("Score: " + to_string(playerscore), font, 20);
        scoretext.setFillColor(Color::White);
        scoretext.setStyle(Text::Bold);
        scoretext.setPosition(10, 12);
        window.draw(scoretext);

        Text count("Moves: " + std::to_string(moveCount), font, 20);
        count.setFillColor(Color::Yellow);
        count.setStyle(Text::Bold);
        count.setPosition(140, 12);
        window.draw(count);

        Text powertext("Powerups [F]: " + to_string(powerupinventory), font, 20);
        powertext.setFillColor(Color::Green);
        powertext.setStyle(Text::Bold);
        powertext.setPosition(270, 12);
        window.draw(powertext);

        if (freezetimer > 0.0f) {
          Text freezetext("FROZEN: " + to_string((int)freezetimer + 1) + "s", font, 20);
          freezetext.setFillColor(Color::Cyan);
          freezetext.setStyle(Text::Bold);
          freezetext.setPosition(460, 12);
          window.draw(freezetext);
        }

        int seconds = (int)gameTime;
        int mins = seconds / 60;
        int secs = seconds % 60;
        std::string timeStr = "Time: " + (mins < 10 ? std::string("0") : std::string("")) + std::to_string(mins) + ":" + (secs < 10 ? std::string("0") : std::string("")) + std::to_string(secs);
        Text timeDisplay(timeStr, font, 20);
        timeDisplay.setFillColor(Color(200, 200, 200));
        timeDisplay.setStyle(Text::Bold);
        timeDisplay.setPosition(N * ts - timeDisplay.getGlobalBounds().width - 10, 12);
        window.draw(timeDisplay);
      } else {
        // --- Two Player HUD ---
        // P1 Row (top)
        Text p1label("P1:", font, 16);
        p1label.setFillColor(Color(255, 100, 100));
        p1label.setStyle(Text::Bold);
        p1label.setPosition(10, 3);
        window.draw(p1label);

        Text p1score("Score:" + to_string(playerscore), font, 15);
        p1score.setFillColor(Color::White);
        p1score.setPosition(45, 3);
        window.draw(p1score);

        Text p1moves("Moves:" + to_string(moveCount), font, 15);
        p1moves.setFillColor(Color::Yellow);
        p1moves.setPosition(165, 3);
        window.draw(p1moves);

        Text p1pwr("Pwr[F]:" + to_string(powerupinventory), font, 15);
        p1pwr.setFillColor(Color::Green);
        p1pwr.setPosition(280, 3);
        window.draw(p1pwr);

        if (!Game) {
          Text p1dead("DEAD", font, 15);
          p1dead.setFillColor(Color::Red);
          p1dead.setStyle(Text::Bold);
          p1dead.setPosition(385, 3);
          window.draw(p1dead);
        }

        // P2 Row (bottom)
        Text p2label("P2:", font, 16);
        p2label.setFillColor(Color(100, 150, 255));
        p2label.setStyle(Text::Bold);
        p2label.setPosition(10, 26);
        window.draw(p2label);

        Text p2score("Score:" + to_string(playerscore2), font, 15);
        p2score.setFillColor(Color::White);
        p2score.setPosition(45, 26);
        window.draw(p2score);

        Text p2moves("Moves:" + to_string(moveCount2), font, 15);
        p2moves.setFillColor(Color::Yellow);
        p2moves.setPosition(165, 26);
        window.draw(p2moves);

        Text p2pwr("Pwr[Q]:" + to_string(powerupinventory2), font, 15);
        p2pwr.setFillColor(Color::Green);
        p2pwr.setPosition(280, 26);
        window.draw(p2pwr);

        if (!Game2) {
          Text p2dead("DEAD", font, 15);
          p2dead.setFillColor(Color::Red);
          p2dead.setStyle(Text::Bold);
          p2dead.setPosition(385, 26);
          window.draw(p2dead);
        }

        // Shared: Freeze + Time (right side)
        if (freezetimer > 0.0f) {
          Text freezetext("FROZEN:" + to_string((int)freezetimer + 1) + "s", font, 16);
          freezetext.setFillColor(Color::Cyan);
          freezetext.setStyle(Text::Bold);
          freezetext.setPosition(460, 3);
          window.draw(freezetext);
        }

        int seconds = (int)gameTime;
        int mins = seconds / 60;
        int secs = seconds % 60;
        std::string timeStr = (mins < 10 ? std::string("0") : std::string("")) + std::to_string(mins) + ":" + (secs < 10 ? std::string("0") : std::string("")) + std::to_string(secs);
        Text timeDisplay(timeStr, font, 16);
        timeDisplay.setFillColor(Color(200, 200, 200));
        timeDisplay.setStyle(Text::Bold);
        timeDisplay.setPosition(N * ts - timeDisplay.getGlobalBounds().width - 10, 26);
        window.draw(timeDisplay);
      }

      for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
          if (grid[i][j] == 0)
            continue;

          if (grid[i][j] == 1)
            sTile.setTextureRect(IntRect(0, 0, ts, ts));

          if (grid[i][j] == 2)
            sTile.setTextureRect(IntRect(54, 0, ts, ts));

          if (grid[i][j] == 3)
            sTile.setTextureRect(IntRect(18, 0, ts, ts));

          sTile.setPosition(j * ts, i * ts + topOffset);
          window.draw(sTile);
        }
      }

      // Draw P1 sprite
      if (Game) {
        sTile.setTextureRect(IntRect(36, 0, ts, ts));
        sTile.setPosition(x * ts, y * ts + topOffset);
        window.draw(sTile);
      }

      // Draw P2 sprite
      if (twoPlayerMode && Game2) {
        sTile.setTextureRect(IntRect(18, 0, ts, ts));
        sTile.setPosition(x2 * ts, y2 * ts + topOffset);
        window.draw(sTile);
      }

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
