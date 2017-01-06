#include "graphics/animation.hpp"
#include "log.hpp"
#include <string>
#include <glm/ext.hpp>

namespace BlueBear {
  namespace Graphics {

    Animation::Animation( double rate, double duration, const glm::mat4& inverseBase ) : rate( rate ), duration( duration ), inverseBase( inverseBase ) {}

    void Animation::addKeyframe( double frame, const Transform& transform ) {
      keyframes.emplace( frame, transform );
    }

    glm::mat4 Animation::getTransformForFrame( double frame ) {
      auto pair = keyframes.find( frame );

      if( pair != keyframes.end() ) {
        // Use keyframe directly
        return inverseBase * pair->second.getUpdatedMatrix();
      } else {
        // Need to interpolate between two frames
        auto lastIterator = keyframes.upper_bound( frame );
        auto firstIterator = std::prev( lastIterator, 1 );

        Transform keyTransform = Transform::interpolate( firstIterator->second, lastIterator->second, ( ( frame - firstIterator->first ) / ( lastIterator->first - firstIterator->first ) ) );

        // Store on "frame" if we are caching interpolations
        if( cacheInterpolations ) {
          keyframes[ frame ] = keyTransform;
        }

        // Interpolate the two animations using the alpha
        return inverseBase * keyTransform.getUpdatedMatrix();
      }
    }
  }
}
