#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include "Window.h"

#include <iostream>
#include <random>

gps::Window myWindow;

int glWindowWidth = 800;
int glWindowHeight = 600;

const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 8192;

const GLfloat near_plane = 0.1f, far_plane = 100.0f;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

gps::Camera myCamera(
	glm::vec3(0.0f, 2.0f, 9.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.2f;

bool pressedKeys[1024];
GLfloat lightAngle;

gps::Model3D screenQuad;
gps::Model3D snowGround;
gps::Model3D snowMobile;
gps::Model3D snowflake;
gps::Model3D floatingPenguin;
gps::Model3D lamp;
gps::Model3D towerFlame;
gps::Model3D compass;
gps::Model3D compassNeedle;
gps::Model3D compassCase;

gps::Shader myCustomShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;
bool showDepthMap = false;

gps::SkyBox skyboxDay;
gps::Shader skyboxShader;
gps::SkyBox skyboxNight;

float pitch = 0.0f;
float yaw = 0.0f;

bool firstMouse = true;
double lastX, lastY;
float sensitivity = 0.02f;

float snowCarZ = 0.0f;

float delta = 0.0f;
float movementSpeed = 2.0f;
float rotationSpeed = 20.0f;
double currentTime = 0;
double lastTimeStamp = glfwGetTime();

glm::vec3 snowflakePosition;
bool snow = false;
int nbOfSnowflakes = 3000;

float angleYPenguin = 0.0f;
float rotationRadius = 1.0f;

bool sunsetTime = false;

glm::vec3 penguinInitialPosition = glm::vec3(10.0f, -0.9f, -8.0f);
glm::vec3 lamp1Pos = glm::vec3(-16.0f, -0.7f, -14.0f);
glm::vec3 lamp2Pos = glm::vec3(-12.0f, -0.7f, -17.0f);
glm::vec3 towerFlamePos = glm::vec3(-28.0f, 0.6f, -31.0f);

bool presentation = true;
float moveLeftPresentation = 0.0f;
float anglePresentation = 0.0f;
float moveForwardPresentation = 0.0f;
float moveBackwardPresentation = 0.0f;


void updateDelta(double elapsedSeconds) {

	delta = static_cast<float>(movementSpeed * elapsedSeconds);
}

void updateSnowMobilePosition() {

	double currentTimeStamp = glfwGetTime();
	updateDelta(currentTimeStamp - lastTimeStamp);
	lastTimeStamp = currentTimeStamp;
}

GLenum glCheckError_(const char* file, int line) {

	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {

	//TODO	
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);

	myCustomShader.useShaderProgram();

	WindowDimensions newWindowDimensions;
	newWindowDimensions.width = width;
	newWindowDimensions.height = height;

	myWindow.setWindowDimensions(newWindowDimensions);

	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

	projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");

	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		showDepthMap = !showDepthMap;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		snow = !snow;
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		sunsetTime = !sunsetTime;
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = static_cast<float>(xpos - lastX);
	float yoffset = static_cast<float>(ypos - lastY);
	lastX = xpos;
	lastY = ypos;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw -= xoffset;
	pitch -= yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -60.0f)
		pitch = -60.0f;

	myCamera.rotate(pitch, yaw);
}

void computeSnow(gps::Shader shader, bool depthPass) {

	for (int i = 0; i < nbOfSnowflakes; i++) {
		model = glm::mat4(1.0f);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> xDist(-70.0f, 70.0f);
		std::uniform_real_distribution<float> yDist(-7.0f, 50.0f);
		std::uniform_real_distribution<float> zDist(-70.0f, 70.0f);

		snowflakePosition = glm::vec3(xDist(gen), yDist(gen), zDist(gen));

		model = glm::translate(model, snowflakePosition);

		glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		if (!depthPass) {
			normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
			glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		}

		snowflake.Draw(shader);
	}
}

void processMovement() {

	if (pressedKeys[GLFW_KEY_J]) {
		lightAngle -= 1.0f;
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle += 1.0f;
	}

	if (pressedKeys[GLFW_KEY_Z]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //solid view
	}

	if (pressedKeys[GLFW_KEY_X]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe view
	}

	if (pressedKeys[GLFW_KEY_C]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); //poligonal view
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}

	if (pressedKeys[GLFW_KEY_I]) {
		updateSnowMobilePosition();
		snowCarZ -= delta;
	}

	if (pressedKeys[GLFW_KEY_K]) {
		updateSnowMobilePosition();
		snowCarZ += delta;
	}

	if (pressedKeys[GLFW_KEY_N]) {
		nbOfSnowflakes += 250;
	}

	if (pressedKeys[GLFW_KEY_B]) {
		nbOfSnowflakes -= 250;
	}

	lastTimeStamp = glfwGetTime();
}

void initOpenGLWindow() {

	myWindow.Create(glWindowWidth, glWindowHeight, "Arctic wildness");
	glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSwapInterval(1);
}

