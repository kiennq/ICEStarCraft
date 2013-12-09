#include "ScoutController.h"
#include "TerrainManager.h"
#include "Config.h"

using namespace BWAPI;
using namespace BWTA;
using namespace ICEStarCraft;
using namespace std;

ScoutController* theScoutController = NULL;


/* Parameters:
*	0: center vortex
*	1: center source, sink
*	2: distance to switch source, sink
*	3: distance to active border flow
*	4: border's vortex
*	5: border's source
*	6: building obstacle
*	7: enemy's needle
*/
ScoutController::ScoutController(void)
{
	// 5 parameters

	double p[] = {400,100,128,160,-600,300,150,300};
	setParams(p);

	//set<BaseLocation*> tmp = BWTA::getStartLocations();
	//_enBaseLoacation.insert(tmp.begin(), tmp.end());

	// add all scv
	/*set<Unit*> scv = Broodwar->self()->getUnits();
	for (set<Unit*>::iterator i = scv.begin(); i != scv.end(); i++) {
		if ((*i)->getType().isWorker()) {
			_scouts.insert(make_pair(*i, make_pair((*i)->getPosition(), 1)));
#ifdef _HUMAN_PLAY
			Broodwar->setScreenPosition((*i)->getPosition()-Position(320,320));
#endif
		}
	}*/

#ifdef _SCOUT_DEBUG
	show_all_p = false;
	show_border_p = true;
	show_object_r = true;
	show_region_p = true;
	show_unit_p = true;
#endif // _SCOUT_DEBUG
#ifdef _LOG_TO_FILE
	_enDiscoveredBuildings = 0;
#endif // _LOG_TO_FILE
}

ScoutController::~ScoutController(void)
{
}

ScoutController* ScoutController::create()
{
	if (theScoutController) return theScoutController;
	else return (theScoutController =  new ScoutController());
}

void ScoutController::destroy()
{
	if (theScoutController) delete theScoutController;
	theScoutController = NULL;
}


