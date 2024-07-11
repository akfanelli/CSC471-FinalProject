/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

// extra technology is adding shadows 
// possible additional thing is trying to make pixel shader

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Bezier.h"
#include "Spline.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>
#define PI 3.1415927

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

typedef struct 
{
	// holds shape info
	float scale;
	vec3 translate;
} ShapeInfo;

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

class camera
{
public:
	glm::vec3 pos, front, target, up;
	float yaw, pitch;
	int w, a, s, d;
    double deltaX, deltaY;
	camera()
	{
		w = a = s = d = 0;
		pos = glm::vec3(0, 0, 0);
        front = glm::vec3(0, 0, -1);
        up = glm::vec3(0, 1, 0);
	}
	glm::mat4 process(float ftime)
	{
		pitch = pitch + deltaY * 0.25;
		// limit looking up and down to radians(80)
		if (pitch > 80)
			pitch = 80;
			
		// allows 360 view, left and right
		yaw = yaw + deltaX * 0.25;
			
		// calculate the front vector
		front.x = cos(radians(yaw)) * cos(radians(pitch));
		front.y = -1.0 * sin(radians(pitch));
		front.z = sin(radians(yaw)) * cos(radians(pitch));
			
		// calculates position
		float speed = 0;
		if (w == 1)
		{
			speed = 10*ftime;
			pos = pos + speed * front;
		}
		else if (s == 1)
		{
			speed = -10*ftime;
			pos = pos - speed * front;
		}
		if (a == 1)
			pos = pos - speed * normalize(cross(front, up));
		else if(d==1)
			pos = pos + speed * normalize(cross(front, up));
			
		target = pos + front;
		
        // new View matrix
        glm::mat4 V = glm::lookAt(pos, target, up);
        return V;
	}
	glm::mat4 processBezier(float ftime)
	{
		target = vec3(1, -0.5, 5);
		glm::mat4 V = glm::lookAt(pos, target, up);
        return V;
	}
};

