#pragma once
#include <string>

enum class EInputActionState
{
	None = 0,
	JustPressed,
	Pressed,
	Released,

	InputActionStateCount
};

using EInputAction = std::string;

struct InputAction
{
	EInputAction m_Action{ };
	EInputActionState m_ActionTriggerState{ EInputActionState::Pressed };
	bool m_Active{ false };

	InputAction(EInputAction action_, EInputActionState activeState_ = EInputActionState::Pressed) :
		m_Action(action_), m_ActionTriggerState(activeState_) { }
};
