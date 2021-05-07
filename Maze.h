#ifndef MAZE_H
#define MAZE_H

#include <glad/glad.h>
#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

class Maze
{
private:
	size_t sizeX, sizeY, mazeLength;
	int currentX, currentY;
	std::pair<int, int> startCell;
	std::pair<int, int> winCell;
	uint8_t *maze;
	std::vector<std::pair<size_t, size_t>> cellLocations;
public:
	Maze(size_t sizeX, size_t sizeY);

	void setMaze(uint8_t *newMaze, size_t mazeLength);
	void addCellLocation(std::pair<size_t, size_t>);
	void setCurrentLocation(unsigned int currentX, unsigned int currentY);
	void setStartCell(int x, int y);
	void setWinCell(int x, int y);

	uint8_t* getMaze();
	size_t getMazeLength();
	int getMazeNumElements();
	std::vector<std::pair<size_t, size_t>> getCellLocations();
	size_t getSizeX();
	size_t getSizeY();
	int getCurrentX();
	int getCurrentY();
	std::pair<int, int> getStartCell();
	std::pair<int, int> getWinCell();
};

#endif
