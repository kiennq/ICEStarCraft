#include "Vector2.h"

using namespace ICEStarCraft;
using namespace BWAPI;

// Get approximate length of this vector
int Vector2::approxLen()
{
	int dx = (int)(_x);
	int dy = (int)(_y);

	int min, max, approx;

	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;

	if (dx < dy)
	{
		min = dx;
		max = dy;
	}
	else
	{
		min = dy;
		max = dx;
	}

	approx = (max * 1007) + (min * 441);
	if (max < (min << 4)) approx -= (max * 40);
	return (approx >> 10 ) + 1;
}