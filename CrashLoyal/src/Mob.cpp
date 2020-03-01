#include "Mob.h"
#include "Mob_Archer.h"
#include "kingTower.h"
#include <memory>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include "Building.h"
#include "Waypoint.h"
#include "GameState.h"
#include "Point.h"
#include "princeTower.h"
#include"Uuid.h"

char* Mob::previousUUID;

Mob::Mob() 
	: pos(-10000.f,-10000.f)
	, nextWaypoint(NULL)
	, targetPosition(new Point)
	, state(MobState::Moving)
	, uuid(Mob::previousUUID)
	, attackingNorth(true)
	, health(-1)
	, targetLocked(false)
	, target(NULL)
	, lastAttackTime(0)
{
}

void Mob::Init(const Point& pos, bool attackingNorth)
{
	//generate uuid for each mob
	this->uuid=Uuid().getUUid();
	health = GetMaxHealth();
	this->pos = pos;
	this->attackingNorth = attackingNorth;
	findClosestWaypoint();
	
}

std::shared_ptr<Point> Mob::getPosition() {
	return std::make_shared<Point>(this->pos);
}






bool Mob::findClosestWaypoint() {
	std::shared_ptr<Waypoint> closestWP = GameState::waypoints[0];
	float smallestDist = (std::numeric_limits<float>::max)();

	for (std::shared_ptr<Waypoint> wp : GameState::waypoints) {
		//std::shared_ptr<Waypoint> wp = GameState::waypoints[i];
		// Filter out any waypoints that are "behind" us (behind is relative to attack dir
		// Remember y=0 is in the top left
		if (attackingNorth && wp->pos.y > this->pos.y) {
			continue;
		}
		else if ((!attackingNorth) && wp->pos.y < this->pos.y) {
			continue;
		}

		float dist = this->pos.dist(wp->pos);
		if (dist < smallestDist) {
			smallestDist = dist;
			closestWP = wp;
		}
	}
	std::shared_ptr<Point> newTarget = std::shared_ptr<Point>(new Point);
	this->targetPosition->x = closestWP->pos.x;
	this->targetPosition->y = closestWP->pos.y;
	this->nextWaypoint = closestWP;
	
	return true;
}

void Mob::moveTowards(std::shared_ptr<Point> moveTarget, double elapsedTime) {
	Point movementVector;
	movementVector.x = moveTarget->x - this->pos.x;
	movementVector.y = moveTarget->y - this->pos.y;
	movementVector.normalize();
	movementVector *= (float)this->GetSpeed();
	movementVector *= (float)elapsedTime;
	pos += movementVector;
}


void Mob::findNewTarget() {
	// Find a new valid target to move towards and update this mob
	// to start pathing towards it

	if (!findAndSetAttackableMob()) { findClosestWaypoint(); }
}

// Have this mob start aiming towards the provided target
// TODO: impliment true pathfinding here
void Mob::updateMoveTarget(std::shared_ptr<Point> target) {
	this->targetPosition->x = target->x;
	this->targetPosition->y = target->y;
}

void Mob::updateMoveTarget(Point target) {
	this->targetPosition->x = target.x;
	this->targetPosition->y = target.y;
}


// Movement related
//////////////////////////////////
// Combat related

int Mob::attack(int dmg) {
	this->health -= dmg;
	return health;
}

bool Mob::findAndSetAttackableMob() {
	// Find an attackable target that's in the same quardrant as this Mob
	// If a target is found, this function returns true
	// If a target is found then this Mob is updated to start attacking it
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		if (otherMob->attackingNorth == this->attackingNorth) { continue; }
		//fix the screen size
		bool imLeft = this->pos.x < SCREEN_WIDTH/20;
		bool otherLeft = otherMob->pos.x < SCREEN_WIDTH / 20;
		bool imTop = this->pos.y < SCREEN_HEIGHT/20;
		bool otherTop = otherMob->pos.y < SCREEN_HEIGHT/20;
		if ((imLeft == otherLeft) && (imTop == otherTop)) {
			this->setAttackTarget(otherMob);
			return true;
		}
	}
	//add buildings to the target.
	float dist = INFINITY;
	std::shared_ptr<Building> build;
	 for (std::shared_ptr<Building> otherMob : GameState::buildings) {
		if (otherMob->isNorthBuilding!= this->attackingNorth) { continue; }
		//find the closet building
		float d = pow(this->getPosition()->x - otherMob->getPosition()->x, 2) + pow(this->getPosition()->y - otherMob->getPosition()->y, 2);
		if (d < dist) {
			dist = d;
			build = otherMob;
		}	
	}
	 //add building to the target 
	 if (build != NULL) {
		
		
			 this->setAttackTarget(build);
			 return true;
		
	 }
	
	return false;
}

