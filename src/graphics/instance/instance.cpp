#include "graphics/instance/instance.hpp"
#include "graphics/model.hpp"
#include "graphics/drawable.hpp"
#include <GL/glew.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace BlueBear {
  namespace Graphics {

    Instance::Instance( const Model& model, GLuint shaderProgram ) : shaderProgram( shaderProgram ) {
      prepareInstanceRecursive( model );
    }

    void Instance::prepareInstanceRecursive( const Model& model ) {
      // Copy the drawable
      if( model.drawable ) {
        drawable = std::make_shared< Drawable >( *model.drawable );
      }

      for( auto& pair : model.children ) {
        auto& child = *( pair.second );

        // Hand down the same transform as the parent to this model
        children.emplace( pair.first, std::make_shared< Instance >( child, shaderProgram ) );
      }
    }

    /**
     * FIXME: This only goes one level of recursion deep, you derp
     */
    std::shared_ptr< Instance > Instance::findChildByName( std::string name ) {
      return children[ name ];
    }

    /**
     * drawEntity public interface
     */
    void Instance::drawEntity() {
      drawEntity( glm::mat4(), false );
    }

    /**
     * drawEntity private implementation
     */
    void Instance::drawEntity( const glm::mat4& parent, bool dirty ) {

      // If we find one transform at this level that's dirty, every subsequent transform needs to get updated
      if( ( dirty = dirty || transform.dirty ) ) {
        transform.update( parent );
      }

      if( drawable ) {
        transform.sendToShader( shaderProgram );
        drawable->render( shaderProgram );
      }

      for( auto& pair : children ) {
        // Apply the same transform of the parent
        // If "dirty" was true here, it'll get passed down to subsequent instances. But if "dirty" was false, and this call ends up being "dirty",
        // it should only propagate to its own children since dirty is passed by value here.
        pair.second->drawEntity( transform.matrix, dirty );
      }
    }

    glm::vec3 Instance::getPosition() {
      return transform.getPosition();
    }

    void Instance::setPosition( const glm::vec3& position ) {
      transform.setPosition( position );
    }

    glm::vec3 Instance::getScale() {
      return transform.getScale();
    }

    void Instance::setScale( const glm::vec3& scale ) {
      transform.setScale( scale );
    }

    GLfloat Instance::getRotationAngle() {
      return transform.getRotationAngle();
    }

    glm::vec3 Instance::getRotationAxes() {
      return transform.getRotationAxes();
    }

    void Instance::setRotationAngle( GLfloat rotationAngle, const glm::vec3& rotationAxes ) {
      // Generate new quat
      transform.setRotationAngle( rotationAngle, rotationAxes );
    }

  }
}
