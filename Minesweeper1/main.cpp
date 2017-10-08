//
//  main.cpp
//  Minesweeper1
//
//  Created by Koissi Adjorlolo on 2/23/16.
//  Copyright © 2016 centuryapps. All rights reserved.
//

#include <iostream>
#include <random>
#include <functional>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "main.h"

static GameState gState;
static bool gRunning = false;
static Difficulty gDifficulty;

// Launcher View
static SDL_Window *gLauncherWindow = nullptr;
static SDL_Renderer *gLauncherRenderer = nullptr;
static std::vector<Button> launcherButtons;

// Game View
static Vector2i gameWindowSize;
static SDL_Window *gGameWindow = nullptr;
static SDL_Renderer *gGameRenderer = nullptr;
static std::vector<Button> gameButtons;
// Maybe I should call this field something else.
// It definitely belongs in Game, though
static MouseMode gMouseMode;

// Global
static SDL_Renderer *gCurrentRenderer = nullptr;

// View
static std::vector<Cell> gameCells;

// Mouse fields
static Uint32 gMouseState;
static Vector2i gMousePosition;
static bool gLeftMouseDown = false;
static bool gRightMouseDown = false;
static bool gMiddleMouseDown = false;
// Mouse::init
// Mouse::getState() for internal use
// Mouse::getPosition()
// Mouse::leftButtonDown()
// Mouse::rightButtonDown()
// Mouse::middleButtonDown() probably not needed

// Keyboard
static bool fPressed = false;
static bool lastFPressed = false;

// Font
static TTF_Font *gDefaultFont;

// I don't know what this is for
// Should be a static method
static Cell defaultCell;

// Game state
static int uncoveredCells = 0;
static Uint32 gTime = 0;

int main(int argc, const char * argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "Unable to init SDL" << std::endl;
        exit(1);
    }
    
    if (TTF_Init() == -1)
    {
        std::cout << "Unable to init SDL_ttf" << std::endl;
        SDL_Quit();
        exit(1);
    }
    
    init();
    
    gRunning = true;
    SDL_Event event;
    
    double previous = (double)SDL_GetTicks();
    double lag = 0.0;
    
    while (gRunning)
    {
        // How should States deal with events?
        // Each State should have an events Map.
        // Each value in the events map should have an anonymous function:
        // Before each update, the events map is looped through, calling the event's anonymous
        // function if the event occurred.
        // That method might be overcomplicated for what this is. Not sure.
        // What if each state is passed a keyboard and mouse in their update methods?
        // That would simplify a lot of things.
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    gRunning = false;
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_RETURN:
                            // This is confusing since "Game" has substates
                            // Add win boolean.  If the game ended and win is true, win
                            //                   If the game ended and win is false, lose
                            if (gState == GameState_Lost ||
                                gState == GameState_Win ||
                                gState == GameState_Game)
                            {
                                quitGame();
                                initLauncher();
                            }
                            break;

                        case SDLK_f:
                            if (gState == GameState_Game) {
                                lastFPressed = fPressed;
                                fPressed = true;
                            }
                            else {
                            }
                            break;
                            
                        default:
                            break;
                    }
                    break;

                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case SDLK_f:
                            if (gState == GameState_Game) {
                                lastFPressed = fPressed;
                                fPressed = false;
                            }
                            break;
                    }
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    switch (event.button.button)
                    {
                        case SDL_BUTTON_LEFT:
                            gLeftMouseDown = true;
                            break;
                            
                        case SDL_BUTTON_RIGHT:
                            gRightMouseDown = true;
                            break;
                            
                        case SDL_BUTTON_MIDDLE:
                            gMiddleMouseDown = true;
                            break;
                            
                        default:
                            break;
                    }
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    switch (event.button.button)
                    {
                    case SDL_BUTTON_LEFT:
                        gLeftMouseDown = false;
                        break;
                        
                    case SDL_BUTTON_RIGHT:
                        gRightMouseDown = false;
                        break;
                        
                    case SDL_BUTTON_MIDDLE:
                        gMiddleMouseDown = false;
                        break;
                        
                    default:
                        break;
                    }
                    
                default:
                    break;
            }
        }
        
        double current = (double)SDL_GetTicks();
        double elapsed = current - previous;
        lag += elapsed;
        previous = current;
        
        while (lag >= MS_PER_UPDATE)
        {
            update();
            lag -= elapsed;
        }
        
        render();
    }
    
    quit();
    
    return 0;
}