void ScoutController::onFrame()
{

#ifdef _HUMAN_PLAY
	return;
#endif

  if (Broodwar->getFrameCount()%1 == 0)
  {
    detectUnseenUnitInsideBunker();
    _registerEnUnitPosition();
    _fadeUnits();
  }

	for (map<Unit*, pair<Position, int> >::iterator i = _scouts.begin(); i!=_scouts.end(); i++) {

#ifdef _SCOUT_DEBUG
		if (show_object_r) {
			Position curCe = BWTA::getRegion(i->first->getPosition())->getCenter();
			Broodwar->drawCircleMap(curCe.x(), curCe.y(), 3, Colors::Red, true);
			Broodwar->drawCircleMap(curCe.x(), curCe.y(), (int)_p[2], Colors::Cyan);
			//TODO: Need to show a cirle around each unit building
			for each (pair<Position,int> _obstacle in _obj[i->first]) {
				Broodwar->drawCircleMap(_obstacle.first.x(), _obstacle.first.y(), _obstacle.second, Colors::Cyan);
			}
		}
		Position scout_pos = i->first->getPosition();
		if (show_unit_p) {
			for each (Vector2 _up in _unit_p[i->first]) {
				Broodwar->drawLineMap(scout_pos.x(), scout_pos.y(), scout_pos.x()+(int)_up.x(), scout_pos.y()+(int)_up.y(), Colors::Red);
				Broodwar->drawCircleMap(scout_pos.x()+(int)_up.x(), (int)scout_pos.y()+(int)_up.y(), 2, Colors::Red, true);
			}
		}
    
		if (show_region_p) {
			for each (Vector2 _rp in _region_p[i->first]) {
				Broodwar->drawLineMap(scout_pos.x(), scout_pos.y(), scout_pos.x()+(int)_rp.x(), scout_pos.y()+(int)_rp.y(), Colors::Yellow);
				Broodwar->drawCircleMap(scout_pos.x()+(int)_rp.x(), scout_pos.y()+(int)_rp.y(), 2, Colors::Yellow, true);
			}
		}
		if (show_border_p) {
			//Broodwar->drawCircleMap(i->first->getPosition().x(), i->first->getPosition().y(), (int)_p[3], Colors::Cyan);
			for each (Vector2 _bp in _border_p[i->first]) {
				Broodwar->drawLineMap(scout_pos.x(), scout_pos.y(), scout_pos.x()+(int)_bp.x(), scout_pos.y()+(int)_bp.y(), Colors::Orange);
				Broodwar->drawCircleMap(scout_pos.x()+(int)_bp.x(), scout_pos.y()+(int)_bp.y(), 2, Colors::Orange, true);
			}
		}
		if (show_all_p) {
			for each (Vector2 _ap in _all_p[i->first]) {
				Broodwar->drawLineMap(scout_pos.x(), scout_pos.y(), scout_pos.x()+(int)_ap.x(), scout_pos.y()+(int)_ap.y(), Colors::Cyan);
				Broodwar->drawCircleMap(scout_pos.x()+(int)_ap.x(), scout_pos.y()+(int)_ap.y(), 2, Colors::Cyan, true);
			}
		}
    /*if (Broodwar->getFrameCount()%24 == 1) {
      if (_screenPos == Position(0,0)) _screenPos = scout_pos;
      Broodwar->setScreenPosition((_screenPos.x() + scout_pos.x()*3)/4 - 320, (_screenPos.y() + scout_pos.y()*3)/4 - 200);
    }*/

    /*for each (Bullet *b in Broodwar->getBullets()) {
      string s =  b->getType().getName();
      if(b->getSource())
        Broodwar->printf("%s: %s", b->getSource()->getType().getName().c_str(), b->getType().getName().c_str());
      else
        Broodwar->printf("%s: %s", "Unknown source", b->getType().getName().c_str());
    }*/
    if (_scoutLastPositions[i->first].fStuck) {
      Position mPos = _scoutLastPositions[i->first].mineral->getPosition();
      Broodwar->drawLineMap(scout_pos.x(), scout_pos.y(), mPos.x(), mPos.y(), Colors::Brown);
      Broodwar->drawCircleMap(mPos.x(), mPos.y(), 3, Colors::Brown);
    }


#endif // _SCOUT_DEBUG

		if(Broodwar->getFrameCount()%1==0 ||Helper::nearReachPos(i->first, i->second.first) || Helper::nearReachPos(i->first, i->first->getTargetPosition(), 32)) 
    {
      Position target = i->first->getPosition();
      _UnitInfo& unitLastLoc = _scoutLastPositions[i->first];
      if (unitLastLoc.fStuck) 
      {
        if (target.getApproxDistance(unitLastLoc.pos) > 16) 
        {
          unitLastLoc.fStuck = false;
          unitLastLoc.iFrame = Broodwar->getFrameCount();
          unitLastLoc.pos = target;
        }
      } 
      else 
      {
        //Seem we stuck
        if (target.getApproxDistance(unitLastLoc.pos) < 4 && Broodwar->getFrameCount()-unitLastLoc.iFrame > 3) 
        {
          unitLastLoc.fStuck = true;
          // Find mineral patch
          if (!unitLastLoc.mineral || !unitLastLoc.mineral->exists()) 
          {
            /*double dis = -9999;
            for each (Unit* u in Broodwar->getMinerals()) {
              int _tmp = u->getPosition().getApproxDistance(target) > 64 ? 0 : -99;
              double _tdis =  (cos(i->first->getAngle()) *(u->getPosition().x()-target.x())
                + sin(i->first->getAngle())*(u->getPosition().y()-target.y())) - _tmp;
              if (BWTA::getRegion(u->getPosition())==BWTA::getRegion(target) && _tdis > dis){
                unitLastLoc.mineral = u;
                dis = _tdis;
              }
            }
            if (dis == -9999) {
              unitLastLoc.mineral = *Broodwar->getMinerals().begin();
            }*/
            unitLastLoc.mineral = *BWTA::getStartLocation(Broodwar->self())->getMinerals().begin();

          }
          i->first->rightClick(unitLastLoc.mineral);
          //Broodwar->printf("Click on mineral");
        }
        else
        {
          if (target.getApproxDistance(unitLastLoc.pos) > 2)
          {
            unitLastLoc.iFrame = Broodwar->getFrameCount();
            unitLastLoc.pos = target;
          }
          else
          {
            //Broodwar->printf("Stuck plus %d", Broodwar->getFrameCount() - unitLastLoc.frame);
          }
          // recalculate potential value
          int rev = i->second.second;
          _p[0] *= rev;
          _p[4] *= rev;
          _p[6] *= rev;
          Vector2 speed = calculatePVal(i->first);
          _p[0] *= rev;
          _p[4] *= rev;
          _p[6] *= rev;

          //Vector2 speed = rotateAroundBuilding(i->first);
          i->second.first = speed + i->first->getPosition();
          //Position target = i->first->getPosition();
          double ratio = 32.0 / speed.r();
          Vector2 seg = speed * ratio;
          target = seg*3 + target;
          while (!Helper::isWalkable(target))
          {
            target = seg + target;
            //if (BWTA::getRegion(target) != BWTA::getRegion(i->first->getPosition())) {
            if (!target.isValid())
            {
              target = seg*(-1) + target;
              break;
            }
          }
          //i->second.first = target;
          i->first->rightClick(target);
        }
      }
    }
	}
}

