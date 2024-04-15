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

int numOfCores = std::thread::hardware_concurrency(); 

int total_dead_cells = 0;
int total_alive_cells = 0;

std::barrier cellUpdateBarrier{16};

//std::barrier cellUpdateBarrierOne{16};
//std::barrier cellUpdateBarrierTwo{8};
//std::barrier cellUpdateBarrierThree{4};

std::mutex textMutex;
std::mutex gridMutex;
std::mutex cellUpdateMtx;
std::mutex aliveCellsMutex;
std::mutex deadCellsMutex;

std::condition_variable cv;

std::atomic<bool> simulationActive;

void incrementAliveCells()
{
	aliveCellsMutex.lock();
	total_alive_cells++ ;
	aliveCellsMutex.unlock();
}

void incrementDeadCells()
{
	deadCellsMutex.lock();
	total_dead_cells++;
	deadCellsMutex.unlock();
}


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
	std::unique_lock<std::mutex> lock(cellUpdateMtx);
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
	bool chunk[3][3]; 
	
	while (simulationActive)
	{
		for (int y = 0; y < 3; y++)
		{
			for (int x = 0; x < 3; x++)
			{
				chunk[x][y] = lifeCheck(startCellX + x, startCellY + y);
				
				// update the total number of alive or dead cells throughout the simulation
				if(chunk[x][y])
				{
					incrementAliveCells();
				}
				else
				{
					incrementDeadCells();
				}
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

	
	//std::vector<bool> values;
	////int x_size, y_size;
	////switch (size_case)
	////{
	////case(1):
	////	x_size = 3;
	////	y_size = 3;
	////	break;
	////case(2):
	////	x_size = 3;
	////	y_size = 6;
	////	break;
	////case(3):
	////	x_size = 6;
	////	y_size = 6;
	////	break;
	////default:
	////	break;
	////}
	//
	////for (int y = 0; y < y_size; y++)
	////{
	////	for (int x = 0; x < x_size; x++)
	////	{
	////		values.push_back(false);
	////	}
	////}
	//
	//while (simulationActive)
	//{
	//	int i = 0;
	//	for (int y = 0; y < y_size; y++)
	//	{
	//		for (int x = 0; x < x_size; x++)
	//		{
	//			values[i] = lifeCheck(startCellX + x, startCellY + y);
	//			i++;
	//		}
	//	}
	//	
	//	cellUpdateBarrier.arrive_and_wait();
	//
	//	//switch (size_case) //this is a very stupid thing, but I'm tired and don't want to think about it anymore
	//	//{
	//	//case(1):
	//	//	cellUpdateBarrierOne.arrive_and_wait();
	//	//	break;
	//	//case(2):
	//	//	cellUpdateBarrierTwo.arrive_and_wait();
	//	//	break;
	//	//case(3):
	//	//	cellUpdateBarrierThree.arrive_and_wait();
	//	//	break;
	//	//}
	//
	//	i = 0;
	//
	//	for (int y = 0; y < y_size; y++)
	//	{
	//		for (int x = 0; x < x_size; x++)
	//		{
	//			updateCell(values[i], startCellX + x, startCellY + y);
	//			i++;
	//		}
	//	}
	//
		//std::vector<bool> values;
	////int x_size, y_size;
	////switch (size_case)
	////{
	////case(1):
	////	x_size = 3;
	////	y_size = 3;
	////	break;
	////case(2):
	////	x_size = 3;
	////	y_size = 6;
	////	break;
	////case(3):
	////	x_size = 6;
	////	y_size = 6;
	////	break;
	////default:
	////	break;
	////}
//
	////for (int y = 0; y < y_size; y++)
	////{
	////	for (int x = 0; x < x_size; x++)
	////	{
	////		values.push_back(false);
	////	}
	////}

}

void displayGrid(bool includeCoords)
{
	gridMutex.lock();
	displayCells(includeCoords);
	gridMutex.unlock();
}

void updateBoard(int numOfSteps) //start cell will be the cell in the top left of the chunk not using threads
{
	bool chunk[12][12];
	
	for(int i = 0; i < numOfSteps; i++)
	{
		for (int y = 0; y < 12; y++)
		{
			for (int x = 0; x < 12; x++)
			{
				chunk[x][y] = lifeCheck(x,y);
			}
		}

		for (int y = 0; y < 12; y++)
		{
			for (int x = 0; x < 12; x++)
			{
				updateCell(chunk[x][y],x,y);
			}
		}
		displayGrid(false);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		
	}

}

void setUpCells()
{
	bool setupComplete = false;
	int chosen = 0;
	int xC, yC;
	std::string inputString;
	for (int i = 0; i < 14; i++)
	{
		for (int j = 0; j < 14; j++)
		{
			cells[i][j] = false;
		}
	}

	displayText("Use a preset cell combination (1) or create your own? (2)");
	std::cin >> chosen;

	switch (chosen)
		{
		case(1):
			while (!setupComplete)
			{
				displayText("Glider (1) \n1-2-3 Oscillator - Dave Buckingham (2) \n1-2-3-4 Oscillator (3)\n");
				std::cin >> chosen;
				switch (chosen)
				{
				case(1): //glider
					toggleCell(10, 11);
					toggleCell(10, 12);
					toggleCell(11, 10);
					toggleCell(11, 11);
					toggleCell(12, 12);
					break;
				case(2): //1-2-3 oscillator
					toggleCell(2, 3);
					toggleCell(2, 4);
					toggleCell(3, 4);
					toggleCell(3, 5);
					toggleCell(3, 6);
					toggleCell(4, 2);
					toggleCell(4, 7);
					toggleCell(5, 2);
					toggleCell(5, 3);
					toggleCell(5, 4);
					toggleCell(5, 5);
					toggleCell(5, 7);
					toggleCell(6, 7);
					toggleCell(6, 9);
					toggleCell(6, 10);
					toggleCell(7, 4);
					toggleCell(7, 8);
					toggleCell(7, 10);
					toggleCell(8, 4);
					toggleCell(8, 5);
					toggleCell(8, 6);
					toggleCell(8, 7);
					toggleCell(10, 6);
					toggleCell(10, 7);
					toggleCell(11, 6);
					toggleCell(11, 7);
					break;
				case(3):
					toggleCell(2, 5);
					toggleCell(2, 6);
					toggleCell(3, 5);
					toggleCell(4, 6);
					toggleCell(5, 3);
					toggleCell(5, 4);
					toggleCell(5, 5);
					toggleCell(5, 7);
					toggleCell(6, 2);
					toggleCell(6, 7);
					toggleCell(6, 10);
					toggleCell(7, 1);
					toggleCell(7, 3);
					toggleCell(7, 5);
					toggleCell(7, 7);
					toggleCell(7, 9);
					toggleCell(7, 11);
					toggleCell(8, 2);
					toggleCell(8, 7);
					toggleCell(8, 10);
					toggleCell(9, 3);
					toggleCell(9, 4);
					toggleCell(9, 5);
					toggleCell(9, 7);
					toggleCell(10, 6);
					toggleCell(11, 5);
					toggleCell(12, 5);
					toggleCell(12, 6);
					break;
				default:
					break;
				} //choose a preset

				displayGrid(false);
				displayText("Ready? (Y/N)");
				std::cin >> inputString;
				if (inputString == "Y" || inputString == "y")
				{
					setupComplete = true;
				}
			}
			break;
		case(2):
			displayGrid(true);	
			while (!setupComplete)
			{					
				displayText("X:");			
				std::cin >> xC;
				displayText("Y:");
				std::cin >> yC;
				toggleCell(yC, xC);
				displayGrid(true);
				displayText("Ready? (Y/N)");
				std::cin >> inputString;
				if (inputString == "Y" || inputString == "y")
				{
					setupComplete = true;
				}
			}
			break;
	}
	
	//get the starting number of alive and dead cells
	for (int y = 1; y < 12; y ++)
	{
		for (int x = 1; x < 12; x++)
		{
			if (cells[x][y])
			{
				total_alive_cells++;
			}
			else
			{
				total_dead_cells++;
			}
		}
	}
	simulationActive = true;
}

int main()
{
	int steps;

	displayText("Enter number of steps");
	std::cin >> steps;

	int MaxSteps = steps;

	setUpCells();

	std::vector<std::thread> chunkThreads;

	//uses 16 threads to make a 4*4 field, will change later to set it up so that it uses a varying number of threads based on cores
	//int interval_y, interval_x, size_case; //based on num of cores create more/less chunks
	//
	//interval_x = 3;
	//interval_y = 3;
	//
	//
	//removed, was originally going to have it cores dependent, but ran out of time
	//if (numOfCores >= 16) //16 cores - 4*4 grid of chunks - each cell is 3*3
	//{
	//	interval_x = 3;
	//	interval_y = 3;
	//	size_case = 1;
	//}
	//else if (numOfCores >= 8) //8 cores - 4*2 grid of chunks - each cell is 3*6
	//{
	//	interval_x = 3;
	//	interval_y = 6;
	//	size_case = 2;
	//}
	//else //anything less than 8 2*2 grid - each cell is 6*6
	//{
	//	interval_x = 6;
	//	interval_y = 6;
	//	size_case = 3;
	//}
	//
	//
	//for (int y = 1; y < 12;y+= interval_y)
	//{
	//	for (int x = 1; x< 12; x+= interval_x)
	//	{
	//		chunkThreads.push_back(std::thread(updateChunk, x, y, size_case));
	//	}
	//}


	//16 threads for a 4*4 grid
	for (int y = 1; y < 12; y+= 3)
	{
		for (int x = 1; x < 12; x+= 3)
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

	for (int i = 0; i < chunkThreads.size(); i++)
	{
		chunkThreads[i].join();
	}


	std::cout << "Throughout the simulation " << MaxSteps << " steps were performed\nThere were a total of " << total_alive_cells << " alive cells, " << float(total_alive_cells / MaxSteps) << " on average per step\nThere were a total of " << total_dead_cells << " dead cells, " << float(total_dead_cells / MaxSteps) << " on average per step" << std::endl;


	//setUpCells();
	//updateBoard(alsoSteps);



	return 0;
}

