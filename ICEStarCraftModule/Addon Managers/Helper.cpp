#include "Helper.h"

using namespace ICEStarCraft;
using namespace BWAPI;
using namespace BWTA;
using namespace std;

ICEStarCraft::Helper::Helper(void)
{
}

ICEStarCraft::Helper::~Helper(void)
{
}

/*
Vector2 Helper::getFPC(set<Unit*> units)
{
	Position p;
	Position c = Helper::getCenter(units);

	double varX, varY, cov;
	double sumX2 = 0, sumY2 = 0, sumXY = 0;

	for each (Unit* u in units)
	{
		p = u->getPosition();
		sumX2 += 1.0 * p.x() * p.x();
		sumY2 += 1.0 * p.y() * p.y();
		sumXY += 1.0 * p.x() * p.y();
	}

	varX = sumX2 / units.size() - 1.0 * c.x() * c.x();
	varY = sumY2 / units.size() - 1.0 * c.y() * c.y();
	cov  = sumXY / units.size() - 1.0 * c.x() * c.y();

	return Vector2((varX - varY + sqrt((varX - varY) * (varX - varY) + 4 * cov * cov)) / (2 * cov),1);
}
*/

bool ICEStarCraft::Helper::isWalkable(int walkX, int walkY)
{
	return Broodwar->isWalkable(walkX, walkY) && Broodwar->getUnitsOnTile(walkX >> 2, walkY >> 2).empty();
}

bool ICEStarCraft::Helper::isWalkable(Position p)
{
	return isWalkable(p.x() >> 3, p.y() >> 3);
}

bool ICEStarCraft::Helper::isWalkable(TilePosition tp)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (!Broodwar->isWalkable(tp.x()*4 + i,tp.y()*4 + j))
				return false;
		}
	}
	return true;
}

bool ICEStarCraft::Helper::isWalkable(Position cur, Position des)
{
	if (!des.isValid() || !Broodwar->isWalkable(des.x() >> 3, des.y() >> 3) || !Broodwar->hasPath(cur, des))
	{
		return false;
	}

	Vector2 v = des - cur;
	Position tmp;
	for (int i = 1; i <= 4; i++)
	{
		tmp = v * 0.25 * i + cur;
		if (!tmp.isValid() || !Broodwar->isWalkable(tmp.x() >> 3, tmp.y() >> 3))
		{
			return false;
		}
	}

	return true;
}

bool ICEStarCraft::Helper::isConnected(TilePosition tp1, TilePosition tp2)
{
	if (!BWTA::isConnected(tp1,tp2))
	{
		return false;
	}

	vector<TilePosition> path = BWTA::getShortestPath(tp1,tp2);

	if (path.empty())
	{
		return false;
	}
	
	for each (TilePosition tp in path)
	{
		if (!Broodwar->getUnitsOnTile(tp.x(),tp.y()).empty())
		{
			return false;
		}
	}

	return true;
}

bool ICEStarCraft::Helper::isChokePointCenter(Position p)
{
	for each (BWTA::Chokepoint* cp in BWTA::getChokepoints())
	{
		if (cp->getCenter() == p)
			return true;
	}
	return false;
}

bool ICEStarCraft::Helper::isDirectlyConnected(BWTA::Region* r1, BWTA::Region* r2)
{
	if (r1 == NULL || r2 == NULL)
	{
		return false;
	}
	
	for each (BWTA::Chokepoint* cp in BWTA::getChokepoints())
	{
		pair<BWTA::Region*,BWTA::Region*> p = cp->getRegions();
		if ((p.first == r1 && p.second == r2) || (p.first == r2 && p.second == r1))
		{	
			return true;
		}
	}
	return false;
}

BWTA::Chokepoint* ICEStarCraft::Helper::getChokePoint(BWTA::Region* r1, BWTA::Region* r2)
{
	for each (BWTA::Chokepoint* cp in BWTA::getChokepoints())
	{
		pair<BWTA::Region*,BWTA::Region*> p = cp->getRegions();
		if ((p.first == r1 && p.second == r2) || (p.first == r2 && p.second == r1))
		{	
			return cp;
		}
	}
	return NULL;
}

void ICEStarCraft::Helper::drawPolygonVertices(BWTA::Region* r)
{
	if (r == NULL)
	{
		Broodwar->printf("Invalid region");
		return;
	}

	Polygon p = r->getPolygon();
	set<Chokepoint*> cp = BWTA::getChokepoints();
	bool isChokePoint = false;

	for (Polygon::iterator i = p.begin(); i != p.end(); i++)
	{
		isChokePoint = false;
		for (set<Chokepoint*>::iterator c = cp.begin(); c != cp.end(); c++)
		{
			if ((*i) == (*c)->getCenter())
			{
				isChokePoint = true;
				break;
			}
		}	

		Broodwar->drawCircleMap((*i).x(),(*i).y(),2,Colors::Yellow,isChokePoint);
	}
}

void ICEStarCraft::Helper::drawRegionCenter(BWTA::Region* r)
{
	if (r == NULL)
	{
		Broodwar->printf("Invalid region");
		return;
	}

	Position center = r->getCenter();
	Broodwar->drawCircleMap(center.x(),center.y(),5,Colors::Red,true);
}

/************************************************************************/
/* Graph                                                                */
/************************************************************************/

Graph::Graph()
{
	vertices.clear();
	edges.clear();
}

