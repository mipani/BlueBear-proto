#ifndef BB_WALL_MODEL_LOADER
#define BB_WALL_MODEL_LOADER

#include "graphics/scenegraph/modelloader/proceduralmodelloader.hpp"
#include "models/infrastructure.hpp"
#include <SFML/Graphics/Image.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace BlueBear::Graphics::SceneGraph { class Model; }
namespace BlueBear::Graphics::SceneGraph::ModelLoader {

  class WallModelLoader : public ProceduralModelLoader {
    const std::vector< Models::Infrastructure::FloorLevel >& levels;

    sf::Image generateTexture( const Models::Infrastructure::FloorLevel& currentLevel );
    std::vector< std::vector< Models::WallJoint > > getJointMap(
      const glm::uvec2& dimensions,
      const std::vector< std::pair< glm::uvec2, glm::uvec2 > >& corners
    );

  public:
    WallModelLoader( const std::vector< Models::Infrastructure::FloorLevel >& levels );

    std::shared_ptr< Model > get() override;
  };

}

#endif
