#include "graphics/animplayer.hpp"
#include "graphics/keyframebundle.hpp"
#include "graphics/armature/armature.hpp"
#include "tools/utility.hpp"
#include "configmanager.hpp"
#include "log.hpp"
#include <glm/ext.hpp>

namespace BlueBear {
  namespace Graphics {

    // Currently configured for a fixed FPS
    AnimPlayer::AnimPlayer( Animation& animation ) :
      animation( animation ),
      interval( animation.frameRate / ConfigManager::getInstance().getIntValue( "fps_overview" ) ) {}

    void AnimPlayer::reset() {
      step = 0.0;
    }

    /**
     * AnimPlayer should "suicide" once this is completed (from instance)
     */
    std::shared_ptr< Armature > AnimPlayer::generateNextFrame( std::shared_ptr< Armature > bindPose ) {
      step += interval;

      if( step <= animation.duration ) {
        // Generate next frame

        std::shared_ptr< Armature > newPose = std::make_shared< Armature >( *bindPose );

        // Spray the replacement keyframes onto the former bind pose
        for( auto& pair : animation.keyframes ) {
          newPose->replaceMatrix( pair.first, pair.second.getTransformForFrame( step ).getUpdatedMatrix() );
        }

        return newPose;
      } else {
        return bindPose;
      }
    }
  }
}
