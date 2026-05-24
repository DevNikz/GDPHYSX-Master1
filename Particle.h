#pragma once

#include <glm/glm.hpp>


namespace Physics{
	class Particle 
	{
		protected:
			void UpdatePosition(float deltaTime);
			void UpdateVelocity(float deltaTime);
	
		public:
			glm::vec3 Position;
			glm::vec3 Velocity;
			glm::vec3 Acceleration;

			void Update(float time);

			Particle();
	};
}