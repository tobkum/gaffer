//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2012, John Haddon. All rights reserved.
//  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
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

#include "GafferScene/Attributes.h"

#include "Gaffer/Metadata.h"
#include "Gaffer/MetadataAlgo.h"
#include "Gaffer/PlugAlgo.h"

#include <boost/algorithm/string/replace.hpp>
#include "boost/bind/bind.hpp"
#include "boost/logic/tribool.hpp"

using namespace std;
using namespace boost::placeholders;
using namespace IECore;
using namespace Gaffer;
using namespace GafferScene;

namespace
{

const InternedString g_defaultValue( "defaultValue" );

} // namespace

GAFFER_NODE_DEFINE_TYPE( Attributes );

size_t Attributes::g_firstPlugIndex = 0;

Attributes::Attributes( const std::string &name )
	:	AttributeProcessor( name, PathMatcher::EveryMatch )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new CompoundDataPlug( "attributes" ) );
	addChild( new BoolPlug( "global", Plug::In, false ) );
	addChild( new CompoundObjectPlug( "extraAttributes", Plug::In, new IECore::CompoundObject ) );

	// Connect to signals we use to manage pass-throughs for globals
	// and attributes based on the value of globalPlug().
	plugSetSignal().connect( boost::bind( &Attributes::plugSet, this, ::_1 ) );
	plugInputChangedSignal().connect( boost::bind( &Attributes::plugInputChanged, this, ::_1 ) );
}


Attributes::Attributes( const std::string &name, const std::string &rendererPrefix )
	:	Attributes( name )
{
	const string targetPattern = fmt::format( "attribute:{}:*", rendererPrefix );
	for( const auto &target : Metadata::targetsWithMetadata( targetPattern, g_defaultValue ) )
	{
		if( auto valuePlug = MetadataAlgo::createPlugFromMetadata( "value", Plug::Direction::In, Plug::Flags::Default, target ) )
		{
			const std::string attributeName = target.string().substr( 10 );
			NameValuePlugPtr attributePlug = new NameValuePlug( attributeName, valuePlug, false, boost::replace_all_copy( attributeName, ".", "_" ) );
			attributesPlug()->addChild( attributePlug );
		}
	}
}

Attributes::~Attributes()
{
}

Gaffer::CompoundDataPlug *Attributes::attributesPlug()
{
	return getChild<Gaffer::CompoundDataPlug>( g_firstPlugIndex );
}

const Gaffer::CompoundDataPlug *Attributes::attributesPlug() const
{
	return getChild<Gaffer::CompoundDataPlug>( g_firstPlugIndex );
}

Gaffer::BoolPlug *Attributes::globalPlug()
{
	return getChild<Gaffer::BoolPlug>( g_firstPlugIndex + 1 );
}

const Gaffer::BoolPlug *Attributes::globalPlug() const
{
	return getChild<Gaffer::BoolPlug>( g_firstPlugIndex + 1 );
}

Gaffer::CompoundObjectPlug *Attributes::extraAttributesPlug()
{
	return getChild<Gaffer::CompoundObjectPlug>( g_firstPlugIndex + 2 );
}

const Gaffer::CompoundObjectPlug *Attributes::extraAttributesPlug() const
{
	return getChild<Gaffer::CompoundObjectPlug>( g_firstPlugIndex + 2 );
}

void Attributes::affects( const Gaffer::Plug *input, AffectedPlugsContainer &outputs ) const
{
	AttributeProcessor::affects( input, outputs );

	if(
		input == globalPlug() ||
		input == inPlug()->globalsPlug() ||
		attributesPlug()->isAncestorOf( input ) ||
		input == extraAttributesPlug()
	)
	{
		// We can only affect a particular output if we haven't
		// connected it as a pass-through in updateInternalConnections().
		if( !outPlug()->globalsPlug()->getInput() )
		{
			outputs.push_back( outPlug()->globalsPlug() );
		}
	}

}

void Attributes::hashGlobals( const Gaffer::Context *context, const ScenePlug *parent, IECore::MurmurHash &h ) const
{
	if( globalPlug()->getValue() )
	{
		// We will modify the globals.
		AttributeProcessor::hashGlobals( context, parent, h );
		inPlug()->globalsPlug()->hash( h );
		attributesPlug()->hash( h );
		extraAttributesPlug()->hash( h );
	}
	else
	{
		// We won't modify the globals - pass through the hash.
		h = inPlug()->globalsPlug()->hash();
	}
}