// TODO Move this somewhere better like a utility class
int randomNumber(int minValue, int maxValue) {
	// Returns a random number between [min, max). Min is inclusive, max is not.
	return (rand() % maxValue) + minValue;
}

void Mob::setAttackTarget(std::shared_ptr<Attackable> newTarget) {
	this->state = MobState::Attacking;
	target = newTarget;
}







bool Mob::targetInRange() {
	float range = this->GetSize(); // TODO: change this for ranged units
	float totalSize = range + target->GetSize();
	//target->getPosition();
	return this->pos.insideOf(*(target->getPosition()), totalSize);
	
}
// Combat related
////////////////////////////////////////////////////////////
// Collisions

// PROJECT 3: 
//  1) return a vector of mobs that we're colliding with
//  2) handle collision with towers & river 
std::vector<std::shared_ptr<Mob>> Mob::checkCollision() {
	//check collision between mobs and buildings.
	std::vector<std::shared_ptr<Mob>> mobs;
	for (std::shared_ptr<Building> building : GameState::buildings) {
		float maxSize = (building->GetSize()+this->GetSize())/2;
		//std::cout << maxSize << std::endl;
		float x = abs(building->getPoint().x - this->getPosition()->x);
		float y = abs(building->getPoint().y - this->getPosition()->y);
		if (x <= maxSize && y <= maxSize) {
			//system("pause");
			//std::cout << "buildyes" << std::endl;
			std::shared_ptr<Mob> m;
			if (building->GetSize() == 5.0f) {
				m = std::shared_ptr<Mob>(new kingTower);
			}
			else{
				m = std::shared_ptr<Mob>(new princeTower);
			}
			m->Init(Point(building->getPoint().x, building->getPoint().y), false);
			mobs.push_back(m);
			//mobs.push_back(building);
		}
	}
	//check collision between mobs and other mobs.
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
			// don't collide with yourself
		
			if (this->sameMob(otherMob)) {
				continue;
			}
			else {
				
				float maxSize = (otherMob->GetSize() + this->GetSize())/2;
				float x = abs(otherMob->getPosition()->x - this->getPosition()->x);
				float y = abs(otherMob->getPosition()->y - this->getPosition()->y);
				if (x <= maxSize && y <= maxSize) {
					//std::cout << "yes" << std::endl;
					mobs.push_back(otherMob);
					//return otherMob;
				}
			}
			
		}
		return mobs;
	}

