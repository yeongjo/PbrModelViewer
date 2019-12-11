#include "stdafx.h"
#include "FrameBuffer.cpp"
#include "inc/bullet.h"
//#include "../GL/GLDebugDrawer.h"

//GLDebugDrawer	gDebugDrawer;


Shader triShader, gridShader;

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

	static int lightCount;

	Light() : Obj() {
		loadObj("model/sphere.obj");
		auto pbr = Shader::get("pbr,pbr");
		setShader(pbr);
		//scale = vec3(0.2f);
	}

	virtual void initName() {
		name = "Light";
	}

	void tick(float dt) {
		if (!isRotating) return;
		/*delta += rotSpeed * dt;
		float x = cos(delta) * length;
		float z = sin(delta) * length;
		pos.x = x;
		pos.y = z;
		updateTransform();*/
	}

	void setLightToShader(Shader& shader) {
		//glm::vec3 newPos = pos + glm::vec3(sin( * 5.0) * 5.0, 0.0, 0.0);
		glm::vec3 newPos = pos;
		shader.setUniform("lightPositions[" + std::to_string(lightCount) + "]", newPos);
		shader.setUniform("lightColors[" + std::to_string(lightCount) + "]", color);
		lightCount++;
		/*shader.setUniform("ambient", vec3(0.1f, 0.1f, 0.1f));
		shader.setUniform("lightPos", getPos());
		shader.setUniform("lightColor", color);*/
	}
};
int Light::lightCount = 0;

class TextureObj : public Obj {
public:
	Texture texture;

	virtual void render() {
		if (shader) {
			shader->setUniform("trans", getTrans());
			//shader->setUniform("texture0", &texture);
			shader->use();
			applyColor(shader->getId());
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
		//rot.y += dt * speed;
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

	float speed = 0;
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
			shader->setUniform("trans", orbitTrans);
			shader->use();
			orbitVO->render();
		}
		Obj::render();
	}

	void initName() {
		name = "Orbit";
	}
};


Camera* cam;

VO gridVO;


ShapeObj* triObj;
TextureObj* floorObj;

Light* light[4];
vector<Orbit*> orbits;
vector<Obj*> objs;
vector<JohnSnow*> johnSnow;



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

void initPbr() {
	auto pbr = Shader::create("pbr", "pbr");
	auto equirectangular_to_cubemap = Shader::create("cubemap", "equirectangular_to_cubemap");
	auto irradianceShader = Shader::create("cubemap", "irradiance_convolution");
	auto prefilterShader = Shader::create("cubemap", "prefilter");
	auto brdf = Shader::create("brdf", "brdf");
	auto background = Shader::create("background", "background");

	glEnable(GL_DEPTH_TEST);
	// set depth function to less than AND equal for skybox depth trick.
	glDepthFunc(GL_LEQUAL);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


	Texture* hdr;
	hdr = Texture::loadHDR("textures/Road_to_MonumentValley_Ref.hdr");

	Cubemap envCubemap;
	envCubemap.init(512, 512);
	envCubemap.convertFromEquirectangular(*hdr, equirectangular_to_cubemap);

	Cubemap irradianceMap;
	irradianceMap.init(32, 32);
	irradianceMap.convertFromEquirectangular(envCubemap, irradianceShader);

	Cubemap prefilterMap;
	prefilterMap.init(128, 128);
	prefilterMap.quasiMonteCarloSimulation(envCubemap, prefilterShader);

	// pbr: generate a 2D LUT from the BRDF equations used.
	FrameBuffer temp;
	auto brdfLUTTexture = Texture::createEmpty(512, 512, GL_RG16F, GL_RG);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	temp.init(*brdfLUTTexture);
	temp.bind();
	brdf->use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderQuad();
	temp.unbind();
	// pbr end

	Window::get().resetViewport();

	background->setUniform("environmentMap", envCubemap);
	pbr->setUniform("irradianceMap", irradianceMap);
	pbr->setUniform("prefilterMap", prefilterMap);
	pbr->setUniform("brdfLUT", temp.colorTex);
	pbr->setUniform("albedo", vec3(0.5f, 0.0f, 0.0f));
	pbr->setUniform("ao", 1.0f);
	pbr->setUniform("metallic", 1.0f);
	pbr->setUniform("roughness", 1.0f);
}