IECore::ConstCompoundObjectPtr Attributes::computeGlobals( const Gaffer::Context *context, const ScenePlug *parent ) const
{
	ConstCompoundObjectPtr inputGlobals = inPlug()->globalsPlug()->getValue();
	if( !globalPlug()->getValue() )
	{
		return inputGlobals;
	}

	const CompoundDataPlug *p = attributesPlug();
	IECore::CompoundObjectPtr result = new CompoundObject;
	// Since we're not going to modify any existing members (only add new ones),
	// and our result becomes const on returning it, we can directly reference
	// the input members in our result without copying. Be careful not to modify
	// them though!
	result->members() = inputGlobals->members();

	std::string name;
	for( NameValuePlug::Iterator it( p ); !it.done(); ++it )
	{
		IECore::DataPtr d = p->memberDataAndName( it->get(), name );
		if( d )
		{
			result->members()["attribute:" + name] = d;
		}
	}

	IECore::ConstCompoundObjectPtr extraAttributes = extraAttributesPlug()->getValue();
	for( const auto &e : extraAttributes->members() )
	{
		result->members()["attribute:" + e.first.string()] = e.second;
	}

	return result;
}

bool Attributes::affectsProcessedAttributes( const Gaffer::Plug *input ) const
{
	if( outPlug()->attributesPlug()->getInput() )
	{
		// We've made a pass-through connection
		return false;
	}

	return
		AttributeProcessor::affectsProcessedAttributes( input ) ||
		attributesPlug()->isAncestorOf( input ) ||
		input == globalPlug() ||
		input == extraAttributesPlug()
	;
}

void Attributes::hashProcessedAttributes( const ScenePath &path, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{
	if( ( attributesPlug()->children().empty() && extraAttributesPlug()->isSetToDefault() ) || globalPlug()->getValue() )
	{
		h = inPlug()->attributesPlug()->hash();
	}
	else
	{
		AttributeProcessor::hashProcessedAttributes( path, context, h );
		attributesPlug()->hash( h );
		extraAttributesPlug()->hash( h );
	}
}

IECore::ConstCompoundObjectPtr Attributes::computeProcessedAttributes( const ScenePath &path, const Gaffer::Context *context, const IECore::CompoundObject *inputAttributes ) const
{
	const CompoundDataPlug *ap = attributesPlug();
	IECore::ConstCompoundObjectPtr extraAttributes = extraAttributesPlug()->getValue();
	if( !ap->children().size() && extraAttributes->members().empty() )
	{
		return inputAttributes;
	}

	if( globalPlug()->getValue() )
	{
		return inputAttributes;
	}

	CompoundObjectPtr result = new CompoundObject;
	// Since we're not going to modify any existing members (only add new ones),
	// and our result becomes const on returning it, we can directly reference
	// the input members in our result without copying. Be careful not to modify
	// them though!
	result->members() = inputAttributes->members();

	ap->fillCompoundObject( result->members() );
	for( const auto &e : extraAttributes->members() )
	{
		result->members()[e.first] = e.second;
	}

	return result;
}

void Attributes::plugSet( Gaffer::Plug *plug )
{
	if( plug == globalPlug() )
	{
		updateInternalConnections();
	}
}

void Attributes::plugInputChanged( Gaffer::Plug *plug )
{
	if( plug == globalPlug() )
	{
		updateInternalConnections();
	}
}

void Attributes::updateInternalConnections()
{
	// Manage internal pass-throughs based on the value of the globalPlug().
	const Plug *p = globalPlug()->source<Gaffer::Plug>();
	boost::tribool global;
	if( p->direction() == Plug::Out && runTimeCast<const ComputeNode>( p->node() ) )
	{
		// Can vary from compute to compute.
		global = boost::indeterminate;
	}
	else
	{
		global = globalPlug()->getValue();
	}

	outPlug()->globalsPlug()->setInput(
		global || boost::indeterminate( global ) ? nullptr : inPlug()->globalsPlug()
	);
	outPlug()->attributesPlug()->setInput(
		!global || boost::indeterminate( global ) ? nullptr : inPlug()->attributesPlug()
	);
}
