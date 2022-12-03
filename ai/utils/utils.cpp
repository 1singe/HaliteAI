#include "utils.hpp"


hlt::Direction* directionTowards(const hlt::Position& kr_source, const hlt::Position& kr_destination) {
	hlt::Direction directions[2];
	int dx = kr_destination.x - kr_source.x;
	int dy = kr_destination.y - kr_source.y;

	//move the highest valued axis first
	bool ypriority = abs(dx) < abs(dy);

	dx = abs(dx) < A_MOD(dx, ai::constants::MAP_SIZE) ? dx : A_MOD(dx, ai::constants::MAP_SIZE);
	dy = abs(dy) < A_MOD(dy, ai::constants::MAP_SIZE) ? dy : A_MOD(dy, ai::constants::MAP_SIZE);

	if (dx == 0)
		directions[ypriority] = hlt::Direction::STILL;
	if (dx >= 0)
		directions[ypriority] = hlt::Direction::EAST;
	if (dx < 0)
		directions[ypriority] = hlt::Direction::WEST;
	if (dy == 0)
		directions[!ypriority] = hlt::Direction::STILL;
	if (dy >= 0)
		directions[!ypriority] = hlt::Direction::SOUTH;
	if (dy < 0)
		directions[!ypriority] = hlt::Direction::NORTH;

	return directions;
}


float calculatePseudoNormalLaw(int dist, float flattener, float sigma)
{
	return (1.f / (sigma * SQRT_2PI)) * pow(E, -0.5 * pow(dist / (flattener * sigma), 2)); // Elle aussi, trouvée sur géogébra
}

int getSign(int n) {
	if (n > 0) return 1;
	if (n == 0) return 0;
	if (n < 0) return -1;
}