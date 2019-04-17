#include <gtest/gtest.h>
#include "../src/Physics.h"
#include "../src/World.h"
#include "../src/Perlin.h"


TEST(Physics, vertical_cases) {
  float p0, p1, b, t;

  auto test = [&]() -> bool {
    return not(t < p0 || b > p1);
  };

  p0 = 1; p1 = 2; b = 3; t = 4;
  ASSERT_FALSE(test());

  p0 = 1; b = 2; p1 = 3; t = 4;
  ASSERT_TRUE(test());

  b = 1; p0 = 2; t = 3; p1 = 4;
  ASSERT_TRUE(test());

  b = 1; t = 2; p0 = 3; p1 = 4;
  ASSERT_FALSE(test());

  p0 = 1; b = 2; t = 3; p1 = 4;
  ASSERT_TRUE(test());
}

TEST(Physics, horizontal_logic) {
  glm::ivec3 A {0, 0, 0};
  glm::vec3 pole {0, 1.7, 0};
  float radius = 0.5;

  ASSERT_TRUE(A.x + 0.5 > pole.x - radius);
  ASSERT_TRUE(A.z + 0.5 > pole.z - radius);
  ASSERT_TRUE(A.x - 0.5 < pole.x + radius);
  ASSERT_TRUE(A.z - 0.5 < pole.z + radius);
}

TEST(Physics, horizontal_cases) {
  ASSERT_TRUE(Physics::horizontalCollision({0, 0, 0}, {0, 0, 0}, 0.5));
  ASSERT_TRUE(Physics::horizontalCollision({0, 0, 0}, {0.75, 0, 0}, 0.5));
}

TEST(Physics, box_collision) {
  glm::ivec3 box {0, 0, 0};
  glm::vec3 eye {0, 1.7, 0};

  bool vertical   = Physics::verticalCollision(box, eye.y - 1.75, eye.y);
  bool horizontal = Physics::horizontalCollision(box, eye, 0.5);

  ASSERT_TRUE(vertical);
  ASSERT_TRUE(horizontal);
}

TEST(Physics, player_collision) {

  glm::vec3 eye {0, 50 + 1.7, 0};
  glm::ivec3 world_index = glm::floor(eye);

  World world{16, 16};
  srand(time(NULL));

  for (int i = 0; i < world._width; ++i) {
    for (int k = 0; k < world._height; ++k) {
      world(i, 50 + perlin(i / 50.f, k / 50.f) * 10, k) = 1;
    }
  }

  auto test = [&]() {
    for (int i = -1; i <= 1; ++i)
    for (int k = -1; k <= 1; ++k) 
    for (int j = -3; j <= 1; ++j) {
      glm::ivec3 box = world_index + glm::ivec3(i, j, k);
            
      if (not world.isAir(box.x, box.y, box.z)) {
        if (Physics::verticalCollision(box, eye.y - 1.75, eye.y)
            && Physics::horizontalCollision(box, eye, 0.5)
          ) { 
          return true;
        }
      }
    }
    return false;
  };

  std::cout << (int)world(0, 50, 0) << std::endl;
  ASSERT_TRUE(test());
}