#pragma once
#include "Vector2.h"

using namespace BWAPI;
using namespace std;

namespace ICEStarCraft
{
	class PFFunctions
	{

	private:

	public:
		
		static Vector2 getVelocitySource(const Position& s, const Position& p);
		static Vector2 getVelocityVortex(const Position& s, const Position& p);
		static Vector2 getVelocityObstacle(const Position& o, const Position& p, const double r, const double c, const double s);
		static Vector2 getVelocityObstacleSource(const Position& s, const Position& p, const Position& o, const double r2);

		static Vector2 getVelocityBorder(Position& p);

	};
}
