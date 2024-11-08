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
    vector2 overall_buf = {608, 613.149};
    vector2 buf_posn_x = {576, 586.298};
    vector2 buf_posn_y = {563.905, 576.149};

    std::cout << getVecLen(overall_buf, buf_posn_x) << " " << getVecLen(overall_buf, buf_posn_y) << std::endl;
}