void init() {
	glLineWidth(3);


	triShader.complie("tri", "tri");
	gridShader.complie("grid");

	initPbr();



	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/



	cam = new Camera();
	cam->armVector.z = -20;

	glm::vec3 lightPositions[] = {
		glm::vec3(-10.0f, 10.0f, 10.0f),
		glm::vec3(10.0f, 10.0f, 10.0f),
		glm::vec3(-10.0f, -10.0f, 10.0f),
		glm::vec3(10.0f, -10.0f, 10.0f),
	};
	glm::vec3 lightColors[] = {
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f)
	};

	

	Texture* tex1;
	Texture* tex2;
	Texture* tex3;
	tex1 = Texture::load("textures/a.jpg");
	tex2 = Texture::load("textures/b.jpg");
	tex3 = Texture::load("textures/c.jpg");

	auto pbr = Shader::get("pbr,pbr");

	for (size_t i = 0; i < 4; i++) {
		light[i] = new Light();
		light[i]->setPos(lightPositions[i]);
		light[i]->color = lightColors[i];
		light[i]->setLightToShader(*pbr);
	}

	triObj = new ShapeObj();
	floorObj = new TextureObj();

	floorObj->loadObj("model/cube.obj");
	triObj->loadObj("model/cube.obj");

	floorObj->setShader(pbr);
	triObj->setShader(pbr);
	floorObj->texture = *tex1;
	triObj->texture = *tex2;

	floorObj->setScale(vec3(5, 0.1f, 5));
	floorObj->getPos().x = -1;

	orbits.push_back(new Orbit());
	orbits.push_back(new Orbit());
	orbits.push_back(new Orbit());
	int armLen = -2;
	int speed = 0;
	for (auto var : orbits) {
		armLen += 2;
		var->arm = armLen;
		var->speed = speed;
		var->setShader(pbr);
		var->parentObj = triObj;
		var->loadObj("model/sphere.obj");
		var->color = vec4(f(), f(), f(), f());
		var->texture = *tex3;
	}

	/*for (size_t i = 0; i < 20; i++) {
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

		johnSnow.back()->texture = *tex2;
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
		objs.back()->setScale(vec3(0.3f, 2, 0.3f));
		objs.back()->color = (vec4(1, 1, 1, 0.3f));
		objs.back()->setShader(&triShader);
	}*/


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
	if (Input::mouse[EMouse::MOUSE_R_BUTTON]) {
		cam->getRotation().x += dt * 10 * Input::mouse[EMouse::MOUSE_OFF_Y];
		cam->getRotation().y += dt * 10 * Input::mouse[EMouse::MOUSE_OFF_X];
	}
	cam->armVector.z = clamp(-10 + Input::mouse[EMouse::MOUSE_WHEEL], -100, -1);

	Bullet::dynamicsWorld->stepSimulation(dt, 10);
	Scene::activeScene->tick(dt);

	Input::getMousePos();

	glutPostRedisplay();
}

float a = 0;
float metallic = 0;
float roughness = 0;
float ao = 0;
vec3 albedo = vec3(1);
GLvoid drawScene() {
	float gray = 0.3f;
	glClearColor(gray, gray, gray, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cam->bind();

	// 텍스처버퍼에 그림
	// -----------------
	sterma::bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Scene::activeScene->render();

	gridShader.use();
	gridVO.render();

	Debug::render();
	/*Bullet::dynamicsWorld->debugDrawWorld();*/

	Shader::get("background,background")->use();
	renderCube();

	sterma::unbind();
	// -----------------

	sterma::render();

	auto pbr = Shader::get("pbr,pbr");

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGLUT_NewFrame();

	//
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(150, 680), ImGuiCond_Once);

	// Main body of the Demo window starts here.
	if (!ImGui::Begin("HI")) {
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}
	//ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
	{
		if (ImGui::Button("shader recomplie")) {
			ImGui::LogToClipboard();
			ImGui::LogText("Hello, world!");
			ImGui::LogFinish();
			pbr->recomplie();
		}
		sterma::renderWindow();
		ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("ao", &ao, 0.0f, 1.0f, "%.2f");
		ImGui::ColorEdit3("albedo##3", (float*)&albedo, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		pbr->setUniform("metallic", metallic);
		pbr->setUniform("roughness", roughness);
		pbr->setUniform("ao", ao);
		pbr->setUniform("albedo", albedo);
	}
	ImGui::End();

	//ImGui::ShowDemoWindow();

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
	Window win(800, 600);
	glutInitWindowPosition(0, 30); // 윈도우의 위치 지정
	glutInitWindowSize(win.getW(), win.getH()); // 윈도우의 크기 지정
	glutCreateWindow(__FILE__); // 윈도우 생성(윈도우 이름)

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
		//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "Unable to initialize GLEW" << std::endl;

		exit(EXIT_FAILURE);
	} else
		std::cout << "GLEW Initialized: " << (char*)(glGetString(GL_VERSION)) << "\n";



	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.DisplaySize.x = win.getW();
	io.DisplaySize.y = win.getH();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	const char* glsl_version = "#version 330";
	// Setup Platform/Renderer bindings
	ImGui_ImplGLUT_Init();
	ImGui_ImplGLUT_InstallFuncs();
	ImGui_ImplOpenGL3_Init(glsl_version);



	Shader unlit;
	unlit.complie("unlit");
	unlit.setUniform("color", vec3(1, 0, 1));
	unlit.setUniform("trans", mat4(1));
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
	ImGui_ImplGLUT_ReshapeFunc(w, h);
}

void exit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGLUT_Shutdown();
	ImGui::DestroyContext();
}
