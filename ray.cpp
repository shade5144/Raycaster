#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#define TILE_SIZE 32
#define ROWS 26
#define COLS 26

#define TAN18 0.3333333333333
#define RAY_MAX 256

const int screen_width = TILE_SIZE * 47;
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

double getVecLen(vector2 vec1, vector2 vec2)
{
    return hypot(vec1.x - vec2.x, vec1.y - vec2.y);
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
    int ray_num{672}; // Number of rays for casting
    int fov{60};
    float player_speed;

    double ray_arr[672]{}; // Stores the length of the rays

    double err_term_x{1};
    double err_term_y{1};
    double tile_err_x{1};
    double tile_err_y{1};
    int step_mod_x{1};
    int step_mod_y{1};
    int col_mod_x{0};
    int col_mod_y{0};
    double slope{0};

    int player_size{5};

    Player(vector2 posn, int ray_length, float speed);

    bool rayInWall(vector2 point, char grid[COLS + 1][ROWS + 1], bool x_chosen, bool y_chosen); // Helper function

    void playerMove(const Uint8 *state, char grid[COLS + 1][ROWS + 1]);
    void playerRotate(const Uint8 *state, int mode = 0, double amt = 1);
    void castRay(char grid[COLS + 1][ROWS + 1]);
    void castWalls(int playerHeight, int wallHeight, double sightAngle);
    void playerRender();
};

Player::Player(vector2 posn, int ray_length, float speed)
{
    player_posn = posn;
    ray_end = (vector2){posn.x + ray_length, posn.y};
    ray_len = ray_length;
    player_speed = speed;
    rotation = 180;
}

bool Player::rayInWall(vector2 point, char grid[COLS + 1][ROWS + 1], bool x_chosen, bool y_chosen)
{

    if ((int)(point.x) % 32 == 0 && (int)(point.y) % 32 == 0)
    {

        if (grid[(int)(point.y / TILE_SIZE) - col_mod_y][(int)(point.x / TILE_SIZE) - col_mod_x] == '#')
        {
            return true;
        }
    }
    else
    {
        if (x_chosen)
        {
            if (grid[(int)(point.y / TILE_SIZE)][(int)(point.x / TILE_SIZE) - col_mod_x] == '#')
            {
                return true;
            }
        }

        else if (y_chosen)
        {
            if (grid[(int)(point.y / TILE_SIZE) - col_mod_y][(int)(point.x / TILE_SIZE)] == '#')
            {
                return true;
            }
        }
    }

    return false;
}

