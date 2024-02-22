#include <iostream>
#include <thread>
#include <vector>
#include <barrier>
#include <mutex>
#include <string>
#include <semaphore>
#include <atomic>

//The rules of conways game of life, taken from wikipedia https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
//Any live cell with fewer than two live neighbors dies, as if by underpopulation.
//Any live cell with two or three live neighbors lives on to the next generation.
//Any live cell with more than three live neighbors dies, as if by overpopulation.
//Any dead cell with exactly three live neighbors becomes a live cell, as if by reproduction

// currently assuming running with 16 threads, will later work on runniing with 8 threads

// ANSI color codes for terminal text coloring
const char* ANSI_RESET = "\033[0m";
const char* ANSI_RED = "\033[41m";
const char* ANSI_GREEN = "\033[42m";
const char* ANSI_BLUE = "\033[44m";
const char* ANSI_YELLOW = "\033[43m";

bool cells[10][10]; //create a 10*10 grid but only use the inner 8*8, so that when parsing through data later issues don't arise, the outer layer should always be false/deaad

int numOfThreads = std::thread::hardware_concurrency();

std::barrier cellUpdateBarrier{16};

std::mutex textMutex;
std::mutex gridMutex;

std::counting_semaphore displaySignaller{numOfThreads};

std::atomic<bool> simulationActive;

void setUpCells()
{
	simulationActive = true;

	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			cells[i][j] = false;
		}
	}


	//X and Y are swapped just now, idk why
	cells[6][7] = true;
	cells[7][6] = true;
	cells[8][6] = true;
	cells[7][7] = true;
	cells[8][8] = true;
}

void displayCells()
{
	std::cout << std::endl;
	std::cout << "\x1B[2J\x1B[H";
	for (int i = 1; i < 9; i++) //only display the inner area of the cell grid
	{
		for (int j = 1; j < 9; j++)
		{
			if (cells[i][j])
			{
				std::cout << ANSI_GREEN << "O";
			}
			else
			{
				std::cout << ANSI_RED << "X";
			}
		}
			std::cout << ANSI_RESET;
		std::cout << std::endl;
	}
	std::cout << ANSI_BLUE << "--------";
	std::cout << ANSI_RESET << std::endl;
}

bool checkCell(int x, int y)
{
	return(cells[x][y]);
}

void updateCell(bool update, int x, int y)
{
	cells[x][y] = update;
}


bool lifeCheck(int x, int y) //check the status of surrounding cells, and then update accordingly
{
	int surrounding = 0; //increment for every alive cell surrounding
	bool currentlyAlive = cells[x][y];

	for (int i = 0; i < 3; i++)
	{
		if (cells[(x - 1) + i][y - 1]) //checks row above
		{
			surrounding++;
		}

		if (cells[(x -1) + i][y + 1]) //checks row below
		{
			surrounding++;
		}
	}

	//checks cells to sides
	if (cells[x-1][y]) 
	{
		surrounding++;
	}

	if (cells[x + 1][y]) 
	{
		surrounding++;
	}

	if (currentlyAlive) //finds what the new value should be
	{
		if (surrounding < 2)
		{
			return false;
		}
		else if (surrounding < 4)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if (surrounding == 3)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

void lifeCheckAllCells()
{
	for (int i = 1; i < 9; i++)
	{
		for (int j = 1; j < 9; j++)
		{
			lifeCheck(i,j);
		}
	}
}

void displayText(std::string text)
{
	textMutex.lock();
	std::cout << text << std::endl;
	textMutex.unlock();
}


void updateChunk(int startCellX, int startCellY) //start cell will be the cell in the top left of the chunk
{
	while (simulationActive)
	{
		bool chunk[2][2];

		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				chunk[x][y] = lifeCheck(startCellX + x, startCellY + y);
			}
		}

		cellUpdateBarrier.arrive_and_wait();

		for (int y = 0; y < 2; y++)
		{
			for (int x = 0; x < 2; x++)
			{
				updateCell(chunk[x][y], startCellX + x, startCellY + y);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void displayGrid()
{
	gridMutex.lock();
	displayCells();
	gridMutex.unlock();
}

int main()
{
	//int runs;
	//std::cout << "Enter number of runs" << std::endl;
	//std::cin >> runs;
	setUpCells();
	displayGrid();

	std::vector<std::thread> chunkThreads;



	//uses 16 threads to make a 4*4 field, will change later to set it up so that it uses a varying number of cores
	for (int y = 1; y < 8;y+=2)
	{
		for (int x = 1; x< 8; x+=2)
		{
			chunkThreads.push_back(std::thread(updateChunk, x, y));
		}
	}

	std::thread displayThread([&]() {
		while (simulationActive) {
			displayGrid();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	});

	for (int i = 0; i < 16; i++)
	{
		chunkThreads[i].join();
	}

	

	return 0;
}

