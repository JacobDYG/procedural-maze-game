# Procedural Maze Game
A prototype for a maze game. On each run, a new maze is procedurally generated from a size given by the user. The user can watch the maze as it is being generated, and fly around freely. A start and end location are randomly selected and shown in the maze; when the user starts the game they can walk around the maze and search for the end location to win.

https://user-images.githubusercontent.com/32328378/117594532-2d391c80-b136-11eb-92f0-26827b7504be.mp4

### Dependencies
* GLAD
* GLFW3
* GLM
* STB Image
* ASSIMP

### Tools Used
* Visual Studio Enterprise 2019
* Blender (Models & Textures)

### Resources Used
* https://www.sketchuptextureclub.com/ - Wall and floor textures

### How to use / Interact with the game
When you run the executable, a terminal will open requiring you to enter two values: the desired maze size in the x and y dimensions. Once you have entered these values, the game will launch and you will be able to watch the maze generation algorithm run. At this point, you can take control of the camera and fly around by clicking anywhere in the window. To release the mouse and return to the automatic camera, press right click.

Once maze generation has finished, you will see two cubes appear in the maze, one blue representing the start, one green representing the end. At this point, you can press the spacebar to start the game. The camera will change to a walking style, and you must navigate the maze to reach the green cube to win. Once you walk into this cube, the game will close and a message will show in the terminal confirming your win.

### Explanation of code
#### Overview
At a high level, the program works by simultaneously generating a maze and rendering the world. This is achieved by maintaining a state using an instance of the class Maze, and using two threads, one to generate and update this state while the maze is being generated, and one to handle rendering, by which the maze state is polled at a given tick rate to check for changes.

Once generation has finished and the game has started, the Maze class will be updated to contain a chosen start and win location, and the generation thread will exit. Within the render loop, the world space coordinates of the camera are transformed into cell locations using the function worldSpaceToCellLocation(), and these coordinates are checked against the location of the winning cell to determine if the player has won.

#### Classes
##### Maze
This class contains the current state of the maze.
###### Attributes:
* sizeX, sizeY, mazeLength: Together these attributes encapsulate the size of the maze
* startCell: Holds an x and y coordinate representing the cell the player will start in. This is not populated until maze generation is complete.
* winCell: Holds an x and y coordinate representing the winning cell. This is not populated until maze generation is complete.
* maze (pointer): Pointer to the start of a dynamically sized array of length sizeX * sizeY, holding the current maze data. Maze data is stored as a byte, with different bits representing different paths - this is further explained in the initMaze() method below.
* cellLocations: A vector holding references to generated cell locations. This is used by a function polling the maze state to determine which cells have been generated, assisting in reading from the maze array.

###### Methods:
Methods include get and set for the above mentioned attributes. There is no more functionality; the primary purpose of this class is encapsulation.

##### Mesh & Model
These classes are responsible for handling the ASSIMP data structures, along with switching textures and VAOs appropriately.

##### Shader
This class handles the compilation of a shader program from GLSL files stored on the disk. It also contains abstraction methods for setting some types of uniforms which have been used in my GLSL files. 

#### Key Methods
##### Maze Generation - initMaze()
This method runs the procedural generation algorithm to create a maze.

A space is allocated of the size specified by the user and contained within a passed Maze object. The maze path is represented by combinations of the following constants, allowing bitwise operations:

* CELL_NULL = 0x00,
* CELL_PATH_N = 0x01,
* CELL_PATH_E = 0x02,
* CELL_PATH_S = 0x04,
* CELL_PATH_W = 0x08,
* CELL_VISITED = 0x10

These constants allow all combinations to be represented within a byte.

A random start point is chosen, and marked as visited. The cell is pushed onto a stack which tracks the path so far. The surrounding cells are checked to see which have not been marked as visited. If one or more surrounding cells are unvisited, one of the cells is randomly selected to be the next. This process is repeated for the next cell. Once a cell is encountered with no unvisited neighbours, the algorithm backtracks by popping cells off the stack, and checking these for unvisited cells. Backtracking continues until an unvisited cell is found. The algorithm runs until the number of cells visited is equal to the number of cells requested. Every time a cell is changed, the maze object is safely updated, which the render thread can then poll.

Once the whole maze is populated, a list of dead ends is created (determined by CELL & 0x0F == 0), two are chosen from this list to be the start and end points and stored in the Maze object. This function ends and its associated thread quits.

##### Render Loop - main()
This loop polls the Maze object at a tick rate to determine the current state of the game, and renders this to the screen.

The maze data is copied from the maze object, and its associated vector of locations is used to read it. All the locations are inspected to draw the maze. The loop must then determine which walls need to be drawn; it will do this by checking for each direction:

1) This cell does not head in that direction

2) The neighbouring cell in this direction does not head towards this one

Once the conditions have been evaluated, the wall objects are drawn using an instance of the Model class.

Importantly, this loop must also check if the winning condition has been met. The current camera location is compared with the winning cells contained in the Maze object. If they are equal, the loop breaks and the terminal tells the player they have won.

##### Input handling - processInput(), mouse_callback()
These methods manipulate the camera and game state.

processInput() listens for movement commands from the player and manipulates the camera by taking into account the cameras heading and the key pressed. The behaviour changes based on the camera mode.

processInput() is responsible in addition to the camera for checking when the user is trying to start the game (spacebar) or change camera modes (mouse buttons).

The mouse callback is called when the user moves the mouse. Providing the correct camera mode is selected, this callback will calculate pitch and yaw values for the amount of mouse movement, and use these to calculate the new camera direction. This camera direction may also be used in processInput() for movement, depending on the camera mode.
