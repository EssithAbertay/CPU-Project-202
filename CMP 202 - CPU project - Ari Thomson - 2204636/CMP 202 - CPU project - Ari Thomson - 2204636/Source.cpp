#include <iostream>
#include <thread>
#include <vector>
#include <barrier>
#include <mutex>
#include <string>

//The rules of conways game of life, taken from wikipedia https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
//Any live cell with fewer than two live neighbors dies, as if by underpopulation.
//Any live cell with two or three live neighbors lives on to the next generation.
//Any live cell with more than three live neighbors dies, as if by overpopulation.
//Any dead cell with exactly three live neighbors becomes a live cell, as if by reproduction

// current error, updating cells before checking reliant cells
// currently assuming running with 16 threads, will later work on runniing with 8 threads

// ANSI color codes for terminal text coloring
const char* ANSI_RESET = "\033[0m";
const char* ANSI_RED = "\033[41m";
const char* ANSI_GREEN = "\033[42m";
const char* ANSI_BLUE = "\033[44m";
const char* ANSI_YELLOW = "\033[43m";

bool cells[10][10]; //create a 10*10 grid but only use the inner 8*8, so that when parsing through data later issues don't arise, the outer layer should always be false/deaad

int numOfThreads = std::thread::hardware_concurrency();
std::barrier cellUpdateBarrier{ numOfThreads };

std::mutex textMutex;

void setUpCells()
{
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			cells[i][j] = false;
		}
	}

	cells[1][2] = true;
	cells[2][1] = true;
	cells[2][2] = true;
	//cells[4][4] = true;
	//cells[5][5] = true;
	//cells[6][6] = true;
	//cells[7][7] = true;
	//cells[8][8] = true;
}

void displayCells()
{
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
	std::cout << ANSI_RESET;
	std::cout << std::endl;
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
	//int cellState = 0;
	int surrounding = 0; //increment for every alive cell surrounding
	bool currentlyAlive = cells[x][y];

	for (int i = 0; i < 3; i++)
	{
		if (cells[(x - 1) + i][y - 1]) //checks row above
		{
			surrounding++;
		}

		if (cells[(x + -1) + i][y + 1]) //checks row below
		{
			surrounding++;
		}
	}

	//checks cells to sides
	if (cells[x-1][y]) //checks row below
	{
		surrounding++;
	}

	if (cells[x - 1][y]) //checks row below
	{
		surrounding++;
	}

	if (currentlyAlive)
	{
		if (surrounding < 2)
		{
			return false;
			//cellState = 0;
			//updateCell(false, x, y);
		}
		else if (surrounding < 4)
		{
			return true;
			//cellState = 1;
			//updateCell(true, x, y);
		}
		else
		{
			return false;
			//cellState = 2;
		//	updateCell(false, x, y);
		}
	}
	else
	{
		if (surrounding == 3)
		{
			return true;
			//cellState = 3;
			//updateCell(true, x, y);
		}
	}

	//switch (cellState)
	//{
	//case 0:
	//	//updateCell(false, x, y);
	//	return false;
	//	break;
	//case 1:
	//	//updateCell(true, x, y);
	//	return true;
	//	break;
	//case 2:
	////	updateCell(false, x, y);
	//	return false;
	//	break;
	//case 3:
	//	//updateCell(true, x, y);
	//	return true;
	//	break;
	//default:
	//	break;
	//}

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
	bool chunk[2][2];
	
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			chunk[i][j] = lifeCheck(startCellX + i, startCellY + j);
		}
	}

	
	displayText(std::string("Chunk " + std::to_string(startCellX) + "," + std::to_string(startCellY) + " completed and waiting"));


	cellUpdateBarrier.arrive_and_wait();

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			updateCell(chunk[i][j],startCellX + i, startCellY + j);
		}
	}

	displayText(std::string("Chunk " + std::to_string(startCellX) + "," + std::to_string(startCellY) + " finished updating"));
}

int main()
{
	std::vector<std::thread> chunkThreads;

	//make sure the number of threads being used can divide into 64 cleanly so that the chunks are fine
	//while (64 % numOfThreads != 0)
	//{
	//	numOfThreads--;
	//}
	setUpCells();
	displayCells();

	for (int i = 1; i < 9; i+=2)
	{
		for (int j = 1; j < 9; j+=2)
		{
			//std::cout << "coords: " << i << ", " << j << std::endl;
			chunkThreads.push_back(std::thread(updateChunk, i, j));
		}
	}


	for (int i = 0; i < 16; i++)
	{
		chunkThreads[i].join();
	}


	displayCells();

	return 0;
}

