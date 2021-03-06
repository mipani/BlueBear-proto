#ifndef TEXTURE_ATLAS
#define TEXTURE_ATLAS

#include "exceptions/genexc.hpp"
#include <SFML/Graphics/Image.hpp>
#include <glm/glm.hpp>
#include <optional>
#include <utility>
#include <vector>
#include <string>
#include <memory>

namespace BlueBear::Graphics::Utilities {

  class TextureAtlas {
    std::vector< std::pair< std::string, std::shared_ptr< sf::Image > > > stored;

    std::optional< std::pair< std::string, std::shared_ptr< sf::Image > > > getPairById( const std::string& id ) const;
    glm::uvec2 computeTotalDimensions() const;

  public:
    EXCEPTION_TYPE( IdNotFoundException, "Texture id not found in atlas!" );

    struct TextureData {
      glm::vec2 lowerCorner;
      glm::vec2 upperCorner;
    };

    void addTexture( const std::string& id, std::shared_ptr< sf::Image > image );
    TextureData getTextureData( const std::string& id ) const;
    std::shared_ptr< sf::Image > generateAtlas();
  };

}

#endif
