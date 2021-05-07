#pragma warning(push, 0)

#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include <glm/glm.hpp> //includes GLM
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <glm/ext/matrix_transform.hpp> // GLM: translate, rotate
#include <glm/ext/matrix_clip_space.hpp> // GLM: perspective and ortho 
#include <glm/gtc/type_ptr.hpp> // GLM: access to the value_ptr
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include <iostream>
#include <vector>
#include <stack>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>

#pragma warning(pop)

#include "Project.h"
#include "Shader.h"
#include "Maze.h"
#include "Model.h"

//Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

//Set up initial camera position and heading
glm::vec3 cameraPosition = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeedModifier = 2.5f;

//Camera modes
enum cameraModes
{
	autoCam, camFly, camWalk
};

int camSizeX, camSizeY;
int startX = 0;
int startY = 0;
int camMode = autoCam;
bool mouseCaptured = false;
float yaw = -90.0f;
float pitch = -89.9f;
float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;

//Track frametimes
const int tickRate = 20;
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

//For sharing access to maze arrays
std::mutex mazeArrayMutex;
std::atomic<bool> generationComplete = false;

//For drawing floor and determining positions within cells
const float cellSize = 10.0f;
const float scaleFactor = 0.05f;

//Constants to define maze paths
enum
{
	CELL_NULL = 0x00,
	CELL_PATH_N = 0x01,	//Constants numbered to have 1 bit each
	CELL_PATH_E = 0x02,
	CELL_PATH_S = 0x04,
	CELL_PATH_W = 0x08,
	CELL_VISITED = 0x10
};

std::pair<float, float> cellLocationToWorldSpace(int x, int y)
{
	float worldX = ((float)x * cellSize * scaleFactor - (cellSize * scaleFactor * camSizeX) / 2) + (cellSize * scaleFactor) / 2;
	float worldZ = ((float)y * cellSize * scaleFactor - (cellSize * scaleFactor * camSizeY) / 2) + (cellSize * scaleFactor) / 2;
	return std::pair<float, float>(worldX, worldZ);
}

std::pair<int, int> worldSpaceToCellLocation(float x, float z)
{
	float camRangeX = camSizeX / 2;
	float camRangeZ = camSizeY / 2;
	float xMapped = (x + camRangeX / 2) * 2;
	float zMapped = (z + camRangeZ / 2) * 2;
	int cellX = (int)xMapped;
	int cellY = (int)zMapped;
	return std::pair<int, int>(cellX, cellY);
}

