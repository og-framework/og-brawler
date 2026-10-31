#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/KeyGlyphs-rationale.md

#include "OGSimulation/InputMapping/InputActionDescriptor.h"

#include <cstdint>
#include <string_view>

namespace dInput
{
namespace keyGlyphs
{

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(error: 4062)
#endif
constexpr std::string_view keyGlyph(KeyId key)
{
	switch (key)
	{
	case KeyId::Key_A: return "A";
	case KeyId::Key_B: return "B";
	case KeyId::Key_C: return "C";
	case KeyId::Key_D: return "D";
	case KeyId::Key_E: return "E";
	case KeyId::Key_F: return "F";
	case KeyId::Key_G: return "G";
	case KeyId::Key_H: return "H";
	case KeyId::Key_I: return "I";
	case KeyId::Key_J: return "J";
	case KeyId::Key_K: return "K";
	case KeyId::Key_L: return "L";
	case KeyId::Key_M: return "M";
	case KeyId::Key_N: return "N";
	case KeyId::Key_O: return "O";
	case KeyId::Key_P: return "P";
	case KeyId::Key_Q: return "Q";
	case KeyId::Key_R: return "R";
	case KeyId::Key_S: return "S";
	case KeyId::Key_T: return "T";
	case KeyId::Key_U: return "U";
	case KeyId::Key_V: return "V";
	case KeyId::Key_W: return "W";
	case KeyId::Key_X: return "X";
	case KeyId::Key_Y: return "Y";
	case KeyId::Key_Z: return "Z";
	case KeyId::Key_Space: return "Space";
	case KeyId::Key_LeftShift: return "LShift";
	case KeyId::Key_LeftControl: return "LCtrl";
	case KeyId::Key_LeftAlt: return "LAlt";
	case KeyId::Key_1: return "1";
	case KeyId::Key_2: return "2";
	case KeyId::Key_3: return "3";
	case KeyId::Key_4: return "4";
	case KeyId::Key_5: return "5";
	case KeyId::Key_6: return "6";
	case KeyId::Key_7: return "7";
	case KeyId::Key_8: return "8";
	case KeyId::Key_9: return "9";
	case KeyId::Key_0: return "0";
	case KeyId::Mouse_X: return "Mouse X";
	case KeyId::Mouse_Y: return "Mouse Y";
	case KeyId::Mouse_XY: return "Mouse";
	case KeyId::Mouse_Left: return "LMB";
	case KeyId::Mouse_Right: return "RMB";
	case KeyId::Mouse_Middle: return "MMB";
	case KeyId::Gamepad_LeftStick_XY: return "LS";
	case KeyId::Gamepad_RightStick_XY: return "RS";
	case KeyId::Gamepad_FaceBottom: return "Pad A";
	case KeyId::Gamepad_FaceRight: return "Pad B";
	case KeyId::Gamepad_FaceLeft: return "Pad X";
	case KeyId::Gamepad_FaceTop: return "Pad Y";
	case KeyId::Gamepad_DPad_Up: return "D-pad Up";
	case KeyId::Gamepad_DPad_Down: return "D-pad Down";
	case KeyId::Gamepad_DPad_Left: return "D-pad Left";
	case KeyId::Gamepad_DPad_Right: return "D-pad Right";
	case KeyId::Gamepad_LeftTrigger: return "LT";
	case KeyId::Gamepad_RightTrigger: return "RT";
	case KeyId::Gamepad_LeftTriggerAxis: return "LT axis";
	case KeyId::Gamepad_RightTriggerAxis: return "RT axis";
	case KeyId::Gamepad_LeftShoulder: return "LB";
	case KeyId::Gamepad_RightShoulder: return "RB";
	case KeyId::Gamepad_Special_Left: return "View";
	}
	return {};
}

constexpr bool isGamepadKey(KeyId key)
{
	switch (key)
	{
	case KeyId::Key_A:
	case KeyId::Key_B:
	case KeyId::Key_C:
	case KeyId::Key_D:
	case KeyId::Key_E:
	case KeyId::Key_F:
	case KeyId::Key_G:
	case KeyId::Key_H:
	case KeyId::Key_I:
	case KeyId::Key_J:
	case KeyId::Key_K:
	case KeyId::Key_L:
	case KeyId::Key_M:
	case KeyId::Key_N:
	case KeyId::Key_O:
	case KeyId::Key_P:
	case KeyId::Key_Q:
	case KeyId::Key_R:
	case KeyId::Key_S:
	case KeyId::Key_T:
	case KeyId::Key_U:
	case KeyId::Key_V:
	case KeyId::Key_W:
	case KeyId::Key_X:
	case KeyId::Key_Y:
	case KeyId::Key_Z:
	case KeyId::Key_Space:
	case KeyId::Key_LeftShift:
	case KeyId::Key_LeftControl:
	case KeyId::Key_LeftAlt:
	case KeyId::Key_1:
	case KeyId::Key_2:
	case KeyId::Key_3:
	case KeyId::Key_4:
	case KeyId::Key_5:
	case KeyId::Key_6:
	case KeyId::Key_7:
	case KeyId::Key_8:
	case KeyId::Key_9:
	case KeyId::Key_0:
	case KeyId::Mouse_X:
	case KeyId::Mouse_Y:
	case KeyId::Mouse_XY:
	case KeyId::Mouse_Left:
	case KeyId::Mouse_Right:
	case KeyId::Mouse_Middle:
		return false;
	case KeyId::Gamepad_LeftStick_XY:
	case KeyId::Gamepad_RightStick_XY:
	case KeyId::Gamepad_FaceBottom:
	case KeyId::Gamepad_FaceRight:
	case KeyId::Gamepad_FaceLeft:
	case KeyId::Gamepad_FaceTop:
	case KeyId::Gamepad_DPad_Up:
	case KeyId::Gamepad_DPad_Down:
	case KeyId::Gamepad_DPad_Left:
	case KeyId::Gamepad_DPad_Right:
	case KeyId::Gamepad_LeftTrigger:
	case KeyId::Gamepad_RightTrigger:
	case KeyId::Gamepad_LeftTriggerAxis:
	case KeyId::Gamepad_RightTriggerAxis:
	case KeyId::Gamepad_LeftShoulder:
	case KeyId::Gamepad_RightShoulder:
	case KeyId::Gamepad_Special_Left:
		return true;
	}
	return false;
}
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

inline constexpr std::uint16_t kLastKeyIdValueAsShipped = 62;

static_assert(static_cast<std::uint16_t>(KeyId::Gamepad_Special_Left) == kLastKeyIdValueAsShipped
		&& !keyGlyph(KeyId::Gamepad_Special_Left).empty()
		&& keyGlyph(static_cast<KeyId>(kLastKeyIdValueAsShipped + 1)).empty(),
	"dInput::keyGlyphs: KeyId::Gamepad_Special_Left must stay the LAST KeyId enumerator (value 62 as "
	"shipped). A new KeyId needs a glyph here in keyGlyph, a case in isGamepadKey, its row in the "
	"brawler test's kExpectedGlyphs (and in kGamepadKeys if it is a pad key; BrawlerControlsHudTextsTest.cpp) "
	"AND a case in the UE translator (InputMappingUETranslator::toUnrealKey, whose default case silently "
	"drops the binding); append it after Gamepad_Special_Left, never insert, then move this pin to the "
	"new last enumerator (the test's loops end at the pin). On MSVC C4062 is an error around keyGlyph "
	"and isGamepadKey, so an appended key without a glyph or a classification fails there first.");

} // namespace keyGlyphs
} // namespace dInput
