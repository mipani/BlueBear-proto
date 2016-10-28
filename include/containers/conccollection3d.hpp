#ifndef CONCCOLLECTION3D
#define CONCCOLLECTION3D

#include "containers/collection3d.hpp"
#include "threading/lockable.hpp"
#include <mutex>
#include <vector>
#include <memory>
#include <utility>
#include <functional>

namespace BlueBear {
  namespace Containers {

    /**
     * A ConcCollection3D protects both its own collection, and the objects within.
     */
    template < typename T > class ConcCollection3D : public Collection3D< T > {

    private:
      std::mutex vectorMutex;
      std::mutex dimensionMutex;

    public:
      ConcCollection3D( unsigned int levels, unsigned int x, unsigned int y ) : Collection3D< T >::Collection3D( levels, x, y ) {}

      // Expose these via superclass call + lock

      typename Collection3D< T >::Dimensions getDimensions() {
        std::unique_lock< std::mutex > lock( dimensionMutex );
        return Collection3D< T >::getDimensions();
      }

      T& getItemDirectByRef( unsigned int direct ) {
        std::unique_lock< std::mutex > lock( vectorMutex );
        return Collection3D< T >::getItemDirectByRef( direct );
      }

      void clear() {
        std::unique_lock< std::mutex > lock( vectorMutex );
        Collection3D< T >::clear();
      }

      unsigned int getLength() {
        std::unique_lock< std::mutex > lock( vectorMutex );
        return Collection3D< T >::getLength();
      }

      void pushDirect( T item ) {
        std::unique_lock< std::mutex > lock( vectorMutex );
        Collection3D< T >::pushDirect( item );
      }

      void moveDirect( T item ) {
        std::unique_lock< std::mutex > lock( vectorMutex );
        Collection3D< T >::getItems().push_back( std::move( item ) );
      }

    };
  }
}

#endif