void Player::playerMove(const Uint8 *state, char grid[COLS + 1][ROWS + 1])
{
    int count = 0;

    vector2 movement_vec = {player_speed, player_speed};

    double mult = 1;

    bool key_pressed = false;

    vector2 dirn_vec = {1, 1};

    if (state[SDL_SCANCODE_W])
    {
        dirn_vec.y = -1;

        key_pressed = true;
        count++;
    }

    if (state[SDL_SCANCODE_S] && count != 1)
    {
        mult = -1;

        movement_vec.y = player_speed;

        key_pressed = true;
        count++;
    }

    dirn_vec = (vector2){mult * cos(rotation * M_PI / 180), mult * sin(rotation * M_PI / 180)};

    movement_vec.x *= dirn_vec.x;
    movement_vec.y *= dirn_vec.y;

    player_size += 2;

    if (key_pressed)
    {
        if (grid[(int)((player_posn.y + movement_vec.y + player_size / 2) / TILE_SIZE)][(int)((player_posn.x + movement_vec.x + player_size / 2) / TILE_SIZE)] == '#')
        {
            player_size -= 2;
            return;
        }

        if (grid[(int)((player_posn.y + movement_vec.y - player_size / 2) / TILE_SIZE)][(int)((player_posn.x + movement_vec.x + player_size / 2) / TILE_SIZE)] == '#')
        {
            player_size -= 2;
            return;
        }

        if (grid[(int)((player_posn.y + movement_vec.y - player_size / 2) / TILE_SIZE)][(int)((player_posn.x + movement_vec.x - player_size / 2) / TILE_SIZE)] == '#')
        {
            player_size -= 2;
            return;
        }

        if (grid[(int)((player_posn.y + movement_vec.y + player_size / 2) / TILE_SIZE)][(int)((player_posn.x + movement_vec.x - player_size / 2) / TILE_SIZE)] == '#')
        {
            player_size -= 2;
            return;
        }

        player_posn.x += movement_vec.x;
        player_posn.y += movement_vec.y;
    }

    player_size -= 2;
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

        step_mod_x = 0;
        step_mod_y = 0;

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

    double rot_amt = (double)fov / (double)ray_num;

    playerRotate(nullptr, -1, fov / 2);

    for (int i = 0; i < ray_num; i++)
    {
        vector2 buf_posn_x = player_posn; // simulated positions for calculating shortest ray
        vector2 buf_posn_y = player_posn;

        vector2 overall_buf = player_posn; // the chosen one

        double step_size_x;
        double step_size_y;

        if ((int)(player_posn.x) % TILE_SIZE == 0)
        {
            step_size_x = TILE_SIZE;
        }
        else
        {
            step_size_x = fabs((player_posn.x) - (((int)(player_posn.x / TILE_SIZE) + step_mod_x) * TILE_SIZE));
        }

        if ((int)(player_posn.y) % TILE_SIZE == 0)
        {
            step_size_y = TILE_SIZE;
        }
        else
        {
            step_size_y = fabs((player_posn.y) - (((int)(player_posn.y / TILE_SIZE) + step_mod_y) * TILE_SIZE));
        }

        bool x_chosen = false;
        bool y_chosen = false;

        bool started_x = false;
        bool started_y = false;

        int count = 0;

        while (!pointOutOfBounds(overall_buf) && !rayInWall(overall_buf, grid, x_chosen, y_chosen))
        {

            if (!x_chosen)
            {
                if (started_x)
                    step_size_y = (double)TILE_SIZE;
                started_x = true;
            }

            else
            {
                step_size_y = fabs((overall_buf.y) - (((int)(overall_buf.y / TILE_SIZE) + step_mod_y) * TILE_SIZE));

                if ((int)(step_size_y) == 0)
                {
                    step_size_y = 32;
                }
            }

            if (!y_chosen)
            {
                if (started_y)
                    step_size_x = (double)TILE_SIZE;
                started_y = true;
            }

            else
            {
                step_size_x = fabs((overall_buf.x) - (((int)(overall_buf.x / TILE_SIZE) + step_mod_x) * TILE_SIZE));

                if ((int)(step_size_x) == 0)
                {
                    step_size_x = 32;
                }
            }

            buf_posn_x = (vector2){overall_buf.x + tile_err_x * step_size_x, overall_buf.y + (double)(err_term_x * step_size_x * slope)};
            buf_posn_y = (vector2){overall_buf.x + (double)(step_size_y * err_term_y * (double)(1.0 / slope)), overall_buf.y + tile_err_y * step_size_y};

            double lx = getVecLen(overall_buf, buf_posn_x);
            double ly = getVecLen(overall_buf, buf_posn_y);

            vector2 temp_buf = overall_buf;

            if (lx < ly)
            {
                temp_buf = buf_posn_x;
                x_chosen = true;
                y_chosen = false;
            }

            else if (lx > ly)
            {
                temp_buf = buf_posn_y;
                y_chosen = true;
                x_chosen = false;
            }

            else
            {
                temp_buf = buf_posn_y;
                y_chosen = false;
                x_chosen = false;
            }

            if (grid[(int)((temp_buf.y + overall_buf.y) / (2 * TILE_SIZE))][(int)((temp_buf.x + overall_buf.x) / (2 * TILE_SIZE))] == '#')
            {
                break;
            }

            overall_buf = temp_buf;

            count++;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0xFF, 0, 0xFF);
        SDL_RenderDrawLineF(renderer, player_posn.x, player_posn.y, overall_buf.x, overall_buf.y);

        ray_arr[i] = getVecLen(player_posn, overall_buf);
        playerRotate(nullptr, 1, rot_amt);
    }

    rotation = base_rot;
}

void Player::castWalls(int playerHeight, int wallHeight, double sightAngle) // min height of wall is 20 pixels
{
    double max_height = 400;

    SDL_FRect to_render = (SDL_FRect){ROWS * TILE_SIZE - 1, (COLS * TILE_SIZE) / 2 - 256, 1, 0};

    double prev_ray_len = 0;
    double theta = fov / 2;
    double dec_amt = (double)fov / (double)ray_num;

    double proj_plane_distance = (21 * TILE_SIZE / (2 * tan((fov / 2) * M_PI / 180))) * TILE_SIZE; // Multiplied by TILE_SIZE for scaling

    for (int i = 0; i < ray_num; i++)
    {
        double ray_proj = ray_arr[i] * cos(theta * M_PI / 180);

        to_render.x += 1;

        if (ray_proj >= RAY_MAX)
        {
            continue;
        }

        int light_intensity = 256;

        to_render.h = proj_plane_distance / ray_proj;
        to_render.y = (float)screen_height / 2 - (float)screen_height * 16 / (ray_proj);

        SDL_SetRenderDrawColor(renderer, 0, (light_intensity - ray_proj), 0, 0xFF);
        SDL_RenderFillRectF(renderer, &to_render);

        prev_ray_len = ray_proj;

        theta -= dec_amt;
    }
}

void Player::playerRender()
{
    drawCircle(player_posn.x, player_posn.y, player_size, 1);
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

        SDL_RenderDrawLine(renderer, 0, i * TILE_SIZE, (screen_width / 2) + 2 * TILE_SIZE, i * TILE_SIZE);
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

void destroyStuff()
{
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;

    SDL_Quit();
}

int main()
{
    char grid[COLS + 1][ROWS + 1] = {
        "##########################",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "#                        #",
        "##########################",
    };

    if (!initStuff())
    {
        std::cout << "INIT error: " << SDL_GetError() << std::endl;
        return 0;
    }

    Player player((vector2){10 * TILE_SIZE, 10 * TILE_SIZE}, 30 * TILE_SIZE, 2);

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

        player.playerMove(state, grid);
        player.playerRotate(state);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_RenderClear(renderer);

        drawGrid(renderer, grid);
        drawGridlines(renderer);

        player.playerRender();

        player.castRay(grid);

        player.castWalls(25, 50, 18.4);

        SDL_RenderPresent(renderer);
    }

    destroyStuff();
}