#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include "OGSimulation/InputMapping/InputActionDescriptor.h"

namespace dInput
{
namespace gameMapping
{

// Action descriptors — addresses are used as identity keys by the translator.
// Declared extern so a single instance lives in the DAttack DLL; avoids
// duplicate addresses across module (DLL) boundaries.
extern OGBRAWLER_API const ActionDescriptor Aim;
extern OGBRAWLER_API const ActionDescriptor Move;
extern OGBRAWLER_API const ActionDescriptor LeftAttack;
extern OGBRAWLER_API const ActionDescriptor RightAttack;
extern OGBRAWLER_API const ActionDescriptor Jump;
extern OGBRAWLER_API const ActionDescriptor Look;
extern OGBRAWLER_API const ActionDescriptor BlockLook;
extern OGBRAWLER_API const ActionDescriptor HoldGuard;
// Movement-scheme switch shortcuts — dev convenience, bound to digit keys 7/8/9.
extern OGBRAWLER_API const ActionDescriptor SetSchemeCameraRelative;
extern OGBRAWLER_API const ActionDescriptor SetSchemeAimRelative;
extern OGBRAWLER_API const ActionDescriptor SetSchemeMoveRelativeAim;

OGBRAWLER_API MappingContext buildDefaultContext();

} // namespace gameMapping
} // namespace dInput
