#include "TerrainGraph.h"

using namespace BWTA;
using namespace std;
using namespace boost;

TerrainGraph* theTerrainGraph = NULL;

TerrainGraph* TerrainGraph::create()
{
	if (theTerrainGraph) return theTerrainGraph;
	return theTerrainGraph = new TerrainGraph();
}

void TerrainGraph::destroy()
{
	if (theTerrainGraph) delete theTerrainGraph;
}

TerrainGraph::TerrainGraph()
{
	// initialize the graph
	set<Region*> reg(getRegions());
	for each (Region *re in reg){
		NodePtr node(new Node(re));
		regNodeMap.insert(make_pair(re, node));
		
		set<Chokepoint*> nChoke(re->getChokepoints());
		
		for each(Chokepoint* c in nChoke) {
			pair<Region*, Region*> regSide (c->getRegions());
			Region* other = regSide.first==re? re : regSide.second;
			map<Region*, NodePtr>::iterator i = regNodeMap.find(other);
			if (i == regNodeMap.end()) {
				NodePtr neighbor(new Node(other));
				regNodeMap.insert(make_pair(other, neighbor));
				
				node->neighbors.insert(make_pair(c, neighbor));
			} else {
				node->neighbors.insert(make_pair(c, i->second));
			}
		}

	}

}

void TerrainGraph::propageEffect( BWTA::Region* r, double initVal )
{

}

TerrainGraph::NodePtr TerrainGraph::getNode( BWTA::Region* r )
{
	map<Region*, NodePtr>::iterator i = regNodeMap.find(r);
	if (i != regNodeMap.end()) return i->second;
	else return NodePtr();
}

void TerrainGraph::draw()
{
	for each (pair<Region*, NodePtr> p in regNodeMap){
		NodePtr n = p.second;
		BWAPI::Position nCenter = n->r->getCenter();
		BWAPI::Broodwar->drawCircleMap(nCenter.x(), nCenter.y(), 3,BWAPI:: Colors::Cyan, true);
		for each (pair<Chokepoint*, NodePtr> cn in n->neighbors){
			BWAPI::Position cnCenter = cn.second->r->getCenter();
			BWAPI::Position cCenter = cn.first->getCenter();
			BWAPI::Broodwar->drawCircleMap(cnCenter.x(), cnCenter.y(), 3, BWAPI::Colors::Cyan, true);
			BWAPI::Broodwar->drawLineMap(nCenter.x(), nCenter.y(), cCenter.x(), cCenter.y(), BWAPI::Colors::Yellow);
			BWAPI::Broodwar->drawLineMap(cnCenter.x(), cnCenter.y(), cCenter.x(), cCenter.y(), BWAPI::Colors::Yellow);
		}
	}
}