camera mycam;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

	std::shared_ptr<Program> skyProg;

	std::shared_ptr<Program> depthProg;

	std::shared_ptr<Program> shadowProg;

	//our geometry
	shared_ptr<Shape> sphere;

	vector<shared_ptr<Shape>> theBunny, barn, cow, log, fence, tree, house, lamp, well;
	vector<ShapeInfo> bunnyInfo, barnInfo, cowInfo, logInfo, fenceInfo, treeInfo, houseInfo, lampInfo, wellInfo;

	//global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	//ground VAO
	GLuint GroundVertexArrayID;

	//the image to use as a texture (ground)
	shared_ptr<Texture> texture0, barnTex, cowTex, textureDaySky, textureNightSky, wallsTex, houseTex, roofTex, fenceTex;
	shared_ptr<Texture> logTex, lampTex, wellWoodTex, wellBrickTex, wellFabricTex, wellMetalTex, wellRoofTex;
	float day_night_ratio = 0;
	
	//global data (larger program should be encapsulated)
	vec3 gMin;
	float gRot = 0;
	float gCamH = 0;
	float gCamZ = 0;
	//animation data
	float lightTrans = 0;
	int matChange = 3;
	float gTrans = -3;
	float sTheta = 0;
	float eTheta = 0;
	float hTheta = 0;

	// camera
	double g_theta;
	Spline splinepath[2];
	bool goCamera = false;

	// shadow mapping
	GLuint depthFBO;
	GLuint depthTex;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
			mycam.pos.z++;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
			mycam.pos.z--;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
			mycam.pos.x++;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
			mycam.pos.x--;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}

		// updates light 
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans -= 0.25;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans += 0.25;
		}
		if (key == GLFW_KEY_M && action == GLFW_PRESS){
			if (matChange == 3)
			{
				matChange = 4;
			}
			else
			{
				matChange = 3;
			}
		}

		// activate bezier curve
		if (key == GLFW_KEY_G && action == GLFW_RELEASE) {
			goCamera = !goCamera;
		}

		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void scrollCallback(GLFWwindow *window, double in_deltaX, double in_deltaY)
    {
        mycam.deltaX = in_deltaX;
        mycam.deltaY = in_deltaY;
        cout << "Delta X: " << mycam.deltaX << " Delta Y: " << mycam.deltaY << endl;
    }

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		g_theta = -PI/2.0;

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor"); 
		prog->addAttribute("vertTex");

		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("Texture0");
		texProg->addUniform("lightPos");
		texProg->addUniform("flip");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex"); 

		skyProg = make_shared<Program>();
		skyProg->setVerbose(true);
		skyProg->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		skyProg->init();
		skyProg->addUniform("P");
		skyProg->addUniform("V");
		skyProg->addUniform("M");
		skyProg->addUniform("tex");
		skyProg->addUniform("tex2");
		skyProg->addUniform("dayNight");
		skyProg->addAttribute("vertPos");
		skyProg->addAttribute("vertNor");
		skyProg->addAttribute("vertTex");

		depthProg = make_shared<Program>();
		depthProg->setVerbose(true);
		depthProg->setShaderNames(resourceDirectory + "/depth_vert.glsl", resourceDirectory + "/depth_frag.glsl");
		depthProg->init();
		depthProg->addUniform("lightSpaceMat");
		depthProg->addAttribute("vertPos");

		shadowProg = make_shared<Program>();
		shadowProg->setVerbose(true);
		shadowProg->setShaderNames(resourceDirectory + "/depthShader_vert.glsl", resourceDirectory + "/depthShader_frag.glsl");
		shadowProg->init();
		shadowProg->addUniform("P");
		shadowProg->addUniform("V");
		shadowProg->addUniform("M");
		shadowProg->addUniform("Texture0");
		shadowProg->addUniform("shadowMap");
		shadowProg->addUniform("lightPos");
		shadowProg->addUniform("viewPos");
		shadowProg->addUniform("lightSpaceMat");
		shadowProg->addAttribute("vertPos");
		shadowProg->addAttribute("vertNor");
		shadowProg->addAttribute("vertTex");

		//read in a load the texture
		initTex(resourceDirectory);

		// init splines up and down
       splinepath[0] = Spline(glm::vec3(-9,4,5), glm::vec3(-1,1,10), glm::vec3(1, 10, 5), glm::vec3(8,1,10), 5);
       splinepath[1] = Spline(glm::vec3(10,1,10), glm::vec3(-2,8,5), glm::vec3(2, -1, 5), glm::vec3(-6,5,10), 10);

	   createFBO(depthFBO, depthTex);
	}

	void createFBO(GLuint& fb, GLuint& tex) 
	{
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		//set up framebuffer
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		//set up texture
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = {1.0, 1.0, 1.0, 1.0};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			cout << "Error setting up frame buffer - exiting" << endl;
			exit(0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
  	}

	void initTex(const std::string& resourceDirectory)
	{
		// code to load in the three textures
		texture0 = make_shared<Texture>();
  		texture0->setFilename(resourceDirectory + "/grass.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		barnTex = make_shared<Texture>();
  		barnTex->setFilename(resourceDirectory + "/barn/barn.jpg");
  		barnTex->init();
  		barnTex->setUnit(1);
  		barnTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		textureDaySky = make_shared<Texture>();
  		textureDaySky->setFilename(resourceDirectory + "/sphere-day.jpg");
  		textureDaySky->init();
  		textureDaySky->setUnit(2);
  		textureDaySky->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		textureNightSky = make_shared<Texture>();
  		textureNightSky->setFilename(resourceDirectory + "/skyview-night.jpg");
  		textureNightSky->init();
  		textureNightSky->setUnit(3);
  		textureNightSky->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		cowTex = make_shared<Texture>();
  		cowTex->setFilename(resourceDirectory + "/cow/cow.jpg");
  		cowTex->init();
  		cowTex->setUnit(4);
  		cowTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		wallsTex = make_shared<Texture>();
  		wallsTex->setFilename(resourceDirectory + "/house/textures/House_Side_D.jpg");
  		wallsTex->init();
  		wallsTex->setUnit(5);
  		wallsTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		houseTex = make_shared<Texture>();
  		houseTex->setFilename(resourceDirectory + "/house/textures/Farm_house_D.jpg");
  		houseTex->init();
  		houseTex->setUnit(6);
  		houseTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		roofTex = make_shared<Texture>();
  		roofTex->setFilename(resourceDirectory + "/house/textures/Roof_D.jpg");
  		roofTex->init();
  		roofTex->setUnit(7);
  		roofTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		fenceTex = make_shared<Texture>();
  		fenceTex->setFilename(resourceDirectory + "/fence/fence.jpg");
  		fenceTex->init();
  		fenceTex->setUnit(8);
  		fenceTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		lampTex = make_shared<Texture>();
  		lampTex->setFilename(resourceDirectory + "/lamp/textures/Marmitespecular.jpg");
  		lampTex->init();
  		lampTex->setUnit(9);
  		lampTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		logTex = make_shared<Texture>();
  		logTex->setFilename(resourceDirectory + "/log/log.jpg");
  		logTex->init();
  		logTex->setUnit(10);
  		logTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		wellBrickTex = make_shared<Texture>();
  		wellBrickTex->setFilename(resourceDirectory + "/well/brick.jpg");
  		wellBrickTex->init();
  		wellBrickTex->setUnit(11);
  		wellBrickTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		wellFabricTex = make_shared<Texture>();
  		wellFabricTex->setFilename(resourceDirectory + "/well/fabric.jpg");
  		wellFabricTex->init();
  		wellFabricTex->setUnit(12);
  		wellFabricTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		wellMetalTex = make_shared<Texture>();
  		wellMetalTex->setFilename(resourceDirectory + "/well/metal.jpg");
  		wellMetalTex->init();
  		wellMetalTex->setUnit(13);
  		wellMetalTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		wellRoofTex = make_shared<Texture>();
  		wellRoofTex->setFilename(resourceDirectory + "/well/roof.jpg");
  		wellRoofTex->init();
  		wellRoofTex->setUnit(14);
  		wellRoofTex->setWrapModes(GL_REPEAT, GL_REPEAT);

		wellWoodTex = make_shared<Texture>();
  		wellWoodTex->setFilename(resourceDirectory + "/well/wood.jpg");
  		wellWoodTex->init();
  		wellWoodTex->setUnit(15);
  		wellWoodTex->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	// set shape info for multiple shapes
	void SetShapeInfo(shared_ptr<Shape> &shape, vector<ShapeInfo> &shapeInfo)
	{
		ShapeInfo info;
		// find the extents
		float xExt = shape->max.x - shape->min.x;
		float yExt = shape->max.y - shape->min.y;
		float zExt = shape->max.z - shape->min.z;
		float maxExt = std::max(xExt, yExt);
		maxExt = std::max(zExt, maxExt);
		// find the center
		float x = shape->min.x + 0.5 * xExt;
		float y = shape->min.y + 0.5 * yExt;
		float z = shape->min.z + 0.5 * zExt;
		// find scale
		info.scale = 2.0 / maxExt;
		// find translate
		info.translate = -1.0f * vec3(x, y, z);
		shapeInfo.push_back(info);
	}

	// loads mesh for multiple shape object 
	void loadMesh(const std::string& resourceDirectory, const std::string& resourceName, vector<shared_ptr<Shape>> &shapes, vector<ShapeInfo> &shapesInfo)
	{
		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		//load in the mesh and make the shapes
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + resourceName).c_str());
		
		if (!rc) {
			cerr << errStr << endl;
		} else {
			//for now all our shapes will not have textures - change in later labs
			for (int i = 0; i < TOshapes.size(); ++i)
			{
				shapes.push_back(make_shared<Shape>());
				shapes[i]->createShape(TOshapes[i]);
				shapes[i]->measure();
				shapes[i]->init();

				SetShapeInfo(shapes[i], shapesInfo);
			}
		}
	}

	void initGeom(const std::string& resourceDirectory)
	{
		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		//load in the mesh and make the shape(s)
 		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		} else {
			sphere = make_shared<Shape>();
			sphere->createShape(TOshapes[0]);
			sphere->measure();
			sphere->init();
		}
		//read out information stored in the shape about its size - something like this...
		//then do something with that information.....
		gMin.x = sphere->min.x;
		gMin.y = sphere->min.y;

		// Initialize bunny mesh.
		loadMesh(resourceDirectory, "/bunnyNoNorm.obj", theBunny, bunnyInfo);
		loadMesh(resourceDirectory, "/barn/Rbarn15.obj", barn, barnInfo);
		loadMesh(resourceDirectory, "/cow/Cow.obj", cow, cowInfo);
		loadMesh(resourceDirectory, "/Tree/Tree3.obj", tree, treeInfo);
		loadMesh(resourceDirectory, "/log/low_poly_log.obj", log, logInfo);
		loadMesh(resourceDirectory, "/fence/fence.obj", fence, fenceInfo);
		loadMesh(resourceDirectory, "/house/Farm_house.obj", house, houseInfo);
		loadMesh(resourceDirectory, "/lamp/Lamp.obj", lamp, lampInfo);
		loadMesh(resourceDirectory, "/well/well.obj", well, wellInfo);
		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();
	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 20;
		float g_groundY = -0.25;

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, 1,
      		1, 1,
      		1, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
      }

      //code to draw the ground plane
     void drawGround(shared_ptr<Program> curS) {
     	curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
		//draw the ground plane 
  		SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		curS->unbind();
     }

	 void drawGroundShadow(shared_ptr<Program> curS) {
     	curS->bind();
     	glBindVertexArray(GroundVertexArrayID);
     	texture0->bind(curS->getUniform("Texture0"));
		//draw the ground plane 
  		SetModel(vec3(0, -1, 0), 0, 0, 1, curS);
  		glEnableVertexAttribArray(0);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
  		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(1);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
  		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

  		glEnableVertexAttribArray(2);
  		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
  		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

   		// draw!
  		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
  		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

  		glDisableVertexAttribArray(0);
  		glDisableVertexAttribArray(1);
  		glDisableVertexAttribArray(2);
  		curS->unbind();
     }

     //helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i) {

    	switch (i) {
    		case 0: // pink 
    			glUniform3f(curS->getUniform("MatAmb"), 0.096, 0.046, 0.095);
    			glUniform3f(curS->getUniform("MatDif"), 0.96, 0.46, 0.95);
    			glUniform3f(curS->getUniform("MatSpec"), 0.45, 0.23, 0.45);
    			glUniform1f(curS->getUniform("MatShine"), 120.0);
    		break;
    		case 1: // purple
    			glUniform3f(curS->getUniform("MatAmb"), 0.063, 0.038, 0.1);
    			glUniform3f(curS->getUniform("MatDif"), 0.63, 0.38, 1.0);
    			glUniform3f(curS->getUniform("MatSpec"), 0.3, 0.2, 0.5);
    			glUniform1f(curS->getUniform("MatShine"), 4.0);
    		break;
    		case 2: // blue
    			glUniform3f(curS->getUniform("MatAmb"), 0.004, 0.05, 0.09);
    			glUniform3f(curS->getUniform("MatDif"), 0.04, 0.5, 0.9);
    			glUniform3f(curS->getUniform("MatSpec"), 0.02, 0.25, 0.45);
    			glUniform1f(curS->getUniform("MatShine"), 27.9);
    		break;
			case 3: // light green
				glUniform3f(curS->getUniform("MatAmb"), 0.004, 0.1, 0.003);
    			glUniform3f(curS->getUniform("MatDif"), 0.002, 0.9, 0.25);
    			glUniform3f(curS->getUniform("MatSpec"), 0.09, 0.64, 0.24);
    			glUniform1f(curS->getUniform("MatShine"), 10.5);
			break;
			case 4: // dark green
				glUniform3f(curS->getUniform("MatAmb"), 0.1, 0.1, 0.03);
    			glUniform3f(curS->getUniform("MatDif"), 0.323, 0.6, 0.0);
    			glUniform3f(curS->getUniform("MatSpec"), 0.9, 0.64, 0.24);
    			glUniform1f(curS->getUniform("MatShine"), 2.0);
			break;
			case 5: // red
				glUniform3f(curS->getUniform("MatAmb"), 0.09, 0.01, 0.0075);
    			glUniform3f(curS->getUniform("MatDif"), 0.85, 0.02, 0.1);
    			glUniform3f(curS->getUniform("MatSpec"), 0.75, 0.16, 0.04);
    			glUniform1f(curS->getUniform("MatShine"), 75.3);
			break;
			case 6: // brown
				glUniform3f(curS->getUniform("MatAmb"), 0.1, 0.05, 0.001);
    			glUniform3f(curS->getUniform("MatDif"), 0.9, 0.3, 0.1);
    			glUniform3f(curS->getUniform("MatSpec"), 0.5, 0.03, 0.1);
    			glUniform1f(curS->getUniform("MatShine"), 150);
			break;
			case 7: // white
				glUniform3f(curS->getUniform("MatAmb"), 0.1, 0.05, 0.001);
    			glUniform3f(curS->getUniform("MatDif"), 0.9, 0.9, 0.9);
    			glUniform3f(curS->getUniform("MatSpec"), 0.5, 0.03, 0.1);
    			glUniform1f(curS->getUniform("MatShine"), 45.0);
			break;
  		}
	}

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

   	/* code to draw waving hierarchical model */
   	void drawHierModel(shared_ptr<Program> prog, shared_ptr<MatrixStack> Model, int num) {
   		// draw hierarchical mesh - replace with your code if desired
		prog->bind();
		float sTheta = sin(glfwGetTime());
		float eTheta = cos(glfwGetTime());
		float wTheta = -sin(glfwGetTime());
		if (num == 1)
			SetMaterial(prog, 7);
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-10, 0, 5));
			Model->scale(vec3(0.4));
			/* draw top cube - aka head */
			Model->pushMatrix();
				Model->translate(vec3(0, 1.4, 0));
				Model->scale(vec3(0.5, 0.5, 0.5));
				setModel(prog, Model);
				sphere->draw(prog);
			Model->popMatrix();
			//draw the torso with these transforms
			Model->pushMatrix();
			  	Model->scale(vec3(1.25, 1.35, 1.25));
				setModel(prog, Model);
				sphere->draw(prog);
			Model->popMatrix();
			// draw the left arm
			//note you must change this to include 3 components!
			Model->pushMatrix();
				//place at shoulder
				Model->translate(vec3(0.8, 0.8, 0));
				//rotate shoulder joint
				Model->rotate(sTheta, vec3(0, 0, 1));
				//move to shoulder joint
				Model->translate(vec3(0.8, 0, 0));

			    //now draw lower arm 
			  	Model->pushMatrix();
					// place at elbow 
					Model->translate(vec3(0.6, 0, 0));
					// rotate elbow
					Model->rotate(eTheta, vec3(0, 0, 2));
					// move to elbow joint
					Model->translate(vec3(0.6, 0, 0));
					// draw hand
					Model->pushMatrix();
						// place at wrist
						Model->translate(vec3(0.5, 0, 0));
						// rotate wrist
						Model->rotate(wTheta, vec3(0, 0, 4));
						// move to wristjoint
						Model->translate(vec3(0.5, 0, 0));
						Model->scale(vec3(0.4, 0.25, 0.25));
						setModel(prog, Model);
						sphere->draw(prog);
					Model->popMatrix();

					Model->scale(vec3(0.8, 0.25, 0.25));
					setModel(prog, Model);
					sphere->draw(prog);
			  	Model->popMatrix();

				//Do final scale ONLY to upper arm then draw
				//non-uniform scale
				Model->scale(vec3(0.8, 0.25, 0.25));
				setModel(prog, Model);
				sphere->draw(prog);
			Model->popMatrix();

			// draw the right arm
			Model->pushMatrix();
				//place at shoulder
				Model->translate(vec3(-0.8, 0.8, 0));
				//rotate shoulder joint
				Model->rotate(sTheta, vec3(0, 0, 1));
				//move to shoulder joint
				Model->translate(vec3(-0.8, 0, 0));
			    //now draw lower arm 
			  	Model->pushMatrix();
					// place at elbow 
					Model->translate(vec3(-0.6, 0, 0));
					// rotate elbow
					Model->rotate(eTheta, vec3(0, 0, 2));
					// move to elbow joint
					Model->translate(vec3(-0.6, 0, 0));
					// draw hand
					Model->pushMatrix();
						// place at wrist
						Model->translate(vec3(-0.5, 0, 0));
						// rotate wrist
						Model->rotate(wTheta, vec3(0, 0, 4));
						// move to wristjoint
						Model->translate(vec3(-0.5, 0, 0));
						Model->scale(vec3(0.4, 0.25, 0.25));
						setModel(prog, Model);
						sphere->draw(prog);
					Model->popMatrix();

					Model->scale(vec3(0.8, 0.25, 0.25));
					setModel(prog, Model);
					sphere->draw(prog);
			  	Model->popMatrix();

				//Do final scale ONLY to upper arm then draw
				//non-uniform scale
				Model->scale(vec3(0.8, 0.25, 0.25));
				setModel(prog, Model);
				sphere->draw(prog);
			Model->popMatrix();
		Model->popMatrix();
		prog->unbind();
   	}

	void transScaleScaleTrans(shared_ptr<Program> curS, ShapeInfo shapeInfo, vec3 trans, vec3 scale)
	{
		// center and normalize
		mat4 transFirst = glm::translate(glm::mat4(1.0f), shapeInfo.translate);
		mat4 scaleFirst = glm::scale(glm::mat4(1.0f), vec3(shapeInfo.scale));
		// scale and move in world
		mat4 scaleSecond = glm::scale(glm::mat4(1.0f), scale);
		mat4 transSecond = glm::translate(glm::mat4(1.0f), trans);
		mat4 ctm = transSecond*scaleSecond*scaleFirst*transFirst;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void transTransScaleScaleTrans(shared_ptr<Program> curS, ShapeInfo shapeInfo, vec3 trans, vec3 scale, vec3 trans2)
	{
		// center and normalize
		mat4 transFirst = glm::translate(glm::mat4(1.0f), shapeInfo.translate);
		mat4 scaleFirst = glm::scale(glm::mat4(1.0f), vec3(shapeInfo.scale));
		// scale and move in world
		mat4 scaleSecond = glm::scale(glm::mat4(1.0f), scale);
		mat4 transSecond = glm::translate(glm::mat4(1.0f), trans);
		mat4 transThird = glm::translate(glm::mat4(1.0f), trans2);
		mat4 ctm = transThird*transSecond*scaleSecond*scaleFirst*transFirst;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void transScaleRotScaleTrans(shared_ptr<Program> curS, ShapeInfo shapeInfo, vec3 trans, vec3 scale, float angle, vec3 rotate)
	{
		// center and normalize
		mat4 transFirst = glm::translate(glm::mat4(1.0f), shapeInfo.translate);
		mat4 scaleFirst = glm::scale(glm::mat4(1.0f), vec3(shapeInfo.scale));
		// scale and move in world
		mat4 scaleSecond = glm::scale(glm::mat4(1.0f), scale);
		mat4 transSecond = glm::translate(glm::mat4(1.0f), trans);
		mat4 rot = glm::rotate(glm::mat4(1.0f), angle, rotate);
		mat4 ctm = transSecond*scaleSecond*rot*scaleFirst*transFirst;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void transTransScaleRotScaleTrans(shared_ptr<Program> curS, ShapeInfo shapeInfo, vec3 trans, vec3 scale, float angle, vec3 rotate, vec3 trans2)
	{
		// center and normalize
		mat4 transFirst = glm::translate(glm::mat4(1.0f), shapeInfo.translate);
		mat4 scaleFirst = glm::scale(glm::mat4(1.0f), vec3(shapeInfo.scale));
		// scale and move in world
		mat4 scaleSecond = glm::scale(glm::mat4(1.0f), scale);
		mat4 transSecond = glm::translate(glm::mat4(1.0f), trans);
		mat4 rot = glm::rotate(glm::mat4(1.0f), angle, rotate);
		mat4 transThird = glm::translate(glm::mat4(1.0f), trans2);
		mat4 ctm = transThird*transSecond*scaleSecond*rot*scaleFirst*transFirst;
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
	}

	void updateUsingCameraPath(float frametime)  
	{
   	  	if (goCamera) 
		{
			if (!splinepath[0].isDone())
			{
				splinepath[0].update(frametime);
				mycam.pos = splinepath[0].getPosition();
			} 
			else 
			{
				splinepath[1].update(frametime);
				mycam.pos = splinepath[1].getPosition();
				goCamera = !goCamera;
			}
      	}
   	}

	void drawHouse(shared_ptr<Program> texProg, shared_ptr<MatrixStack> Model, int num)
	{
		texProg->bind();
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-15, 2.5, 10));
			Model->scale(vec3(0.075));
			// base of house
			Model->pushMatrix();
				setModel(texProg, Model);
				if (num == 1)
					houseTex->bind(texProg->getUniform("Texture0"));
				house[0]->draw(texProg);
			Model->popMatrix();
			// walls
			Model->pushMatrix();
				setModel(texProg, Model);
				if (num == 1)
					wallsTex->bind(texProg->getUniform("Texture0"));
				house[1]->draw(texProg);
			Model->popMatrix();
			// roof
			Model->pushMatrix();
				setModel(texProg, Model);
				if (num == 1)
					roofTex->bind(texProg->getUniform("Texture0"));
				house[2]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();
		texProg->unbind();
	}

	void drawAnimals(int num)
	{
		if (num == 1)
			SetMaterial(prog, 7);
		// jumping bunnies
		transTransScaleScaleTrans(prog, bunnyInfo[0], vec3(0, -0.75, 0), vec3(0.5), vec3(0, fabs(0.1*sin(glfwGetTime())), 0));
		theBunny[0]->draw(prog);
		transTransScaleRotScaleTrans(prog, bunnyInfo[0], vec3(-2, -0.75, 2), vec3(0.5), 90.0f, vec3(0, 1, 0), vec3(0, fabs(0.1*cos(glfwGetTime())), 0));
		theBunny[0]->draw(prog);

		// cows
		transScaleScaleTrans(prog, cowInfo[0], vec3(1, -0.5, 5), vec3(1));
		cow[0]->draw(prog);
		transScaleRotScaleTrans(prog, cowInfo[0], vec3(-3, -0.5, 2), vec3(1), 135.0f, vec3(0, 1, 0));
		cow[0]->draw(prog);
	}

	void drawLogs(shared_ptr<Program> texProg, shared_ptr<MatrixStack> Model, int num)
	{
		texProg->bind();
		if (num == 1)
			logTex->bind(texProg->getUniform("Texture0"));
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-4, -1.25, 5));
			Model->rotate(45.0f, vec3(0, 1, 0));
			Model->scale(vec3(0.009));
			Model->pushMatrix();
				setModel(texProg, Model);
				log[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(4, -1.25, 8));
			Model->rotate(90.0f, vec3(0, 1, 0));
			Model->scale(vec3(0.009));
			Model->pushMatrix();
				setModel(texProg, Model);
				log[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();
		texProg->unbind();
	}

	void drawTrees(shared_ptr<Program> texProg, shared_ptr<MatrixStack> Model, int num)
	{
		texProg->bind();
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(2, -1.25, 0));
			Model->scale(vec3(0.5));
			for (int i = 0; i < tree.size(); i++)
			{
				if (num == 1)
				{
					if (i == 0)
						SetMaterial(texProg, 6);
					else
						SetMaterial(texProg, matChange);
				}
				Model->pushMatrix();
					setModel(texProg, Model);
					tree[i]->draw(texProg);
				Model->popMatrix();
			}
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(0, -1.25, -5));
			Model->scale(vec3(0.5));
			for (int i = 0; i < tree.size(); i++)
			{
				if (num == 1)
				{
					if (i == 0)
						SetMaterial(texProg, 6);
					else
						SetMaterial(texProg, matChange);
				}
				Model->pushMatrix();
					setModel(texProg, Model);
					tree[i]->draw(texProg);
				Model->popMatrix();
			}
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-9, -1.25, -0));
			Model->scale(vec3(0.5));
			for (int i = 0; i < tree.size(); i++)
			{
				if (num == 1)
				{
					if (i == 0)
						SetMaterial(texProg, 6);
					else
						SetMaterial(texProg, matChange);
				}
				Model->pushMatrix();
					setModel(texProg, Model);
					tree[i]->draw(texProg);
				Model->popMatrix();
			}
		Model->popMatrix();
		texProg->unbind();
	}

	void drawFence(shared_ptr<Program> texProg, shared_ptr<MatrixStack> Model, int num)
	{
		texProg->bind();
		if (num == 1)
			fenceTex->bind(texProg->getUniform("Texture0"));
		// back fences
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(0, -0.95, -1));
			Model->scale(vec3(0.5));
			Model->rotate(80.0, vec3(0.0, 1.0, 0.0));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(2, -0.95, -1));
			Model->scale(vec3(0.5));
			Model->rotate(80.0, vec3(0.0, 1.0, 0.0));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();
		// front fences
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-3, -0.95, 9));
			Model->scale(vec3(0.5));
			Model->rotate(80.0, vec3(0.0, 1.0, 0.0));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-5, -0.95, 9));
			Model->scale(vec3(0.5));
			Model->rotate(80.0, vec3(0.0, 1.0, 0.0));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(0, -0.95, 9));
			Model->scale(vec3(0.5));
			Model->rotate(80.0, vec3(0.0, 1.0, 0.0));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(2, -0.95, 9));
			Model->scale(vec3(0.5));
			Model->rotate(80.0, vec3(0.0, 1.0, 0.0));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();
		// side fences
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(3, -0.95, -1));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(3, -0.95, 1));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(3, -0.95, 3));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(3, -0.95, 5));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(3, -0.95, 7));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-6.5, -0.95, -1));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-6.5, -0.95, 1));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-6.5, -0.95, 3));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-6.5, -0.95, 5));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-6.5, -0.95, 7));
			Model->scale(vec3(0.5));
			Model->pushMatrix();
				setModel(texProg, Model);
				fence[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();
		texProg->unbind();
	}

	void drawBarn(shared_ptr<Program> texProg, shared_ptr<MatrixStack> Model, int num)
	{
		texProg->bind();
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-4, -1.25, -4.5));
			Model->scale(vec3(0.004));
			Model->pushMatrix();
				setModel(texProg, Model);
				if (num == 1)
					barnTex->bind(texProg->getUniform("Texture0"));
				barn[0]->draw(texProg);
			Model->popMatrix();
		Model->popMatrix();
		texProg->unbind();
	}

	void drawLamp(shared_ptr<Program> prog, shared_ptr<MatrixStack> Model, int num)
	{
		prog->bind();
		if (num == 1)
			lampTex->bind(prog->getUniform("Texture0"));
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(0, -1.25, 10));
			Model->scale(vec3(0.2));
			for (int i = 0; i < lamp.size(); i++)
			{
				Model->pushMatrix();
					setModel(prog, Model);
					lamp[i]->draw(prog);
				Model->popMatrix();
			}
		Model->popMatrix();
		prog->unbind();
	}

	void drawWell(shared_ptr<Program> prog, shared_ptr<MatrixStack> Model, int num)
	{
		prog->bind();
		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(vec3(-4, -0.75, 10));
			Model->scale(vec3(1));
			if (num == 1)
				wellWoodTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[0]->draw(prog);
			Model->popMatrix();
			Model->pushMatrix();
				setModel(prog, Model);
				well[1]->draw(prog);
			Model->popMatrix();
			Model->pushMatrix();
				setModel(prog, Model);
				well[2]->draw(prog);
			Model->popMatrix();
			Model->pushMatrix();
				setModel(prog, Model);
				well[3]->draw(prog);
			Model->popMatrix();
			if (num == 1)
				wellRoofTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[4]->draw(prog);
			Model->popMatrix();
			if (num == 1)
				wellFabricTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[5]->draw(prog);
			Model->popMatrix();
			if (num == 1)
				wellWoodTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[6]->draw(prog);
			Model->popMatrix();
			if (num == 1)
				wellMetalTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[7]->draw(prog);
			Model->popMatrix();
			if (num == 1)
				wellFabricTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[8]->draw(prog);
			Model->popMatrix();
			if (num == 1)
				wellBrickTex->bind(prog->getUniform("Texture0"));
			Model->pushMatrix();
				setModel(prog, Model);
				well[9]->draw(prog);
			Model->popMatrix();
		Model->popMatrix();
		prog->unbind();
	}

	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		//update the camera position
		mat4 View;
		if (!goCamera)
			View = mycam.process(frametime);
		else
		{
			updateUsingCameraPath(frametime);
			View = mycam.processBezier(frametime);
		}

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// render scene from lights pov
		mat4 lightP = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.01f, 100.0f);
		mat4 lightV = glm::lookAt(vec3(0.0, 2.0, 10.0), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0));
		mat4 lightSpaceMatrix = lightP * lightV;

		depthProg->bind();
		glUniformMatrix4fv(depthProg->getUniform("lightSpaceMat"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));
		glViewport(0, 0, width * 4, height * 4);
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		// glCullFace(GL_FRONT);
		drawHouse(depthProg, Model, 0);
		drawBarn(depthProg, Model, 0);
		drawFence(depthProg, Model, 0);
		drawLamp(depthProg, Model, 0);
		drawLogs(depthProg, Model, 0);
		drawTrees(depthProg, Model, 0);
		drawAnimals(0);
		drawHierModel(depthProg, Model, 0);
		drawWell(depthProg, Model, 0);
		// glCullFace(GL_BACK);
		drawGround(depthProg);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		depthProg->unbind();

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draws the skysphere
		skyProg->bind();
		glUniformMatrix4fv(skyProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		mat4 ViewBox = mat4(mat3(View));
		glUniformMatrix4fv(skyProg->getUniform("V"), 1, GL_FALSE, value_ptr(ViewBox));
		glUniform1f(skyProg->getUniform("dayNight"), day_night_ratio);
		textureDaySky->bind(skyProg->getUniform("tex"));
		textureNightSky->bind(skyProg->getUniform("tex2"));

		glDisable(GL_DEPTH_TEST);

		//draw big background sphere
		Model->pushMatrix();
			// inside sphere, so want normals pointing in
			Model->loadIdentity();
			Model->scale(vec3(0.25));
			setModel(skyProg, Model);
			sphere->draw(skyProg);
		Model->popMatrix();

		glEnable(GL_DEPTH_TEST);

		skyProg->unbind();

		shadowProg->bind();
		glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		glUniform3f(shadowProg->getUniform("viewPos"), mycam.pos.x, mycam.pos.y, mycam.pos.z);
		glUniform3f(shadowProg->getUniform("lightPos"), 0.0, 2.0, 10.0);
		glUniformMatrix4fv(shadowProg->getUniform("lightSpaceMat"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTex);

		drawHouse(shadowProg, Model, 1);
		drawBarn(shadowProg, Model, 1);
		drawFence(shadowProg, Model, 1);
		drawLamp(shadowProg, Model, 1);
		drawLogs(shadowProg, Model, 1);
		drawWell(shadowProg, Model, 1);
		drawGroundShadow(shadowProg);
		shadowProg->unbind();

		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniform3f(prog->getUniform("lightPos"), 0.0, 2.0, 10.0);
		drawAnimals(1);
		drawTrees(prog, Model, 1);
		drawHierModel(prog, Model, 1);
		prog->unbind();

		// texProg->bind();
		// glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		// glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		// glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		// glUniform3f(texProg->getUniform("lightPos"), 0.0, 2.0, 10.0);
		// drawHouse(texProg, Model, 1);
		// drawBarn(texProg, Model, 1);
		// drawFence(texProg, Model, 1);
		// drawLamp(texProg, Model, 1);
		// drawLogs(texProg, Model, 1);
		// drawWell(texProg, Model, 1);
		// drawGroundShadow(texProg);
		// texProg->unbind();
		
		//animation update example
		sTheta = sin(glfwGetTime());
		eTheta = std::max(0.0f, (float)sin(glfwGetTime()));
		hTheta = std::max(0.0f, (float)cos(glfwGetTime()));

		day_night_ratio = clamp((float)sin(glfwGetTime()), 0.0f, 1.0f);
		
		// Pop matrix stacks.
		Projection->popMatrix();
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
