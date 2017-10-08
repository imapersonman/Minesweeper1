#include <functional>
typedef struct
{
    int x, y;
} Vector2i;

typedef enum
{
    // There should only be GameState_Launcher and GameState_Game
    // GameState_Lost and GameState_Win (should be GameState_Won) are substates of GameState_Game
    GameState_Launcher,
    GameState_Game,
    GameState_Lost,
    GameState_Win
} GameState;

typedef struct
{
    int nRows;
    int nCols;
    int nMines;
} Difficulty;

typedef enum {
    MouseMode_ClearMode,
    MouseMode_FlagMode
} MouseMode;

typedef enum
{
    ButtonState_None,
    ButtonState_Hover,
    ButtonState_Pressed
} ButtonState;

typedef struct
{
    const char *text;
    Vector2i size;
    Vector2i position;
    ButtonState state;
    // Could also use a function override but whatevs
    std::function<void()> pressedCallback;
} Button;

typedef enum
{
    CellState_Closed,
    CellState_Open
} CellState;

typedef struct
{
    CellState state = CellState_Closed;
    Vector2i position;
    // This is sooooooooo confusing.
    bool hasMine = false;
    bool hasFlag = false;
    bool hadFlag = false;
    bool assignedAdjMines = false;
    int adjacentMines = 0;
} Cell;

typedef enum
{
    MouseButton_Left,
    MouseButton_Right,
    MouseButton_Center
} MouseButton;

static const char *LAUNCHER_TITLE = "Minesweeper Launcher";
static const int LAUNCHER_POSX = SDL_WINDOWPOS_UNDEFINED;
static const int LAUNCHER_POSY = SDL_WINDOWPOS_UNDEFINED;
static const int LAUNCHER_WIDTH = 300;
static const int LAUNCHER_HEIGHT = 140;
static const Uint32 LAUNCHER_FLAGS = 0;
static const Uint32 LAUNCHER_RENDERER_FLAGS = SDL_RENDERER_ACCELERATED |
                                              SDL_RENDERER_PRESENTVSYNC;
static const int LAUNCHER_BUTTON_WIDTH = 100;
static const int LAUNCHER_BUTTON_HEIGHT = 30;

static const char *GAME_TITLE = "Minesweeper";
static const int GAME_POSX = SDL_WINDOWPOS_UNDEFINED;
static const int GAME_POSY = SDL_WINDOWPOS_UNDEFINED;
static const Uint32 GAME_FLAGS = 0;
static const Uint32 GAME_RENDERER_FLAGS = SDL_RENDERER_ACCELERATED |
                                          SDL_RENDERER_PRESENTVSYNC;
static const int GAME_HEADER_OFFSET = 32;

static const double MS_PER_UPDATE = 1000.0 / 60.0;
static const int CELL_WIDTH = 16;
static const int CELL_HEIGHT = 16;

static const int ADJ_MINE_BOMB = -1;
static const int ADJ_MINE_1 = 1;
static const int ADJ_MINE_2 = 2;
static const int ADJ_MINE_3 = 3;
static const int ADJ_MINE_4 = 4;
static const int ADJ_MINE_5 = 5;
static const int ADJ_MINE_6 = 6;
static const int ADJ_MINE_7 = 7;
static const int ADJ_MINE_8 = 8;

// Global
static void init();
static void quit();
static void update();
static void render();

// Launcher/Game should implement some "State" interface that has yet to be made

// Launcher
static void initLauncher();
static void quitLauncher();
static void updateLauncher();
static void renderLauncher();

// Game
static void initGame();
static void quitGame();
static void updateGame();
static void renderGame();
static void setDifficulty(Difficulty difficulty);
static Cell &getCellAtPosition(Vector2i position);
static void putMinesInNRandomCells(int nCells);
static void loseGame();
static void revealMines();
static void assignAdjacentMineCounts(Cell &rootCell);
static void assignCellsAdjacentMineCounts();
static Cell &getCellAtBlockPosition(Vector2i position);
static void winGame();
// I don't like that this is in game.  Maybe pass in a mouse?  Use mouseWithinBounds?
static bool mouseIsTouchingCell();
static void uncoverPartOfBoard(Cell &rootCell);

// Button
static void updateButton(Button &button);
static void renderButton(Button button);

// Font
static TTF_Font *loadFont(const char *path, int ptsize);
static void renderText(const char *text, Vector2i position, SDL_Color color);

// Cell
static void renderCell(Cell cell);

// Mouse
// I might change this signature depending on where it's being called
static bool mouseOverButton(Button button);
static bool mouseButtonDown(MouseButton mouseButton);

// Utility
static int get1dIndexFor2dIndex(Vector2i index2d, Vector2i arraySize);
static int random(int min, int max);
// This only kind of goes in Utility (Game?)
static SDL_Color getColorForAdjacentMineCount(int adjMineCount);
