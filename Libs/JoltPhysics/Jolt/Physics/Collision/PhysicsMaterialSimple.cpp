// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <../JoltPhysics/Jolt/Jolt.h>

#include <../JoltPhysics/Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <../JoltPhysics/Jolt/ObjectStream/TypeDeclarations.h>
#include <../JoltPhysics/Jolt/Core/StreamIn.h>
#include <../JoltPhysics/Jolt/Core/StreamOut.h>

JPH_NAMESPACE_BEGIN

JPH_IMPLEMENT_SERIALIZABLE_VIRTUAL(PhysicsMaterialSimple)
{
	JPH_ADD_BASE_CLASS(PhysicsMaterialSimple, PhysicsMaterial)

	JPH_ADD_ATTRIBUTE(PhysicsMaterialSimple, mDebugName)
	JPH_ADD_ATTRIBUTE(PhysicsMaterialSimple, mDebugColor)
}

void PhysicsMaterialSimple::SaveBinaryState(StreamOut &inStream) const
{
	PhysicsMaterial::SaveBinaryState(inStream);

	inStream.Write(mDebugName);
	inStream.Write(mDebugColor);
}

void PhysicsMaterialSimple::RestoreBinaryState(StreamIn &inStream)
{
	PhysicsMaterial::RestoreBinaryState(inStream);

	inStream.Read(mDebugName);
	inStream.Read(mDebugColor);
}

JPH_NAMESPACE_END
