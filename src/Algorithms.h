#pragma once

#include <algorithm>

template <typename T>
void EraseInvalid(T& in_container) {
	in_container.erase(std::remove_if(in_container.begin(), in_container.end(), [](const auto& obj) {
		return !IsAlive(obj);
		}), in_container.end());
}
