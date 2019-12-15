#include "stdafx.h"
#include "FrameBuffer.cpp"
#include "inc/bullet.h"
#include "portable-file-dialog.h"
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
			
			/*if (name == (char*)"Robot") {
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
	int idx;

	vector<Shader*> lightShaders;

	static int lightCount;

	Light() : Obj() {
		loadObj("model/sphere.obj");
	}

	void tick(float dt) {
		if (!isRotating) return;
		/*delta += rotSpeed * dt;
		float x = cos(delta) * length;
		float z = sin(delta) * length;
		pos.x = x;
		pos.y = z;
		*/
		updateTransform();
		for (size_t i = 0; i < lightShaders.size(); i++) {
			lightShaders[i]->setUniform("lightPositions[" + std::to_string(idx) + "]", pos);
			lightShaders[i]->setUniform("lightColors[" + std::to_string(idx) + "]", color);
		}
	}

	void setLightToShader(Shader& shader) {
		//glm::vec3 newPos = pos + glm::vec3(sin( * 5.0) * 5.0, 0.0, 0.0);
		lightShaders.push_back(&shader);
		setShader(*Shader::get("unlit,unlit"));
		idx = lightCount;
		name = "light"+ std::to_string(idx);
		lightCount++;
	}

	virtual void gui() {
		if (guiStart()) {
			ImGui::ColorEdit3((name + "color").c_str(), (float*)&color, ImGuiColorEditFlags_HDR);
			ImGui::TreePop();
		}
	}
};
int Light::lightCount = 0;

class PbrObj : public Obj {
public:
	Texture albedoMap;
	Texture ao_r_mMap;
	Texture normalMap;

	float metallic = 0;
	float roughness = 0;
	float ao = 1;
	vec3 albedo = vec3(1);

	void setTextures(Texture& albedoMap,
		Texture& ao_r_m,
		Texture& normal) {
		this->albedoMap = albedoMap;
		this->ao_r_mMap = ao_r_m;
		this->normalMap = normal;
	}

	virtual void render() {
		if (shader) {
			shader->setUniform("model", getTrans());
			shader->setUniform("metallic", metallic);
			shader->setUniform("roughness", roughness);
			shader->setUniform("ao", ao);
			shader->setUniform("albedo", albedo);
			shader->setUniform("albedoMap", albedoMap);
			shader->setUniform("ao_r_m", ao_r_mMap);
			shader->setUniform("normalMap", normalMap);
			shader->use();
		} else {
			assert(0 && name.c_str()); // shader 없음
		}
		vo->render();
	}

	virtual bool guiStart() {
		if (Obj::guiStart()) {
			ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("roughness", &roughness, 0.0f, 1.0f, "%.2f");
			ImGui::SliderFloat("ao", &ao, 0.0f, 1.0f, "%.2f");
			ImGui::ColorEdit3("albedo##3", (float*)&albedo);
			if (ImGui::ImageButton((void*)albedoMap.getId(), ImVec2(32, 32)))
				openFile(albedoMap);
			ImGui::SameLine(); ImGui::Text("albedo");
			if(ImGui::ImageButton((void*)ao_r_mMap.getId(), ImVec2(32, 32)))
				openFile(ao_r_mMap);
			ImGui::SameLine(); ImGui::Text("ao_r_m");
			if(ImGui::ImageButton((void*)normalMap.getId(), ImVec2(32, 32)))
				openFile(normalMap);
			ImGui::SameLine(); ImGui::Text("normal");
			return true;
		}
		return false;
	}

#define DEFAULT_PATH "/"
	void openFile(Texture& texture) {
		// File open
		auto f = pfd::open_file("Choose files to read", DEFAULT_PATH,
			{ "*.png *.jpg *.tga", "*" });
		std::cout << "Selected files:";
		for (auto const& name : f.result()) {
			texture = *Texture::load(name.c_str());
			return;
		}
	}
};

