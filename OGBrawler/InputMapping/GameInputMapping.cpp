// SPDX-License-Identifier: BUSL-1.1
#include "OGBrawler/InputMapping/GameInputMapping.h"

namespace dInput
{
namespace gameMapping
{

const ActionDescriptor Aim         { "Aim",         ActionValueType::Axis2D };
const ActionDescriptor Move        { "Move",        ActionValueType::Axis2D };
const ActionDescriptor LeftAttack  { "LeftAttack",  ActionValueType::Boolean };
const ActionDescriptor RightAttack { "RightAttack", ActionValueType::Boolean };
const ActionDescriptor Jump        { "Jump",        ActionValueType::Boolean };
const ActionDescriptor Look        { "Look",        ActionValueType::Axis2D };
const ActionDescriptor BlockLook   { "BlockLook",   ActionValueType::Boolean };

MappingContext buildDefaultContext()
{
	MappingContext ctx;
	ctx.name = "IMC_Default";

	// Move: WASD + Gamepad Left Stick
	{
		ActionMapping mapping;
		mapping.action = &Move;
		mapping.bindings = {
			{ KeyId::Key_W, KeyModifier::SwizzleAxis },
			{ KeyId::Key_S, KeyModifier::NegateAndSwizzle },
			{ KeyId::Key_A, KeyModifier::Negate },
			{ KeyId::Key_D, KeyModifier::None },
			{ KeyId::Gamepad_LeftStick_XY, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// Aim: Gamepad Right Stick
	{
		ActionMapping mapping;
		mapping.action = &Aim;
		mapping.bindings = {
			{ KeyId::Gamepad_RightStick_XY, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// Look: Mouse XY (Negate to invert pitch axis)
	{
		ActionMapping mapping;
		mapping.action = &Look;
		mapping.bindings = {
			{ KeyId::Mouse_XY, KeyModifier::Negate },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// Jump: Space + Gamepad Face Bottom
	{
		ActionMapping mapping;
		mapping.action = &Jump;
		mapping.bindings = {
			{ KeyId::Key_Space, KeyModifier::None },
			{ KeyId::Gamepad_FaceBottom, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// LeftAttack: Mouse Left + Gamepad Left Trigger
	{
		ActionMapping mapping;
		mapping.action = &LeftAttack;
		mapping.bindings = {
			{ KeyId::Mouse_Left, KeyModifier::None },
			{ KeyId::Gamepad_LeftTrigger, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// RightAttack: Mouse Right + Gamepad Right Trigger
	{
		ActionMapping mapping;
		mapping.action = &RightAttack;
		mapping.bindings = {
			{ KeyId::Mouse_Right, KeyModifier::None },
			{ KeyId::Gamepad_RightTrigger, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// BlockLook: Left Shift + Gamepad Left Trigger Axis
	{
		ActionMapping mapping;
		mapping.action = &BlockLook;
		mapping.bindings = {
			{ KeyId::Key_LeftShift, KeyModifier::None },
			{ KeyId::Gamepad_LeftTriggerAxis, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	return ctx;
}

} // namespace gameMapping
} // namespace dInput
