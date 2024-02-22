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

bool cells[14][14]; //create a 10*10 grid but only use the inner 8*8, so that when parsing through data later issues don't arise, the outer layer should always be false/deaad

int numOfThreads = std::thread::hardware_concurrency();

std::barrier cellUpdateBarrier{16};

std::mutex textMutex;
std::mutex gridMutex;

std::counting_semaphore displaySignaller{numOfThreads};

std::atomic<bool> simulationActive;

void displayCells(bool includeCoords)
{
	bool toggle = true; //will clean up later, this is the best solution i can see just now
	std::cout << "\x1B[2J\x1B[H";
	if(includeCoords)
	{ 
		std::cout << ANSI_RESET << "  1 2 3 4 5 6 7 8 9 101112" << std::endl;
		for (int i = 1; i < 13; i++) //only display the inner area of the cell grid
		{
			for (int k = 0; k < 2; k++)
			{
				if (toggle)
				{
					if (i < 10)
					{
						std::cout << ANSI_RESET << i << " ";
					}
					else
					{
						std::cout << ANSI_RESET << i;
					}
					toggle = false;
				}
				else
				{
					std::cout << ANSI_RESET << "  ";
					toggle = true;
				}

				for (int j = 1; j < 13; j++)
				{

					if (cells[i][j])
					{
						std::cout << ANSI_GREEN << "OO";
					}
					else
					{
						std::cout << ANSI_RED << "XX";
					} 
					
				}
				
				std::cout << ANSI_RESET << std::endl;
			}
		}
	
	}
	else
	{
		std::cout << std::endl;
		for (int i = 1; i < 13; i++) //only display the inner area of the cell grid
		{
			for (int k = 0; k < 2; k++)
			{
				for (int j = 1; j < 13; j++)
				{
					if (cells[i][j])
					{
						std::cout << ANSI_GREEN << "OO";
					}
					else
					{
						std::cout << ANSI_RED << "XX";
					}
				}
				std::cout << ANSI_RESET;
				std::cout << std::endl;
			}
		}
		//std::cout << ANSI_BLUE << "--------";
		std::cout << ANSI_RESET << std::endl;
	}
}

bool checkCell(int x, int y)
{
	return(cells[x][y]);
}

void updateCell(bool update, int x, int y)
{
	cells[x][y] = update;
}

void toggleCell(int x, int y)
{
	if (cells[x][y])
	{
		cells[x][y] = false;
	}
	else
	{
		cells[x][y] = true;
	}
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
		bool chunk[3][3];

		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				chunk[x][y] = lifeCheck(startCellX + x, startCellY + y);
			}
		}

		cellUpdateBarrier.arrive_and_wait();

		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				updateCell(chunk[x][y], startCellX + x, startCellY + y);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void displayGrid(bool includeCoords)
{
	gridMutex.lock();
	displayCells(includeCoords);
	gridMutex.unlock();
}

void setUpCells()
{
	bool setupComplete = false;
	int chosenCell = 0;
	int xC, yC;
	for (int i = 0; i < 14; i++)
	{
		for (int j = 0; j < 14; j++)
		{
			cells[i][j] = false;
		}
	}

	while (!setupComplete)
	{	
		displayText("Current state of cell grid");
		displayGrid(true);
		displayText("Toggle a cell (1), or continue (2)");
		std::cin >> chosenCell;

		if (chosenCell == 2)
		{
			setupComplete = true;
		}
		else
		{
			displayText("X:");
			std::cin >> xC;
			displayText("Y:");
			std::cin >> yC;
			toggleCell(yC, xC);
		}
	}
	simulationActive = true;
}


int main()
{
	int steps;
	std::cout << "Enter number of steps" << std::endl;
	std::cin >> steps;
	setUpCells();
	displayGrid(false);

	std::vector<std::thread> chunkThreads;



	//uses 16 threads to make a 4*4 field, will change later to set it up so that it uses a varying number of threads based on cores
	for (int y = 1; y < 12;y+=3)
	{
		for (int x = 1; x< 12; x+=3)
		{
			chunkThreads.push_back(std::thread(updateChunk, x, y));
		}
	}

	std::thread displayThread([&steps]() {
		while (simulationActive) {
			displayGrid(false);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			steps--;
			if (steps <= 0)
			{
				simulationActive = false;
			}
		}
	});
	
	displayThread.join();

	for (int i = 0; i < 16; i++)
	{
		chunkThreads[i].join();
	}

	

	return 0;
}

