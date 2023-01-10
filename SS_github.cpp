#define STB_IMAGE_IMPLEMENTATION
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "textFile.h"
#include "stb_image.h"
#include <GL/glu.h>


double limitFPS = 30.0;
glm::vec3 camPos(-30, 0, 10);
glm::vec3 Front(1.0f, 0.0f, 0.0);
glm::vec3 Right(0.0f, 1.0f, 0.0);
glm::vec3 Up(0.0f, 0.0f, 1.0);

int width = 1000, height = 1000;
float lastX = width / 2.0f;
float lastY = height / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;


GLenum error;

namespace GLMAIN {
	GLFWwindow* window;			// Storage For glfw window
    int frameBufferWidth = width, frameBufferHeight = height;
	bool paused = false;        // Store whether the animation is paused
	GLuint vao;					// Storage For vao
	// Storage locations of uniforms
	GLint planetLocaionLoc, planetColorLoc, radiusLoc, mvpLoc, mvLoc,
        lightAmbientLoc, reflectAmbientLoc, lightPosLoc;
    GLuint elementBufferHandle;
    int highlightSphere = -1;

	// Store the locations of 9 spheres
	float planetlocations[9][3]=
    {
        {0.0f, 0.0f , 0.0f},
        {0.0f, 15.9f , 0.0f},
        {0.0f, 20.8f , 0.0f},
        {0.0f, 25.0f , 0.0f},
        {0.0f, 33.0f , 0.0f},
        {0.0f, 88.0f , 0.0f},
        {0.0f, 153.0f, 0.0f},
        {0.0f, 297.0f, 0.0f},
        {0.0f, 460.0f, 0.0f},
    };
    // Store the radius of 9 spheres
    float planerRadius[9] = {10.0f, 0.24f, 0.6f, 0.63f, 0.34f, 7.1f, 6.0f, 2.5f, 2.4f};
    // Store the rotate speed of 9 spheres
    float planetSpeed[9] = {0.0f, 2.5f, 1.75f, 1.5f, 1.2f, 0.65f, 0.45f, 0.3f, 0.25f};
    // Store the angles of 9 spheres
    float planetAngle[9] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0.0f};
    // Store the distances to the star of the 9 spheres
    float planetDistance[9] = {0.0f, 15.9f, 210.8f, 25.0f, 33.0f, 88.0f, 153.0f, 297.0f, 460.0f}; // Planetdistance to the star.
    // Store the base colors  of the 9 spheres
    float planetColor[9][3]=
    {
        {1.0f, 1.0f , 0.1f},
        {0.12f, 0.12f , 0.12f},
        {0.73f, 0.6f , 0.4f},
        {0.38f, 0.74f , 0.58f},
        {0.67f, 0.47f , 0.35f},
        {0.8f, 0.75f , 0.69f},
        {0.75f, 0.66f , 0.49f},
        {0.53f, 0.64f , 0.69f},
        {0.44f, 0.55f , 0.8f}

    };

    // Points and faces of icosphere
    const float X=0.525731112119133606f;
    const float Z=0.850650808352039932f;
    const float N=0.0f;
    const float Verts[12][3]=
    {
        {-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
        {N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
        {Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
    };
    const int Faces[20][3]=
    {
        {0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
        {8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
        {7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
        {6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
    };

}


void updateHighlightSphere();

// Calulate the position of every sphere based on the angle and distance
void calcLocations()
{
    if(GLMAIN::paused)
        return;
    for (int i = 0 ; i < 9; i ++)
    {
        GLMAIN::planetAngle[i] +=  GLMAIN::planetSpeed[i];
        while (GLMAIN::planetAngle[i] > 360.0)
            GLMAIN::planetAngle[i] -= 360.0;
        float tempAngle = (GLMAIN::planetAngle[i] / 180.0) * 3.14159;
        GLMAIN::planetlocations[i][0] = sin(tempAngle) * GLMAIN::planetDistance[i];
        GLMAIN::planetlocations[i][1] = cos(tempAngle) * GLMAIN::planetDistance[i];
    }
}


// Generate perspective projection matrix
void initPerspective(glm::mat4 & m)
{
    const float aspect = 1;
    const float zNear = 10.0f;
    const float zFar = -1.0f;
    const float zRange = zNear - zFar;
    const float tanHalfFOV = tanf(0.5);
    m[0][0] = 1.0f / (tanHalfFOV * aspect);
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = 1.0f / tanHalfFOV;
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = (zNear + zFar) / (zNear - zFar);
    m[2][3] = 2.0f * zFar * zNear /  (zNear - zFar);
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = -1.0f;
    m[3][3] = 0.0f;
}




// Load a texture from a file
unsigned int loadTexture(const char* fileName) {
    // Load image data
    int width = 8192, height = 4096, n = 3;
    unsigned char* data = stbi_load(fileName, &width, &height, &n, 0);
    if (data == nullptr) {
        printf("nao deu\n");
        return 0;
    }

    // Generate and bind a new texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free image data
    stbi_image_free(data);

    return texture;
}


// Display method, draw six spheres.
void display(void)
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //para mostrar o retangulo
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_MODELVIEW);
	
    glm::mat4 modelMatrix = glm::mat4(3.0f);
	glm::mat4 mvp = glm::mat4(1.0f);

   	glm::mat4  projectionMatrix;
   	initPerspective(projectionMatrix);


    	// Init view matrix
	// camera position
	const glm::vec3 lookAt(0.0, 0.0, 0.0);
	const glm::vec3 camOffset = lookAt - camPos;
	const glm::vec3 camForward = camOffset /
		glm::length(camOffset);
	const glm::vec3 camUp0(.0f, 0.0f, -1.0f);
	const glm::vec3 camRight = glm::cross(camForward, camUp0);
	const glm::vec3 camUp = glm::cross(camRight, camForward);

	const glm::mat4 viewRotation(
		camRight.x, camUp.x, camForward.
		x, 0.f, // column 0
		camRight.y, camUp.y, camForward.
		y, 0.f, // column 1
		camRight.z, camUp.z, camForward.
		z, 0.f, // column 2
		0.f, 0.f, 0.f, 1.f);// column 3
	const glm::mat4 viewTranslation(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		camPos.x, camPos.y, camPos.z, 1);
	glm::mat4 viewMatrix = viewRotation * viewTranslation;
	glm::mat4 modelViewMareix = viewMatrix * modelMatrix;

	// model -view - projection matrix
	mvp = projectionMatrix * viewMatrix * modelMatrix;


    // Set uniform  mvp
	float mvpFloat[16];
	for (int i = 0; i < 4; i++)
	{
		mvpFloat[i * 4] = mvp[i].x;
		mvpFloat[i * 4 + 1] = mvp[i].y;
		mvpFloat[i * 4 + 2] = mvp[i].z;
		mvpFloat[i * 4 + 3] = mvp[i].w;
	}
	if(GLMAIN::mvpLoc != -1)
		glUniformMatrix4fv(GLMAIN::mvpLoc, 1, false, mvpFloat);

   	 // Set uniform  mv
	float mvFloat[16];
	for (int i = 0; i < 4; i++)
	{
		mvFloat[i * 4] = modelViewMareix[i].x;
		mvFloat[i * 4 + 1] = modelViewMareix[i].y;
		mvFloat[i * 4 + 2] = modelViewMareix[i].z;
		mvFloat[i * 4 + 3] = modelViewMareix[i].w;

	}
   	 if(GLMAIN::mvLoc != -1)
		glUniformMatrix4fv(GLMAIN::mvLoc, 1, false, mvFloat);

	glBindVertexArray(GLMAIN::vao);
    glPatchParameteri(GL_PATCH_VERTICES, 3);
    // Draw 6 spheres, the first one is the star
    
	for(int i = 0; i < 9; i ++)
	{
        glLoadName(i);
        // set planet location
        if(GLMAIN::planetLocaionLoc != -1)
            glUniform3fv(GLMAIN::planetLocaionLoc, 1, &GLMAIN::planetlocations[i][0]);
        // set the raduis of current sphere
        if(GLMAIN::radiusLoc != -1)
            glUniform1f(GLMAIN::radiusLoc, GLMAIN::planerRadius[i]);
        // set color od current sphere
        if(GLMAIN::planetColorLoc != -1)
            glUniform3fv(GLMAIN::planetColorLoc, 1, GLMAIN::planetColor[i]);
        // Set ambient light
        if(GLMAIN::lightAmbientLoc != -1 && GLMAIN::reflectAmbientLoc != -1)
        {
            if(i == 0) // It is the star
            {
                // The star has a bright ambient light.
                float la[3] = {1.0f, 1.0f, 1.0f};
                float ra[3] = {1.0f, 1.0f, 1.0f};
                glUniform3fv(GLMAIN::lightAmbientLoc, 1, la);
                glUniform3fv(GLMAIN::reflectAmbientLoc, 1, ra);
            }
            else if (GLMAIN::highlightSphere == i)
            {
                // The highlighted planet
                float la[3] = {1.0f, 1.0f, 1.0f};
                float ra[3] = {1.0f, 1.0f, 1.0f};
                glUniform3fv(GLMAIN::lightAmbientLoc, 1, la);
                glUniform3fv(GLMAIN::reflectAmbientLoc, 1, ra);

            }
            else
            {
                // The normal planets have normal ambient light.
                float la[3] = {0.8f,0.8f,0.8f};
                float ra[3] = {0.5f,0.5f,0.5f};
                glUniform3fv(GLMAIN::lightAmbientLoc, 1, la);
                glUniform3fv(GLMAIN::reflectAmbientLoc, 1, ra);
            }
        }
        // Set point light
        if(GLMAIN::lightPosLoc != -1)
        {
            float lp[3] = {0.0f,0.0f,0.0f};  // Only have one point light source located at the center of the star(the first sphere).
            glUniform3fv(GLMAIN::lightPosLoc, 1, lp);
        }
        glDrawElements(GL_PATCHES, sizeof(GLMAIN::Faces),  GL_UNSIGNED_INT,  (void*)0 );
	}

    // Update highlight sphere here and in next display the new highlighted sphere could be seen.
    updateHighlightSphere();
	glfwSwapBuffers(GLMAIN::window);
	glfwPollEvents();
}


void initVAO() // Init vao, vbo.
{

	glGenVertexArrays(1, &GLMAIN::vao);
	glBindVertexArray(GLMAIN::vao);

    GLuint positionBufferHandle;
	glGenBuffers(1, &positionBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLMAIN::Verts), GLMAIN::Verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &GLMAIN::elementBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,  GLMAIN::elementBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLMAIN::Faces), GLMAIN::Faces, GL_STATIC_DRAW);
}



// Init shaders.
int setShaders()
{
	GLint vertCompiled, fragCompiled;
	GLint tcsCompiled, tesCompiled, gsCompiled;

	GLint linked;
	char *vs = NULL, *fs = NULL;
	char *cs = NULL, *es = NULL, *gs = NULL;

	GLuint VertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);;
    	GLuint tcsObject = glCreateShader(GL_TESS_CONTROL_SHADER);
    	GLuint tesObject = glCreateShader(GL_TESS_EVALUATION_SHADER);
    	GLuint gsObject = glCreateShader(GL_GEOMETRY_SHADER);

	GLuint ProgramObject = glCreateProgram();
	vs = textFileRead((char *)"test.vert");   // vertex shader
	fs = textFileRead((char *)"test.frag");   // fragment shader
    	cs = textFileRead((char *)"test.cont");   // TCS shader
	es = textFileRead((char *)"test.eval");   // TES shader
	gs = textFileRead((char *)"test.gs");     // Geometry shader

    printf("An error occurred: %d\n", glGetError());
	glUseProgram(ProgramObject);
    printf("An error occurred: %d\n", glGetError());
	// Load source code into shaders.
	glShaderSource(VertexShaderObject, 1, (const char **)&vs, NULL);
	glShaderSource(FragmentShaderObject, 1, (const char **)&fs, NULL);

	glShaderSource(tcsObject, 1, (const char **)&cs, NULL);
	glShaderSource(tesObject, 1, (const char **)&es, NULL);
	glShaderSource(gsObject, 1, (const char **)&gs, NULL);
	// Compile the  vertex shader.
	glCompileShader(VertexShaderObject);
	glGetShaderiv(VertexShaderObject, GL_COMPILE_STATUS, &vertCompiled);
	// Compile the fragment shader
	glCompileShader(FragmentShaderObject);
	glGetShaderiv(FragmentShaderObject, GL_COMPILE_STATUS, &fragCompiled);
	// Compile the tessellation control shader
    	glCompileShader(tcsObject);
	glGetShaderiv(tcsObject, GL_COMPILE_STATUS, &tcsCompiled);
    	// Compile the tessellation evaluate shader
	glCompileShader(tesObject);
	glGetShaderiv(tesObject, GL_COMPILE_STATUS, &tesCompiled);
	// Compile the geometry shader
	glCompileShader(gsObject);
	glGetShaderiv(gsObject, GL_COMPILE_STATUS, &gsCompiled);

	if (!vertCompiled || !fragCompiled||!tcsCompiled || !tesCompiled||!gsCompiled)
	{
		printf("Shader compile failed, vertCompiled:%d, fragCompiled:%d, tcsCompiled:%d, tesCompiled:%d, gsCompiled:%d\n",
               vertCompiled, fragCompiled, tcsCompiled, tesCompiled, gsCompiled);
		return 0;
	}

	glAttachShader(ProgramObject, VertexShaderObject);
	glAttachShader(ProgramObject, FragmentShaderObject);
	glAttachShader(ProgramObject, tcsObject);
	glAttachShader(ProgramObject, tesObject);
	glAttachShader(ProgramObject, gsObject);

	glLinkProgram(ProgramObject);
	glGetProgramiv(ProgramObject, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		// Print logs if link shaders failed.
		GLsizei len;
		glGetProgramiv(ProgramObject, GL_INFO_LOG_LENGTH, &len);
		GLchar* log = new GLchar[len + 1];
		glGetProgramInfoLog(ProgramObject, len, &len, log);
		printf("Shader linking failed: %s\n", log);
		delete[] log;
		return 0;
	}

	glUseProgram(ProgramObject);


	GLMAIN::planetLocaionLoc = glGetUniformLocation(ProgramObject, "planetLocaion");
	GLMAIN::radiusLoc = glGetUniformLocation(ProgramObject, "radius");
	GLMAIN::planetColorLoc = glGetUniformLocation(ProgramObject, "planetColor");
	GLMAIN::mvpLoc = glGetUniformLocation(ProgramObject, "mvp");
	GLMAIN::mvLoc = glGetUniformLocation(ProgramObject, "mv");
	GLMAIN::lightAmbientLoc = glGetUniformLocation(ProgramObject, "La");  // //Ambient light intensity
	GLMAIN::reflectAmbientLoc = glGetUniformLocation(ProgramObject, "Ra"); //Ambient reflectivity
	GLMAIN::lightPosLoc = glGetUniformLocation(ProgramObject, "lightPos"); //Ambient reflectivity


	glDeleteShader(VertexShaderObject);
	glDeleteShader(FragmentShaderObject);
	glDeleteShader(tcsObject);
	glDeleteShader(tesObject);
	glDeleteShader(gsObject);
	glDeleteProgram(ProgramObject);
	return 1;
}


// callback of key event
void keyfunc(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_RIGHT_ALT && action == GLFW_PRESS)  // If space key is pressed, pause/resume animation
        GLMAIN::paused = !GLMAIN::paused;
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        limitFPS *= 2;
        printf("debug: tecla 1, FPS a: %f\n", limitFPS);
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        limitFPS /= 2;
        printf("debug: tecla 2, FPS a: %f\n", limitFPS);
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camPos += Front;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camPos -= Front;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camPos += Right;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camPos -= Right;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camPos += Up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        camPos -= Up;
    }
    printf("cam:%f, %f, %f\n", camPos[0], camPos[1], camPos[2]);
}