void initMaze(Maze *maze, int waitTimeMs)
{
	mazeArrayMutex.lock();
	size_t sizeX = maze->getSizeX();
	size_t sizeY = maze->getSizeY();
	mazeArrayMutex.unlock();
	int sizeXoffset = static_cast<int>(sizeX);
	int sizeYoffset = static_cast<int>(sizeY);

	//Cast values to platform max as they will be used for determining memory addresses
	if (sizeX == 0 || sizeY == 0)
	{
		return;	//Do not generate a maze as invalid input was used
	}
	const size_t xSize = static_cast<size_t>(sizeX);
	const size_t ySize = static_cast<size_t>(sizeY);

	//Allocate array for chosen route
	uint8_t* chosen;
	chosen = new uint8_t[xSize * ySize];
	memset(chosen, 0x00, xSize * ySize);

	//Route generation
	std::stack<std::pair<unsigned int, unsigned int>> visitedLog;
	//Choose a random start point
	srand(time(0));
	unsigned int randX = rand() % sizeX;
	unsigned int randY = rand() % sizeY;
	//Push it to the stack
	visitedLog.push(std::pair<unsigned int, unsigned int>(randX, randY));
	unsigned int numVisited = 1;
	unsigned int currentX;
	unsigned int currentY;

	bool stop = false;
	while (!stop)
	{
		currentX = visitedLog.top().first;
		currentY = visitedLog.top().second;
		uint8_t* thisCell = chosen + ((currentY * xSize) + currentX);
		mazeArrayMutex.lock();
		maze->setCurrentLocation(currentX, currentY);
		//Do not add this cell to the list if its already been visited
		if (!(*thisCell & CELL_VISITED))
		{
			maze->addCellLocation(std::pair<size_t, size_t>(currentX, currentY));
			//Mark this cell as visited
			*thisCell |= CELL_VISITED;
			maze->setMaze(chosen, numVisited * sizeof(uint8_t));
		}
		mazeArrayMutex.unlock();
		//Try to find an unvisited neighbour
		std::vector<uint8_t> possibleRoutes;
		//Check N, E, S & W
		uint8_t* checkCell;
		if (currentY > 0)
		{
			//Check north
			checkCell = thisCell - xSize;
			if (!(*checkCell & CELL_VISITED))
			{
				possibleRoutes.push_back(CELL_PATH_N);
			}
		}
		if (currentX < sizeX - 1)
		{
			//Check east
			checkCell = thisCell + 1;
			if (!(*checkCell & CELL_VISITED))
			{
				possibleRoutes.push_back(CELL_PATH_E);
			}
		}
		if (currentY < sizeY - 1)
		{
			//Check south
			checkCell = thisCell + xSize;
			if (!(*checkCell & CELL_VISITED))
			{
				possibleRoutes.push_back(CELL_PATH_S);
			}
		}
		if (currentX > 0)
		{
			//Check west
			checkCell = thisCell - 1;
			if (!(*checkCell & CELL_VISITED))
			{
				possibleRoutes.push_back(CELL_PATH_W);
			}
		}
		if (possibleRoutes.size() == 0)
		{
			if (numVisited == sizeX * sizeY)
			{
				//All cells have been set
				stop = true;
			}
			else
			{
				//Backtrack
				visitedLog.pop();
			}
		}
		else
		{
			//There is a viable cell
			int chosenRoute = rand() % possibleRoutes.size();
			int newX, newY;
			std::pair<GLfloat*, GLuint*> newPointers;
			//Set path and push new cell onto stack
			*thisCell |= possibleRoutes[chosenRoute];
			switch (possibleRoutes[chosenRoute])
			{
			case CELL_PATH_N:
				newX = currentX;
				newY = currentY - 1;
				break;
			case CELL_PATH_E:
				newX = currentX + 1;
				newY = currentY;
				break;
			case CELL_PATH_S:
				newX = currentX;
				newY = currentY + 1;
				break;
			case CELL_PATH_W:
				newX = currentX - 1;
				newY = currentY;
				break;
			}
			visitedLog.push(std::pair<unsigned int, unsigned int>(newX, newY));
			numVisited++;
			std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMs));
		}
	}
	//Choose a start and end location
	uint8_t* checkCell = chosen;
	std::vector<std::pair<int, int>> potentialCells;
	for (int y = 0; y < sizeY; y++)
	{
		for (int x = 0; x < sizeX; x++)
		{
			if (*checkCell == 0x10)
			{
				potentialCells.push_back(std::pair<int, int>(x, y));
			}
			checkCell++;
		}
	}
	if (potentialCells.size() > 2)
	{
		int chosenCell = rand() % potentialCells.size();
		maze->setStartCell(potentialCells[chosenCell].first, potentialCells[chosenCell].second);
		potentialCells.erase(potentialCells.begin() + chosenCell);
		chosenCell = rand() % potentialCells.size();
		maze->setWinCell(potentialCells[chosenCell].first, potentialCells[chosenCell].second);
	}
	else if (potentialCells.size() == 2)
	{
		maze->setStartCell(potentialCells[0].first, potentialCells[0].second);
		maze->setWinCell(potentialCells[1].first, potentialCells[1].second);
	}
	else
	{
		maze->setStartCell(0, 0);
		maze->setWinCell(sizeX - 1, sizeY - 1);
	}
	generationComplete = true;
	return;
}

