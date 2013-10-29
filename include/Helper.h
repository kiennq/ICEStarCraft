#pragma once
#include <BWAPI.h>
#include <BWTA.h>
#include <algorithm>
#include <math.h>
#include <limits>
#include "Vector2.h"

using namespace std;
using namespace BWAPI;
using namespace BWTA;

namespace ICEStarCraft
{
	class Helper
	{
	public:
		Helper(void);
		~Helper(void);

		static bool isWalkable(Position p);
		static bool isWalkable(TilePosition tp);
		static bool isWalkable(int walkX, int walkY);
		static bool isWalkable(Position cur, Position des);
		static bool isConnected(TilePosition tp1, TilePosition tp2);

		static bool nearReachPos(BWAPI::Unit* u, BWAPI::Position& p){return u->getPosition().getApproxDistance(p) < 8;}
    static bool nearReachPos(BWAPI::Unit* u, BWAPI::Position& p, int _thres){return u->getPosition().getApproxDistance(p) < _thres;}

		// get first principal component
		//static Vector2 getFPC(set<Unit*> units);

		static void drawPolygonVertices(BWTA::Region* r);
		static void drawRegionCenter(BWTA::Region* r);
		static bool isChokePointCenter(Position p);
		static bool isDirectlyConnected(BWTA::Region* r1, BWTA::Region* r2);
		static BWTA::Chokepoint* getChokePoint(BWTA::Region* r1, BWTA::Region* r2);
	};

	class Edge
	{
	public:

		Edge(BWAPI::Position p1, BWAPI::Position p2, int w): first(p1), second(p2), weight(w) {}
		Edge(BWAPI::Position p1, BWAPI::Position p2): first(p1), second(p2), weight(0) {}

		BWAPI::Position first;
		BWAPI::Position second;
		int weight;
		void setWeight(int w) {weight = w;}
		int getWeight() const {return weight;}
	};

	class Graph
	{
	public:

		Graph();

		std::set<BWAPI::Position> vertices;
		std::set<Edge*> edges;
	
		void addVertex(BWAPI::Position p);
		Edge* addEdge(BWAPI::Position p1, BWAPI::Position p2);
		Edge* addEdge(BWAPI::Position p1, BWAPI::Position p2, int weight);
		void removeEdge(BWAPI::Position p1, BWAPI::Position p2);
		int getWeight(BWAPI::Position p1, BWAPI::Position p2);

		std::set<BWAPI::Position> getNeighbors(BWAPI::Position p);
		void Dijkstra(std::map<BWAPI::Position,BWAPI::Position>& previous, BWAPI::Position start, BWAPI::Position end = BWAPI::Positions::None);
		void Dijkstra(std::map<BWAPI::Position,std::set<BWAPI::Position>>& previous, BWAPI::Position start, BWAPI::Position end = BWAPI::Positions::None);
		std::vector<BWAPI::Position> getShortestPath(BWAPI::Position start, BWAPI::Position end);

		void drawGraph(BWAPI::Color color = BWAPI::Colors::Green);
	};
}
