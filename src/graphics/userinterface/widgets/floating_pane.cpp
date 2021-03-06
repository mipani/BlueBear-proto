#include "graphics/userinterface/widgets/floating_pane.hpp"
#include "graphics/userinterface/style/style.hpp"
#include "graphics/vector/renderer.hpp"
#include <glm/glm.hpp>

namespace BlueBear::Graphics::UserInterface::Widgets {

	static bool valueIsLiteral( int r ) {
		return ( Requisition ) r != Requisition::AUTO &&
			( Requisition ) r != Requisition::NONE &&
			( Requisition ) r != Requisition::FILL_PARENT;
	};

	static glm::uvec2 getFinalRequisition( std::shared_ptr< Element > prospect ) {
		int width = prospect->getPropertyList().get< int >( "width" );
		int height = prospect->getPropertyList().get< int >( "height" );

		return glm::uvec2{
			valueIsLiteral( width ) ? width : prospect->getRequisition().x,
			valueIsLiteral( height ) ? height : prospect->getRequisition().y
		};
	};

	FloatingPane::FloatingPane( const std::string& id, const std::vector< std::string >& classes ) :
		Element::Element( "FloatingPane", id, classes ) {}

	std::shared_ptr< FloatingPane > FloatingPane::create( const std::string& id, const std::vector< std::string >& classes ) {
		std::shared_ptr< FloatingPane > floatingPane( new FloatingPane( id, classes ) );

    	return floatingPane;
	}

	void FloatingPane::reflow( bool selectorsInvalidated ) {
		if( auto parent = getParent() ) {
			parent->reflow( selectorsInvalidated );
		} else {
			Element::reflow( selectorsInvalidated );
		}
	}

	void FloatingPane::positionAndSizeChildren() {
		if( getParent() == nullptr ) {
			calculate();
		}

		if( !children.empty() ) {
			std::shared_ptr< Element > child = children[ 0 ];
			children[ 0 ]->setAllocation( { 5, 5, allocation[ 2 ] - 10, allocation[ 3 ] - 10 }, false );
		}
	}

	void FloatingPane::calculate() {
		// Call calculate on all child elements per parent object's responsibility
		for( std::shared_ptr< Element > child : children ) {
			child->calculate();
		}

		int styleWidth = localStyle.get< int >( "width" );
		int styleHeight = localStyle.get< int >( "height" );

		if( ( Requisition ) styleHeight == Requisition::AUTO ) {
			// Compute styleHeight based on requested items
			styleHeight = 0;

			for( const auto child : children ) {
				styleHeight += getFinalRequisition( child ).y;
			}
		}

		if( ( Requisition ) styleWidth == Requisition::AUTO ) {
			// Compute styleWidth based on requested items
			styleWidth = 0;

			for( const auto child : children ) {
				styleWidth += getFinalRequisition( child ).x;
			}
		}

		requisition = glm::uvec2{ styleWidth, styleHeight };
	}

	void FloatingPane::render( Vector::Renderer& renderer ) {
		glm::ivec2 origin = { 5, 5 };
		glm::ivec2 dimensions = { allocation[ 2 ] - 5, allocation[ 3 ] - 5 };

		// Drop shadow
		// Left segment
		renderer.drawLinearGradient(
			{ 0, origin.y, origin.x, dimensions.y },
			{ origin.x, ( origin.y + ( ( dimensions.y - origin.y ) / 2 ) ), 0, ( origin.y + ( ( dimensions.y - origin.y ) / 2 ) ) },
			{ 0, 0, 0, 128 },
			{ 0, 0, 0, 0 }
		);
		// Top left corner
		renderer.drawScissored( { 0, 0, origin.x, origin.y }, [ & ]() {
			renderer.drawRadialGradient( origin, 0.05f, 4.95f, { 0, 0, 0, 128 }, { 0, 0, 0, 0 } );
		} );
		// Top segment
		renderer.drawLinearGradient(
			{ 5, 0, dimensions.x, 5 },
			{ ( origin.x + ( ( dimensions.x - origin.x ) / 2 ) ), 5, ( origin.x + ( ( dimensions.x - origin.x ) / 2 ) ), 0 },
			{ 0, 0, 0, 128 },
			{ 0, 0, 0, 0 }
		);
		// Top right corner
		renderer.drawScissored( { allocation[ 2 ] - origin.x, 0, allocation[ 2 ], origin.y }, [ & ]() {
			renderer.drawRadialGradient( { allocation[ 2 ] - origin.x, origin.y }, 0.05f, 4.95f, { 0, 0, 0, 128 }, { 0, 0, 0, 0 } );
		} );
		// Right segment
		renderer.drawLinearGradient(
			{ allocation[ 2 ] - origin.x, origin.y, allocation[ 2 ], dimensions.y },
			{ allocation[ 2 ] - origin.x, ( origin.y + ( ( dimensions.y - origin.y ) / 2 ) ), allocation[ 2 ], ( origin.y + ( ( dimensions.y - origin.y ) / 2 ) ) },
			{ 0, 0, 0, 128 },
			{ 0, 0, 0, 0 }
		);
		// Bottom segment
		renderer.drawLinearGradient(
			{ origin.x, dimensions.y, allocation[ 2 ] - origin.x, allocation[ 3 ] },
			{ ( ( dimensions.x - origin.x ) / 2 ), dimensions.y, ( ( dimensions.x - origin.x ) / 2 ), allocation[ 3 ] },
			{ 0, 0, 0, 128 },
			{ 0, 0, 0, 0 }
		);
		// Bottom left corner
		renderer.drawScissored( { 0, dimensions.y, origin.x, allocation[ 3 ] }, [ & ]() {
			renderer.drawRadialGradient( { origin.x, dimensions.y }, 0.05f, 4.95f, { 0, 0, 0, 128 }, { 0, 0, 0, 0 } );
		} );
		// Bottom right corner
		renderer.drawScissored( { dimensions.x, dimensions.y, allocation[ 2 ], allocation[ 3 ] }, [ & ]() {
			renderer.drawRadialGradient( { dimensions.x, dimensions.y }, 0.05f, 4.95f, { 0, 0, 0, 128 }, { 0, 0, 0, 0 } );
		} );

		// Background color
		renderer.drawRect(
			glm::uvec4{ origin.x, origin.y, dimensions.x, dimensions.y },
			localStyle.get< glm::uvec4 >( "background-color" )
		);
	}

}