// Mark the max value of RGB components, for example, maxFlag of RGB[255,0,0] is [1,0,0]
void getMaxFlagfromRGB(bool maxFlag[], float RGB[3])
{
    float maxValue = RGB[0];
    for(int i = 1; i < 3; i ++)
    {
        if(maxValue < RGB[i])
            maxValue = RGB[i];
    }
    for(int i = 0; i < 3; i ++)
    {
        if(maxValue != 0 && maxValue == RGB[i])
            maxFlag[i] = true;
        else
            maxFlag[i] = false;
    }

}

// Check if a mouse position hovers a sphere by checking color
// As the color has been affected by light, so we only check if the max components of
// RGB matches, if YES, the colors are matched. This could only appiled in this situation
// because we define the RGB of spheres by only 0 or 1.
void updateHighlightSphere()
{
    double x, y;
    glfwGetCursorPos (GLMAIN::window, &x, &y);

    if(x < 0 || y < 0 || x > GLMAIN::frameBufferWidth || y > GLMAIN::frameBufferHeight)
    {
        GLMAIN::highlightSphere = -1;
        return;
    }
    GLubyte pixels[3];

    glReadPixels(x,GLMAIN::frameBufferHeight - y,1,1,GL_RGB,GL_UNSIGNED_BYTE, pixels);

    bool maxFlagPixel[3];
    float pixelRGB[3];
    pixelRGB[0] = pixels[0];
    pixelRGB[1] = pixels[1];
    pixelRGB[2] = pixels[2];

    getMaxFlagfromRGB(maxFlagPixel, pixelRGB);

    int i;
    for(i = 0; i < 9; i ++)
    {
        bool maxFlagSphere[3];
        getMaxFlagfromRGB(maxFlagSphere, GLMAIN::planetColor[i]);
        bool match = true;
        for(int j = 0; j < 3; j ++)
        {
            if(maxFlagSphere[j] != maxFlagPixel[j])  // If any component does not match, check next sphere
            {
                match = false;
                break;
            }
        }
        if(match) // We have found the matched sphere i.
            break;
    }
    
    GLMAIN::highlightSphere = i; // i is the found sphere, if it is 6, match failed, no object is selected.
    if (i < 9) {
        printf("debug: the sphere %d is hovered\n", i);
        glColor3f(1.0f, 1.0f, 1.0f); // set color to white
        glBegin(GL_QUADS);
        glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
        glVertex2f(0.2f, 0.2f);
        glVertex2f(0.8f, 0.2f);
        glVertex2f(0.8f, 0.8f);
        glVertex2f(0.2f, 0.8f);
        glEnd();
        
    }
    return;
}