void ScoutController::onUnitDestroy( Unit *u )
{
  if (u->getType() == UnitTypes::Terran_Bunker)
    _enBunker.erase(u);

	map<Unit*, pair<Position, int> >::iterator i = _scouts.find(u);
	if (i != _scouts.end()) _scouts.erase(i);
  _fadeUnit(u);

#ifdef _LOG_TO_FILE
	if (_scouts.empty()) Broodwar->restartGame();
#endif
}


void ScoutController::onUnitDiscover( Unit *u )
{
	if (Broodwar->self()->isEnemy(u->getPlayer()) && u->getType().isBuilding())
  {
    if (u->getType() == UnitTypes::Terran_Bunker) {
      map<Unit*, UnitType>::iterator bunker = _enBunker.find(u);
      if (bunker == _enBunker.end()) 
        _enBunker.insert(make_pair(u, UnitTypes::None));
    }
    _enBuildings.insert(u);

#ifdef _LOG_TO_FILE
		_enDiscoveredBuildings++;
#endif
	}
	if (Broodwar->self()->isEnemy(u->getPlayer()))
  {
		_enUnits[u->getType()].insert(u);
	}
}



void ScoutController::onSendText(const std::string& text )
{
#ifdef _SCOUT_DEBUG
	if (text == "/u") show_unit_p = !show_unit_p;
	if (text == "/r") show_region_p = !show_region_p;
	if (text == "/b") show_border_p = !show_border_p;
	if (text == "/a") show_all_p = !show_all_p;
	if (text == "/o") show_object_r = !show_object_r;
#endif
}