class ShapeObj : public PbrObj {
public:
	float speed = 6;
	virtual void tick(float dt) {
		updateTransform();
		//rot.y += dt * speed;
	}
	void initName() {
		name = (char*)"ShapeObj";
	}

};

	void APIENTRY MessageCallback(GLenum source, GLenum type, GLuint id,
		GLenum severity, GLsizei length,
		const GLchar* msg, const void* data) {
	char* _source;
	char* _type;
	char* _severity;

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		_source = (char*)"API";
		break;

	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		_source = (char*)"WINDOW SYSTEM";
		break;

	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		_source = (char*)"SHADER COMPILER";
		break;

	case GL_DEBUG_SOURCE_THIRD_PARTY:
		_source = (char*)"THIRD PARTY";
		break;

	case GL_DEBUG_SOURCE_APPLICATION:
		_source = (char*)"APPLICATION";
		break;

	case GL_DEBUG_SOURCE_OTHER:
		_source = (char*)"UNKNOWN";
		break;

	default:
		_source = (char*)"UNKNOWN";
		break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		_type = (char*)"ERROR";
		break;

	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		_type = (char*)"DEPRECATED BEHAVIOR";
		break;

	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		_type = (char*)"UDEFINED BEHAVIOR";
		break;

	case GL_DEBUG_TYPE_PORTABILITY:
		_type = (char*)"PORTABILITY";
		break;

	case GL_DEBUG_TYPE_PERFORMANCE:
		_type = (char*)"PERFORMANCE";
		break;

	case GL_DEBUG_TYPE_OTHER:
		_type = (char*)"OTHER";
		break;

	case GL_DEBUG_TYPE_MARKER:
		_type = (char*)"MARKER";
		break;

	default:
		_type = (char*)"UNKNOWN";
		break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		_severity = (char*)"HIGH";
		break;

	case GL_DEBUG_SEVERITY_MEDIUM:
		_severity = (char*)"MEDIUM";
		break;

	case GL_DEBUG_SEVERITY_LOW:
		_severity = (char*)"LOW";
		break;

	case GL_DEBUG_SEVERITY_NOTIFICATION:
		_severity = (char*)"NOTIFICATION";
		break;

	default:
		_severity = (char*)"UNKNOWN";
		break;
	}

	if(severity != GL_DEBUG_SEVERITY_NOTIFICATION)
	printf("%d: %s of %s severity, raised from %s: %s\n",
		id, _type, _severity, _source, msg);
}

Camera* cam;

VO gridVO;


ShapeObj* triObj;
vector<ShapeObj*> orbits;
PbrObj* floorObj;

Light* light[4];



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
Cubemap envCubemap;
Cubemap irradianceMap;
Cubemap prefilterMap;

void loadPbrCubemap(const char* path= "textures/Road_to_MonumentValley_Ref.hdr") {
	auto equirectangular_to_cubemap = Shader::create("cubemap", "equirectangular_to_cubemap");
	auto irradianceShader = Shader::create("cubemap", "irradiance_convolution");
	auto prefilterShader = Shader::create("cubemap", "prefilter");
	auto pbr = Shader::create("pbr", "pbr");
	auto background = Shader::create("background", "background");

	auto hdr = Texture::loadHDR(path);


	envCubemap.init(512, 512);
	envCubemap.convertFromEquirectangular(*hdr, equirectangular_to_cubemap);

	irradianceMap.init(32, 32);
	irradianceMap.convertFromEquirectangular(envCubemap, irradianceShader);

	prefilterMap.init(128, 128);
	// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	prefilterMap.quasiMonteCarloSimulation(envCubemap, prefilterShader);

	background->setUniform("environmentMap", envCubemap);
	pbr->setUniform("prefilterMap", prefilterMap);
	pbr->setUniform("irradianceMap", irradianceMap);
}

//Texture albedoMap;
//Texture ao_r_mMap;
//Texture normalMap;
void initPbr() {
	auto pbr = Shader::create("pbr", "pbr");
	
	auto brdf = Shader::create("brdf", "brdf");

	glEnable(GL_DEPTH_TEST);
	// set depth function to less than AND equal for skybox depth trick.
	glDepthFunc(GL_LEQUAL);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	loadPbrCubemap();
	

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

	auto defaultTexture = Texture::load("textures/black.png");

	
	pbr->setUniform("brdfLUT", temp.colorTex);
	pbr->setUniform("albedoMap", *defaultTexture);
	pbr->setUniform("ao_r_m", *defaultTexture);
	pbr->setUniform("normalMap", *defaultTexture);

	//glDepthFunc(GL_LESS);
}

