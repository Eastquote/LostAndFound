#pragma once

#include "Object.h"
#include "FunctionGuard.h"

template <typename T>
auto Guard(T t)
{
	return MakeFnGuard(t);
}
template <typename T>
auto Guard(std::shared_ptr<T> t)
{
	return MakeObjectGuard(t);
}