//a method to check collision between mobs and the river.
bool Mob::riverCollision() {
	if (this->getPosition()->y >= RIVER_TOP_Y-(this->GetSize())/2 && this->getPosition()->y <= RIVER_BOT_Y + (this->GetSize()) / 2) {
		if (this->getPosition()->x > (LEFT_BRIDGE_CENTER_X- BRIDGE_WIDTH / 2)+this->GetSize()/2&& this->getPosition()->x < (LEFT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2) - (this->GetSize()) / 2) {
					return false;
		}
		else if (this->getPosition()->x > (RIGHT_BRIDGE_CENTER_X - BRIDGE_WIDTH / 2) + this->GetSize() / 2 && this->getPosition()->x < (RIGHT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2) - (this->GetSize()) / 2) {
			return false;
		}
		return true;
	}
	
	return false;
}
//handle collision with the river.
void Mob::processriverCollision(double elapsedTime) {
	float d1 = abs(this->pos.x - LEFT_BRIDGE_CENTER_X);
	float d2 = abs(this->pos.x - RIGHT_BRIDGE_CENTER_X);
	if (d1 <= d2) {
		std::shared_ptr<Point>newPlace = std::shared_ptr<Point>(new Point(LEFT_BRIDGE_CENTER_X, this->pos.y));
		this->moveTowards(newPlace, elapsedTime);
	}
	else {
		std::shared_ptr<Point>newPlace = std::shared_ptr<Point>(new Point(RIGHT_BRIDGE_CENTER_X, this->pos.y));
		this->moveTowards(newPlace, elapsedTime);
	}	
}
//handle collision near the river.
// To solve the situation that mobs would push each others into the rivers.
bool Mob::processriversideCollision(double elapsedTime, std::shared_ptr<Point> newPlace) {
	if (!((this->getPosition()->x > (LEFT_BRIDGE_CENTER_X - BRIDGE_WIDTH / 2) + this->GetSize() / 2 && this->getPosition()->x < (LEFT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2) - (this->GetSize()) / 2) ||
		(this->getPosition()->x > (RIGHT_BRIDGE_CENTER_X - BRIDGE_WIDTH / 2) + this->GetSize() / 2 && this->getPosition()->x < (RIGHT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2) - (this->GetSize()) / 2)))
	{
		if ((newPlace->y >= RIVER_TOP_Y - (this->GetSize()) / 2 && newPlace->y <= 50)) {
			newPlace = std::shared_ptr<Point>(new Point(this->pos.x, this->pos.y - 1));
			this->moveTowards(newPlace, elapsedTime);
			return true;
		}
		else if ((newPlace->y >= 50 && newPlace->y <= RIVER_BOT_Y + (this->GetSize()) / 2)) {
			newPlace = std::shared_ptr<Point>(new Point(this->pos.x, this->pos.y + 1));
			this->moveTowards(newPlace, elapsedTime);
			return true;
		}
		else {
			return false;
		}
		
	}
	else {
		return false;
	}
}


//handle collision with the mobs and buildings.
void Mob::processCollision(std::vector<std::shared_ptr<Mob>> otherMobs, double elapsedTime) {
	// PROJECT 3: YOUR COLLISION HANDLING CODE GOES HERE
	for (int i = 0; i < otherMobs.size(); i++)
	{
		if (this->sameMob(otherMobs[i])) {
			continue;
		}
		//the situation that two mobs has the same mass and are on diffrent sides.
		else if (this->GetMass() == otherMobs[i]->GetMass() && this->attackingNorth != otherMobs[i]->attackingNorth) {
			float x = this->getPosition()->x + (this->getPosition()->x - otherMobs[i]->getPosition()->x);
			float y = this->getPosition()->y + (this->getPosition()->y - otherMobs[i]->getPosition()->y);
			std::shared_ptr<Point>newPlace = std::shared_ptr<Point>(new Point(x, y));
			if (!(processriversideCollision(elapsedTime, newPlace))) {
				this->moveTowards(newPlace, elapsedTime);
			}
		}
		//the situation that two mobs has the same mass and are on one side.
		else if (this->GetMass() == otherMobs[i]->GetMass() && this->attackingNorth == otherMobs[i]->attackingNorth) {
			float x = this->getPosition()->x + (this->getPosition()->x - otherMobs[i]->getPosition()->x);
			float y = this->getPosition()->y + (this->getPosition()->y - otherMobs[i]->getPosition()->y);
			float dist1;
			float dist2;
			if (this->target) {
				dist1 = pow(this->pos.x - this->target->getPosition()->x, 2) + pow(this->pos.y - this->target->getPosition()->y, 2);
				dist2 = pow(otherMobs[i]->getPosition()->x - this->target->getPosition()->x, 2) + pow(otherMobs[i]->getPosition()->y - this->target->getPosition()->y, 2);
			}
			else {
				dist1 = pow(this->pos.x - this->nextWaypoint->pos.x, 2) + pow(this->pos.y - this->nextWaypoint->pos.y, 2);
				dist2 = pow(otherMobs[i]->getPosition()->x - otherMobs[i]->nextWaypoint->pos.x, 2) + pow(otherMobs[i]->getPosition()->y - otherMobs[i]->nextWaypoint->pos.y, 2);
			}
			
			if (dist1 > dist2) {
				std::shared_ptr<Point>newPlace = std::shared_ptr<Point>(new Point(x, y));

				if (!(processriversideCollision(elapsedTime, newPlace))) {
					this->moveTowards(newPlace, elapsedTime);
				}
			}
		}
		//the situation that a mob has smaller mass than another.
		else if (this->GetMass() < otherMobs[i]->GetMass()) {
			float x = this->getPosition()->x + (this->getPosition()->x - otherMobs[i]->getPosition()->x);
			float y = this->getPosition()->y + (this->getPosition()->y - otherMobs[i]->getPosition()->y);
			std::shared_ptr<Point>newPlace = std::shared_ptr<Point>(new Point(x, y));
			if (!(processriversideCollision(elapsedTime, newPlace))) {
				this->moveTowards(newPlace, elapsedTime);
			}
		}
		//the situation that a mob has bigger mass than another.
		else if (this->GetMass() > otherMobs[i]->GetMass()) {
			if (!this->riverCollision())
			{
				
				if (this->target) {
					this->moveTowards(target->getPosition(), elapsedTime);
				}
				else {
					this->moveTowards(targetPosition, elapsedTime);
				}
				
			}
		}


	}

}