void init() {
	glLineWidth(3);


	triShader.complie("tri", "tri");
	gridShader.complie("grid");


	

	

	// During init, enable debug output
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);
	GLuint unusedIds = 0;
	glDebugMessageControl(GL_DONT_CARE,
		GL_DONT_CARE,
		GL_DONT_CARE,
		0,
		&unusedIds,
		true);


	cam = new Camera();
	cam->armVector.z = -20;
	cam->rotateY(30);
	cam->rotateX(-30);

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


	initPbr();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	auto pbr = Shader::get("pbr,pbr");

	for (size_t i = 0; i < 4; i++) {
		light[i] = new Light();
		light[i]->setPos(lightPositions[i]);
		light[i]->color = lightColors[i];
		light[i]->setLightToShader(*pbr);
	}

	/*triObj = new ShapeObj();
	triObj->loadObj("model/gun.obj");
	triObj->setShader(*pbr);
	triObj->setScale(vec3(9));
	triObj->name = "gun";*/
	auto defaultTex = Texture::load("textures/black.png");
	auto fractal = Shader::create("tri", "fractal");

	floorObj = new PbrObj();
	floorObj->loadObj("model/cube.obj");
	floorObj->setShader(*fractal);
	floorObj->setScale(vec3(5, 0.1f, 5));
	floorObj->setPos(vec3(-1, 0, 0));
	floorObj->name = "floor";
	floorObj->setTextures(*defaultTex, *defaultTex, *defaultTex);

	orbits.push_back(new ShapeObj());
	orbits.push_back(new ShapeObj());
	orbits.push_back(new ShapeObj());
	int armLen = -2;
	for (auto var : orbits) {
		armLen += 4;
		var->setPos(vec3(armLen, 0, 0));
		var->setShader(*pbr);
		var->loadObj("model/sphere.obj");
		var->color = vec4(f(), f(), f(), f());
		var->name = "sphere";
		var->setTextures(*defaultTex, *defaultTex, *defaultTex);
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
		cam->rotateX(dt * 10 * Input::mouse[EMouse::MOUSE_OFF_Y]);
		cam->rotateY(dt * 10 * Input::mouse[EMouse::MOUSE_OFF_X]);
	}
	if (Input::mouse[EMouse::MOUSE_M_BUTTON]) {
		vec3 move(dt * 1 * Input::mouse[EMouse::MOUSE_OFF_X], dt * -1 * Input::mouse[EMouse::MOUSE_OFF_Y],0);
		cam->translate(move);
	}
	Input::mouse[EMouse::MOUSE_WHEEL] = clamp(Input::mouse[EMouse::MOUSE_WHEEL], -100, -1);
	cam->armVector.z = Input::mouse[EMouse::MOUSE_WHEEL];

	Bullet::tick(dt);
	Scene::activeScene->tick(dt);

	Input::getMousePos();

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		// Process/log the error.
	}

	glutPostRedisplay();
}

vec2 fractalOffset = vec2(0, 0);
float fractalZoom = 1;
vec2 fractalJc = vec2(0, 0);
void imguiRender() {
	auto pbr = Shader::get("pbr,pbr");
	// Main body of the Demo window starts here.
	if (!ImGui::Begin("HI")) {
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}
	if (ImGui::TreeNode("etc")) {
		if (ImGui::Button("shader recomplie")) {
			Shader::recomplieAll();
			//pbr->recomplie();
		}
		if (ImGui::Button("Load Obj##3b")) {
			auto f = pfd::open_file("Choose files to read", DEFAULT_PATH,
				{ "*.obj"});
			std::cout << "Selected files:";

			auto vec = f.result();
			if (vec.size() > 0) {

				auto temp = new PbrObj();
				temp->loadObj(vec[0].c_str());
				auto defaultTexture = Texture::load("textures/black.png");
				temp->albedoMap = *defaultTexture;
				temp->ao_r_mMap = *defaultTexture;
				temp->normalMap = *defaultTexture;
				temp->setShader(*Shader::get("pbr,pbr"));
				temp->name = "load";
			}
		}
		if (ImGui::Button("Load HDR##3b")) {
			auto f = pfd::open_file("Choose files to read", DEFAULT_PATH,
				{ "*.hdr *.jpg *.png" });
			std::cout << "Selected files:";
			auto vec = f.result();
			if (vec.size() > 0) {
				loadPbrCubemap(vec[0].c_str());
			}
		}
		
		ImGui::DragFloat2("offset", (float*)&fractalOffset, 0.001f);
		ImGui::DragFloat("zoom", (float*)&fractalZoom, 0.001f);
		ImGui::DragFloat2("jc", (float*)&fractalJc, 0.05f);
		Shader::get("tri,fractal")->setUniform("offset", fractalOffset);
		Shader::get("tri,fractal")->setUniform("zoom", fractalZoom);
		Shader::get("tri,fractal")->setUniform("jc", fractalJc);

		ImGui::TreePop();
	}
	sterma::renderWindow();
	Scene::self().gui();
	ImGui::End();
}


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


	
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGLUT_NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(200, 500), ImGuiCond_Once);

	imguiRender();


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
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE
#if _DEBUG
		| GLUT_DEBUG
#endif
	); glutCreateWindow(__FILE__); // 윈도우 생성(윈도우 이름)

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

	const char* glsl_version = (char*)"#version 330";
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
	printf("Reshape %d %d\n", w,h);
	Window::get().init(w, h);
	ImGui_ImplGLUT_ReshapeFunc(w, h);
	glViewport(0, 0, w, h);
}

void exit() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGLUT_Shutdown();
	ImGui::DestroyContext();
}