void ScoutController::detectUnseenUnitInsideBunker()
{
  for each (Bullet *b in Broodwar->getBullets()) 
	{
    Unit *underAtkUnit = b->getTarget();
    if (underAtkUnit && !b->getSource() && underAtkUnit->getPlayer() == Broodwar->self()) 
		{
      set<Unit*> inRadius = underAtkUnit->getUnitsInRadius(underAtkUnit->getType().sightRange() + 16);
      set<Unit*> bunkerInsight;
      for each (Unit *u in inRadius) 
			{
        if (u->getType() == UnitTypes::Terran_Bunker) 
				{
          Vector2 line2Bunker = Vector2(underAtkUnit->getPosition() - u->getPosition());
          Vector2 bulletSpeed = Vector2(underAtkUnit->getPosition() - b->getPosition());
          if (line2Bunker.cos(bulletSpeed) > 0.9) {
            if (b->getType() == BulletTypes::Gauss_Rifle_Hit ) 
						{
              _enBunker[u] = UnitTypes::Terran_Marine;
            } 
						else if (b->getType() == BulletTypes::C_10_Canister_Rifle_Hit) 
						{
              _enBunker[u] = UnitTypes::Terran_Ghost;
            }
						else if (b->getType() == BulletTypes::Invisible) 
						{
              _enBunker[u] = UnitTypes::Terran_Firebat;
            }

#ifdef _SCOUT_DEBUG
            Broodwar->printf("Inside Bunker is : %s", _enBunker[u].c_str());
#endif

          }
        }
      }
    }
  }
}


void ScoutController::_registerEnUnitPosition()
{
  for each (Unit* u in Broodwar->enemy()->getUnits())
  {
    int fadeTime = u->getType().isBuilding()? Config::i().TIME_BUILDING_FADE()
                                            : Config::i().TIME_MOVING_UNIT_FADE();
    // Insert into unit's list
    if (u->getType() == UnitTypes::Terran_Bunker) 
		{
      for each (Unit* inBunker in u->getLoadedUnits()) 
			{
        _eUnitPos[inBunker] =  _UnitInfo(u->getPosition(), Broodwar->getFrameCount(), fadeTime);
      }
    }
    _eUnitPos[u] =  _UnitInfo(u->getPosition(), Broodwar->getFrameCount(), fadeTime);
  }
}

void ScoutController::_fadeUnit(Unit* u)
{
  _eUnitPos.erase(u);
}

void ScoutController::_fadeUnits()
{
  for (map<Unit*, _UnitInfo>::iterator i = _eUnitPos.begin(); i != _eUnitPos.end();)
  {
    if (Broodwar->getFrameCount()-i->second.iFrame > i->second.ctFadeThres)
    {
      _eUnitPos.erase(i++);
    }
    else
    {
      i++;
    }
  }
}

list<Position>& ScoutController::getBorder(BWTA::Region* r)
{
  map<BWTA::Region*, list<Position>>::iterator bor = _border.find(r);
  if (bor != _border.end()) return bor->second;
  bor = _border.insert(bor, make_pair(r, list<Position>()));

  BWTA::Polygon b = r->getPolygon();
  for (vector<Position>::const_iterator i = b.begin(); i != b.end(); i++)
	{
    vector<Position>::const_iterator n = i + 1;
    if (n == b.end()) n = b.begin();

    int num = i->getApproxDistance(*n)/48;
    if (!num) num = 1;

    for (int j = 0; j < num; j++) 
		{
      int ix = i->x() < 64 ? -16 : (i->x() > Broodwar->mapWidth()*TILE_SIZE-96 ? Broodwar->mapWidth()*TILE_SIZE+16 : i->x());
      int nx = n->x() < 64 ? -16 : (n->x() > Broodwar->mapWidth()*TILE_SIZE-96 ? Broodwar->mapWidth()*TILE_SIZE+16 : n->x());
      int iy = i->y() < 64 ? -16 : (i->y() > Broodwar->mapHeight()*TILE_SIZE-96 ? Broodwar->mapHeight()*TILE_SIZE+16 : i->y());
      int ny = n->y() < 64 ? -16 : (n->y() > Broodwar->mapHeight()*TILE_SIZE-96 ? Broodwar->mapHeight()*TILE_SIZE+16 : n->y());
      bor->second.push_back(Position((ix*(num-j)+nx*j)/num, (iy*(num-j)+ny*j)/num));
    }
  }
  return bor->second;

}

