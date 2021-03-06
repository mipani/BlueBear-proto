#include "graphics/userinterface/widgets/layout.hpp"
#include "graphics/userinterface/propertylist.hpp"
#include <algorithm>
#include <functional>

#include "log.hpp"

namespace BlueBear {
  namespace Graphics {
    namespace UserInterface {
      namespace Widgets {

        Layout::Layout( const std::string& id, const std::vector< std::string >& classes ) : Element::Element( "Layout", id, classes ) {}

        std::shared_ptr< Layout > Layout::create( const std::string& id, const std::vector< std::string >& classes ) {
          std::shared_ptr< Layout > layout( new Layout( id, classes ) );

          return layout;
        }

        /**
         * A layout needs to propagate its reflow up to the nearest non-layout parent (if no parent, then simply reflow one's self)
         */
        void Layout::reflow( bool selectorsInvalidated ) {
          if( auto parent = getParent() ) {
            parent->reflow( selectorsInvalidated );
          } else {
            Element::reflow( selectorsInvalidated );
          }
        }

        void Layout::calculate() {
          glm::ivec2 total{ 0, 0 };
          int padding = localStyle.get< int >( "padding" );
          Gravity gravity = localStyle.get< Gravity >( "gravity" );
          bool horizontal = ( gravity == Gravity::LEFT || gravity == Gravity::RIGHT );

          // Do all children first
          cachedRequisitions.clear();
          for( std::shared_ptr< Element > child : children ) {
            child->calculate();
            cachedRequisitions[ child ] = getFinalRequisition( child );
          }

          if( horizontal ) {
            total.x = padding;
            for( auto& requisition : cachedRequisitions ) {
              total.x += requisition.second.x + padding;
            }

            unsigned maxHeight = 0;
            for( auto& requisition : cachedRequisitions ) {
              maxHeight = std::max( maxHeight, requisition.second.y );
            }
            total.y = maxHeight + ( padding * 2 );
          } else {
            unsigned int maxWidth = 0;
            for( auto& requisition : cachedRequisitions ) {
              maxWidth = std::max( maxWidth, requisition.second.x );
            }
            total.x = maxWidth + ( padding * 2 );

            total.y = padding;
            for( auto& requisition : cachedRequisitions ) {
              total.y += requisition.second.y + padding;
            }
          }

          requisition = total;
        }

        bool Layout::drawableDirty() {
          // Never dirty - don't ever draw.
          return false;
        }

        void Layout::generateDrawable() { /* Nothing to generate, don't generate. */ }

        Layout::Relations Layout::getRelations( Gravity gravity, int padding ) {
          bool xAxis = ( gravity == Gravity::LEFT || gravity == Gravity::RIGHT );

          int totalSpace = allocation[ xAxis ? 2 : 3 ] - padding;
          int totalWeight = 0;
          for( std::shared_ptr< Element > child : children ) {
            if( child->getPropertyList().get< Placement >( "placement" ) == Placement::FLOW ) {
              int layoutWeight = child->getPropertyList().get< int >( "layout-weight" );
              if( layoutWeight >= 1 ) {
                // This child will be sized by its layout proportion
                totalWeight += layoutWeight;
              } else {
                // This child will be sized using its requisition
                glm::uvec2 finalRequisition = cachedRequisitions[ child ];
                totalSpace -= ( ( xAxis ? finalRequisition.x : finalRequisition.y ) + padding );
              }
            }
          }

          return Relations {
            xAxis ? "vertical-orientation" : "horizontal-orientation",
            xAxis ? "width" : "height",
            xAxis ? "height" : "width",
            padding,

            //int rFlowSize;
            xAxis ? 0 : 1,
            //int rPerpSize;
            xAxis ? 1 : 0,

            //int aFlowPos;
            xAxis ? 0 : 1,
            //int aPerpPos;
            xAxis ? 1 : 0,
            //int aFlowSize;
            xAxis ? 2 : 3,
            //int aPerpSize;
            xAxis ? 3 : 2,

            totalSpace,
            totalWeight,
            ( xAxis ? ( int ) allocation[ 3 ] : ( int ) allocation [ 2 ] ) - ( padding * 2 )
          };
        }