// If the frame buffer is resized by resing the window, update viewport and frameBufferWidth/frameBufferHeight
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{

    glViewport(0, 0, width, height);
    GLMAIN::frameBufferWidth = width;
    GLMAIN::frameBufferHeight = height;
}

// Entry point
int main(int argc, char* argv[])
{

	// init glfw
	if (!glfwInit())
		return -1;
	// create a windowed mode window and its OpenGL context
	GLMAIN::window = glfwCreateWindow(GLMAIN::frameBufferWidth, GLMAIN::frameBufferHeight, "Solar System", NULL, NULL);
	if (!GLMAIN::window)
	{
		glfwTerminate();
		return -1;
	}
    
    glfwMakeContextCurrent(GLMAIN::window);
    glfwSetFramebufferSizeCallback(GLMAIN::window, framebuffer_size_callback);
	//make the window's context current
	glfwMakeContextCurrent(GLMAIN::window);
	glfwSetKeyCallback(GLMAIN::window,keyfunc);
	glfwSetFramebufferSizeCallback(GLMAIN::window, framebuffer_size_callback);
	// init glew
	glewInit();
	glewExperimental = GL_TRUE;
	
    setShaders();
	initVAO();
    

	double lastTime = glfwGetTime();
	double deltaTime = 0, nowTime = 0;

    glEnable(GL_DEPTH_TEST);
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(GLMAIN::window))
	{
		nowTime = glfwGetTime();
		deltaTime += (nowTime - lastTime) * limitFPS;
		lastTime = nowTime;

		if (deltaTime < 1.0)
			continue;
		// - Only update at x frames / second
		while (deltaTime >= 1.0) {
			deltaTime--;
		}
		calcLocations();
		display(); //  Render function
	}
	glfwTerminate();
	return 0;
}