//Input processing
void processInput(GLFWwindow* window)
{
	//Cursor capture
	if (glfwGetMouseButton(window, 0) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (!(camMode == camWalk))
		{
			camMode = camFly;
		}
		mouseCaptured = true;
		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);
		lastX = xPos;
		lastY = yPos;
	}
	if (glfwGetMouseButton(window, 1) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		if (!(camMode == camWalk))
		{
			camMode = autoCam;
		}
		mouseCaptured = false;
	}
	//Exiting
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
	//Camera position manipulation
	float cameraSpeed = cameraSpeedModifier * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && generationComplete && !(camMode == camWalk))
	{
		std::pair<float, float> worldCoords = cellLocationToWorldSpace(startX, startY);

		cameraPosition = glm::vec3(worldCoords.first, 0.4f, worldCoords.second);
		camMode = camWalk;
		cameraSpeedModifier = 1.0f;
	}
	glm::vec3 cameraCross = glm::normalize(glm::cross(cameraFront, cameraUp));
	if (camMode == camFly)
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			cameraPosition += cameraSpeed * cameraFront;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			cameraPosition -= cameraSpeed * cameraFront;
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			cameraPosition -= cameraCross * cameraSpeed; //Normalize (reduce to unit vector)
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			cameraPosition += cameraCross * cameraSpeed;
		}
	}
	else if (camMode == camWalk)
	{
		glm::vec2 cameraFrontNoY = glm::normalize(glm::vec2(cameraFront.x, cameraFront.z));
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			cameraPosition = glm::vec3(cameraPosition.x + cameraSpeed * cameraFrontNoY.x, cameraPosition.y, cameraPosition.z + cameraSpeed  * cameraFrontNoY.y);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			cameraPosition = glm::vec3(cameraPosition.x - cameraSpeed * cameraFrontNoY.x, cameraPosition.y, cameraPosition.z - cameraSpeed * cameraFrontNoY.y);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			cameraPosition = glm::vec3(cameraPosition.x - cameraSpeed * cameraCross.x, cameraPosition.y, cameraPosition.z - cameraSpeed * cameraCross.z);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			cameraPosition = glm::vec3(cameraPosition.x + cameraSpeed * cameraCross.x, cameraPosition.y, cameraPosition.z + cameraSpeed * cameraCross.z);
		}
	}
	
}

