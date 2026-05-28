#pragma once

#include <glm/glm.hpp>


namespace Physics{
	class Particle 
	{
		protected:
			bool isDestroyed = false;

			void UpdatePosition(float deltaTime);
			void UpdateVelocity(float deltaTime);
	
		public:
			void Destroy();
			bool IsDestroyed();

			glm::vec3 Position;
			glm::vec3 Velocity;
			glm::vec3 Acceleration;

			void Update(float time);
			glm::vec3 GetPosition();

			Particle();
	};
}