void setWindowCallbacks() {

	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
	glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
	glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels() {

	screenQuad.LoadModel("objects/quad/quad.obj");
	snowGround.LoadModel("objects/scene/snowScene.obj");
	snowMobile.LoadModel("objects/snowmobile/snowmobile.obj");
	snowflake.LoadModel("objects/snowflake/snowflake.obj");
	floatingPenguin.LoadModel("objects/penguin/penguin.obj");
	lamp.LoadModel("objects/lamp/lamp.obj");
	towerFlame.LoadModel("objects/towerFlame/towerFlame.obj");
}

void initShaders() {

	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	depthMapShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initUniforms() {

	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(20.0f, 30.0f, 0.0f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
}

void initFBO() {

	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	//generate FBO ID 
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//attach texture to FBO 
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initSkyboxDay() {

	std::vector<const GLchar*> faces;
	faces.push_back("skyboxDay/right.tga");
	faces.push_back("skyboxDay/left.tga");
	faces.push_back("skyboxDay/top.tga");
	faces.push_back("skyboxDay/bottom.tga");
	faces.push_back("skyboxDay/back.tga");
	faces.push_back("skyboxDay/front.tga");
	skyboxDay.Load(faces);
}

void initSkyboxNight() {

	std::vector<const GLchar*> faces;
	faces.push_back("skyboxNight/right.tga");
	faces.push_back("skyboxNight/left.tga");
	faces.push_back("skyboxNight/top.tga");
	faces.push_back("skyboxNight/bottom.tga");
	faces.push_back("skyboxNight/back.tga");
	faces.push_back("skyboxNight/front.tga");
	skyboxNight.Load(faces);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix
	glm::mat4 lightView = glm::lookAt(glm::mat3(lightRotation) * lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

void floatingPenguinAnimation() {

	double currentTimeStamp = glfwGetTime();
	updateDelta(currentTimeStamp - lastTimeStamp);
	lastTimeStamp = currentTimeStamp;

	angleYPenguin = static_cast<float>(glm::radians(rotationSpeed * currentTimeStamp));
}

void drawObjects(gps::Shader shader, bool depthPass) {

	shader.useShaderProgram();

	model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	snowGround.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-6.0f, -0.7f, snowCarZ));
	model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
	model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	// do not send the normal matrix if we are rendering in the depth map
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	snowMobile.Draw(shader);

	if (snow) {
		computeSnow(shader, depthPass);
	}

	floatingPenguinAnimation();

	model = glm::translate(glm::mat4(1.0f), penguinInitialPosition);
	model = glm::rotate(model, angleYPenguin, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(rotationRadius * cos(angleYPenguin), 0.0f, rotationRadius * sin(angleYPenguin)));
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	floatingPenguin.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), lamp1Pos);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	lamp.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), lamp2Pos);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	lamp.Draw(shader);

	model = glm::translate(glm::mat4(1.0f), towerFlamePos);
	model = glm::scale(model, glm::vec3(2.5f, 2.5f, 2.5f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}

	towerFlame.Draw(shader);
}

void renderScene() {

	//depth maps creation pass
	//TODO - Send the light-space transformation matrix to the depth map creation shader and render the scene in the depth map
	//render depth map on screen - toggled with the M key
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	drawObjects(depthMapShader, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (showDepthMap) {
		glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {
		// final scene rendering pass (with shadows)
		glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		if (sunsetTime) {
			//set the light direction (direction towards the light)
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
		}
		else {
			//set the light direction (direction towards the light)
			lightDir = glm::vec3(20.0f, 30.0f, 0.0f);
			lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
			lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
			glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		}

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "posLightPoint1"), 1, glm::value_ptr(lamp1Pos));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "posLightPoint2"), 1, glm::value_ptr(lamp2Pos));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "posLightPoint3"), 1, glm::value_ptr(glm::vec3(-28.305f, 7.9f, -30.798f )));
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "linear"), 1.2f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "constant"), 1.0f);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "quadratic"), 1.7f);
		drawObjects(myCustomShader, false);
	}
	if (sunsetTime) {
		skyboxNight.Draw(skyboxShader, view, projection);
	}
	else {
		skyboxDay.Draw(skyboxShader, view, projection);
	}
}

void cleanup() {

	myWindow.Delete();
	//cleanup code for your own data
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwTerminate();
}

void cameraAnimation() {
	if (presentation) {
		if (moveLeftPresentation< 70.0f) {
			myCamera.move(gps::MOVE_LEFT, cameraSpeed);
			moveLeftPresentation++;
		}
		else if (anglePresentation < 89.9f) {
			myCamera.rotate(pitch, yaw - anglePresentation);
			anglePresentation++;
		}
		else if (anglePresentation == 90.0f && moveForwardPresentation < 30.0f) {
			myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
			moveForwardPresentation++;
		}
		else if (moveForwardPresentation == 30.0f) {
			myCamera.rotate(pitch, anglePresentation - yaw);
			moveForwardPresentation++;
		}
		else if (moveBackwardPresentation < 200.0f) {
			myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
			moveBackwardPresentation++;
		}
		else {
			presentation = false;
		}

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}
}

int main(int argc, const char* argv[]) {

	try {
		initOpenGLWindow();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	initOpenGLState();
	initModels();
	initSkyboxDay();
	initSkyboxNight();
	initShaders();
	initUniforms();
	initFBO();
	setWindowCallbacks();

	glCheckError();

	while (!glfwWindowShouldClose(myWindow.getWindow())) {
		cameraAnimation();
		processMovement();
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());
		glCheckError();
	}

	cleanup();

	return EXIT_SUCCESS;
}
