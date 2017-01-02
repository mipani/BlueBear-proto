#ifndef MODELINSTANCE
#define MODELINSTANCE

#include "graphics/transform.hpp"
#include "graphics/drawable.hpp"
#include <string>
#include <memory>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>

namespace BlueBear {
  namespace Graphics {
    class Model;

    /**
     * A GFXInstance is a specific instance of a graphic model placed on a lot. It contains
     * a position vector and other associated states (depending on the type of model).
     */
    class Instance {

      private:
        GLuint shaderProgram;
        Transform transform;

        void prepareInstanceRecursive( const Model& model );

        void drawEntity( const glm::mat4& parent, bool dirty );

      public:
        std::shared_ptr< Drawable > drawable;
        std::map< std::string, std::shared_ptr< Instance > > children;

        Instance( const Model& model, GLuint shaderProgram );

        std::shared_ptr< Instance > findChildByName( std::string name );

        void drawEntity();

        glm::vec3 getPosition();

        void setPosition( const glm::vec3& position );

        glm::vec3 getScale();

        void setScale( const glm::vec3& scale );

        GLfloat getRotationAngle();

        glm::vec3 getRotationAxes();

        void setRotationAngle( GLfloat rotationAngle, const glm::vec3& rotationAxes = glm::vec3( 0.0f, 0.0f, 1.0f ) );
    };
  }
}
#endif
