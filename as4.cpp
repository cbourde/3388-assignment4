#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float SCREEN_WIDTH = 1280;
const float SCREEN_HEIGHT = 720;
const float FOV = 45.0f;
const float CAMERA_MOVE_SPEED = 0.05f;
const float CAMERA_ROTATION_SPEED = 1.0f;

GLFWwindow* window;


struct VertexData{
	float x, y, z;
	float nx, ny, nz;
	float r, g, b;
	float u, v;
};

struct TriData{
	GLuint v1, v2, v3;
};

/*
	Loads data from a PLY file
	Returns 0 if successful, -1 for file IO error, -2 for file format error
*/
int loadPLY(std::string path, std::vector<VertexData>& vertices, std::vector<TriData>& faces){
	// Try to open the file
	printf("Reading PLY file %s\n", path.data());
	std::ifstream file(path);
	if (file.fail()){
		printf("Error opening file\n");
		return -1;
	}

	// Read the file header
	std::string currentLine;
	std::string currentWord;
	// Check for the "ply" line
	std::getline(file, currentLine);
	if (currentLine != "ply"){
		printf("Invalid PLY file: Missing PLY header line\n");
		return -2;
	}

	int numVertices = 0;
	int numFaces = 0;

	while (std::getline(file, currentLine)){
		std::istringstream iss(currentLine);
		
		// Create a list of all the words in the line
		std::vector<std::string> words;
		while (std::getline(iss, currentWord, ' ')){
			words.push_back(currentWord);
		}

		// Ignore the format line, comments, and property lines (since all the files have the properties in the same order)
		if (words[0] == "format" || words[0] == "comment" || words[0] == "property"){
			continue;
		}
		// For "element" lines, save the number of elements
		else if (words[0] == "element"){
			if (words[1] == "vertex"){
				try{
					numVertices = std::stoi(words[2]);
				}
				catch (...){
					printf("Invalid PLY file: Missing or invalid vertex count\n");
					return -2;
				}
			}
			else if (words[1] == "face"){
				try{
					numFaces = std::stoi(words[2]);
				}
				catch (...){
					printf("Invalid PLY file: Missing or invalid face count\n");
					return -2;
				}
			}
			else{
				printf("Invalid PLY file: Missing or invalid element type\n");
				return -2;
			}
		}
		// Break out of the loop once the end_header line is read
		else if (words[0] == "end_header"){
			break;
		}
	}

	// If either numFaces or numVertices is zero after the header, the file is bad
	if (numFaces == 0 || numVertices == 0){
		printf("Invalid PLY file: Missing face or vertex count\n");
		return -2;
	}

	// Read the vertices
	for (int i = 0; i < numVertices; i++){
		std::getline(file, currentLine);
		std::istringstream iss(currentLine);
		std::vector<float> values;
		while (std::getline(iss, currentWord, ' ')){
			try{
				values.push_back(std::stof(currentWord));
			}
			catch (...){
				printf("Invalid PLY file: Missing or invalid vertex property value\n");
				return -2;
			}
		}

		if (values.size() < 8){
			printf("Invalid PLY file: Wrong number of vertex properties\n");
			return -2;
		}

		// If we made it this far, there are 8 valid numbers, so create a VertexData from them and add it to the list
		VertexData vd;
		vd.x = values[0];
		vd.y = values[1];
		vd.z = values[2];
		vd.nx = values[3];
		vd.ny = values[4];
		vd.nz = values[5];
		vd.u = values[6];
		vd.v = values[7];
		vertices.push_back(vd);
	}

	// Read the faces
	for (int i = 0; i < numFaces; i++){
		std::getline(file, currentLine);
		std::istringstream iss(currentLine);
		std::vector<int> values;
		int vertexCount = 0;
		// Get the vertex count from the first entry in the line
		std::getline(iss, currentWord, ' ');
		try {
			vertexCount = std::stoi(currentWord);
		}
		catch (...) {
			printf("Invalid PLY file: Invalid face vertex count\n");
			return -2;
		}

		while (std::getline(iss, currentWord, ' ')){
			try{
				values.push_back(std::stoi(currentWord));
			}
			catch (...){
				printf("Invalid PLY file: Missing or invalid face vertex index value\n");
				return -2;
			}
		}

		if (values.size() != vertexCount){
			printf("Invalid PLY file: Number of vertices does not match specified vertex count\n");
			return -2;
		}

		if (vertexCount < 3 || values.size() < 3){
			printf("Invalid PLY file: Not enough vertices to form a triangle\n");
		}

		TriData td;
		td.v1 = values[0];
		td.v2 = values[1];
		td.v3 = values[2];
		faces.push_back(td);
	}
	return 0;
}

