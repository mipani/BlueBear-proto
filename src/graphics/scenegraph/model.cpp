#include "graphics/scenegraph/model.hpp"
#include "geometry/methods.hpp"
#include "graphics/scenegraph/animation/animator.hpp"
#include "graphics/scenegraph/bounding_volume/axis_aligned_bounding_volume.hpp"
#include "graphics/scenegraph/mesh/boneuniform.hpp"
#include "graphics/scenegraph/mesh/mesh.hpp"
#include "graphics/scenegraph/mesh/meshdefinition.hpp"
#include "graphics/scenegraph/mesh/texturedriggedvertex.hpp"
#include "graphics/scenegraph/mesh/riggedvertex.hpp"
#include "graphics/scenegraph/material.hpp"
#include "tools/utility.hpp"
#include "configmanager.hpp"
#include "eventmanager.hpp"
#include "log.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <stack>

namespace BlueBear {
  namespace Graphics {
    namespace SceneGraph {

      Model::Model( const std::string& id, const std::vector< Drawable >& drawables ) :
        id( id ), drawables( drawables ) {}

      std::shared_ptr< Model > Model::create( const std::string& id, const std::vector< Drawable >& drawables ) {
        return std::shared_ptr< Model >( new Model( id, drawables ) );
      }

      void Model::submitLuaContributions( sol::state& lua ) {
        sol::table types = lua[ "bluebear" ][ "util" ][ "types" ];

        Transform::submitLuaContributions( lua );

        types.new_usertype< Model >( "GFXModel",
          "new", sol::no_constructor,
          "get_transform", []( Model& self ) -> Transform& {
            return self.getLocalTransform();
          },
          "set_current_animation", []( Model& self, const std::string& animation ) {
            auto animator = self.getAnimator();

            if( animator ) {
              animator->setCurrentAnimation( animation );
            } else {
              Log::getInstance().warn( "Model::submitLuaContributions", "No animator is attached to this model" );
            }
          }
        );
      }

      std::shared_ptr< Model > Model::copy() {
        std::shared_ptr< Model > copy( new Model() );

        copy->id = id;
        copy->drawables = drawables;
        copy->transform = transform;
        if( animator ) {
          copy->animator = std::make_shared< Animation::Animator >( *animator );
        }

        for( std::shared_ptr< Model > child : submodels ) {
          copy->addChild( child->copy() );
        }

        for( const auto& pair : uniforms ) {
          copy->uniforms.emplace( pair.first, pair.second->copy() );
        }

        return copy;
      }

      const std::string& Model::getId() const {
        return id;
      }

      void Model::setId( const std::string& id ) {
        this->id = id;
      }

      std::shared_ptr< Model > Model::getParent() const {
        return parent.lock();
      }

      const std::vector< std::shared_ptr< Model > >& Model::getChildren() const {
        return submodels;
      }

      const std::map< std::string, std::unique_ptr< Uniform > >& Model::getUniforms() const {
        return uniforms;
      }

      void Model::detach() {
        if( std::shared_ptr< Model > realParent = getParent() ) {
          realParent->submodels.erase(
            std::remove( realParent->submodels.begin(), realParent->submodels.end(), shared_from_this() ),
            realParent->submodels.end()
          );

          parent = std::weak_ptr< Model >();
        }
      }

      void Model::addChild( std::shared_ptr< Model > child, std::optional< int > index ) {
        child->detach();

        if( index ) {
          auto it = std::next( submodels.begin(), *index );
          submodels.insert( it, child );
        } else {
          submodels.emplace_back( child );
        }

        child->parent = shared_from_this();
      }

      Drawable& Model::getDrawable( unsigned int index ) {
        return drawables.at( index );
      }

      const std::vector< Drawable >& Model::getDrawableList() const {
        return drawables;
      }

      Uniform* Model::getUniform( const std::string& id ) {
        auto it = uniforms.find( id );
        if( it != uniforms.end() ) {
          return it->second.get();
        }

        return nullptr;
      }