void ScoutController::addToScoutSet( BWAPI::Unit *u )
{
  if (_scouts.find(u) == _scouts.end()) 
	{
    _scouts.insert(make_pair(u, make_pair(u->getPosition(), 1)));
    _scoutLastPositions.insert(make_pair(u, _UnitInfo(u->getPosition(), Broodwar->getFrameCount(), false, NULL)));
  }
}

void ScoutController::removeFromScoutSet( BWAPI::Unit *u )
{
  _scouts.erase(u);
  _scoutLastPositions.erase(u);
}

Vector2 ScoutController::calculatePVal( Unit* scout )
{
	BWTA::Region* curReg = BWTA::getRegion(scout->getPosition());

	//Broodwar->printf("Perimeter: %.2f, radius: %d", curReg->getPolygon().getPerimeter(), (int)(curReg->getPolygon().getPerimeter()/(2*M_PI)));

	Vector2 s;
	Vector2 en_dir;
	int enNum = 0;
	Vector2 obstacleVal;
	int obstacleNum = 0;

#ifdef _SCOUT_DEBUG
	list<Vector2> _list_u;
	list<Vector2> _list_a;
	list<pair<Position,int> >_list_o;
  _screenPos = Position(0,0);
#endif // _SCOUT_DEBUG


	// Calculate unitPVal
  for (map<Unit*, _UnitInfo>::iterator im = _eUnitPos.begin(); im != _eUnitPos.end(); im++) 
	{
    Unit* u = im->first;
    if (u->isBeingConstructed() && u->getRemainingBuildTime() > 40)
    {
      continue;
    }
		Vector2 _dir = vortexPotential(curReg->getCenter(), (u)->getPosition())*_p[0];

		Vector2 u_tmp = unitPVal(u, scout);
		UnitType ut = (u)->getType();
    if (Broodwar->self()->isEnemy((u)->getPlayer()) &&
        ut.canAttack() &&
        (!ut.isWorker() || (u)->getTarget()==scout || (u)->getOrderTarget()==scout)) 
		{
				en_dir += u_tmp;	
				enNum++;
				s += u_tmp;
#ifdef _SCOUT_DEBUG
				_list_o.push_back(make_pair((u)->getPosition(), ut.groundWeapon().maxRange()));
        _screenPos += (u)->getPosition();
#endif
		} else {
			// obstacle
			obstacleNum++;
			obstacleVal += u_tmp;
#ifdef _SCOUT_DEBUG
			_list_o.push_back(make_pair((u)->getPosition(), (ut.dimensionRight() + ut.dimensionLeft() + ut.dimensionUp() + ut.dimensionDown())/2));
#endif
		}
#ifdef _SCOUT_DEBUG
		_list_u.push_back(u_tmp*100);
#endif // _SCOUT_DEBUG
	}
	// Add back obstacle value after averaged
	if (obstacleNum) s+= obstacleVal*(1.0/obstacleNum);

#ifdef _SCOUT_DEBUG
	if (!_list_u.empty())	_unit_p[scout] = _list_u;
	if (!_list_o.empty()) _obj[scout] = _list_o;
	_list_a.push_back(s*100);
  if (enNum) _screenPos = Position(_screenPos.x()/enNum, _screenPos.y()/enNum);
#endif

	// Calculate regionPVal
	Vector2 r_tmp = regionPVal(curReg, scout);
	s += r_tmp;
#ifdef _SCOUT_DEBUG
	_list_a.push_back(r_tmp*100);
#endif

	// Calculate borderPVal
	Vector2 b_tmp = borderPVal(curReg, scout);
	s += b_tmp;
#ifdef _SCOUT_DEBUG
	_list_a.push_back(b_tmp*100);
	_all_p[scout] = _list_a;
#endif

	// Checking if enemy infront of us
	double cosEn = en_dir.cos(r_tmp);
	if ((cosEn < -0.5 && enNum >=3) || (cosEn < -0.85)) 
	{
		/*_p[0] = -_p[0];
		_p[4] = -_p[4];
		_p[6] = -_p[6];*/
    _scouts[scout].second *= -1;
	}

	return s;

}

