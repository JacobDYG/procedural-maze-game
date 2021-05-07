#include "Maze.h"
#include <vcruntime_string.h>

Maze::Maze(size_t sizeX, size_t sizeY)
{
	Maze::sizeX = sizeX;
	Maze::sizeY = sizeY;
	Maze::maze = new uint8_t[sizeX * sizeY];
	memset(Maze::maze, 0x00, sizeX * sizeY);
}

void Maze::setMaze(uint8_t* newMaze, size_t mazeLength)
{
	maze = newMaze;
	Maze::mazeLength = mazeLength;
}

//Adds a offset for the memory location of a corresponding cell in the maze
void Maze::addCellLocation(std::pair<size_t, size_t> cellLocation)
{
	Maze::cellLocations.push_back(cellLocation);
}

void Maze::setCurrentLocation(unsigned int currentX, unsigned int currentY)
{
	Maze::currentX = currentX;
	Maze::currentY = currentY;
}

void Maze::setStartCell(int x, int y)
{
	Maze::startCell = std::pair<int, int>(x, y);
}

void Maze::setWinCell(int x, int y)
{
	Maze::winCell = std::pair<int, int>(x, y);
}

//Returns a pointer to the maze. Correct locations 
uint8_t* Maze::getMaze()
{
	return maze;
}

//Gets the amount of ram required for maze in bytes
size_t Maze::getMazeLength()
{
	return sizeof(uint8_t) * sizeX * sizeY;
}

int Maze::getMazeNumElements()
{
	return mazeLength / sizeof(int8_t);
}

std::vector<std::pair<size_t, size_t>> Maze::getCellLocations()
{
	return Maze::cellLocations;
}

size_t Maze::getSizeX()
{
	return Maze::sizeX;
}

size_t Maze::getSizeY()
{
	return Maze::sizeY;
}

int Maze::getCurrentX()
{
	return Maze::currentX;
}

int Maze::getCurrentY()
{
	return Maze::currentY;
}

std::pair<int, int> Maze::getStartCell()
{
	return startCell;
}

std::pair<int, int> Maze::getWinCell()
{
	return winCell;
}
