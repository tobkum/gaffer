//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2022, John Haddon. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#ifndef GAFFERVDB_POINTSTOLEVELSET_H
#define GAFFERVDB_POINTSTOLEVELSET_H

#include "GafferVDB/Export.h"
#include "GafferVDB/TypeIds.h"

#include "GafferScene/ObjectProcessor.h"

#include "Gaffer/NumericPlug.h"

namespace Gaffer
{
class StringPlug;
}

namespace GafferVDB
{

class GAFFERVDB_API PointsToLevelSet : public GafferScene::ObjectProcessor
{

	public :

		PointsToLevelSet( const std::string &name=defaultName<PointsToLevelSet>() );
		~PointsToLevelSet() override;

		GAFFER_NODE_DECLARE_TYPE( GafferVDB::PointsToLevelSet, PointsToLevelSetTypeId, GafferScene::ObjectProcessor );

		Gaffer::StringPlug *widthPlug();
		const Gaffer::StringPlug *widthPlug() const;

		Gaffer::FloatPlug *widthScalePlug();
		const Gaffer::FloatPlug *widthScalePlug() const;

		Gaffer::BoolPlug *useVelocityPlug();
		const Gaffer::BoolPlug *useVelocityPlug() const;

		Gaffer::StringPlug *velocityPlug();
		const Gaffer::StringPlug *velocityPlug() const;

		Gaffer::FloatPlug *velocityScalePlug();
		const Gaffer::FloatPlug *velocityScalePlug() const;

		Gaffer::StringPlug *gridPlug();
		const Gaffer::StringPlug *gridPlug() const;

		Gaffer::FloatPlug *voxelSizePlug();
		const Gaffer::FloatPlug *voxelSizePlug() const;

		Gaffer::FloatPlug *halfBandwidthPlug();
		const Gaffer::FloatPlug *halfBandwidthPlug() const;

	protected :

		bool affectsProcessedObject( const Gaffer::Plug *plug ) const override;
		void hashProcessedObject( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const override;
		IECore::ConstObjectPtr computeProcessedObject( const ScenePath &path, const Gaffer::Context *context, const IECore::Object *inputObject ) const override;
		Gaffer::ValuePlug::CachePolicy processedObjectComputeCachePolicy() const override;

	private :

		static size_t g_firstPlugIndex;

};

IE_CORE_DECLAREPTR( PointsToLevelSet )

} // namespace GafferVDB

#endif // GAFFERVDB_POINTSTOLEVELSET_H