        glm::uvec2 Layout::getFinalRequisition( std::shared_ptr< Element > prospect ) {
          int width = prospect->getPropertyList().get< int >( "width" );
          int height = prospect->getPropertyList().get< int >( "height" );

          return glm::uvec2{
            valueIsLiteral( width ) ? width : prospect->getRequisition().x,
            valueIsLiteral( height ) ? height : prospect->getRequisition().y
          };
        }

        /**
         * Boudnaries already defined by parent element
         */
        void Layout::positionAndSizeChildren() {
          if( getParent() == nullptr ) {
            calculate();
          }

          int padding = localStyle.get< int >( "padding" );
          Gravity gravity = localStyle.get< Gravity >( "gravity" );

          Layout::Relations relations = getRelations( gravity, padding );
          for( std::shared_ptr< Element > child : children ) {
            glm::ivec4 childAllocation;
            glm::uvec2 childRequisition = cachedRequisitions[ child ];

            if( child->getPropertyList().get< Placement >( "placement" ) == Placement::FLOW ) {
              // flow size - either a proportion derived from layout-weight or the requisition size
              int layoutWeight = child->getPropertyList().get< int >( "layout-weight" );
              if( layoutWeight >= 1 ) {
                childAllocation[ relations.aFlowSize ] = ( ( float ) ( ( float ) layoutWeight / ( float ) relations.flowTotalWeight ) * ( float ) relations.flowTotalSpace ) - padding;
              } else {
                childAllocation[ relations.aFlowSize ] = childRequisition[ relations.rFlowSize ];
              }

              // perp size - fill parent or use requisition
              childAllocation[ relations.aPerpSize ] = ( ( Requisition ) child->getPropertyList().get< int >( relations.perpProperty ) == Requisition::FILL_PARENT ) ?
                relations.perpSizeAdjusted :
                childRequisition[ relations.rPerpSize ];

              // flow position - will just be current position of the cursor. However, you need to increment the cursor by the above calculated flow dimension
              childAllocation[ relations.aFlowPos ] = relations.cursor;
              relations.cursor += ( childAllocation[ relations.aFlowSize ] + padding );

              // perp position - this is the one determined by "orientation"
              Orientation orientation = child->getPropertyList().get< Orientation >( relations.orientation );
              if( orientation == Orientation::MIDDLE ) {
                childAllocation[ relations.aPerpPos ] = ( ( relations.perpSizeAdjusted / 2 ) + padding ) - ( childAllocation[ relations.aPerpSize ] / 2 );
              } else if( orientation == Orientation::BOTTOM || orientation == Orientation::RIGHT ) {
                childAllocation[ relations.aPerpPos ] = allocation[ relations.aPerpSize ] - padding - childAllocation[ relations.aPerpSize ];
              } else {
                // left, top, as well as default
                childAllocation[ relations.aPerpPos ] = padding;
              }
            } else {
              // This element breaks the flow - use its settings directly
              int left = child->getPropertyList().get< int >( "left" );
              int top = child->getPropertyList().get< int >( "top" );

              if( !valueIsLiteral( left ) ) { left = 0; }
              if( !valueIsLiteral( top ) ) { top = 0; }

              childAllocation[ 0 ] = left;
              childAllocation[ 1 ] = top;
              childAllocation[ 2 ] = ( ( Requisition ) child->getPropertyList().get< int >( "width" ) == Requisition::FILL_PARENT ) ? allocation[ 2 ] : childRequisition.x;
              childAllocation[ 3 ] = ( ( Requisition ) child->getPropertyList().get< int >( "height" ) == Requisition::FILL_PARENT ) ? allocation[ 3 ] : childRequisition.y;
            }

            // That's everything: compute the allocation
            child->setAllocation( childAllocation, false );
          }
        }

      }
    }
  }
}
