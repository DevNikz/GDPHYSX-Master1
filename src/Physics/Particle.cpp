#include "Particle.h"

namespace Physics {
	void Particle::UpdatePosition(float deltaTime)
	{
		this->Position = this->Position + (this->Velocity * deltaTime) +
			((1.0f / 2.0f) * (this->Acceleration * deltaTime * deltaTime));
	}

	void Particle::UpdateVelocity(float deltaTime)
	{
		this->Velocity = this->Velocity + (this->Acceleration * deltaTime);
	}

	void Particle::Destroy()
	{
		isDestroyed = true;
	}

	bool Particle::IsDestroyed()
	{
		return isDestroyed;
	}

	void Particle::Update(float time)
	{
		this->UpdatePosition(time);
		this->UpdateVelocity(time);
	}

	glm::vec3 Particle::GetPosition()
	{
		return this->Position;
	}

	Particle::Particle()
	{
		this->Position = glm::vec3(0, 0, 0);
		this->Velocity = glm::vec3(0, 0, 0);
		this->Acceleration = glm::vec3(0, 0, 0);
	}
}