// Collisions
///////////////////////////////////////////////
// Procedures

void Mob::attackProcedure(double elapsedTime) {
	if (this->target == nullptr || this->target->isDead()) {
		this->targetLocked = false;
		this->target = nullptr;
		this->state = MobState::Moving;
		return;
	}

	if (targetInRange()) {

		if (this->lastAttackTime >= this->GetAttackTime()) {
			// If our last attack was longer ago than our cooldown
			this->target->attack(this->GetDamage());
			/*if (this->target->GetSize() == 5 && this->target->isDead()) {
				exit(0);
			}*/
			this->lastAttackTime = 0; // lastAttackTime is incremented in the main update function
			return;
		}
	}
	else {
		//	// If the target is not in range
		//	moveTowards(target->getPosition(), elapsedTime);
		//}

		std::vector<std::shared_ptr<Mob>> otherMobs = this->checkCollision();
		bool flag = true;
		if (!otherMobs.empty()) {
			this->processCollision(otherMobs, elapsedTime);
			flag = false;
		}
		if (this->riverCollision())
		{
			this->processriverCollision(elapsedTime);
			flag = false;
		}
		else if(flag) {
			this->moveTowards(target->getPosition(), elapsedTime);
		}

			
		
	}
}
void Mob::moveProcedure(double elapsedTime) {
	
	if (targetPosition) {
		

		// Check for collisions
		if (this->nextWaypoint->pos.insideOf(this->pos, (this->GetSize() + WAYPOINT_SIZE))) {
			std::shared_ptr<Waypoint> trueNextWP = this->attackingNorth ?
												   this->nextWaypoint->upNeighbor :
												   this->nextWaypoint->downNeighbor;
			setNewWaypoint(trueNextWP);
		}
		
		// PROJECT 3: You should not change this code very much, but this is where your 
		// collision code will be called from
		std::vector<std::shared_ptr<Mob>> otherMobs = this->checkCollision();
		bool flag = true;
		if (!otherMobs.empty()) {
			this->processCollision(otherMobs, elapsedTime);
			flag = false;
		}
		
		if(this->riverCollision())
		{
			this->processriverCollision(elapsedTime);
			flag = false;
		}
		else if(flag) {
			this->moveTowards(targetPosition, elapsedTime);
			
		}

		// Fighting otherMob takes priority always
		if (!findAndSetAttackableMob()) {
			this->findClosestWaypoint();
		}
		//findAndSetAttackableMob();


	} else {
		// if targetPosition is nullptr
		findNewTarget();
	}
}

void Mob::update(double elapsedTime) {
	//try to update the target so they can attack new closer mobs.
	this->findAndSetAttackableMob();
	switch (this->state) {
	case MobState::Attacking:
		this->attackProcedure(elapsedTime);

		break;
	case MobState::Moving:
	//default:
		this->moveProcedure(elapsedTime);
		break;
	}

	this->lastAttackTime += (float)elapsedTime;
}
