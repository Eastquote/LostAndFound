#pragma once

#include "GameActor.h"

/* These spawn in the location where a destroyable tile is "destroyed" -- they are essentially a sprite class that spawns and 
plays an anim sequence before restoring the original destroyable tile art/collision in their location and finally self-destructing*/
class DestroyedTile : public GameActor {
public:
	DestroyedTile(int32_t in_tileID);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

private:
	std::shared_ptr<SpriteComponent> m_sprite;
	int32_t m_tileId = 0;
};