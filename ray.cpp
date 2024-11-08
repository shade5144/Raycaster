#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#define TILE_SIZE 32
#define ROWS 38
#define COLS 25

const int screen_width = TILE_SIZE * ROWS;
const int screen_height = TILE_SIZE * COLS;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void drawCircle(int centerX, int centerY, int radius, int fill = 0)
{
    int x = radius;
    int y = 0;
    int error = 1 - radius;

    while (x >= y)
    {
        if (fill)
        {
            SDL_RenderDrawLine(renderer, (centerX - x), (centerY + y), (centerX + x), (centerY + y));
            SDL_RenderDrawLine(renderer, (centerX - y), (centerY + x), (centerX + y), (centerY + x));
            SDL_RenderDrawLine(renderer, (centerX - x), (centerY - y), (centerX + x), (centerY - y));
            SDL_RenderDrawLine(renderer, (centerX - y), (centerY - x), (centerX + y), (centerY - x));
        }

        else
        {
            SDL_RenderDrawPoint(renderer, (centerX - x), (centerY + y));
            SDL_RenderDrawPoint(renderer, (centerX - y), (centerY + x));
            SDL_RenderDrawPoint(renderer, (centerX - x), (centerY - y));
            SDL_RenderDrawPoint(renderer, (centerX - y), (centerY - x));

            SDL_RenderDrawPoint(renderer, (centerX + x), (centerY - y));
            SDL_RenderDrawPoint(renderer, (centerX + y), (centerY - x));
            SDL_RenderDrawPoint(renderer, (centerX + x), (centerY + y));
            SDL_RenderDrawPoint(renderer, (centerX + y), (centerY + x));
        }

        y++;

        if (error < 0)
        {
            error += 2 * y + 1;
        }
        else
        {
            x--;
            error += 2 * (y - x + 1);
        }
    }
}

struct vector2
{
    double x;
    double y;
};

float getVecLen(vector2 vec1, vector2 vec2)
{
    return pow(vec1.x - vec2.x, 2) + pow(vec1.y - vec2.y, 2);
}

bool pointOutOfBounds(vector2 point)
{
    if (point.x > screen_width || point.x < 0)
    {
        return true;
    }

    if (point.y > screen_height || point.y < 0)
    {
        return true;
    }

    return false;
}

class Player
{
public:
    double rotation;
    vector2 player_posn;
    vector2 ray_end;
    int ray_len;
    float player_speed;

    // Check the modifiers man, there seem to be too many
    int inc{2};
    int err_term_x{1};
    int err_term_y{1};
    int tile_err_x{1};
    int tile_err_y{1};
    int step_mod_x{1};
    int step_mod_y{1};
    int col_mod_x{0};
    int col_mod_y{0};
    double slope{0};

    Player(vector2 posn, int ray_length, float speed);

    bool rayInWall(vector2 point, char grid[COLS + 1][ROWS + 1], bool x_chosen, bool y_chosen); // Helper function

    void playerMove(const Uint8 *state);
    void playerRotate(const Uint8 *state, int mode = 0, double amt = 1);
    void castRay(char grid[COLS + 1][ROWS + 1]);
    void playerRender();
};

Player::Player(vector2 posn, int ray_length, float speed)
{
    player_posn = posn;
    ray_end = (vector2){posn.x + ray_length, posn.y};
    ray_len = ray_length;
    player_speed = speed;
    rotation = 220;
}

bool Player::rayInWall(vector2 point, char grid[COLS + 1][ROWS + 1], bool x_chosen, bool y_chosen)
{
    if (x_chosen)
    {
        if (grid[(int)(point.y / TILE_SIZE)][(int(point.x / TILE_SIZE)) - col_mod_x] == '#')
        {
            return true;
        }
    }

    else if (y_chosen)
    {
        if (grid[(int)(point.y / TILE_SIZE) - col_mod_y][(int(point.x / TILE_SIZE))] == '#')
        {
            return true;
        }
    }

    return false;
}