/** 
* Unit's (building and enemy) emitted potential flow 
* Rotate an (-anpha) angle. Input is the anpha angle
*/
Vector2 ScoutController::unitPVal( BWAPI::Unit* u, BWAPI::Unit* s)
{
	UnitType ut = u->getType();
	Position p = s->getPosition();
  BWTA::Region* re = BWTA::getRegion(s->getPosition());
  Position c;
  if (re) { 
    c = re->getCenter();
    //if (!re->getBaseLocations().empty()) c = (*re->getBaseLocations().begin())->getPosition();
  }

	// Unit's radius
	int r = (ut.dimensionRight() + ut.dimensionLeft() + ut.dimensionUp() + ut.dimensionDown())/2;
	if (u->isInvincible()) {
		// Mineral and other indestructive obstacle
		if (c.getApproxDistance(p) < _p[2]) {
			return obsVortexPotential(u->getPosition(), p, c, r*r)*_p[0] + obsSourcePotential(u->getPosition(), p, c, r*r)*_p[1];;
		} else {
			return obsVortexPotential(u->getPosition(), p, c, r*r)*_p[0] - obsSourcePotential(u->getPosition(), p, c, r*r)*_p[1];;
		}

		//return vortexPotential(u->getPosition(), p)*_p[6];
  } else if(ut.isBuilding() && !u->isLifted() && !ut.canAttack() && ut != UnitTypes::Terran_Bunker){
		//Broodwar->drawCircleMap(u->getPosition().x(), u->getPosition().y(), 2, Colors::Red, true);
		//Broodwar->drawCircleMap(u->getPosition().x(), u->getPosition().y(), u->getPosition().getDistance(s->getPosition()), Colors::Orange);
		if (c.getApproxDistance(p) < _p[2]) {
			return obsVortexPotential(u->getPosition(), p, c, r*r)*_p[0] + obsSourcePotential(u->getPosition(), p, c, r*r)*_p[1];;
		} else {
			return obsVortexPotential(u->getPosition(), p, c, r*r)*_p[0] - obsSourcePotential(u->getPosition(), p, c, r*r)*_p[1];;
		}
		//return vortexPotential(u->getPosition(), p)*_p[6];
	}
  // Bunker, special case
  else if(ut == UnitTypes::Terran_Bunker && _enBunker[u] != UnitTypes::None) {
    return needlePotentialVal(u->getPosition(), p, s->getPosition(), (_enBunker[u].groundWeapon().maxRange() + 48)*(1.0/r))*_p[7]*4;
  }
	else if(Broodwar->self()->isEnemy(u->getPlayer()) && ut.canAttack() && 
		(!ut.isWorker() || u->getTarget()==s || u->getOrderTarget()==s)) {
			// If is can ttacking enemy unit or worker who aim at our scout's position
			Unit* eTarget = u->getTarget()?u->getTarget():u->getOrderTarget();
			int atkRange = ut.groundWeapon().maxRange() + 16;
			if (eTarget)
				return needlePotentialVal(u->getPosition(), p, eTarget->getPosition(), atkRange*1.0/r)*_p[7]*2.5;
			else 
				return sourcePotential(u->getPosition(), p)*_p[7]*(atkRange-16)*(1.0/r);
	} else if (!ut.canAttack() && (ut.isFlyer() || u->isBurrowed() || u->isLifted())){
		return Vector2();
	}
	// else
	if (c.getApproxDistance(p) < _p[2]) {
		return obsVortexPotential(u->getPosition(), p, c, r*r)*_p[0] + obsSourcePotential(u->getPosition(), p, c, r*r)*_p[1];;
	} else {
		return obsVortexPotential(u->getPosition(), p, c, r*r)*_p[0] - obsSourcePotential(u->getPosition(), p, c, r*r)*_p[1];;
	}
}