      void Model::setUniform( const std::string& id, std::unique_ptr< Uniform > uniform ) {
        uniforms[ id ] = std::move( uniform );
      }

      void Model::removeUniform( const std::string& id ) {
        uniforms.erase( id );
      }

      Transform Model::getComputedTransform() const {
        if( std::shared_ptr< Model > realParent = parent.lock() ) {
          return realParent->getComputedTransform() * transform;
        }

        return transform;
      }

      glm::mat4 Model::getHierarchicalTransform() {
        std::stack< glm::mat4 > levels;
        levels.push( transform.getMatrix() );

        std::shared_ptr< Model > current = parent.lock();
        while( current ) {
          levels.push( current->transform.getMatrix() );
          current = current->parent.lock();
        }

        glm::mat4 result = levels.top();
        levels.pop();

        while( !levels.empty() ) {
          result *= levels.top();
          levels.pop();
        }

        return result;
      }

      Shader::Uniform Model::getTransformUniform( const Shader* shader ) {
        auto it = transformUniform.find( shader );
        if( it != transformUniform.end() ) {
          return it->second;
        }

        return transformUniform[ shader ] = shader->getUniform( "model" );
      }

      Transform& Model::getLocalTransform() {
        boundingVolume = nullptr;
        return transform;
      }

      const Transform& Model::getLocalTransform() const {
        return transform;
      }

      void Model::setLocalTransform( Transform transform ) {
        boundingVolume = nullptr;
        this->transform = transform;
      }

      void Model::optimizeTransform() {
        this->transform.recalculate();
      }

      std::shared_ptr< Animation::Animator > Model::getAnimator() const {
        return animator;
      }

      void Model::setAnimator( std::shared_ptr< Animation::Animator > animator ) {
        this->animator = animator;
      }

      std::shared_ptr< Model > Model::findChildById( const std::string& id ) const {
        for( std::shared_ptr< Model > submodel : submodels ) {
          if( submodel->getId() == id ) {
            return submodel;
          }

          if( std::shared_ptr< Model > result = submodel->findChildById( id ) ) {
            return result;
          }
        }

        return std::shared_ptr< Model >();
      }

      void Model::generateBoundingVolume() {
        const std::string method = ConfigManager::getInstance().getValue( "bounding_volume_method" );
        switch( Tools::Utility::hash( method.c_str() ) ) {
          default: {
            Log::getInstance().warn( "Model::generateBoundingVolume", "Unknown bounding volume method: " + method + ", defaulting to \"aabb\"" );
            [[fallthrough]];
          }
          case Tools::Utility::hash( "aabb" ): {
            boundingVolume = std::make_unique< BoundingVolume::AxisAlignedBoundingVolume >();
            boundingVolume->generate( getModelTriangles( animator.get() ) );
          }
        }
      }

      void Model::sendDeferredObjects() {

        for( const Drawable& drawable : drawables ) {
          if( drawable.shader ) {
            drawable.shader->sendDeferred();
          }

          if( drawable.mesh ) {
            drawable.mesh->sendDeferred();
          }

          if( drawable.material ) {
            drawable.material->sendDeferredTextures();
          }
        }

        for( std::shared_ptr< Model > child : submodels ) {
          child->sendDeferredObjects();
        }
      }