//Calculates new yaw and pitch based on mouse change
void mouse_callback(GLFWwindow* window, double xPos, double yPos)
{
	if ((camMode == autoCam) || !mouseCaptured)
	{
		return;
	}
	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos;
	lastX = xPos;
	lastY = yPos;

	const float sensitivity = 0.1f;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	yaw = yaw + xOffset;
	float newPitch = pitch + yOffset;
	//Clip pitch if user tries to look too far up
	if (!(newPitch < 90.0f))
		pitch = 89.999;
	else if (!(newPitch > -90.0f))
		pitch = -89.999f;
	else
		pitch = newPitch;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

//Callback for window resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main(int argc, char** argv)
{
	std::cout << "Enter desired maze size X (4 =< x =< 128): ";
	int inputX, inputY;
	std::cin >> inputX;
	if (inputX < 4)
	{
		inputX = 4;
		std::cout << "Input was too small, clipped to 4" << std::endl;
	}
	if (inputX > 128)
	{
		inputX = 128;
		std::cout << "Input was too large, clipped to 128" << std::endl;
	}
	std::cout << "Enter desired maze size Y (4 =< y =< 128): ";
	std::cin >> inputY;
	if (inputY < 4)
	{
		inputY = 4;
		std::cout << "Input was too small, clipped to 4" << std::endl;
	}
	if (inputY > 128)
	{
		inputY = 128;
		std::cout << "Input was too large, clipped to 128" << std::endl;
	}

	camSizeX = inputX;
	camSizeY = inputY;
	size_t sizeX = static_cast<size_t>(inputX);
	size_t sizeY = static_cast<size_t>(inputY);

	glfwInit();
	//Set OpenGL version and profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Maze Game", NULL, NULL);
	if (window == NULL)
	{
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return -1;
		}
	}

	glfwMakeContextCurrent(window);

	//Initialize GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	//Set callback function for changing window size
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	//Compile shaders
	Shader surfaceShader = Shader("media/SurfaceShader.vert", "media/SurfaceShader.frag");
	surfaceShader.use();
	
	//Lighting
	//Directional light
	surfaceShader.setVec3fv("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.3f));
	surfaceShader.setVec3fv("directionalLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	surfaceShader.setVec3fv("directionalLight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
	surfaceShader.setVec3fv("directionalLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));

	surfaceShader.setInt("numPointLights", 0);

	surfaceShader.setFloat("material.shininess", 16.0f);

	//Set up view and projection matrices
	glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.01f, 1000.0f);

	//Build maze
	Maze maze = Maze(sizeX, sizeY);
	int generateMinTimeSecs = 5;
	int generateWaitMs = std::max((int)(1000 / (float)((sizeX * sizeY) / generateMinTimeSecs)), 1);
	//Start generation on seperate thread
	std::thread th1(initMaze, &maze, generateWaitMs);

	//Load models for walls and floor
	stbi_set_flip_vertically_on_load(true);
	Model wall = Model("media/models/wall.obj");
	Model floor = Model("media/models/floor.obj");
	stbi_set_flip_vertically_on_load(false);
	Model startCube = Model("media/models/startcube.obj");
	Model winCube = Model("media/models/wincube.obj");

	//For storing maze data when it is polled from class
	uint8_t *mazeData;
	size_t mazeSizeX = maze.getSizeX();
	size_t mazeSizeY = maze.getSizeY();
	mazeData = new uint8_t[mazeSizeX * mazeSizeY];
	memset(mazeData, 0x00, mazeSizeX * mazeSizeY);
	std::vector<std::pair<size_t, size_t>> cellLocations;

	float startTime = glfwGetTime();
	float waitTime = 1.0f / (float)tickRate;
	//RENDER LOOP
	while (!glfwWindowShouldClose(window))
	{
		//Uncomment to draw only wireframe
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//Set background
		glClearColor(0.52734375f, 0.8046875f, 0.91796875f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Set view matrix based on camera location
		if (camMode == autoCam)
		{
			cameraPosition = glm::vec3(0.0f, std::max(sizeX, sizeY) / 2.0f + std::pow(log(std::max(sizeX, sizeY)), 2), 0.0f);
			cameraPosition += glm::vec3(sin(glfwGetTime() * 0.25f) * std::max(sizeX, sizeY) / 2.0f, 0.0f, cos(glfwGetTime() * 0.25f) * std::max(sizeX, sizeY) / 2.0f);
			glm::vec3 cameraToPoint = glm::normalize(glm::vec3(0.0f) - cameraPosition);
			pitch = glm::degrees(asin(cameraToPoint.y));
			yaw = glm::degrees(-atan2(cameraToPoint.x, cameraToPoint.z)) + 90.0f;
			glm::vec3 direction;
			direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.y = sin(glm::radians(pitch));
			direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(direction);
		}
		viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);

		//Generate seperate model matrices for each object
		glm::mat4 modelMatrix;

		//Render floor
		surfaceShader.use();
		//Update view position for lighting
		surfaceShader.setVec3fv("viewPosition", cameraPosition);

		//Send transformation matrices to shader via uniforms
		surfaceShader.setMat4fv("viewMatrix", viewMatrix);
		surfaceShader.setMat4fv("projectionMatrix", projectionMatrix);

		//Track time to determine when to poll maze
		float currentTime = glfwGetTime();
		deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;

		//Update the game state at tick rate
		if (currentTime >= startTime + waitTime)
		{
			if (mazeArrayMutex.try_lock())
			{
				memcpy(mazeData, maze.getMaze(), maze.getMazeLength());
				cellLocations = maze.getCellLocations();
				mazeArrayMutex.unlock();

				startTime = currentTime;
			}
		}
		//Iterate through all cells and draw required walls
		for (std::vector<std::pair<size_t, size_t>>::iterator it = cellLocations.begin(); it != cellLocations.end(); ++it)
		{
			std::pair<float, float> worldSpace = cellLocationToWorldSpace(it->first, it->second);
			float offsetX = worldSpace.first;
			float offsetY = worldSpace.second;

			modelMatrix = glm::mat4(1.0f);
			
			modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX, 0.0f, offsetY));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));
			
			surfaceShader.setMat4fv("modelMatrix", modelMatrix);

			//Draw floors
			floor.Draw(surfaceShader);

			//Set offset for this cell
			size_t offset = ((sizeof(uint8_t) * mazeSizeX * it->second) + (sizeof(uint8_t) * it->first));
			uint8_t *thisCell = mazeData + offset;
			//Data on surrounding cells
			uint8_t northCell, eastCell, southCell, westCell;
			//If this cell is within range, return its data, else return 0
			//Cell to the north: check Y
			if (it->second > 0)
			{
				northCell = *(mazeData + (offset - sizeof(uint8_t) * mazeSizeX));
			}
			else
			{
				northCell = CELL_NULL;
			}
			//Cell to the east: check X
			if (it->first < mazeSizeX - 1)
			{
				eastCell = *(mazeData + (offset + sizeof(uint8_t)));
			}
			else
			{
				eastCell = CELL_NULL;
			}
			//Cell to the south: check Y
			if (it->second < mazeSizeY - 1)
			{
				southCell = *(mazeData + (offset + sizeof(uint8_t) * mazeSizeX));
			}
			else
			{
				southCell = CELL_NULL;
			}
			//Cell to the west: check X
			if (it->first > 0)
			{
				westCell = *(mazeData + (offset - sizeof(uint8_t)));
			}
			else
			{
				westCell = CELL_NULL;
			}

			//Check each cell wall to see if it should be rendered
			if (!((*thisCell & CELL_PATH_N) || (northCell & CELL_PATH_S)))
			{
				//Draw north wall
				modelMatrix = glm::mat4(1.0f);

				modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX, 0.0f, offsetY - cellSize * scaleFactor / 2));
				modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));

				surfaceShader.setMat4fv("modelMatrix", modelMatrix);

				wall.Draw(surfaceShader);
			}
			if (!((*thisCell & CELL_PATH_E) || (eastCell & CELL_PATH_W)))
			{
				//Draw east wall
				modelMatrix = glm::mat4(1.0f);

				modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX + cellSize * scaleFactor / 2, 0.0f, offsetY));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));

				surfaceShader.setMat4fv("modelMatrix", modelMatrix);

				wall.Draw(surfaceShader);
			}
			if (!((*thisCell & CELL_PATH_S) || (southCell & CELL_PATH_N)))
			{
				//Draw south wall
				modelMatrix = glm::mat4(1.0f);

				modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX, 0.0f, offsetY + cellSize * scaleFactor / 2));
				modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));

				surfaceShader.setMat4fv("modelMatrix", modelMatrix);

				wall.Draw(surfaceShader);
			}
			if (!((*thisCell & CELL_PATH_W) || (westCell & CELL_PATH_E)))
			{
				//Draw west wall
				modelMatrix = glm::mat4(1.0f);

				modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX - cellSize * scaleFactor / 2, 0.0f, offsetY));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));

				surfaceShader.setMat4fv("modelMatrix", modelMatrix);

				wall.Draw(surfaceShader);
			}
		}
		//If the maze has been generated, show the win and lose locations
		if (generationComplete)
		{
			float offsetX, offsetY;
			//Start
			offsetX = ((float)maze.getStartCell().first * cellSize - (cellSize * maze.getSizeX()) / 2) + cellSize / 2;
			offsetY = ((float)maze.getStartCell().second * cellSize - (cellSize * maze.getSizeY()) / 2) + cellSize / 2;

			modelMatrix = glm::mat4(1.0f);

			modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor));
			modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX, 1.0f, offsetY));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f));

			surfaceShader.setMat4fv("modelMatrix", modelMatrix);

			startCube.Draw(surfaceShader);

			//Win
			offsetX = ((float)maze.getWinCell().first * cellSize - (cellSize * maze.getSizeX()) / 2) + cellSize / 2;
			offsetY = ((float)maze.getWinCell().second * cellSize - (cellSize * maze.getSizeY()) / 2) + cellSize / 2;

			modelMatrix = glm::mat4(1.0f);

			modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleFactor));
			modelMatrix = glm::translate(modelMatrix, glm::vec3(offsetX, 1.0f, offsetY));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f));

			surfaceShader.setMat4fv("modelMatrix", modelMatrix);

			winCube.Draw(surfaceShader);

			startX = maze.getStartCell().first;
			startY = maze.getStartCell().second;
		}
		//Check the current location to see if the player has won
		std::pair<int, int> locationCell = worldSpaceToCellLocation(cameraPosition.x, cameraPosition.z);
		if ((camMode == camWalk) && (maze.getWinCell().first == locationCell.first && maze.getWinCell().second == locationCell.second))
		{
			std::cout << "You win!!! Press any key to quit" << std::endl;;
			glfwSetWindowShouldClose(window, true);
		}

		processInput(window);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	th1.detach();

	glfwTerminate();

	std::string quit;
	std::cin >> quit;
}
