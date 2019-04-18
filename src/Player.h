#include "Camera.h"
#include "Physics.h"

#include "World.h"

#include <GLFW/glfw3.h>

struct Player {
  Camera camera;
  bool _grounded = false;
  float _velocity_y = 0;
  enum class Mode { Survival, Creative, Menger };
  Mode _current_mode = Mode::Creative;

  glm::vec3 acceleration {0};

  void handleTick(const World& world, TextRenderer& tr) {
    // generate impulse to jerk player if collided
    // if movement resolves collision, no impulse
    // otherwise impulse

    // if not on ground, tick gravity
    if (_current_mode == Mode::Survival) {
      if (not _grounded) {
        // tick gravity
        _velocity_y -= 0.5 * 1/60.f;

        // FIXME: cast ray to ground and then ground if reached by current velocity and then place player on ground

        // apply vel_y
        camera.translate(glm::vec3(0, _velocity_y, 0));
      }
      auto e = camera.eye();

      // if now grounded, vel_y = 0
      auto blocks = collided(world);
      if (blocks.size() != 0) {
        
      }
    
      _grounded = grounded(world);
    }
    // FIXME: if the tiles below you don't collide with you, then you are ungrounded. redo collision
  }

	void handleKey(int key, int scancode, int action, int mods) {
    // creative flying
    if (key == GLFW_KEY_F && action == GLFW_RELEASE) {
      if (_current_mode == Mode::Creative) {
        _current_mode = Mode::Survival;
      } else {
        _velocity_y = 0;
        _current_mode = Mode::Creative;
      }
    }

    // FIXME: menger flying


    if (key == GLFW_KEY_SPACE && _grounded && action == GLFW_PRESS) {
      _grounded = false;
      _velocity_y = 0.2;
    }
  }

  bool grounded(const World& world) {
    glm::vec3 eye = camera.eye();

    glm::ivec3 world_index = glm::floor(eye);

    for (int i = -1; i <= 1; ++i)
    for (int k = -1; k <= 1; ++k) 
    for (int j = -3 ; j <= -1; ++j) {
      glm::ivec3 box = world_index + glm::ivec3(i, j, k);
            
      if (not world.isAir(box.x, box.y, box.z)) {
        if (Physics::verticalCollision(box, feet().y, eye.y)
            && Physics::horizontalCollision(box, eye, 0.5)
          ) { 
          return true;
        }
      }
    }

    return false;
  }

  std::vector<glm::ivec3> collided(const World& world) {
    glm::vec3 eye = camera.eye();

    glm::ivec3 world_index = glm::floor(eye);

    std::vector<glm::ivec3> acc;

    for (int i = -1; i <= 1; ++i)
    for (int k = -1; k <= 1; ++k) 
    for (int j = -5; j <= 1; ++j) {
      glm::ivec3 box = world_index + glm::ivec3(i, j, k);
      if (not world.isAir(box.x, box.y, box.z)) {
        if (Physics::verticalCollision(box, feet().y, eye.y)
            && Physics::horizontalCollision(box, eye, 0.5))
        {
          acc.emplace_back(box);
        }
      }
    }

    return acc;
  }

  glm::vec3 feet() const {
    return camera.eye() - glm::vec3(0, 1.75, 0);
  }
};