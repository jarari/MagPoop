#pragma once

inline RE::NiMatrix3 GetRotationMatrix33(float a_pitch, float a_yaw, float a_roll)
{
	RE::NiMatrix3 roll;
	roll.entry[0] = { std::cos(a_roll), -std::sin(a_roll), 0.0F, 0.0F };
	roll.entry[1] = { std::sin(a_roll), std::cos(a_roll), 0.0F, 0.0F };
	roll.entry[2] = { 0.0F, 0.0F, 1.0F, 0.0F };
	RE::NiMatrix3 yaw;
	yaw.entry[0] = { 1.0F, 0.0F, 0.0F, 0.0F };
	yaw.entry[1] = { 0.0F, std::cos(a_yaw), -std::sin(a_yaw), 0.0F };
	yaw.entry[2] = { 0.0F, std::sin(a_yaw), std::cos(a_yaw), 0.0F };
	RE::NiMatrix3 pitch;
	pitch.entry[0] = { std::cos(a_pitch), 0.0F, std::sin(a_pitch), 0.0F };
	pitch.entry[1] = { 0.0F, 1.0F, 0.0F, 0.0F };
	pitch.entry[2] = { -std::sin(a_pitch), 0.0F, std::cos(a_pitch), 0.0F };
	return roll * pitch * yaw;
}
