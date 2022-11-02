#pragma once

#include "GameEnums.h"
#include "Engine/MathCore.h"
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <array>

// A struct that stores a queryable collision distance in all four cardinal directions
struct Cardinals {
	float threshold = 0.0f;
	// Initialize cardinal array to -1.0f defaults (no sweep result)
	std::array<float, 4> cardinals = {
		-1.0f, //< Up
		-1.0f, //< Down
		-1.0f, //< Left
		-1.0f  //< Right
	};
	// Query whether any cardinals are "blocked" (i.e. distance to collision < distance of sweep)
	bool Up() const {
		return cardinals[eDir::Up] < threshold;
	}
	bool Down() const {
		return cardinals[eDir::Down] < threshold;
	}
	bool Left() const {
		return cardinals[eDir::Left] < threshold;
	}
	bool Right() const {
		return cardinals[eDir::Right] < threshold;
	}
	// Query distance to point of collision (if no collision, returns -1.0f)
	float UpDist() const {
		return cardinals[eDir::Up];
	}
	float DownDist() const {
		return cardinals[eDir::Down];
	}
	float LeftDist() const {
		return cardinals[eDir::Left];
	}
	float RightDist() const {
		return cardinals[eDir::Right];
	}
	// Returns cardinal direction with the smallest value (i.e. shortest distance to a surface)
	eDir Min() {
		eDir min = eDir::Up;
		std::array<eDir, 4> dirs = {eDir::Up, eDir::Down, eDir::Left, eDir::Right};
		for(auto dir : dirs) {
			if(cardinals[dir] != -1.0f && cardinals[dir] < cardinals[min]) {
				min = dir;
			}
		}
		return min;
	}
};