/**
 * Given a file path imagepath, read the data in that bitmapped image
 * and return the raw bytes of color in the data pointer.
 * The width and height of the image are returned in the weight and height pointers,
 * respectively.
 *
 * usage:
 *
 * unsigned char* imageData;
 * unsigned int width, height;
 * loadARGB_BMP("mytexture.bmp", &imageData, &width, &height);
 *
 * Modified from https://github.com/opengl-tutorials/ogl.
 */
void loadARGB_BMP(const char* imagepath, unsigned char** data, unsigned int* width, unsigned int* height) {

    printf("Reading image %s\n", imagepath);

    // Data read from the header of the BMP file
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int imageSize;
    // Actual RGBA data

    // Open the file
    FILE * file = fopen(imagepath,"rb");
    if (!file){
        printf("%s could not be opened. Are you in the right directory?\n", imagepath);
        getchar();
        return;
    }

    // Read the header, i.e. the 54 first bytes

    // If less than 54 bytes are read, problem
    if ( fread(header, 1, 54, file)!=54 ){
        printf("Not a correct BMP file1\n");
        fclose(file);
        return;
    }

    // Read the information about the image
    dataPos    = *(int*)&(header[0x0A]);
    imageSize  = *(int*)&(header[0x22]);
    *width      = *(int*)&(header[0x12]);
    *height     = *(int*)&(header[0x16]);
    // A BMP files always begins with "BM"
    if ( header[0]!='B' || header[1]!='M' ){
        printf("Not a correct BMP file2\n");
        fclose(file);
        return;
    }
    // Make sure this is a 32bpp file
    if ( *(int*)&(header[0x1E])!=3  ) {
        printf("Not a correct BMP file3\n");
        fclose(file);
        return;
    }
    // fprintf(stderr, "header[0x1c]: %d\n", *(int*)&(header[0x1c]));
    // if ( *(int*)&(header[0x1C])!=32 ) {
    //     printf("Not a correct BMP file4\n");
    //     fclose(file);
    //     return;
    // }

    // Some BMP files are misformatted, guess missing information
    if (imageSize==0)    imageSize=(*width)* (*height)*4; // 4 : one byte for each Red, Green, Blue, Alpha component
    if (dataPos==0)      dataPos=54; // The BMP header is done that way

    // Create a buffer
    *data = new unsigned char [imageSize];

    if (dataPos != 54) {
        fread(header, 1, dataPos - 54, file);
    }

    // Read the actual data from the file into the buffer
    fread(*data,1,imageSize,file);

    // Everything is in memory now, the file can be closed.
    fclose (file);
}


class TexturedMesh {	
		std::string PLYPath, texturePath;
		std::vector<VertexData> vertices;
		std::vector<TriData> faces;
		GLuint vertexVBO, textureVBO, vertexIndicesVBO, textureObj, meshVAO, programID;
		
		unsigned char* textureData;
		unsigned int textureWidth, textureHeight;

	public:
		
		TexturedMesh(std::string ply_path, std::string tex_path){
			PLYPath = ply_path;
			texturePath = tex_path;
			loadARGB_BMP(texturePath.data(), &textureData, &textureWidth, &textureHeight);
			loadPLY(PLYPath, vertices, faces);


			// Create VAO
			glGenVertexArrays(1, &meshVAO);
			glBindVertexArray(meshVAO);

			// Create VBOs
			// Vertices
			glGenBuffers(1, &vertexVBO);
			glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * 11 * vertices.size(), &(vertices[0]), GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(
				0,
				3,
				GL_FLOAT,
				GL_FALSE,
				sizeof(GL_FLOAT) * 11,
				(void*) 0
			);

			// Texture coordinates
			glGenBuffers(1, &textureVBO);
			glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * 11 * vertices.size(), &(vertices[0]), GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(
				1,
				2,
				GL_FLOAT,
				GL_FALSE,
				sizeof(GL_FLOAT) * 11,
				(void*) 36
			);
			

			// Face vertex indices
			glGenBuffers(1, &vertexIndicesVBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndicesVBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GL_UNSIGNED_INT) * 3 * faces.size(), &(faces[0]), GL_STATIC_DRAW);

			glBindVertexArray(0);

			// Create shader program
			// Create shaders (shamelessly stolen from class demo code as instructed)
			GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
			GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
			std::string VertexShaderCode = "\
			#version 330 core\n\
			// Input vertex data, different for all executions of this shader.\n\
			layout(location = 0) in vec3 vertexPosition;\n\
			layout(location = 1) in vec2 uv;\n\
			// Output data ; will be interpolated for each fragment.\n\
			out vec2 uv_out;\n\
			// Values that stay constant for the whole mesh.\n\
			uniform mat4 MVP;\n\
			void main(){ \n\
				// Output position of the vertex, in clip space : MVP * position\n\
				gl_Position =  MVP * vec4(vertexPosition,1);\n\
				// The color will be interpolated to produce the color of each fragment\n\
				uv_out = uv;\n\
			}\n";

