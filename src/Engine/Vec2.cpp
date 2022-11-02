#include "Vec2.h"

// Identity vectors (static definitions)
const Vec2<float> Vec2<float>::Zero = { 0.0f, 0.0f };
const Vec2<float> Vec2<float>::One = { 1.0f, 1.0f };
const Vec2<float> Vec2<float>::Up = { 0.0f, 1.0f };
const Vec2<float> Vec2<float>::Down = { 0.0f, -1.0f };
const Vec2<float> Vec2<float>::Left = { -1.0f, 0.0f };
const Vec2<float> Vec2<float>::Right = { 1.0f, 0.0f };

const Vec2<double> Vec2<double>::Zero = { 0.0, 0.0 };
const Vec2<double> Vec2<double>::One = { 1.0, 1.0 };
const Vec2<double> Vec2<double>::Up = { 0.0, 1.0 };
const Vec2<double> Vec2<double>::Down = { 0.0, -1.0 };
const Vec2<double> Vec2<double>::Left = { -1.0, 0.0 };
const Vec2<double> Vec2<double>::Right = { 1.0, 0.0 };

const Vec2<int32_t> Vec2<int32_t>::Zero = { 0, 0 };
const Vec2<int32_t> Vec2<int32_t>::One = { 1, 1 };
const Vec2<int32_t> Vec2<int32_t>::Up = { 0, 1 };
const Vec2<int32_t> Vec2<int32_t>::Down = { 0, -1 };
const Vec2<int32_t> Vec2<int32_t>::Left = { -1, 0 };
const Vec2<int32_t> Vec2<int32_t>::Right = { 1, 0 };

const Vec2<uint32_t> Vec2<uint32_t>::Zero = { 0, 0 };
const Vec2<uint32_t> Vec2<uint32_t>::One = { 1, 1 };
//warning: because they're unsigned, these make no sense:
const Vec2<uint32_t> Vec2<uint32_t>::Up = { 0, 1 };
const Vec2<uint32_t> Vec2<uint32_t>::Down = { 0, 1 };
const Vec2<uint32_t> Vec2<uint32_t>::Left = { 1, 0 };
const Vec2<uint32_t> Vec2<uint32_t>::Right = { 1, 0 };
