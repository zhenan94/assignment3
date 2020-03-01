#pragma once
#include "Mob.h"
class princeTower :public Mob{
	public:
	virtual int GetMaxHealth() const { return 0; }
	virtual float GetSpeed() const { return 0.f; }
	virtual float GetSize() const { return 3.0f; }
	virtual float GetMass() const { return INFINITY; }
	virtual int GetDamage() const { return 0; }
	virtual float GetAttackTime() const { return 0.f; }
	const char* GetDisplayLetter() const { return "P"; }
};