static void init()
{
    gDefaultFont = loadFont("Resources/Fonts/Anonymice.ttf", 16);
    gMouseState = SDL_GetMouseState(&gMousePosition.x, &gMousePosition.y);
    gState = GameState_Launcher;
    initLauncher();
}

static void initLauncher()
{
    gLeftMouseDown = false;
    gRightMouseDown = false;
    gMiddleMouseDown = false;
    
    gLauncherWindow = SDL_CreateWindow(LAUNCHER_TITLE,
                                                  LAUNCHER_POSX,
                                                  LAUNCHER_POSY,
                                                  LAUNCHER_WIDTH,
                                                  LAUNCHER_HEIGHT,
                                                  LAUNCHER_FLAGS);
    
    if (gLauncherWindow == nullptr)
    {
        std::cout << "Unable to create launcher window" << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    gLauncherRenderer = SDL_CreateRenderer(gLauncherWindow,
                                           -1,
                                           LAUNCHER_RENDERER_FLAGS);
    
    if (gLauncherRenderer == nullptr)
    {
        std::cout << "Unable to create launcher renderer" << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    gCurrentRenderer = gLauncherRenderer;
    
    int nButtons = 3;
    
    launcherButtons.push_back({
        "Easy",  // text
        {
            LAUNCHER_BUTTON_WIDTH,
            LAUNCHER_BUTTON_HEIGHT
        },  // size
        {
            LAUNCHER_WIDTH / 2 - LAUNCHER_BUTTON_WIDTH / 2,
            LAUNCHER_HEIGHT / (nButtons + 1) * ((int)launcherButtons.size() + 1)
        },  // position
        ButtonState_None,  // state
        []() {
            setDifficulty({
                16,  // nRows
                16,  // nCols
                24,  // nMines
            });
        }  // pressedCallback
    });
    
    launcherButtons.push_back({
        "Medium",  // text
        {
            LAUNCHER_BUTTON_WIDTH,
            LAUNCHER_BUTTON_HEIGHT
        },  // size
        {
            LAUNCHER_WIDTH / 2 - LAUNCHER_BUTTON_WIDTH / 2,
            LAUNCHER_HEIGHT / (nButtons + 1) * ((int)launcherButtons.size() + 1)
        },  // position
        ButtonState_None,  // state
        []() {
            setDifficulty({
                32,  // nRows
                32,  // nCols
                100,  // nMines
            });
        }  // pressedCallback
    });
    
    launcherButtons.push_back({
        "Hard",  // text
        {
            LAUNCHER_BUTTON_WIDTH,
            LAUNCHER_BUTTON_HEIGHT
        },  // size
        {
            LAUNCHER_WIDTH / 2 - LAUNCHER_BUTTON_WIDTH / 2,
            LAUNCHER_HEIGHT / (nButtons + 1) * ((int)launcherButtons.size() + 1)
        },  // position
        ButtonState_None,  // state
        []() {
            setDifficulty({
                64,  // nRows
                64,  // nCols
                400,  // nMines
            });
        }  // pressedCallback
    });
    
    SDL_SetRenderDrawBlendMode(gCurrentRenderer, SDL_BLENDMODE_BLEND);
}

static void quit()
{
    switch (gState)
    {
        case GameState_Launcher:
            quitLauncher();
            break;
            
        default:
            quitGame();
            break;
    }
    
    TTF_CloseFont(gDefaultFont);
    
    SDL_Quit();
    TTF_Quit();
}

static void quitLauncher()
{
    gState = GameState_Game;
    
    SDL_DestroyRenderer(gLauncherRenderer);
    SDL_DestroyWindow(gLauncherWindow);
    
    gLauncherRenderer = nullptr;
    gLauncherWindow = nullptr;
    
    initGame();
}

static void quitGame()
{
    gState = GameState_Launcher;
    SDL_DestroyRenderer(gGameRenderer);
    SDL_DestroyWindow(gGameWindow);
    
    gGameRenderer = nullptr;
    gGameWindow = nullptr;
}

static void initGame()
{
    uncoveredCells = 0;
    gTime = 0;
    gLeftMouseDown = false;
    gRightMouseDown = false;
    gMiddleMouseDown = false;
    gameCells.clear();
    gMouseMode = MouseMode_ClearMode;
    
    int gameWidth = gDifficulty.nRows * CELL_WIDTH;
    int gameHeight = gDifficulty.nCols * CELL_HEIGHT + GAME_HEADER_OFFSET;
    
    gameWindowSize = {
        gameWidth,
        gameHeight
    };
    
    gGameWindow = SDL_CreateWindow(GAME_TITLE,
                                   GAME_POSX,
                                   GAME_POSY,
                                   gameWidth,
                                   gameHeight,
                                   GAME_FLAGS);
    
    if (gGameWindow == nullptr)
    {
        std::cout << "Unable to create game window" << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    gGameRenderer = SDL_CreateRenderer(gGameWindow,
                                       -1,
                                       GAME_RENDERER_FLAGS);
    
    if (gGameRenderer == nullptr)
    {
        std::cout << "Unable to create game renderer" << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    gCurrentRenderer = gGameRenderer;
    
    for (int y = 0; y < gDifficulty.nCols; y++)
    {
        for (int x = 0; x < gDifficulty.nRows; x++)
        {
            Cell cell;
            cell.position = {
                x * CELL_WIDTH,
                y * CELL_HEIGHT + GAME_HEADER_OFFSET
            };
            
            gameCells.push_back(cell);
        }
    }
    
    putMinesInNRandomCells(gDifficulty.nMines);
    assignCellsAdjacentMineCounts();
    
    SDL_SetRenderDrawBlendMode(gCurrentRenderer, SDL_BLENDMODE_BLEND);
}

static void update()
{
    gMouseState = SDL_GetMouseState(&gMousePosition.x, &gMousePosition.y);
    
    switch (gState)
    {
        case GameState_Launcher:
            updateLauncher();
            break;
            
        case GameState_Game:
            updateGame();
            break;
            
        case GameState_Lost:
            break;
            
        default:
            break;
    }
}

static void updateLauncher()
{
    for (int buttonIndex = 0;
         buttonIndex < launcherButtons.size();
         buttonIndex++)
    {
        updateButton(launcherButtons[buttonIndex]);
    }
}

static void updateGame()
{
    for (int buttonIndex = 0;
         buttonIndex < gameButtons.size();
         buttonIndex++)
    {
        updateButton(gameButtons[buttonIndex]);
    }

    if (fPressed && !lastFPressed) {
        if (gMouseMode == MouseMode_ClearMode) {
            gMouseMode = MouseMode_FlagMode;
        }
        else if (gMouseMode == MouseMode_FlagMode) {
            gMouseMode = MouseMode_ClearMode;
        }
        lastFPressed = fPressed;
    }

    if (mouseIsTouchingCell())
    {
        Cell &cell = getCellAtPosition(gMousePosition);
        
        switch (cell.state)
        {
            case CellState_Closed:
            {
                if (mouseButtonDown(MouseButton_Left))
                {
                    if (gMouseMode == MouseMode_ClearMode) {
                        if (cell.hasMine && !cell.hasFlag)
                        {
                            loseGame();
                        }
                        else if (!cell.hasFlag)
                        {
                            uncoverPartOfBoard(cell);
                        }
                        
                        int nCells = gDifficulty.nCols * gDifficulty.nRows;
                        
                        if (nCells - uncoveredCells == gDifficulty.nMines)
                        {
                            winGame();
                        }
                    }
                    else if (gMouseMode == MouseMode_FlagMode) {
                        if (cell.hadFlag) {
                            cell.hasFlag = false;
                        }
                        else {
                            cell.hasFlag = true;
                        }
                    }
                }
                else {
                    if (cell.hasFlag) {
                        cell.hadFlag = true;
                    }
                    else {
                        cell.hadFlag = false;
                    }
                }
                
                break;
            }
                
            case CellState_Open:
                break;
                
            default:
                break;
        }
    }
    
    gTime += MS_PER_UPDATE;
}

static void updateButton(Button &button)
{
    switch (button.state)
    {
        case ButtonState_None:
            if (mouseOverButton(button))
            {
                button.state = ButtonState_Hover;
            }
            break;
            
        case ButtonState_Hover:
            if (!mouseOverButton(button))
            {
                button.state = ButtonState_None;
            }
            
            if (mouseButtonDown(MouseButton_Left))
            {
                button.pressedCallback();
                button.state = ButtonState_Pressed;
            }
            break;
            
        case ButtonState_Pressed:
            if (!mouseOverButton(button))
            {
                button.state = ButtonState_None;
            }
            
            if (!mouseButtonDown(MouseButton_Left))
            {
                button.state = ButtonState_Hover;
            }
            break;
            
        default:
            break;
    }
}

static void render()
{
    SDL_SetRenderDrawColor(gCurrentRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gCurrentRenderer);
    
    switch (gState)
    {
        case GameState_Launcher:
            renderLauncher();
            break;
            
        case GameState_Game: {
            renderText("Press Enter to Restart", {
                gameWindowSize.x / 2,
                8
            }, { 255, 255, 255 });
            std::string mouseMode;
            if (gMouseMode == MouseMode_FlagMode) {
                mouseMode = "Flag";
            }
            else {
                mouseMode = "Clear";
            }
            // Ahh this is gross
            // TODO: Change 'MouseMode_*' and 'MouseMode" to 
            //       'ClickMode_*' and 'ClickMode'.
            renderText(std::string("Click Mode: ").append(mouseMode).c_str(), {
                gameWindowSize.x / 2,
                22
            }, { 255, 255, 255 });
            renderGame();
        } break;
            
        case GameState_Lost:
            renderGame();
            renderText("Press Enter to Restart", {
                gameWindowSize.x / 2,
                8
            }, { 255, 255, 255 });
            renderText("You Lost.", {
                gameWindowSize.x / 2,
                22
            }, { 255, 255, 255 });
            break;
            
        case GameState_Win:
        {
            renderGame();
            std::string winString = "You Won in " + std::to_string(gTime / 1000) + " seconds.";
            
            renderText("Press Enter to Restart", {
                gameWindowSize.x / 2,
                8
            }, { 255, 255, 255 });
            renderText(winString.c_str(), {
                gameWindowSize.x / 2,
                22
            }, { 255, 255, 255 });
        }
            break;
            
        default:
            break;
    }
    
    SDL_RenderPresent(gCurrentRenderer);
}

static void renderLauncher()
{
    for (int buttonIndex = 0;
         buttonIndex < launcherButtons.size();
         buttonIndex++)
    {
        renderButton(launcherButtons[buttonIndex]);
    }
}

static void renderGame()
{
    for (int buttonIndex = 0;
         buttonIndex < gameButtons.size();
         buttonIndex++)
    {
        renderButton(gameButtons[buttonIndex]);
    }
    
    for (int cellIndex = 0;
         cellIndex < gameCells.size();
         cellIndex++)
    {
        renderCell(gameCells[cellIndex]);
    }
    
    if (mouseIsTouchingCell())
    {
        SDL_Rect mouseRect = {
            (gMousePosition.x / CELL_WIDTH) * CELL_WIDTH,
            (gMousePosition.y / CELL_HEIGHT) * CELL_HEIGHT,
            CELL_WIDTH,
            CELL_HEIGHT
        };
        // Mouse Rect Color.  Laziness.
        SDL_Color mrc = { 255, 255, 0 };
        if (gMouseMode == MouseMode_ClearMode) {
            mrc = { 255, 0, 0 };
        }
        SDL_SetRenderDrawColor(gCurrentRenderer, mrc.r, mrc.g, mrc.b, 63);
        SDL_RenderFillRect(gCurrentRenderer, &mouseRect);
    }
}

static void renderButton(Button button)
{
    SDL_Rect rect = {
        button.position.x,
        button.position.y,
        button.size.x,
        button.size.y
    };
    
    switch (button.state)
    {
        case ButtonState_None:
            SDL_SetRenderDrawColor(gCurrentRenderer, 200, 200, 200, 255);
            break;
            
        case ButtonState_Hover:
            SDL_SetRenderDrawColor(gCurrentRenderer, 100, 100, 100, 255);
            break;
            
        case ButtonState_Pressed:
            SDL_SetRenderDrawColor(gCurrentRenderer, 50, 50, 50, 255);
            break;
            
        default:
            SDL_SetRenderDrawColor(gCurrentRenderer, 255, 0, 0, 255);
            break;
    }
    
    SDL_RenderFillRect(gCurrentRenderer, &rect);
    renderText(button.text, {
        button.position.x + button.size.x / 2,
        button.position.y + button.size.y / 2
    }, { 0, 0, 0 });
}

static void renderText(const char *text, Vector2i position, SDL_Color color)
{
    // Super inefficient, but it will do for now.
    SDL_Surface *fontSurface = TTF_RenderText_Blended(gDefaultFont, text, color);
    
    if (fontSurface == nullptr)
    {
        std::cout << "Unable to render font" << std::endl;
        std::cout << TTF_GetError() << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    SDL_Texture *fontTexture = SDL_CreateTextureFromSurface(gCurrentRenderer,
                                                            fontSurface);
    
    if (fontTexture == nullptr)
    {
        std::cout << "Unable to render font" << std::endl;
        std::cout << TTF_GetError() << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    SDL_Rect srcRect = {
        0,
        0,
        0,
        0
    };
    
    SDL_QueryTexture(fontTexture, nullptr, nullptr, &srcRect.w, &srcRect.h);
    
    SDL_Rect destRect = {
        position.x - srcRect.w / 2,
        position.y - srcRect.h / 2,
        srcRect.w,
        srcRect.h
    };
    
    SDL_RenderCopy(gCurrentRenderer, fontTexture, &srcRect, &destRect);
    
    SDL_FreeSurface(fontSurface);
    SDL_DestroyTexture(fontTexture);
    
    fontSurface = nullptr;
    fontTexture = nullptr;
}

static void renderCell(Cell cell)
{
    std::string adjMinesString;
    
    SDL_Rect rect = {
        cell.position.x,
        cell.position.y,
        CELL_WIDTH,
        CELL_HEIGHT
    };
    
    switch (cell.state)
    {
        case CellState_Closed:
            SDL_SetRenderDrawColor(gCurrentRenderer, 100, 100, 100, 255);
            SDL_RenderFillRect(gCurrentRenderer, &rect);
            
            if (cell.hasFlag)
            {
                int flagWidth = CELL_WIDTH / 2;
                int flagHeight = CELL_HEIGHT / 2;
                
                SDL_Rect flagRect = {
                    cell.position.x + CELL_WIDTH / 2 - flagWidth / 2,
                    cell.position.y + CELL_HEIGHT / 2 - flagHeight / 2,
                    flagWidth,
                    flagHeight
                };
                
                SDL_SetRenderDrawColor(gCurrentRenderer, 255, 255, 0, 255);
                SDL_RenderFillRect(gCurrentRenderer, &flagRect);
            }
            break;
            
        case CellState_Open:
            SDL_SetRenderDrawColor(gCurrentRenderer, 200, 200, 200, 255);
            if (!cell.hasMine)
            {
                adjMinesString = std::to_string(cell.adjacentMines);
                
                SDL_RenderFillRect(gCurrentRenderer, &rect);
                
                if (cell.adjacentMines > 0)
                {
                    renderText(adjMinesString.c_str(), {
                        cell.position.x + CELL_WIDTH / 2,
                        cell.position.y + CELL_HEIGHT / 2
                    }, getColorForAdjacentMineCount(cell.adjacentMines));
                }
            }
            else
            {
                adjMinesString = "B";
                SDL_RenderFillRect(gCurrentRenderer, &rect);
                renderText(adjMinesString.c_str(), {
                    cell.position.x + CELL_WIDTH / 2,
                    cell.position.y + CELL_HEIGHT / 2
                }, getColorForAdjacentMineCount(ADJ_MINE_BOMB));
            }
            break;
            
        default:
            break;
    }
}

static bool mouseOverButton(Button button)
{
    int left = button.position.x;
    int right = button.position.x + button.size.x;
    int top = button.position.y;
    int bottom = button.position.y + button.size.y;
    
    return (gMousePosition.x > left && gMousePosition.x < right &&
            gMousePosition.y > top && gMousePosition.y < bottom);
}

static bool mouseButtonUp(MouseButton mouseButton)
{
    switch (mouseButton)
    {
        case MouseButton_Left:
            return !gLeftMouseDown;
            break;
            
        case MouseButton_Right:
            return !gRightMouseDown;
            break;
            
        case MouseButton_Center:
            return !gMiddleMouseDown;
            break;
            
        default:
            return false;
            break;
    }
}

static bool mouseButtonDown(MouseButton mouseButton)
{
    switch (mouseButton)
    {
        case MouseButton_Left:
            return gLeftMouseDown;
            break;
            
        case MouseButton_Right:
            return gRightMouseDown;
            break;
            
        case MouseButton_Center:
            return gMiddleMouseDown;
            break;
            
        default:
            return false;
            break;
    }
}

static TTF_Font *loadFont(const char *path, int ptsize)
{
    SDL_RWops *fontRWops = SDL_RWFromFile(path, "rb");
    
    if (fontRWops == nullptr)
    {
        std::cout << "Unable to open file at " << path << std::endl;
        std::cout << SDL_GetError() << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    TTF_Font *font = TTF_OpenFontRW(fontRWops, 1, ptsize);
    
    if (font == nullptr)
    {
        std::cout << "Unable to open file at " << path << std::endl;
        std::cout << TTF_GetError() << std::endl;
        SDL_Quit();
        TTF_Quit();
        exit(1);
    }
    
    return font;
}

static void setDifficulty(Difficulty difficulty)
{
    gDifficulty = difficulty;
    
    if (gState == GameState_Launcher)
    {
        // Quit launcher, restart current game.
        quitLauncher();
    }
    else
    {
        // Restart current game.
        initGame();
    }
}

static Cell &getCellAtPosition(Vector2i position)
{
    Vector2i cellIndex2d = {
        position.x / CELL_WIDTH,
        (position.y - GAME_HEADER_OFFSET) / CELL_HEIGHT
    };
    
    return getCellAtBlockPosition(cellIndex2d);
}

static int get1dIndexFor2dIndex(Vector2i index2d, Vector2i arraySize)
{
    return index2d.y * arraySize.x + index2d.x;
}

static bool mouseIsTouchingCell()
{
    SDL_Rect mouseRect = {
        gMousePosition.x,
        gMousePosition.y,
        1,
        1
    };
    
    SDL_Rect cellFieldRect = {
        0,
        GAME_HEADER_OFFSET,
        gDifficulty.nCols * CELL_WIDTH,
        gDifficulty.nRows * CELL_HEIGHT
    };
    
    return SDL_HasIntersection(&mouseRect, &cellFieldRect);
}

static void putMinesInNRandomCells(int nCells)
{
    if (nCells > gameCells.size())
    {
        nCells = (int)gameCells.size();
    }
    
    for (int cellIndex = 0;
         cellIndex < nCells;
         cellIndex++)
    {
        int randomIndex = random(0, (int)gameCells.size());
        
        while (gameCells[randomIndex].hasMine)
        {
            randomIndex = random(0, (int)gameCells.size());
        }
        
        gameCells[randomIndex].hasMine = true;
        gameCells[randomIndex].adjacentMines = -1;
    }
}

static int random(int min, int max)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max - 1);
    
    return dis(gen);
}

static SDL_Color getColorForAdjacentMineCount(int adjMineCount)
{
    switch (adjMineCount)
    {
        case ADJ_MINE_BOMB:
            return { 255, 0, 0 };
            break;
            
        case ADJ_MINE_1:
            return { 0, 0, 200 };
            break;
            
        case ADJ_MINE_2:
            return { 0, 200, 0 };
            break;
            
        case ADJ_MINE_3:
            return { 255, 0, 0 };
            break;
            
        case ADJ_MINE_4:
            return { 0, 0, 100 };
            break;
            
        case ADJ_MINE_5:
            return { 100, 0, 0 };
            break;
            
        case ADJ_MINE_6:
            return { 0, 255, 255 };
            break;
            
        case ADJ_MINE_7:
            return { 0, 0, 0 };
            break;
            
        case ADJ_MINE_8:
            return { 100, 100, 100 };
            break;
            
        default:
            break;
    }
    
    return { 0, 0, 0 };
}

static void loseGame()
{
    std::cout << "You lost" << std::endl;
    revealMines();
    
    gState = GameState_Lost;
}

static void revealMines()
{
    for (int cellIndex = 0;
         cellIndex < gameCells.size();
         cellIndex++)
    {
        if (gameCells[cellIndex].hasMine)
        {
            gameCells[cellIndex].state = CellState_Open;
        }
    }
}

static void assignAdjacentMineCounts(Cell &rootCell)
{
    if (rootCell.hasMine)
    {
        return;
    }
    
    Vector2i rcPos = {
        rootCell.position.x / CELL_WIDTH,
        (rootCell.position.y - GAME_HEADER_OFFSET) / CELL_HEIGHT
    };
    
    bool tL = getCellAtBlockPosition({ rcPos.x - 1, rcPos.y - 1 }).hasMine;
    bool tC = getCellAtBlockPosition({ rcPos.x, rcPos.y - 1 }).hasMine;
    bool tR = getCellAtBlockPosition({ rcPos.x + 1, rcPos.y - 1 }).hasMine;
    bool cL = getCellAtBlockPosition({ rcPos.x - 1, rcPos.y }).hasMine;
    bool cR = getCellAtBlockPosition({ rcPos.x + 1, rcPos.y }).hasMine;
    bool bL = getCellAtBlockPosition({ rcPos.x - 1, rcPos.y + 1 }).hasMine;
    bool bC = getCellAtBlockPosition({ rcPos.x, rcPos.y + 1 }).hasMine;
    bool bR = getCellAtBlockPosition({ rcPos.x + 1, rcPos.y + 1 }).hasMine;
    
    if (tL) rootCell.adjacentMines++;
    if (tC) rootCell.adjacentMines++;
    if (tR) rootCell.adjacentMines++;
    if (cL) rootCell.adjacentMines++;
    if (cR) rootCell.adjacentMines++;
    if (bL) rootCell.adjacentMines++;
    if (bC) rootCell.adjacentMines++;
    if (bR) rootCell.adjacentMines++;
}

static void uncoverPartOfBoard(Cell &rootCell)
{
    uncoveredCells++;
    rootCell.state = CellState_Open;
    
    if (rootCell.adjacentMines != 0)
    {
        return;
    }
    
    Vector2i rcPos = {
        rootCell.position.x / CELL_WIDTH,
        (rootCell.position.y - GAME_HEADER_OFFSET) / CELL_HEIGHT
    };
    
    Cell &tL = getCellAtBlockPosition({ rcPos.x - 1, rcPos.y - 1 });
    Cell &tC = getCellAtBlockPosition({ rcPos.x, rcPos.y - 1 });
    Cell &tR = getCellAtBlockPosition({ rcPos.x + 1, rcPos.y - 1 });
    Cell &cL = getCellAtBlockPosition({ rcPos.x - 1, rcPos.y });
    Cell &cR = getCellAtBlockPosition({ rcPos.x + 1, rcPos.y });
    Cell &bL = getCellAtBlockPosition({ rcPos.x - 1, rcPos.y + 1 });
    Cell &bC = getCellAtBlockPosition({ rcPos.x, rcPos.y + 1 });
    Cell &bR = getCellAtBlockPosition({ rcPos.x + 1, rcPos.y + 1 });
    
    if (tL.adjacentMines >= 0 && tL.state != CellState_Open) uncoverPartOfBoard(tL);
    if (tC.adjacentMines >= 0 && tC.state != CellState_Open) uncoverPartOfBoard(tC);
    if (tR.adjacentMines >= 0 && tR.state != CellState_Open) uncoverPartOfBoard(tR);
    if (cL.adjacentMines >= 0 && cL.state != CellState_Open) uncoverPartOfBoard(cL);
    if (cR.adjacentMines >= 0 && cR.state != CellState_Open) uncoverPartOfBoard(cR);
    if (bL.adjacentMines >= 0 && bL.state != CellState_Open) uncoverPartOfBoard(bL);
    if (bC.adjacentMines >= 0 && bC.state != CellState_Open) uncoverPartOfBoard(bC);
    if (bR.adjacentMines >= 0 && bR.state != CellState_Open) uncoverPartOfBoard(bR);
}

static void assignCellsAdjacentMineCounts()
{
    for (int cellIndex = 0;
         cellIndex < gameCells.size();
         cellIndex++)
    {
        Cell &currentCell = gameCells[cellIndex];
        if (currentCell.assignedAdjMines) continue;
        
        assignAdjacentMineCounts(currentCell);
    }
}

static Cell &getCellAtBlockPosition(Vector2i position)
{
    if (position.x < 0 || position.x >= gDifficulty.nCols ||
        position.y < 0 || position.y >= gDifficulty.nRows)
    {
        return defaultCell;
    }
    
    int cellIndex = get1dIndexFor2dIndex(position, {
        gDifficulty.nCols,
        gDifficulty.nRows
    });
    
    return gameCells[cellIndex];
}

static void winGame()
{
    gState = GameState_Win;
    revealMines();
}
