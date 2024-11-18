#include <iostream>
#include <SDL2/SDL.h>

struct vector2
{
    double x;
    double y;
};

double getVecLen(vector2 vec1, vector2 vec2)
{
    return hypot(vec2.x - vec1.x, vec2.y - vec1.y, 2);
}

int main()
{
    std::cout << round(351.914) << std::endl;
}