//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2024, Cinesite VFX Ltd. All rights reserved.
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

#include "MaterialCache.h"

#include "IECoreRenderMan/ShaderNetworkAlgo.h"

using namespace std;
using namespace IECore;
using namespace IECoreScene;
using namespace IECoreRenderMan;

namespace
{

const RtUString g_shadowSubset( "shadowSubset" );

} // namespace

MaterialCache::MaterialCache( Session *session )
	:	m_session( session )
{
}

ConstMaterialPtr MaterialCache::getMaterial( const IECoreScene::ShaderNetwork *network )
{
	Cache::accessor a;
	m_cache.insert( a, network->Object::hash() );
	if( !a->second )
	{
		std::vector<riley::ShadingNode> nodes = ShaderNetworkAlgo::convert( network );
		riley::MaterialId id = m_session->riley->CreateMaterial( riley::UserId(), { (uint32_t)nodes.size(), nodes.data() }, RtParamList() );
		a->second = new Material( id, m_session );
	}
	return a->second;
}

ConstDisplacementPtr MaterialCache::getDisplacement( const IECoreScene::ShaderNetwork *network )
{
	DisplacementCache::accessor a;
	m_displacementCache.insert( a, network->Object::hash() );
	if( !a->second )
	{
		std::vector<riley::ShadingNode> nodes = ShaderNetworkAlgo::convert( network );
		riley::DisplacementId id = m_session->riley->CreateDisplacement( riley::UserId(), { (uint32_t)nodes.size(), nodes.data() }, RtParamList() );
		a->second = new Displacement( id, m_session );
	}
	return a->second;
}

ConstLightShaderPtr MaterialCache::getLightShader( const IECoreScene::ShaderNetwork *network, const IECoreScene::ShaderNetwork *lightFilter, RtUString shadowSubset )
{
	auto convert = [&] {

		std::vector<riley::ShadingNode> nodes = ShaderNetworkAlgo::convert( network );
		if( nodes.size() && !shadowSubset.Empty() )
		{
			nodes.back().params.SetString( g_shadowSubset, shadowSubset );
		}
		std::vector<riley::ShadingNode> filterNodes;
		if( lightFilter )
		{
			filterNodes = ShaderNetworkAlgo::convert( lightFilter );
		}
		riley::LightShaderId id = m_session->createLightShader( { (uint32_t)nodes.size(), nodes.data() }, { (uint32_t)filterNodes.size(), filterNodes.data() } );
		return new LightShader( id, m_session );

	};

	if( auto *outputShader = network->outputShader() )
	{
		if( outputShader->getName() == "PxrPortalLight" )
		{
			// We can't cache portal shaders, because they are subject to last-minute edits
			// in `Session::updatePortals()`, and those edits are dependent on the light's
			// transform. Hence two lights with the same transform can't share a portal shader.
			/// \todo Refactor portal handling to be more like LightFilter handling, so the
			/// shader modifications are performed on a ShaderNetwork before being passed to
			/// `getLightShader()`.
			return convert();
		}
	}

	IECore::MurmurHash h = network->Object::hash();
	if( lightFilter )
	{
		lightFilter->hash( h );
	}
	h.append( shadowSubset.CStr() ? shadowSubset.CStr() : "" );

	LightShaderCache::accessor a;
	m_lightShaderCache.insert( a, h );
	if( !a->second )
	{
		a->second = convert();
	}
	return a->second;
}

// Must not be called concurrently with anything.
void MaterialCache::clearUnused()
{
	vector<IECore::MurmurHash> toErase;
	for( const auto &m : m_cache )
	{
		if( m.second->refCount() == 1 )
		{
			// Only one reference - this is ours, so
			// nothing outside of the cache is using the
			// shader.
			toErase.push_back( m.first );
		}
	}
	for( const auto &e : toErase )
	{
		m_cache.erase( e );
	}

	toErase.clear();
	for( const auto &m : m_displacementCache )
	{
		if( m.second->refCount() == 1 )
		{
			toErase.push_back( m.first );
		}
	}
	for( const auto &e : toErase )
	{
		m_displacementCache.erase( e );
	}

	toErase.clear();
	for( const auto &m : m_lightShaderCache )
	{
		if( m.second->refCount() == 1 )
		{
			toErase.push_back( m.first );
		}
	}
	for( const auto &e : toErase )
	{
		m_lightShaderCache.erase( e );
	}
}
