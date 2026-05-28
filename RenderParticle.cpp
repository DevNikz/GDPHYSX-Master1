#include "RenderParticle.h"

using namespace Physics;

void RenderParticle::Draw()
{
	RenderModel->Color(Color);
	RenderModel->Position(PhysicsParticle->Position);
	RenderModel->DrawModel();

}