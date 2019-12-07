#include "stdafx.h"
#include "FrameBuffer.cpp"
#include "inc/bullet.h"
//#include "../GL/GLDebugDrawer.h"

//GLDebugDrawer	gDebugDrawer;



class PhysicsObj : public Obj {
public:
	btRigidBody* body;
	float mass;
	vec3 physicsOrigin;

	void setBoxPhysics(float mass, const vec3& origin=vec3(0), const vec3& boxSize=vec3(1)) {
		btVector3 localInertia(0, 0, 0);
		bool isDynamic = (mass != 0.f);
		physicsOrigin = origin;

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btVector3(origin.x+pos.x, origin.y + pos.y, origin.z + pos.z));

		btCollisionShape* colShape = new btBoxShape(btVector3(boxSize.x, boxSize.y, boxSize.z));
		if (isDynamic)
			colShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);

		this->mass = mass;
		body = new btRigidBody(rbInfo);
		Bullet::dynamicsWorld->addRigidBody(body);
	}

	void tick(float dt) {
		Obj::tick(dt);
		if (body && body->getMotionState()) {
			btTransform trans;
			body->getMotionState()->getWorldTransform(trans);
			pos = vec3(float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ())) - physicsOrigin;

			btQuaternion temRot = trans.getRotation();
			quat q = toQuat(temRot);
			quat_2_euler_ogl(q, rot.x, rot.z, rot.y);
			rot = degrees(rot);
			
			/*if (name == "Robot") {
				cout << glm::to_string(pos) << endl;
			}*/
		}
	}

	void move(vec3 vec) {
		body->translate(btVector3(vec.x, vec.y, vec.z));
		//body->translate
	}

	void rotate(vec3 vec) {
		btTransform trans;
		trans = body->getWorldTransform();
		btQuaternion quaternion;
		quaternion.setEuler(vec.y, vec.x, vec.z);
		trans.setRotation(quaternion);
		body->setWorldTransform(trans);
		body->setAngularVelocity(btVector3(0, 0, 0));
	}
};

class Light : public Obj {
public:
	float rotSpeed = 2;
	bool isRotating = true;
	float length = 20;
	float delta = 0;

	Light() : Obj() {
		loadObj("../model/sphere.obj");
		setShader(Shader::shaders["tri"]);
		scale = vec3(0.2f);
		setLightToShader(*shader);
	}

	virtual void initName() {
		name = "Light";
	}

	void tick(float dt) {
		if (!isRotating) return;
		delta += rotSpeed * dt;
		float x = cos(delta) * length;
		float z = sin(delta) * length;
		pos.x = x;
		pos.y = z;
		updateTransform();
	}

	void setLightToShader(Shader& shader) {
		shader.addUniform("ambient", new vec3(0.1f, 0.1f, 0.1f));
		shader.addUniform("lightPos", &getPos());
		shader.addUniform("lightColor", &color);
	}
};

class TextureObj : public Obj {
public:
	Texture texture;

	virtual void render() {
		if (shader) {
			shader->changeUniformValue("trans", &getTrans());
			TextureBindInfo tex{texture.id, 0};
			shader->changeUniformValue("texture0", &texture);
			shader->use();
			applyColor(shader->id);
		} else {
			assert(0 && name.c_str()); // shader 없음
		}
		vo->render();
	}
};

class JohnSnow : public TextureObj {
public:
	float speed = -2;

	virtual void tick(float dt) {
		updateTransform();
		pos.y += dt * speed;
		if (pos.y < 0) {
			pos.y = 2;
		}
	}

	void initName() {
		name = "JohnSnow";
	}
};


class ShapeObj : public TextureObj {
public:
	float speed = 6;
	virtual void tick(float dt) {
		updateTransform();
		rot.y += dt * speed;
	}
	void initName() {
		name = "ShapeObj";
	}

};

class Orbit : public TextureObj {
public:
	Obj* parentObj = nullptr;
	vector<Obj*> childObjs;

	VO* orbitVO = nullptr;

	mat4 orbitTrans;

	float speed = 2;
	float arm = 3;

	void initOrbit() {
		orbitVO = new VO();

		for (size_t i = 0; i < 60; i++) {
			float _x = cos(De2Ra(i * 6)) * arm;
			float _y = sin(De2Ra(i * 6)) * arm;
			orbitVO->vertex.push_back(vec3(_x, 0, _y));
		}
		orbitVO->bind();
		orbitVO->drawStyle = GL_LINE_LOOP;
	}

