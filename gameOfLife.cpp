#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <unistd.h>

typedef struct ScreenState {
  int screenX;
  int screenY;
  int screenPositionX = 0;
  int screenPositionY = 0;
} ScreenState;

typedef struct FieldInfo {
  bool **field;
  int sizeX = 250;
  int sizeY = 50;
  bool activeness = true;
} FieldInfo;

int calculateAliveNeighbours(FieldInfo field, int y, int x) {
  int aliveNeighbours = 0;
  for (int neighbourX = -1; neighbourX <= 1; neighbourX++) {
    for (int neighbourY = -1; neighbourY <= 1; neighbourY++) {
      if (neighbourX == 0 && neighbourY == 0) {
        continue;
      }
      if (y + neighbourY >= 0 && x + neighbourX >= 0 &&
          y + neighbourY <= field.sizeY-- && x + neighbourX <= field.sizeX) {
        aliveNeighbours += field.field[y + neighbourY][x + neighbourX];
      }
    }
  }
  return aliveNeighbours;
}

void randField(FieldInfo *field) {
  for (int y = 0; y < field->sizeY; y++) {
    for (int x = 0; x < field->sizeX; x++) {
      field->field[y][x] = rand() % 2;
    }
  }
}

void calculateCellsState(FieldInfo *field) {
  bool nextField[field->sizeY][field->sizeX];
  for (int y = 0; y < field->sizeY; y++) {
    for (int x = 0; x < field->sizeX; x++) {
      int aliveNeighbours = calculateAliveNeighbours(*field, y, x);
      if (field->field[y][x]) {
        if (aliveNeighbours < 2 || aliveNeighbours > 3) {
          nextField[y][x] = false;
          continue;
        }
        nextField[y][x] = true;
        continue;
      }
      if (aliveNeighbours == 3) {
        nextField[y][x] = true;
        continue;
      }
      nextField[y][x] = false;
    }
  }
  for (int y = 0; y < field->sizeY; y++) {
    for (int x = 0; x < field->sizeX; x++) {
      field->field[y][x] = nextField[y][x];
    }
  }
}

void drawField(FieldInfo *field, ScreenState *screen) {
  for (int y = 0; y < screen->screenY; y++) {
    for (int x = 0; x < screen->screenX; x++) {
      attron(COLOR_PAIR(field->field[screen->screenPositionY + y]
                                    [screen->screenPositionX + x / 3]));
      mvaddch(y, x, ' ');
      mvaddch(y, x + 1, ' ');
      mvaddch(y, x + 2, ' ');
      attroff(COLOR_PAIR(field->field[screen->screenPositionY + y]
                                     [screen->screenPositionX + x / 3]));
    }
  }
}

void saveField(const char *filename, FieldInfo field) {
  std::ofstream outFile(filename, std::ios::binary);
  if (!outFile) {
    return;
  }

  outFile.write(reinterpret_cast<const char *>(&field.sizeY),
                sizeof(field.sizeY));
  outFile.write(reinterpret_cast<const char *>(&field.sizeX),
                sizeof(field.sizeX));

  for (int i = 0; i < field.sizeY; i++) {
    outFile.write(reinterpret_cast<const char *>(field.field[i]),
                  field.sizeX * sizeof(bool));
  }

  outFile.close();
}

void loadField(const char *filename, FieldInfo *field) {
  std::ifstream inFile(filename, std::ios::binary);
  if (!inFile) {
    return;
  }

  inFile.read(reinterpret_cast<char *>(field->sizeY), sizeof(field->sizeY));
  inFile.read(reinterpret_cast<char *>(field->sizeX), sizeof(field->sizeX));

  if (field->field != nullptr) {
    for (int i = 0; i < field->sizeY; i++) {
      delete[] (field->field)[i];
    }
    delete[] field->field;
  }

  field->field = new bool *[field->sizeY];
  for (int i = 0; i < field->sizeY; i++) {
    (field->field)[i] = new bool[field->sizeX];
  }

  for (int i = 0; i < field->sizeY; i++) {
    inFile.read(reinterpret_cast<char *>((field->field)[i]),
                field->sizeX * sizeof(bool));
  }

  inFile.close();
}

