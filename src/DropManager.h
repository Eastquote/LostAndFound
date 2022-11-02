#pragma once

#include "Engine/Actor.h"

class Creature;
class Pickup;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ---- DROP MANAGER SYSTEM OVERVIEW ----
// Every creature in the game has its own "drop odds" which dictate the chances it will drop any of the game's five pickups when it
// is destroyed. This is an example of what a Creature's dropOdds might look like:
// 
//		{ 40.0f, 52.0f, 0.0f, 10.0f, 0.0f, 0.0f },	//< Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
// 
// By convention of the old 8-bit systems this behavior is based on, these dropOdds always sum to a value of 102. That means these 
// values are very close to being spawn percentages already -- for comparison, here are the actual percentages:
// 
//		{ 39.2%, 51%, 0%, 9.8%, 0%, 0% },	//< these sum to 100%, as you would expect
// 
// The game selects which pickups to spawn (or, in the Nothing case, not to spawn) by simply rolling a random float between 0.0f and
// 102.0f and then seeing which of the dropOdds ranges it falls into. So:
// 
//							 LOWER BRACKET		RANDVAL		UPPER BRACKET
//		- Rolling a 35.9f...					   (35.9f	<=	40.0f)				   ...would spawn Nothing.
//		- Rolling a 47.2f...  (40.0f			<	47.2f	<=	40.0f + 52.0f)		   ...would spawn a SmEnergy.
//		- Rolling a 100.4f... (40.0f + 52.0f	<  100.4f	<=	40.0f + 52.0f + 10.0f) ...would spawn a Grenade.
// 
// However, some drops aren't valuable to the player depending on context: Health is useless if they have full health, as are 
// Missiles if they're full up on missiles, Grenades have no value if you haven't unlocked the grenade ability yet, and so on.
// 
// So: the game will modify a creature's "base" dropOdds depending on player state, in order to *never* drop irrelevant items. It 
// accomplishes this by proportionately distributing the dropOdds of anything irrelevant back into the odds of items that are useful.
// 
// ---- EXAMPLE ----
// If, for example, the player was full up on missiles when they destroyed the creature above, then the DropManager would take that
// ~10% chance the creature had of spawning a missile pickup and proportionately distribute it into the two relevant items (Nothing 
// and SmEnergy, which now have dropodds of 43.48% and 56.52% respectively). So:
// 
//		43.48% of 10.0f = 4.348f	//< portion of Missile odds that goes to Nothing odds
//		56.52% of 10.0f = 5.652f	//< portion of Missile odds that goes to SmEnergy odds
//		
//		BASE			 MODIFIED
//		40.0f + 4.348f = 44.348f	//< modified Nothing odds
//		52.0f + 5.652f = 57.652f	//< modified SmEnergy odds
// 
// Which give us our new, modified dropodds:
// 
//		{ 44.348f, 57.652f, 0.0f, 0.0f, 0.0f, 0.0f },	// Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
// 
//		(n.b. these odds still sum to 102, which is how we know we did the math right)
// 
// Thusly, because it makes no sense to spawn Missiles right now, Nothing and SmEnergy are both a little more likely.
// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Controls which pickups spawn when enemies are destroyed
class DropManager : public Actor {
public:
	// Determines what (if any) pickup should spawn -- currently called whenever an enemy is destroyed
	bool TrySpawning(std::shared_ptr<Creature> in_instigator);

private:
	// Actually spawns one of the pickups -- called from within TrySpawning() when it returns true
	std::shared_ptr<Pickup> Spawn(int32_t in_dropItemIndex, std::shared_ptr<Creature> in_instigator);
};
