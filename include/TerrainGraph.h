#pragma once

#include <BWAPI.h>
#include <BWTA.h>
#include <vector>
#include <boost/shared_ptr.hpp>

class TerrainGraph {

public:

	//typedef shared_ptr<set<BWTA::Region*> > RegionSetPtr;

	struct Node;
	typedef boost::shared_ptr<Node> NodePtr;

	struct Node{
		Node(BWTA::Region* r): r(r), influentValue(0){}
		BWTA::Region* r;
		std::map<BWTA::Chokepoint*, NodePtr> neighbors;
		double influentValue;
	};

	static TerrainGraph* create();
	static void destroy();
	
	void update();
	// Draw some enhanced UI
	void draw();
	
	// Propage the effect on all region
	void propageEffect(BWTA::Region* r, double initVal);

	NodePtr getNode(BWTA::Region* r);
protected:
	// Should be constant map
	std::map<BWTA::Region*, NodePtr> regNodeMap;
	// Init mapping 1-1 from Region* to Node*
	TerrainGraph();

private:
	~TerrainGraph(){}

};