void mouseInputs(FieldInfo *field, ScreenState screen, MEVENT mouseEvent) {
  if (mouseEvent.bstate & BUTTON1_PRESSED) {
    field->field[mouseEvent.y + screen.screenPositionY]
                [mouseEvent.x / 3 + screen.screenPositionX] =
        !field->field[mouseEvent.y + screen.screenPositionY]
                     [mouseEvent.x / 3 + screen.screenPositionX];
  }
}

void keyInputs(FieldInfo *field, ScreenState *screen, bool *running,
               MEVENT *mouseEvent) {
  int key = getch();
  int pauseKey;
  if (key != ERR) {
    switch (key) {
    case 'w':
      if (screen->screenPositionY > 0) {
        screen->screenPositionY--;
      }
      break;
    case 's':
      if (screen->screenPositionY + screen->screenY < field->sizeY) {
        screen->screenPositionY++;
      }
      break;
    case 'a':
      if (screen->screenPositionX > 0) {
        screen->screenPositionX--;
      }
      break;
    case 'd':
      if (screen->screenPositionX * 3 + screen->screenX < field->sizeX) {
        screen->screenPositionX++;
      }
      break;
    case 'S':
      saveField("info", *field);
      break;
    case 'L':
      loadField("info", field);
      break;
    case 'q':
      *running = false;
      break;
    case 'p':
      field->activeness = !field->activeness;
      break;
    case KEY_MOUSE:
      getmouse(mouseEvent);
      mouseInputs(field, *screen, *mouseEvent);
      break;
    case 'R':
      randField(field);
      break;
    case 'g':
      init_pair(1, COLOR_GREEN, COLOR_GREEN);
      break;
    case 'b':
      init_pair(1, COLOR_BLUE, COLOR_BLUE);
      break;
    case 'r':
      init_pair(1, COLOR_RED, COLOR_RED);
      break;
    case 'W':
      init_pair(1, COLOR_WHITE, COLOR_WHITE);
      break;
    }
  }
}
int main(int argc, char **argv) {

  if (argc > 1 && !std::string(argv[1]).compare("-h")) {
    printf("use: 'gameOfLife <fieldSizeX> <fieldSizeY> <frameRate>'\nNote that "
           "your values supposed to be bigger than terminal size\nUSAGE: \n  - "
           "w,a,s,d to move through the field\n  - S to save the current state "
           "of field\n  - L to load the state from file\n  - R to set field to "
           "the random state\n  - p to pause the field\n  - q to leave the "
           "simulation\n  - r,b,g,w - change color to red, blue, green or "
           "white\n");
    return 1;
  }

  FieldInfo field;
  ScreenState screen;
  int targetFPS = 20;

  if (argc > 3) {
    field.sizeX = std::stoi(argv[1]);
    field.sizeY = std::stoi(argv[2]);
    targetFPS = std::stoi(argv[3]);
  }

  initscr();
  noecho();
  curs_set(0);
  nodelay(stdscr, true);
  keypad(stdscr, true);
  mousemask(BUTTON1_PRESSED | BUTTON3_PRESSED, NULL);
  MEVENT mouseEvent;
  getmaxyx(stdscr, screen.screenY, screen.screenX);
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_WHITE);
  init_pair(0, COLOR_BLACK, COLOR_BLACK);

  field.field = new bool *[field.sizeY];
  for (int i = 0; i < field.sizeY; i++) {
    field.field[i] = new bool[field.sizeX];
    for (int j = 0; j < field.sizeX; j++) {
      field.field[i][j] = false;
    }
  }

  const int frameTimeTarget = 1000000 / targetFPS;

  bool running = true;

  while (running) {
    auto frameStart = std::chrono::steady_clock::now();

    if (field.activeness) {
      calculateCellsState(&field);
    }
    keyInputs(&field, &screen, &running, &mouseEvent);
    drawField(&field, &screen);
    refresh();

    auto frameEnd = std::chrono::steady_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(
                             frameEnd - frameStart)
                             .count();

    if (frameDuration < frameTimeTarget) {
      usleep(frameTimeTarget - frameDuration);
    }
  }
  for (int i = 0; i < field.sizeY; i++) {
    delete[] field.field[i];
  }
  delete[] field.field;
  getch();
  endwin();
  return 0;
}
