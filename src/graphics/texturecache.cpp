#include "graphics/texturecache.hpp"
#include "graphics/texture.hpp"
#include <string>
#include <map>
#include <memory>

namespace BlueBear {
  namespace Graphics {

    std::shared_ptr< Texture > TextureCache::get( const std::string& path ) {
      auto textureIterator = textureCache.find( path );

      if( textureIterator != textureCache.end() ) {
        return textureIterator->second;
      }

      // Value needs to be created
      // This should throw an exception if it fails
      textureCache[ path ] = std::make_shared< Texture >( path );
      return textureCache[ path ];
    }

    /**
     * Prune all shared_ptrs in the map that only have one count
     */
    void TextureCache::prune() {
      auto iterator = std::begin( textureCache );

      while( iterator != std::end( textureCache ) ) {
        if( iterator->second.unique() ) {
          iterator = textureCache.erase( iterator );
        } else {
          iterator++;
        }
      }
    }

  }
}
