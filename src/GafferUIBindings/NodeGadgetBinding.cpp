//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011-2012, John Haddon. All rights reserved.
//  Copyright (c) 2011-2013, Image Engine Design Inc. All rights reserved.
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

#include "boost/python.hpp"

#include "Gaffer/Plug.h"
#include "GafferBindings/ExceptionAlgo.h"
#include "GafferBindings/SignalBinding.h"

#include "GafferUI/NodeGadget.h"
#include "GafferUI/Nodule.h"

#include "GafferUIBindings/NodeGadgetBinding.h"
#include "GafferUIBindings/GadgetBinding.h"

using namespace boost::python;
using namespace IECorePython;
using namespace GafferBindings;
using namespace GafferUI;
using namespace GafferUIBindings;

namespace
{

struct NoduleSlotCaller
{
	boost::signals::detail::unusable operator()( boost::python::object slot, NodeGadget *nodeGadget, Nodule *nodule )
	{
		try
		{
			slot( NodeGadgetPtr( nodeGadget ), NodulePtr( nodule ) );
		}
		catch( const error_already_set &e )
		{
			ExceptionAlgo::translatePythonException();
		}
		return boost::signals::detail::unusable();
	}
};

struct NodeGadgetCreator
{
	NodeGadgetCreator( object fn )
		:	m_fn( fn )
	{
	}

	NodeGadgetPtr operator()( Gaffer::NodePtr node )
	{
		IECorePython::ScopedGILLock gilLock;
		NodeGadgetPtr result = extract<NodeGadgetPtr>( m_fn( node ) );
		return result;
	}

	private :

		object m_fn;

};

void registerNodeGadget1( IECore::TypeId nodeType, object creator )
{
	NodeGadget::registerNodeGadget( nodeType, NodeGadgetCreator( creator ) );
}

void registerNodeGadget2( const std::string &nodeGadgetType, object creator, IECore::TypeId nodeType )
{
	NodeGadget::registerNodeGadget( nodeGadgetType, NodeGadgetCreator( creator ), nodeType );
}

} // namespace

void GafferUIBindings::bindNodeGadget()
{
	typedef NodeGadgetWrapper<NodeGadget> Wrapper;

	NodeGadgetClass<NodeGadget, Wrapper>()
		.def( "node", (Gaffer::Node *(NodeGadget::*)())&NodeGadget::node, return_value_policy<CastToIntrusivePtr>() )
		.def( "noduleAddedSignal", &NodeGadget::noduleAddedSignal, return_internal_reference<1>() )
		.def( "noduleRemovedSignal", &NodeGadget::noduleRemovedSignal, return_internal_reference<1>() )
		.def( "create", &NodeGadget::create ).staticmethod( "create" )
		.def( "registerNodeGadget", &registerNodeGadget1 )
		.def( "registerNodeGadget", &registerNodeGadget2,
			(
				arg( "nodeGadgetType" ),
				arg( "creator" ),
				arg( "nodeType" ) = IECore::InvalidTypeId
			)
		)
		.staticmethod( "registerNodeGadget" )
	;

	SignalClass<NodeGadget::NoduleSignal, DefaultSignalCaller<NodeGadget::NoduleSignal>, NoduleSlotCaller >( "NoduleSignal" );

}