void Player::playerMove(const Uint8 *state)
{
    int count = 0;

    if (state[SDL_SCANCODE_W])
    {
        player_posn.y -= player_speed;
        ray_end.y -= player_speed;

        count++;
        if (count == 2)
        {
            return;
        }
    }

    if (state[SDL_SCANCODE_A])
    {
        player_posn.x -= player_speed;
        ray_end.x -= player_speed;

        count++;
        if (count == 2)
        {
            return;
        }
    }

    if (state[SDL_SCANCODE_S])
    {
        player_posn.y += player_speed;
        ray_end.y += player_speed;

        count++;
        if (count == 2)
        {
            return;
        }
    }

    if (state[SDL_SCANCODE_D])
    {
        player_posn.x += player_speed;
        ray_end.x += player_speed;

        count++;
        if (count == 2)
        {
            return;
        }
    }
}

void Player::playerRotate(const Uint8 *state, int mode, double amt) // mode and amt have default values 0 and 1
{
    if (state != nullptr)
    {
        if (state[SDL_SCANCODE_RIGHT])
        {
            rotation += amt;
        }

        if (state[SDL_SCANCODE_LEFT])
        {
            rotation -= amt;
        }
    }
    else
    {
        if (mode == 1)
        {
            rotation += amt;
        }

        if (mode == -1)
        {
            rotation -= amt;
        }
    }

    if (rotation >= 360)
    {
        rotation = 0;
    }

    if (rotation < 0)
    {
        rotation = 360 + rotation;
    }

    if (rotation >= 0 && rotation < 90)
    {
        err_term_x = 1;
        err_term_y = 1;
        tile_err_x = 1;
        tile_err_y = 1;

        step_mod_x = 1;
        step_mod_y = 1;

        col_mod_x = 0;
        col_mod_y = 0;
    }

    if (rotation >= 90 && rotation < 180)
    {
        err_term_x = 1;
        err_term_y = -1;
        tile_err_x = -1;
        tile_err_y = 1;

        step_mod_x = 0;
        step_mod_y = 1;

        col_mod_x = 1;
        col_mod_y = 0;
    }

    if (rotation >= 180 && rotation < 270)
    {
        err_term_x = -1;
        err_term_y = -1;
        tile_err_x = -1;
        tile_err_y = -1;

        step_mod_x = -1;
        step_mod_y = -1;

        col_mod_x = 1;
        col_mod_y = 1;
    }

    if (rotation >= 270 && rotation < 360)
    {
        err_term_x = -1;
        err_term_y = 1;
        tile_err_x = 1;
        tile_err_y = -1;

        step_mod_x = 1;
        step_mod_y = 0;

        col_mod_x = 0;
        col_mod_y = 1;
    }

    slope = fabs(tan(rotation * M_PI / 180));
}

void Player::castRay(char grid[COLS + 1][ROWS + 1])
{
    SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xFF);

    double base_rot = rotation;

    // playerRotate(nullptr, -1, 30);

    // std::cout << player_posn.y << " " << player_posn.x << " " << step_size_y << " " << step_size_x << std::endl;

    for (int i = 0; i < 60; i++)
    {
        vector2 buf_posn_x = player_posn; // simulated positions for calculating shortest ray
        vector2 buf_posn_y = player_posn;

        vector2 overall_buf = player_posn; // the chosen one

        // playerRotate(nullptr, 1, 1);

        double step_size_x;
        double step_size_y;

        if ((int)(player_posn.x) % TILE_SIZE == 0)
        {
            step_size_x = TILE_SIZE;
        }
        else
        {
            step_size_x = fabs((int)(player_posn.x) - (((int)(player_posn.x / TILE_SIZE) + step_mod_x) * TILE_SIZE));
        }

        if ((int)(player_posn.y) % TILE_SIZE == 0)
        {
            step_size_y = TILE_SIZE;
        }
        else
        {
            step_size_y = fabs((int)(player_posn.y) - (((int)(player_posn.y / TILE_SIZE) + step_mod_y) * TILE_SIZE));
        }

        bool x_chosen = false;
        bool y_chosen = false;

        bool started_x = false;
        bool started_y = false;

        int count = 0;

        while (!pointOutOfBounds(overall_buf) && !rayInWall(overall_buf, grid, x_chosen, y_chosen))
        {
            // Figure out why the step_mods work, and collision detection for 2nd 3rd and 4th quadrants
            std::cout << count << " " << step_size_y << " " << step_size_x << " " << rotation << std::endl;

            if (!x_chosen)
            {
                if (started_x)
                    step_size_y = (double)TILE_SIZE;
                started_x = true;
            }

            else
            {
                step_size_y = fabs((int)(overall_buf.y) - (((int)(overall_buf.y / TILE_SIZE) + step_mod_y) * TILE_SIZE));
            }

            if (!y_chosen)
            {
                if (started_y)
                    step_size_x = (double)TILE_SIZE;
                started_y = true;
            }

            else
            {
                step_size_x = fabs((int)(overall_buf.x) - (((int)(overall_buf.x / TILE_SIZE) + step_mod_x) * TILE_SIZE));
            }

            buf_posn_x = (vector2){overall_buf.x + tile_err_x * step_size_x, overall_buf.y + (double)(err_term_x * step_size_x * slope)};
            buf_posn_y = (vector2){overall_buf.x + (double)(step_size_y * err_term_y * (double)(1.0 / slope)), overall_buf.y + tile_err_y * step_size_y};

            if (getVecLen(overall_buf, buf_posn_x) < getVecLen(overall_buf, buf_posn_y))
            {
                overall_buf = buf_posn_x;
                x_chosen = true;
                y_chosen = false;
            }

            else
            {
                overall_buf = buf_posn_y;
                y_chosen = true;
                x_chosen = false;
            }

            SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xFF);
            drawCircle(overall_buf.x, overall_buf.y, 2, 1);

            count++;
        }

        std::cout << "------------" << std::endl;

        SDL_SetRenderDrawColor(renderer, 0, 0xFF, 0, 0xFF);
        SDL_RenderDrawLineF(renderer, player_posn.x, player_posn.y, overall_buf.x, overall_buf.y);
    }

    rotation = base_rot;
}