      std::vector< ModelTriangle > Model::getModelTriangles( Animation::Animator* parentAnimator ) const {
        std::vector< ModelTriangle > triangles;

        glm::mat4 modelTransform = getComputedTransform().getMatrix();

        if( animator ) {
          parentAnimator = animator.get();
        }

        // Add local mesh triangles
        for( auto& drawable : drawables ) {
          if( drawable.mesh ) {
            // If animator is set:
            // * Get bone list from mesh bone uniform (should be located at "bone")
            // * Attempt to downcast Mesh to either Mesh::MeshDefinition< Mesh::RiggedVertex > or Mesh::MeshDefinition< Mesh::TexturedRiggedVertex >
            // * Set mesh's generic transform method to a function that multiplies each point by the same matrix in the shader
            // * Take triangles
            // * Reset generic transform method
            std::vector< Geometry::Triangle > meshTriangles;

            if( parentAnimator ) {
              // This complicated mess
              auto it = drawable.mesh->meshUniforms.find( "bone" );
              if( it != drawable.mesh->meshUniforms.end() ) {
                Mesh::BoneUniform* boneUniform = ( Mesh::BoneUniform* ) it->second.get();
                // This doesn't hurt too much because the bone uniform will have configure called again before send
                boneUniform->configure( parentAnimator->getComputedMatrices() );

                std::vector< glm::mat4 > boneList = boneUniform->getBoneList();

                if( auto downcast = std::dynamic_pointer_cast< Mesh::MeshDefinition< Mesh::TexturedRiggedVertex > >( drawable.mesh ) ) {
                  downcast->setGenericTransformMethod( [ &boneList ]( const Mesh::TexturedRiggedVertex& vertex ) {
                    glm::mat4 boneTransform =
                      ( boneList[ vertex.boneIDs[ 0 ] ] * vertex.boneWeights[ 0 ] ) +
                      ( boneList[ vertex.boneIDs[ 1 ] ] * vertex.boneWeights[ 1 ] ) +
                      ( boneList[ vertex.boneIDs[ 2 ] ] * vertex.boneWeights[ 2 ] ) +
                      ( boneList[ vertex.boneIDs[ 3 ] ] * vertex.boneWeights[ 3 ] );

                    return boneTransform * glm::vec4( vertex.position, 1.0f );
                  } );
                  meshTriangles = downcast->getTriangles();
                  downcast->setGenericTransformMethod( {} );
                } else if ( auto downcast = std::dynamic_pointer_cast< Mesh::MeshDefinition< Mesh::RiggedVertex > >( drawable.mesh ) ) {
                  downcast->setGenericTransformMethod( [ &boneList ]( const Mesh::RiggedVertex& vertex ) {
                    glm::mat4 boneTransform =
                      ( boneList[ vertex.boneIDs[ 0 ] ] * vertex.boneWeights[ 0 ] ) +
                      ( boneList[ vertex.boneIDs[ 1 ] ] * vertex.boneWeights[ 1 ] ) +
                      ( boneList[ vertex.boneIDs[ 2 ] ] * vertex.boneWeights[ 2 ] ) +
                      ( boneList[ vertex.boneIDs[ 3 ] ] * vertex.boneWeights[ 3 ] );

                    return boneTransform * glm::vec4( vertex.position, 1.0f );
                  } );
                  meshTriangles = downcast->getTriangles();
                  downcast->setGenericTransformMethod( {} );
                } else {
                  Log::getInstance().error( "Model::getModelTriangles", "Exhausted rigged vertex types attempting to build cpuBoneTransform" );
                }
              } else {
                Log::getInstance().error( "Model::getModelTriangles", "Animator set without mesh bone uniform" );
              }
            } else {
              // This nice, simple line
              meshTriangles = drawable.mesh->getTriangles();
            }

            for( const auto& triangle : meshTriangles ) {
              triangles.emplace_back( triangle, modelTransform );
            }
          }
        }

        for( const auto& child : submodels ) {
          triangles = Tools::Utility::concatArrays( triangles, child->getModelTriangles( parentAnimator ) );
        }

        return triangles;
      }

      bool Model::intersectsBoundingVolume( const Geometry::Ray& ray ) {
        if( !boundingVolume ) {
          generateBoundingVolume();
        }

        return boundingVolume->intersects( ray );
      }

      void Model::invalidateBoundingVolume() {
        boundingVolume = nullptr;
      }

    }
  }
}
