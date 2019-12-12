#include "btBulletDynamicsCommon.h"

namespace Bullet {
	btDiscreteDynamicsWorld* dynamicsWorld;

	void initBullet() {
		/////collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		//btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();

		/////use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		//btCollisionDispatcher* dispatcher = new	btCollisionDispatcher(collisionConfiguration);

		/////btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		//btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();

		/////the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		//btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

		//dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

		//dynamicsWorld->setGravity(btVector3(0, -10, 0));
	}

	void tick(float dt) {
		//Bullet::dynamicsWorld->stepSimulation(dt, 10);
	}
}

void quat_2_euler_ogl(const quat& q, float& yaw, float& pitch, float& roll) {
	float sqw = q.w * q.w;
	float sqx = q.x * q.x;
	float sqy = q.y * q.y;
	float sqz = q.z * q.z;
	pitch = asinf(2.0f * (q.y * q.z + q.w * q.x)); // rotation about x-axis
	roll = atan2f(2.0f * (q.w * q.y - q.x * q.z), (-sqx - sqy + sqz + sqw)); // rotation about y-axis
	yaw = atan2f(2.0f * (q.w * q.z - q.x * q.y), (-sqx + sqy - sqz + sqw)); // rotation about z-axis
}

const vec3 toVec3(btVector3 vec) {
	return vec3(vec.getX(), vec.getY(), vec.getZ());
}

const quat toQuat(btQuaternion quaternion) {
	quat t(quaternion.w(), quaternion.x(), quaternion.y(), quaternion.z());
	return t;
}