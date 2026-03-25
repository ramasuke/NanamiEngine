// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <../JoltPhysics/Jolt/Jolt.h>

#include <../JoltPhysics/Jolt/Physics/Vehicle/VehicleController.h>
#include <../JoltPhysics/Jolt/ObjectStream/TypeDeclarations.h>

JPH_NAMESPACE_BEGIN

JPH_IMPLEMENT_SERIALIZABLE_ABSTRACT(VehicleControllerSettings)
{
	JPH_ADD_BASE_CLASS(VehicleControllerSettings, SerializableObject)
}

JPH_NAMESPACE_END