/** Start from center of the region and the combine of source and vortex potential flow */
Vector2 ScoutController::regionPVal(BWTA::Region* r, BWAPI::Unit* s)
{
  Position c = r->getCenter();
  //if (!r->getBaseLocations().empty()) c = (*r->getBaseLocations().begin())->getPosition();
	Position p = s->getPosition();
  int d2Center = c.getApproxDistance(p);
  _p[2] = r->getPolygon().getPerimeter()/(M_PI * 7);
#ifdef _SCOUT_DEBUG
	Vector2 vor, sor;
	list<Vector2> list_r;
	vor = vortexPotential(c,p)*_p[0];
	sor = sourcePotential(c,p)*_p[1];
	list_r.push_back(vor*100);
#endif
	if (d2Center < _p[2]) {
#ifdef _SCOUT_DEBUG
		list_r.push_back(sor*100);
		_region_p[s] = list_r;
#endif
		return vortexPotential(c,p)*_p[0] + sourcePotential(c,p)*_p[1];
	}
	else {
#ifdef _SCOUT_DEBUG
		list_r.push_back(-sor*100);
		_region_p[s] = list_r;
#endif
		return vortexPotential(c,p)*_p[0] - sourcePotential(c,p)*_p[1];
	}
}

/** Find all close points and calculate sum of potential emit from them 
* Should I change it to B(z) = z^n ???
*/
Vector2 ScoutController::borderPVal( BWTA::Region* r, BWAPI::Unit* s)
{
  const BWTA::Polygon border = r->getPolygon();
  const list<Position> detailBorder =  getBorder(r);
	Position p = s->getPosition();
	int numBorder = 0;
	Vector2 b;
  int borderCo = r->getPolygon().getPerimeter()/(M_PI * 15);
  _p[3] = borderCo > _p[3]? borderCo : _p[3];
#ifdef _SCOUT_DEBUG
	list<Vector2> list_b;
#endif
	for (list<Position>::const_iterator i = detailBorder.begin(); i != detailBorder.end(); i++) {
		if (p.getApproxDistance(*i) < _p[3]) {
      Chokepoint* chkPoint = BWTA::getNearestChokepoint(*i);
      /*if (chkPoint && i->getApproxDistance(chkPoint->getCenter()) <= chkPoint->getWidth()/2)
        continue;*/
#ifdef _SCOUT_DEBUG
			list_b.push_back(vortexPotential(*i, p)*_p[4]*100 + sourcePotential(*i, p)*_p[5]*100);
			Broodwar->drawCircleMap(i->x(), i->y(), 2, Colors::Red, true);
			//Broodwar->drawCircleMap(i->x(), i->y(), p.getApproxDistance(*i), Colors::Yellow);
			//list_b.push_back(vortexPotential(*i, p)*_p[4]*100);
#endif
			b += vortexPotential(*i, p)*_p[4] + sourcePotential(*i, p)*_p[5];
			numBorder++;
		}
	}
  for (vector<BWTA::Polygon>::const_iterator i = border.holes.begin(); i != border.holes.end(); i++) {
    for (BWTA::Polygon::const_iterator j = i->begin(); j != i->end(); j++) {
			if (p.getApproxDistance(*j) < _p[3]) {
#ifdef _SCOUT_DEBUG
				list_b.push_back(vortexPotential(*j, p)*(-_p[4])*100 + sourcePotential(*j, p)*_p[5]*100);
				//list_b.push_back(vortexPotential(*j, p)*_p[4]*100);
#endif
				b += vortexPotential(*j, p)*(-_p[4]) + sourcePotential(*j, p)*_p[5];
				numBorder++;
			}
		}
	}

#ifdef _SCOUT_DEBUG
	if(!list_b.empty()) _border_p[s] = list_b;
#endif
	return b;
}

/** V(z) = ilog(z-z_s) */
Vector2 ScoutController::vortexPotential(const BWAPI::Position& s, const BWAPI::Position& p)
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	double r2 = x*x + y*y;

	return Vector2(y/r2, -x/r2);

}