void Player::playerRender()
{
    drawCircle(player_posn.x, player_posn.y, 10, 1);
}

bool initStuff()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL couldnt be initialized! SDL_Error:" << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);

    if (window == NULL)
    {
        std::cerr << "Window couldnt be initialized: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == NULL)
    {
        std::cerr << "Renderer couldnt be initialized: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);

    return true;
}

void drawGridlines(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0xFF, 0, 0, 0xFF);

    int i;

    for (i = 1; i < COLS; i++)
    {

        SDL_RenderDrawLine(renderer, 0, i * TILE_SIZE, screen_width, i * TILE_SIZE);
    }

    for (i = 1; i < ROWS; i++)
    {

        SDL_RenderDrawLine(renderer, i * TILE_SIZE, 0, i * TILE_SIZE, screen_height);
    }
}

void drawGrid(SDL_Renderer *renderer, char grid[COLS + 1][ROWS + 1])
{

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    SDL_Rect wall_unit = {0, 0, TILE_SIZE, TILE_SIZE};

    for (int i = 0; i < COLS; i++)
    {
        for (int j = 0; j < ROWS; j++)
        {
            if (grid[i][j] == '#')
            {
                wall_unit.x = j * TILE_SIZE;
                wall_unit.y = i * TILE_SIZE;

                SDL_RenderFillRect(renderer, &wall_unit);
            }
        }
    }
}

void spawnWalls(SDL_Event *e, char grid[COLS + 1][ROWS + 1])
{
    if (e->type == SDL_MOUSEBUTTONDOWN)
    {
        int x, y;
        SDL_GetMouseState(&x, &y);

        x = x / TILE_SIZE;
        y = y / TILE_SIZE;

        if (x <= ROWS && y <= COLS)
        {
            if (grid[y][x] != '#')
                grid[y][x] = '#';
            else
                grid[y][x] = ' ';
        }
    }
}

int main()
{
    char grid[COLS + 1][ROWS + 1] = {
        "     #################################",
        "                                     #",
        "                                     #",
        "                                     #",
        "                                     #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "#                                    #",
        "######################################"};

    if (!initStuff())
    {
        std::cout << "INIT error: " << SDL_GetError() << std::endl;
        return 0;
    }

    Player player((vector2){20 * TILE_SIZE, 20 * TILE_SIZE}, 30 * TILE_SIZE, 2);

    SDL_Event e;
    bool quit = false;

    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }

            spawnWalls(&e, grid);
        }

        const Uint8 *state = SDL_GetKeyboardState(NULL);

        player.playerMove(state);
        player.playerRotate(state);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_RenderClear(renderer);

        drawGrid(renderer, grid);
        drawGridlines(renderer);

        player.playerRender();

        player.castRay(grid);

        SDL_RenderPresent(renderer);
    }
}