			// Read the Fragment Shader code from the file
			std::string FragmentShaderCode = "\
			#version 330 core\n\
			in vec2 uv_out; \n\
			uniform sampler2D tex;\n\
			void main() {\n\
				gl_FragColor = texture(tex, uv_out);\n\
			}\n";
			char const *VertexSourcePointer = VertexShaderCode.c_str();
			glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
			glCompileShader(VertexShaderID);

			// Compile Fragment Shader
			char const *FragmentSourcePointer = FragmentShaderCode.c_str();
			glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
			glCompileShader(FragmentShaderID);

			programID = glCreateProgram();
			glAttachShader(programID, VertexShaderID);
			glAttachShader(programID, FragmentShaderID);
			glLinkProgram(programID);

			glDetachShader(programID, VertexShaderID);
			glDetachShader(programID, FragmentShaderID);

			glDeleteShader(VertexShaderID);
			glDeleteShader(FragmentShaderID);

			// Create texture object
			glGenTextures(1, &textureObj);
			glBindTexture(GL_TEXTURE_2D, textureObj);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RGBA,
				textureWidth,
				textureHeight,
				0,
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				textureData
			);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void draw(glm::mat4 mvp){			

			// Set active texture unit
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textureObj);
			
			// Enable blending
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Get location of uniform MVP matrix
			GLuint matrixID = glGetUniformLocation(programID, "MVP");
			
			// Set shader program and uniform MVP matrix
			glUseProgram(programID);
			glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);
			
			glBindVertexArray(meshVAO);

			glDrawElements(
				GL_TRIANGLES,
				faces.size() * 3,
				GL_UNSIGNED_INT,
				(void*) 0
			);
			glBindVertexArray(0);
			glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);

		}
};


int main(){

	// Initialize window
	if (!glfwInit()){
		printf("Failed to initialize GLFW\n");
		return -1;
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Assignment 4", NULL, NULL);
	if (window == NULL){
		printf("Failed to open window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK){
		printf("Failed to initialize GLEW\n");
		glfwTerminate();
		return -1;
	}		

	// Load data from files
	std::vector<TexturedMesh> meshes;
	meshes.push_back(TexturedMesh("./assets/Walls.ply", "./assets/walls.bmp"));
	meshes.push_back(TexturedMesh("./assets/WoodObjects.ply", "./assets/woodobjects.bmp"));
	meshes.push_back(TexturedMesh("./assets/Table.ply", "./assets/table.bmp"));
	meshes.push_back(TexturedMesh("./assets/WindowBG.ply", "./assets/windowbg.bmp"));
	meshes.push_back(TexturedMesh("./assets/Patio.ply", "./assets/patio.bmp"));
	meshes.push_back(TexturedMesh("./assets/Floor.ply", "./assets/floor.bmp"));
	meshes.push_back(TexturedMesh("./assets/Bottles.ply", "./assets/bottles.bmp"));
	
	meshes.push_back(TexturedMesh("./assets/DoorBG.ply", "./assets/doorbg.bmp"));
	meshes.push_back(TexturedMesh("./assets/MetalObjects.ply", "./assets/metalobjects.bmp"));
	meshes.push_back(TexturedMesh("./assets/Curtains.ply", "./assets/curtains.bmp"));

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0,0,0,1);

	// Set up initial camera position and direction
	float yaw = 0.0f;
	glm::vec3 cameraDirection = {cos(glm::radians(yaw)), 0.0f, sin(glm::radians(yaw))};
	glm::vec3 cameraPosition = {0.0f, 0.5f, 0.0f};
	glm::vec3 up = {0.0f, 1.0f, 0.0f};

	// Set up perspective projection
	glm::mat4 projection = glm::perspective(glm::radians(FOV), SCREEN_WIDTH / SCREEN_HEIGHT, 0.001f, 1000.0f);
	glm::mat4 mvp;

	// Main loop
	while (!glfwWindowShouldClose(window)){
		glfwPollEvents();
		// Process keyboard inputs
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
			yaw -= CAMERA_ROTATION_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
			yaw += CAMERA_ROTATION_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
			cameraPosition += (cameraDirection * CAMERA_MOVE_SPEED);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
			cameraPosition -= (cameraDirection * CAMERA_MOVE_SPEED);
		}

		// Set camera look
		cameraDirection = {cos(glm::radians(yaw)), 0.0f, sin(glm::radians(yaw))};
		glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, up);

		// Clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glm::mat4 model = glm::mat4(1.0f);
		mvp = projection * view * model;

		// Draw meshes
		for (int i = 0; i < meshes.size(); i++){
			meshes[i].draw(mvp);
		}

		glfwSwapBuffers(window);
	}

	return 0;
}