/** S(z) = log(z-z_s) */
Vector2 ScoutController::sourcePotential(const BWAPI::Position& s, const BWAPI::Position& p )
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	double r2 = 1.0*x*x + y*y;

	return Vector2(x/r2, y/r2);
}

/** Circle theorem obstacle by a vortex O(z) = -ilog(a^2/(z-Z)-conj(z_c-Z))
* s : obstacle position
* p : considered position
* c: center vortex position
*/
Vector2 ScoutController::obsVortexPotential(const Position& s, const Position& p, const Position& c, double a2)
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	int xc = c.x() - s.x();
	int yc = c.y() - s.y();
	double x2 = 1.0*x*x;
	double y2 = 1.0*y*y;
	double r2 = (x2 + y2);

	double deno = r2*(a2*a2 - 2*a2*(x*xc+y*yc) + r2*(xc*xc + yc*yc));
	
	double vx = a2*(a2*y - 2*x*y*xc - y2*yc + x2*yc)/deno;
	double vy = -a2*(a2*x - 2*x*y*yc - x2*xc + y2*xc)/deno;

	return Vector2(vx, vy);
}

/** O(z) = log(a^2/(z-Z)-conj(z_c-Z))
* s : obstacle position
* p : considered position
* c: center vortex position
*/
Vector2 ScoutController::obsSourcePotential(const Position& s, const Position& p, const Position& c, double a2)
{
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	int xc = c.x() - s.x();
	int yc = c.y() - s.y();
	double x2 = 1.0*x*x;
	double y2 = 1.0*y*y;
	double r2 = (x2 + y2);

	double deno = r2*(a2*a2 - 2*a2*(x*xc+y*yc) + r2*(xc*xc + yc*yc));

	double vx = -a2*(a2*x - 2*x*y*yc - x2*xc + y2*xc)/deno;
	double vy = -a2*(a2*y - 2*x*y*xc - y2*yc + x2*yc)/deno;

	return Vector2(vx, vy);
}

/* Like source/sink potential but the shape change to a direction */
Vector2 ScoutController::needlePotentialVal(const BWAPI::Position& s, 
																						const BWAPI::Position& p, 
																						const BWAPI::Position& target, 
																						double bias)
{
	if (target == Positions::None) return Vector2();
	int x = p.x() - s.x();
	int y = p.y() - s.y();
	double r2 = 1.0*x*x + 1.0*y*y;

	Vector2 V(target.x()-s.x(), target.y()-s.y());
	Vector2 v(x, y);
	Vector2 r(x/r2, y/r2);

	// cos(anpha)
	double cosn = 1/bias;
	// cos(theta)
	double cost = V.cos(v);

	if (cost >= cosn) {
		double sinn =sqrt(1-cosn*cosn);
		double sint = V.sin(v);
		double t = 1;
		if (sint >= 0) {
			// 1/cos(anpha - theta)
			r *= (1/(cosn*cost + sinn*sint));
		} else {
			// 1/cos(-anpha - theta)
			r *= 1/(cosn*cost - sinn*sint);
		}
	}

	return r;
}

Vector2 ScoutController::rotateAroundBuilding( BWAPI::Unit* scout )
{
	BWTA::Region* curReg = BWTA::getRegion(scout->getPosition());
	set<Unit*> insightUnits = scout->getUnitsInRadius(scout->getType().sightRange()+96);
	Vector2 _dir;

	for each (Unit* u in insightUnits) {
		//if (u->getType().isBuilding()) {
			_dir += Vector2(u->getPosition() - scout->getPosition()).inv();
		//}
	}

	if (!!_dir) {
		_dir += curReg->getCenter() - scout->getPosition();
	} else {
		_dir.rotate(cos(M_PI*80/180), sin(M_PI*80/180));
	}
	return _dir;
}