	virtual void tick(float dt) {
		pos.x = arm;
		rot.y += speed;
		trans = parentObj->getTrans();
		trans = glm::rotate(trans, glm::radians(rot.x), glm::vec3(1, 0, 0));
		trans = glm::rotate(trans, glm::radians(rot.y), glm::vec3(0, 1, 0));
		trans = glm::translate(trans, pos);
		trans = glm::scale(trans, scale);

		if (parentObj) {
			orbitTrans = parentObj->getTrans();
			orbitTrans = glm::rotate(orbitTrans, glm::radians(rot.x), glm::vec3(1, 0, 0));
		}
	}

	void render() {
		if (orbitVO) {
			shader->changeUniformValue("trans", &orbitTrans);
			shader->use();
			orbitVO->render();
		}
		Obj::render();
	}

	void initName() {
		name = "Orbit";
	}
};


Window win;
Camera* cam;

VO gridVO;


ShapeObj* triObj;
TextureObj* floorObj;

Light* light;
vector<Orbit*> orbits;
vector<Obj*> objs;
vector<JohnSnow*> johnSnow;


Shader triShader, gridShader;

Texture texture0;

bool cameraFirstView = false;


float upPlaneSpinSpeed = 0;
float cubeYSpinSpeed = 0;
float cubeFrontOpenSpeed = 1;
float triSideOpenSpeed = 1;

void onKeyboard(unsigned char key, int x, int y, bool isDown) {
	if (isDown) {
		float moveScale = 10;
		switch (key) {
		case 'r':
			cam->armVector.z = cam->armVector.z < -1 ? -0.001f : -50;
			printf("%f\n", cam->armVector.z);
			break;
		case '2': {
		}
			break;

		case 'y':
			cubeYSpinSpeed = !cubeYSpinSpeed;
			break;
		case 't':
			upPlaneSpinSpeed = !upPlaneSpinSpeed;
			break;
		case 'f':
			cubeFrontOpenSpeed = -cubeFrontOpenSpeed;
			break;
		case 'o':
			triSideOpenSpeed = -triSideOpenSpeed;
			break;
		case 'p':
			cam->isPerspective = !cam->isPerspective;
			break;
		case 'z':
			cam->rotateY(moveScale);
			break;
		case 'Z':
			cam->rotateY(-moveScale);
			break;
		}
		
	}
}

void init() {
	glLineWidth(3);


	triShader.complieShader("tri");
	gridShader.complieShader("grid");

	



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



	cam = new Camera();
	cam->armVector.z = -20;

	light = new Light();

	Texture tex1;
	Texture tex2;
	Texture tex3;
	tex1.load("../textures/a.jpg");
	tex2.load("../textures/b.jpg");
	tex3.load("../textures/c.jpg");
	triShader.addUniform("texture0", texture0);

	triObj = new ShapeObj();
	floorObj = new TextureObj();

	floorObj->loadObj("../model/cube.obj");
	triObj->loadObj("../model/tri.obj");

	floorObj->setShader(&triShader);
	triObj->setShader(&triShader);
	floorObj->texture = tex1;
	triObj->texture = tex2;

	floorObj->setScale(vec3(5, 0.1f, 5));
	floorObj->getPos().x = -1; 

	orbits.push_back(new Orbit());
	orbits.push_back(new Orbit());
	orbits.push_back(new Orbit());
	int armLen = 3;
	int speed = 1;
	for (auto var : orbits) {
		armLen += 2;
		speed++;
		var->arm = armLen;
		var->speed = speed;
		var->setShader(&triShader);
		var->parentObj = triObj;
		var->loadObj("../model/sphere.obj");
		var->color = vec4(f(), f(), f(), f());
		var->texture = tex3;
	}

	for (size_t i = 0; i < 20; i++) {
		johnSnow.push_back(new JohnSnow());
		float x = f() * 10 - 5;
		float y = f() * 1;
		float z = f() * 10 - 5;

		johnSnow.back()->loadObj("../model/cube.obj");
		string name = "snow";
		johnSnow.back()->name = name + std::to_string(i);
		johnSnow.back()->setPos(vec3(x, y, z));
		johnSnow.back()->setScale(vec3(0.3f));
		johnSnow.back()->setShader(&triShader);

		johnSnow.back()->texture = tex2;
	}

	
	for (size_t i = 0; i < 20; i++) {
		objs.push_back(new Obj());
		float x = f() * 10 - 5;
		float y = 0;
		float z = f() * 10 - 5;

		objs.back()->loadObj("../model/cube.obj");
		string name = "snow";
		objs.back()->name = name + std::to_string(i);
		objs.back()->setPos(vec3(x, y, z));
		objs.back()->setScale(vec3(0.3f, 2,0.3f));
		objs.back()->color = (vec4(1,1,1,0.3f));
		objs.back()->setShader(&triShader);
	}


	sterma::Init();


	gridVO.drawStyle = GL_LINES;
	gridVO.vertex.push_back(vec3(0, 100, 0));
	gridVO.vertex.push_back(vec3(0, -100, 0));
	gridVO.vertex.push_back(vec3(100, 0, 0));
	gridVO.vertex.push_back(vec3(-100, 0, 0));
	gridVO.vertex.push_back(vec3(0, 0, 100));
	gridVO.vertex.push_back(vec3(0, 0, -100));
	gridVO.bind();

	auto objs = Scene::activeScene->objs;
	for (set<TickObj*>::iterator it = objs.begin(); it != objs.end(); ++it) {
		(*it)->initName();
	}

	onKeyboardEvent = onKeyboard;
}


