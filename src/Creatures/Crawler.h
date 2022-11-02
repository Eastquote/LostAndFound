#pragma once

#include "Creature.h"

// Crawler creature (walks steadily along floors/walls/ceilings)
class Crawler : public Creature {
	DECLARE_SPAWNABLE_TYPE(Crawler);
public:
	Crawler(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	Vec2f SetFloorDir(const std::string& in_dir);
	std::string GetFloorDir(bool in_bIsInitial = true);

protected:
	virtual Task<> ManageAI() override;
	virtual void ExplodeAndDie() override;
	void ConfigSprites(std::string in_animName, std::string in_animNameLit,
		bool in_bFlipVert, bool in_bFlipHori, bool in_bFlipVertLit, bool in_bFlipHoriLit);

private:
	std::string m_floorDir = "Down";
};
