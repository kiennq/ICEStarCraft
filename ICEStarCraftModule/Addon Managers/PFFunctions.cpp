#include "PFFunctions.h"

using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;
using namespace std;

/* Calculate velocity of a source */
Vector2 PFFunctions::getVelocitySource(const Position& s, const Position& p)
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	double r2 = 1.0*x*x + 1.0*y*y;

	if (r2 == 0)
	{
		return Vector2();
	}
	
	return Vector2(x/r2, y/r2);
}

/* Calculate velocity of a vortex */
Vector2 PFFunctions::getVelocityVortex(const Position& s, const Position& p)
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	double r2 =  1.0*x*x +  1.0*y*y;

	if (r2 == 0)
	{
		return Vector2();
	}

	return Vector2(y/r2, -x/r2);
}

/*
 * obstacle in uniform flow
 * o: obstacle's position
 * p: unit's position
 * r: obstacle's radius
 * c: cos alpha 
 * s: sin alpha
 */
Vector2 PFFunctions::getVelocityObstacle(const Position& o, const Position& p, const double r, const double c, const double s)
{
	int x = p.x() - o.x();
	int y = p.y() - o.y();
	double x2 = 1.0*x*x;
	double y2 = 1.0*y*y;
	double r2 = r*r;
	double r4 = (x2 + y2)*(x2 + y2);

	double vx = r2 * (c*(y2-x2) - 2*s*x*y) / r4;
	double vy = r2 * (s*(x2-y2) - 2*c*x*y) / r4;

	if (r4 == 0)
	{
		return Vector2();
	}
	return Vector2(vx,vy);
}

/* Calculate velocity near a obstacle in a source */
Vector2 PFFunctions::getVelocityObstacleSource(const Position& s, const Position& p, const Position& o, const double r2)
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	int xc = o.x() - s.x();
	int yc = o.y() - s.y();
	double x2 = 1.0*x*x;
	double y2 = 1.0*y*y;
	double _r2 = (x2 + y2);

	double deno = r2*(r2*r2 - 2*r2*(x*xc+y*yc) + _r2*(xc*xc + yc*yc));

	double vx = -r2*(r2*x - 2*x*y*yc - x2*xc + y2*xc)/deno;
	double vy = -r2*(r2*y - 2*x*y*xc - y2*yc + x2*yc)/deno;

	if (deno == 0)
	{
		return Vector2();
	}
	return Vector2(vx, vy);
}

/* Calculate velocity of potential flow created by border */
Vector2 PFFunctions::getVelocityBorder(Position& p)
{
	if (!p.isValid())
	{
		return Vector2();
	}

	BWTA::Region* reg = BWTA::getRegion(p);
	Polygon border = reg->getPolygon();
	Vector2 b = Vector2();

	for (Polygon::iterator i = border.begin(); i != border.end(); i++)
	{
		if (p.getApproxDistance(*i) <= 32)
		{
			Broodwar->drawCircleMap((*i).x(),(*i).y(),2,Colors::Yellow);
			b += getVelocitySource(*i,p);
		}
	}

	return b;
}