DWORD prevTime = 0;
DWORD thisTickTime = 0;
float dt;
bool looping = false;

void loop() {
	if (prevTime == 0) {
		prevTime = GetTickCount() - 10;
	}
	thisTickTime = GetTickCount();
	dt = (thisTickTime - prevTime) * 0.001f;
	dt = dt > 0.05f ? dt = 0.05f : dt;
	prevTime = thisTickTime;

	cam->tick(dt);
	if (Input::mouse[EMouse::MOUSE_L_BUTTON]) {
		cam->getRotation().x += dt * 10 * Input::mouse[EMouse::MOUSE_OFF_Y];
		cam->getRotation().y += dt * 10 * Input::mouse[EMouse::MOUSE_OFF_X];
	}
	cam->armVector.z = clamp(-10 + Input::mouse[EMouse::MOUSE_WHEEL], -100, -1);

	Bullet::dynamicsWorld->stepSimulation(dt, 10);
	Scene::activeScene->tick(dt);

	Input::getMousePos();

	glutPostRedisplay();
}

GLvoid drawScene()
{
	float gray = 0.3f;
	glClearColor(gray, gray, gray, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cam->bind(win);

	// 텍스처버퍼에 그림
	// -----------------
	sterma::bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Scene::activeScene->render();

	gridShader.use();
	gridVO.render();
	 
	Debug::render();
	/*Bullet::dynamicsWorld->debugDrawWorld();*/

	sterma::unbind();
	// -----------------

	sterma::render();


	

	glutSwapBuffers(); // 화면에 출력하기
}

void timerFunc(int v) {
	loop();
	glutPostRedisplay();
	if (!looping)
		glutTimerFunc(10, timerFunc, 0);
}
 
GLvoid drawScene(GLvoid);
GLvoid Reshape(int w, int h);

void main(int argc, char** argv) // 윈도우 출력하고 콜백함수 설정 
{ //--- 윈도우 생성하기
	glutInit(&argc, argv); // glut 초기화
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // 디스플레이 모드 설정
	win.init(800, 600);
	glutInitWindowPosition(0, 30); // 윈도우의 위치 지정
	glutInitWindowSize(win.w, win.h); // 윈도우의 크기 지정
	glutCreateWindow(__FILE__); // 윈도우 생성(윈도우 이름)

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
		//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	}
	else
		std::cout << "GLEW Initialized\n";

	Shader unlit;
	unlit.complieShader("unlit");
	unlit.addUniform("color", new vec3(1,0,1));
	unlit.addUniform("trans", new mat4(1));
	Bullet::initBullet();
	//gDebugDrawer.setDebugMode(~0);
	//Bullet::dynamicsWorld->setDebugDrawer(&gDebugDrawer);
	init();

	glutDisplayFunc(drawScene); // 출력 함수의 지정
	glutReshapeFunc(Reshape); // 다시 그리기 함수 지정

	bindInput();

	timerFunc(1);

	glutMainLoop(); // 이벤트 처리 시작 
}


GLvoid Reshape(int w, int h) {
	printf("Reshape\n");
	glViewport(0, 0, w, h);
}