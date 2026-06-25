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
const ActionDescriptor HoldGuard   { "HoldGuard",   ActionValueType::Boolean };
const ActionDescriptor SetSchemeCameraRelative  { "SetSchemeCameraRelative",  ActionValueType::Boolean };
const ActionDescriptor SetSchemeAimRelative     { "SetSchemeAimRelative",     ActionValueType::Boolean };
const ActionDescriptor SetSchemeMoveRelativeAim { "SetSchemeMoveRelativeAim", ActionValueType::Boolean };

MappingContext buildDefaultContext()
{
	MappingContext ctx;
	ctx.name = "IMC_Default";

	// Move: WASD + Gamepad Left Stick + Gamepad D-pad
	// The D-pad mirrors the WASD modifier shapes exactly: Up=SwizzleAxis (Y+1),
	// Down=NegateAndSwizzle (Y-1), Left=Negate (X-1), Right=None (X+1). Each D-pad
	// direction is a digital duplicate of the left-stick Move; diagonals aggregate
	// through Enhanced Input the same way W+D etc. do.
	{
		ActionMapping mapping;
		mapping.action = &Move;
		mapping.bindings = {
			{ KeyId::Key_W, KeyModifier::SwizzleAxis },
			{ KeyId::Key_S, KeyModifier::NegateAndSwizzle },
			{ KeyId::Key_A, KeyModifier::Negate },
			{ KeyId::Key_D, KeyModifier::None },
			{ KeyId::Gamepad_LeftStick_XY, KeyModifier::None },
			{ KeyId::Gamepad_DPad_Up, KeyModifier::SwizzleAxis },
			{ KeyId::Gamepad_DPad_Down, KeyModifier::NegateAndSwizzle },
			{ KeyId::Gamepad_DPad_Left, KeyModifier::Negate },
			{ KeyId::Gamepad_DPad_Right, KeyModifier::None },
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

	// LeftAttack: Mouse Left + Gamepad Right Shoulder (RB)
	// RB chosen over LT because LeftShoulder is taken by HoldGuard and the
	// LT axis form is taken by BlockLook; RightShoulder was previously unbound.
	// Gamepad_LeftTrigger is intentionally left unbound (freed for future use).
	{
		ActionMapping mapping;
		mapping.action = &LeftAttack;
		mapping.bindings = {
			{ KeyId::Mouse_Left, KeyModifier::None },
			{ KeyId::Gamepad_RightShoulder, KeyModifier::None },
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

	// BlockLook: Left Alt + Gamepad Left Trigger Axis
	{
		ActionMapping mapping;
		mapping.action = &BlockLook;
		mapping.bindings = {
			{ KeyId::Key_LeftAlt, KeyModifier::None },
			{ KeyId::Gamepad_LeftTriggerAxis, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// HoldGuard: Left Shift + Gamepad Left Bumper.
	// Distinct physical inputs from BlockLook so the two semantics don't overlap on the
	// same key — HoldGuard gates combat-stance behavior (freezes CMC movement so the
	// character roots), while BlockLook owns cursor/mouse-aim suppression.
	{
		ActionMapping mapping;
		mapping.action = &HoldGuard;
		mapping.bindings = {
			{ KeyId::Key_LeftShift, KeyModifier::None },
			{ KeyId::Gamepad_LeftShoulder, KeyModifier::None },
		};
		ctx.actionMappings.push_back(std::move(mapping));
	}

	// Movement-scheme switch shortcuts: 7 = CameraRelative, 8 = AimRelative,
	// 9 = MoveRelativeAim. Dev convenience for hot-swapping schemes in PIE.
	{
		ActionMapping mapping;
		mapping.action = &SetSchemeCameraRelative;
		mapping.bindings = { { KeyId::Key_7, KeyModifier::None } };
		ctx.actionMappings.push_back(std::move(mapping));
	}
	{
		ActionMapping mapping;
		mapping.action = &SetSchemeAimRelative;
		mapping.bindings = { { KeyId::Key_8, KeyModifier::None } };
		ctx.actionMappings.push_back(std::move(mapping));
	}
	{
		ActionMapping mapping;
		mapping.action = &SetSchemeMoveRelativeAim;
		mapping.bindings = { { KeyId::Key_9, KeyModifier::None } };
		ctx.actionMappings.push_back(std::move(mapping));
	}

	return ctx;
}

} // namespace gameMapping
} // namespace dInput
