#ifndef WALLINSTANCE
#define WALLINSTANCE

#include "graphics/instance/instance.hpp"
#include "graphics/texturecache.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>
#include <unordered_map>

namespace BlueBear {
  namespace Graphics {
    class Model;

    class WallInstance : public Instance {
    public:
      using ImageMap = std::unordered_map< std::string, std::shared_ptr< sf::Image > >;

      static ImageMap imageMap;

      struct WallpaperSide {
        std::shared_ptr< sf::Image > image;
        sf::Image leftSegment;
        sf::Image rightSegment;
        std::string path;
      };

      WallInstance( const Model& model, GLuint shaderProgram, TextureCache& hostTextureCache );
      void setFrontWallpaper( const std::string& path );
      void setBackWallpaper( const std::string& path );

    private:
      TextureCache& hostTextureCache;
      WallpaperSide front;
      WallpaperSide back;

      void setWallpaper( const std::string& path, WallpaperSide& side );
      std::shared_ptr< sf::Image > getImage( const std::string& path );
    };

  }
}

#endif