void Graph::addVertex(Position p)
{
	if (p != Positions::None && vertices.find(p) == vertices.end())
	{
		vertices.insert(p);
	}
}

Edge* Graph::addEdge(Position p1, Position p2)
{
	return addEdge(p1,p2,0);
}

Edge* Graph::addEdge(Position p1, Position p2, int weight)
{
	if (p1 == Positions::None || p2 == Positions::None || p1 == p2)
	{
		return NULL;
	}

	if (vertices.find(p1) == vertices.end())
	{
		vertices.insert(p1);
	}

	if (vertices.find(p2) == vertices.end())
	{
		vertices.insert(p2);
	}

	for each (Edge* e in edges)
	{
		if ((e->first == p1 && e->second == p2) || (e->first == p2 && e->second == p1))
		{
			e->setWeight(weight);
			return e;
		}
	}

	Edge* e = new Edge(p1,p2,weight);
	edges.insert(e);
	return e;
}

void Graph::removeEdge(Position p1, Position p2)
{
	if (p1 == Positions::None || p2 == Positions::None || p1 == p2)
	{
		return;
	}

	for each (Edge* e in edges)
	{
		if ((e->first == p1 && e->second == p2) || (e->first == p2 && e->second == p1))
		{
			edges.erase(e);
			return;
		}
	}
}

int Graph::getWeight(BWAPI::Position p1, BWAPI::Position p2)
{
	if (p1 == Positions::None || p2 == Positions::None || p1 == p2)
	{
		return -1;
	}

	for each (Edge* e in edges)
	{
		if ((e->first == p1 && e->second == p2) || (e->first == p2 && e->second == p1))
		{
			return e->getWeight();
		}
	}

	return -1;
}

std::set<Position> Graph::getNeighbors(Position p)
{
	std::set<Position> neighbors;

	if (p == Positions::None || vertices.find(p) == vertices.end())
	{
		return neighbors;
	}
	
	for each (Edge* e in edges)
	{
		if (e->first == p)
		{
			neighbors.insert(e->second);
		}
		else if (e->second == p)
		{
			neighbors.insert(e->first);
		}
	}

	return neighbors;
}

void Graph::Dijkstra(map<Position,Position>& previous, Position start, Position end)
{
	map<Position,int> distance;

	for each (Position p in vertices)
	{
		distance[p] = std::numeric_limits<int>::max();
		previous[p] = Positions::None;
	}
	
	distance[start] = 0;
	std::set<Position> Q = vertices;

	while (!Q.empty())
	{
		Position u;
		int min = std::numeric_limits<int>::max();
		for each (Position v in Q)
		{
			if (distance[v] <= min)
			{
				min = distance[v];
				u = v;
			}
		}

		Q.erase(u);

		if (u == end)
		{
			return;
		}
		
		if (distance[u] == std::numeric_limits<int>::max())
		{
			break;
		}

		for each (Position v in getNeighbors(u))
		{
			int tmp = distance[u] + getWeight(u,v);
			if (tmp < distance[v])
			{
				distance[v] = tmp;
				previous[v] = u;
			}
		}
	}
}

void Graph::Dijkstra(map<Position,set<BWAPI::Position>>& previous, Position start, Position end)
{
	set<Position> positions;
	map<Position,int> distance;
	
	for each (Position p in vertices)
	{
		distance[p] = std::numeric_limits<int>::max();
		previous[p] = positions;
	}

	distance[start] = 0;
	std::set<Position> Q = vertices;

	while (!Q.empty())
	{
		Position u;
		int min = std::numeric_limits<int>::max();
		for each (Position v in Q)
		{
			if (distance[v] <= min)
			{
				min = distance[v];
				u = v;
			}
		}

		Q.erase(u);

		if (u == end)
		{
			return;
		}

		if (distance[u] == std::numeric_limits<int>::max())
		{
			break;
		}

		for each (Position v in getNeighbors(u))
		{
			int tmp = distance[u] + getWeight(u,v);
			if (tmp <= distance[v])
			{
				if (tmp < distance[v])
				{
					previous[v].clear();
				}
				distance[v] = tmp;
				previous[v].insert(u);
			}
		}
	}
}

vector<Position> Graph::getShortestPath(Position start, Position end)
{
	vector<Position> path;

	if (start == Positions::None || end == Positions::None)
	{
		return path;
	}
	
	if (vertices.find(start) == vertices.end() || vertices.find(end) == vertices.end())
	{
		return path;
	}

	map<Position,Position> previous;
	Dijkstra(previous,start,end);

	Position p = end;
	while (previous[p] != Positions::None)
	{
		path.push_back(p);
		p = previous[p];
	}
	path.push_back(p);

	std::reverse(path.begin(),path.end());
	return path;
}

void Graph::drawGraph(Color color)
{
	//Broodwar->printf("%d vertices, %d edges",vertices.size(),edges.size());
	for each (Position p in vertices)
	{
		Broodwar->drawCircleMap(p.x(),p.y(),2,color,true);
	}
	
	for each (Edge* e in edges)
	{
		Broodwar->drawLineMap(e->first.x(),e->first.y(),e->second.x(),e->second.y(),color);
		Broodwar->drawTextMap((e->first.x()+e->second.x())/2,(e->first.y()+e->second.y())/2,"%d",e->getWeight());
	}
}