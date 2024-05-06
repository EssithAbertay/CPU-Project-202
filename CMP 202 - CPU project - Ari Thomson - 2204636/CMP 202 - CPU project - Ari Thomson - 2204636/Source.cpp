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


// ANSI color codes for terminal text coloring
const char* ANSI_RESET = "\033[0m";
const char* ANSI_RED = "\033[41m";
const char* ANSI_GREEN = "\033[42m";
const char* ANSI_BLUE = "\033[44m";
const char* ANSI_YELLOW = "\033[43m";

const int sizeOfGrid = 12; //change this number to update grid size, must be multiple of 12

bool tempCells[sizeOfGrid + 2][sizeOfGrid + 2];
bool cells[sizeOfGrid + 2][sizeOfGrid + 2]; //create a 10*10 grid but only use the inner 8*8, so that when parsing through data later issues don't arise, the outer layer should always be false/deaad


std::mutex textMutex;
std::mutex gridMutex;


std::atomic<bool> simulationActive;


void displayCells(bool includeCoords)
{
	//bool toggle = true; //will clean up later, this is the best solution i can see just now
	std::cout << "\x1B[2J\x1B[H";
	//if(includeCoords)
	//{ 
	//	std::cout << ANSI_RESET << "  1 2 3 4 5 6 7 8 9 101112" << std::endl;
	//	for (int i = 1; i < 13; i++) //only display the inner area of the cell grid
	//	{
	//		for (int k = 0; k < 2; k++)
	//		{
	//			if (toggle)
	//			{
	//				if (i < 10)
	//				{
	//					std::cout << ANSI_RESET << i << " ";
	//				}
	//				else
	//				{
	//					std::cout << ANSI_RESET << i;
	//				}
	//				toggle = false;
	//			}
	//			else
	//			{
	//				std::cout << ANSI_RESET << "  ";
	//				toggle = true;
	//			}

	//			for (int j = 1; j < 13; j++)
	//			{

	//				if (cells[i][j])
	//				{
	//					std::cout << ANSI_GREEN << "OO";
	//				}
	//				else
	//				{
	//					std::cout << ANSI_RED << "XX";
	//				} 
	//				
	//			}
	//			
	//			std::cout << ANSI_RESET << std::endl;
	//		}
	//	}
	//
	//}
	//else
	//{
	//	std::cout << std::endl;
	//	for (int i = 1; i < 13; i++) //only display the inner area of the cell grid
	//	{
	//		for (int k = 0; k < 2; k++)
	//		{
	//			for (int j = 1; j < 13; j++)
	//			{
	//				if (cells[i][j])
	//				{
	//					std::cout << ANSI_GREEN << "OO";
	//				}
	//				else
	//				{
	//					std::cout << ANSI_RED << "XX";
	//				}
	//			}
	//			std::cout << ANSI_RESET;
	//			std::cout << std::endl;
	//		}
	//	}
	//	//std::cout << ANSI_BLUE << "--------";
	//	std::cout << ANSI_RESET << std::endl;
	//}

	//back to minimal grid display, cleaner but less intuitive
	for (int i = 1; i < sizeOfGrid + 1; i++)
	{
		for (int j = 1; j < sizeOfGrid + 1; j++)
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
		std::cout << ANSI_RESET << std::endl;
	}
}

bool checkCell(int x, int y)
{
	return(cells[x][y]);
}

void updateCell(bool update, int x, int y)
{
	//std::unique_lock<std::mutex> lock(cellUpdateMtx);
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

void updateChunk(int startCellX, int startCellY, std::barrier<void(*)(void) noexcept>& cellUpdateBarrier, int chunk_size) //start cell will be the cell in the top left of the chunk
{
	//old method used to use an array this is outdated as arrays require constant values for their parameters
	//bool chunk[3][3]; 
	//
	//while (simulationActive)
	//{
	//	for (int y = 0; y < 3; y++)
	//	{
	//		for (int x = 0; x < 3; x++)
	//		{
	//			chunk[x][y] = lifeCheck(startCellX + x, startCellY + y);
	//			
	//			// update the total number of alive or dead cells throughout the simulation
	//			if(chunk[x][y])
	//			{
	//				total_alive_cells++;
	//			}
	//			else
	//			{
	//				total_dead_cells++;
	//			}
	//		}
	//	}
	//
	//	cellUpdateBarrier.arrive_and_wait();
	//
	//	for (int y = 0; y < 3; y++)
	//	{
	//		for (int x = 0; x < 3; x++)
	//		{
	//			updateCell(chunk[x][y], startCellX + x, startCellY + y);
	//
	//			
	//		}
	//	}
	//	
	//	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//
	//}

	
	std::vector<bool> values;
	
	for (int y = 0; y < chunk_size; y++)
	{
		for (int x = 0; x < chunk_size; x++)
		{
			values.push_back(false);
		}
	}
	
	while (simulationActive)
	{
		int i = 0;
		for (int y = 0; y < chunk_size; y++)
		{
			for (int x = 0; x < chunk_size; x++)
			{
				values[i] = lifeCheck(startCellX + x, startCellY + y);
				i++;
			}
		}

		cellUpdateBarrier.arrive_and_wait();

		i = 0;

		for (int y = 0; y < chunk_size; y++)
		{
			for (int x = 0; x < chunk_size; x++)
			{
				updateCell(values[i], startCellX + x, startCellY + y);
				i++;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}


}

void updateChunk_NoDisplay(int startCellX, int startCellY, std::barrier<void(*)(void) noexcept>& cellUpdateBarrier, int chunk_size, int number_of_steps) //start cell will be the cell in the top left of the chunk
{


	//Variant one used in testing
	//std::vector<bool> values;
	//
	//for (int y = 0; y < chunk_size; y++)
	//{
	//	for (int x = 0; x < chunk_size; x++)
	//	{
	//		values.push_back(false);
	//	}
	//}
//
	//for (int steps = 0; steps < number_of_steps; steps++)
	//{
	//	int i = 0;
	//	for (int y = 0; y < chunk_size; y++)
	//	{
	//		for (int x = 0; x < chunk_size; x++)
	//		{
	//			values[i] = lifeCheck(startCellX + x, startCellY + y);
	//			i++;
	//		}
	//	}
//
	//	cellUpdateBarrier.arrive_and_wait();
	//
	//	i = 0;
	//
	//	for (int y = 0; y < chunk_size; y++)
	//	{
	//		for (int x = 0; x < chunk_size; x++)
	//		{
	//			updateCell(values[i], startCellX + x, startCellY + y);
	//			i++;
	//		}
	//	}
	//
	//}

	//variant 2 used in testing
	
	for (int steps = 0; steps < number_of_steps; steps++)
	{
		//int i = 0;
		for (int y = 0; y < chunk_size; y++)
		{
			for (int x = 0; x < chunk_size; x++)
			{
				tempCells[startCellX + x][startCellY + y] = lifeCheck(startCellX + x, startCellY + y);

				//i++;
			}
		}

		cellUpdateBarrier.arrive_and_wait();

		//i = 0;

		for (int y = 0; y < chunk_size; y++)
		{
			for (int x = 0; x < chunk_size; x++)
			{
				updateCell(tempCells[startCellX + x][startCellY + y], startCellX + x, startCellY + y);
				//i++;
			}
		}

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
	int chosen = 0;
	int xC, yC;
	std::string inputString;
	for (int i = 0; i < sizeOfGrid + 2; i++)
	{
		for (int j = 0; j < sizeOfGrid + 2; j++)
		{
			cells[i][j] = false;
		}
	}

	displayText("Use a preset cell combination (1) or create your own? (2)");
	std::cin >> chosen;
	while (!(chosen == 1) && !(chosen == 2))
	{
		std::cout << "Please enter a valid option, use a preset cell combination (1) or create your own (2)" << std::endl;
		std::cin >> chosen;
	}
	

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

				//displayGrid(false);
				displayText("Ready? (Y/N)");
				std::cin >> inputString;
				while (!(inputString == "Y") && !(inputString == "y") && !(inputString == "N") && !(inputString == "n"))
				{
					std::cout << "Please enter a valid response, ready? (Y/N)" << std::endl;
					std::cin >> inputString;
				}
			
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
				displayText("Toggle Cell\n X:");			
				std::cin >> xC;

				while (xC < 1 && xC > sizeOfGrid)
				{
					std::cout << "Please enter a valid x coordinate (between 1 and " << sizeOfGrid << ")" << std::endl;
					std::cin >> xC;
				}

				displayText("Y:");
				std::cin >> yC;

				while (yC < 1 && yC > 12)
				{
					std::cout << "Please enter a valid y coordinate (between 1 and " << sizeOfGrid << ")" << std::endl;
					std::cin >> xC;
				}

				toggleCell(yC, xC);
				displayGrid(true);
				displayText("Ready? (Y/N)");
				std::cin >> inputString;
				while (!(inputString == "Y") && !(inputString == "y") && !(inputString == "N") && !(inputString == "n"))
				{
					std::cout << "Please enter a valid response, ready? (Y/N)" << std::endl;
					std::cin >> inputString;
				}
				if (inputString == "Y" || inputString == "y")
				{
					setupComplete = true;
				}
			}
			break;
	}
	
	simulationActive = true;
}

int main()
{


	//int numOfCores = std::thread::hardware_concurrency(); was planning originally to make the program decide how many threads to use, instead opted to have the user decide
	int numOfThreads,chunkDimension;

	std::cout << "How many threads would you like to run? (1/4/9/16/36)" << std::endl;
	std::cin >> numOfThreads;

	// input validation
	while (!(numOfThreads == 1) && !(numOfThreads == 4) && !(numOfThreads == 9) && !(numOfThreads == 16) && !(numOfThreads == 36))
	{
		std::cout << "Please enter a valid number of threads to run (1/4/9/16/36)" << std::endl;
		std::cin >> numOfThreads;
	}


	//find the size of each chunk based on how many are on the side
	chunkDimension = sizeOfGrid / sqrt(numOfThreads);


	std::string input;
	std::cout << "Would you like to have an output display? (say no to find timings) (Y/N)" << std::endl;
	std::cin >> input;

	// input validation
	while (!(input == "Y") && !(input == "y") && !(input == "N") && !(input == "n"))
	{
		std::cout << "Please enter a valid response (Y/N)" << std::endl;
		std::cin >> input;
	}

	std::barrier<void(*)(void) noexcept> cellUpdateBarrier(numOfThreads, []() noexcept { });


	//std::barrier<> cellUpdateBarrier{ 16 };

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

	typedef std::chrono::steady_clock the_clock;
	the_clock::time_point startTime;

	if(input == "y" || input == "Y")
	{ 
		if(numOfThreads == 1)
		{
		chunkThreads.push_back(std::thread(updateChunk, 1, 1, std::ref(cellUpdateBarrier), chunkDimension));
		}
		else
		{
		for (int y = 1; y < sizeOfGrid; y += chunkDimension)
		{
			for (int x = 1; x < sizeOfGrid; x += chunkDimension)
			{
				chunkThreads.push_back(std::thread(updateChunk, x, y, std::ref(cellUpdateBarrier), chunkDimension));
			}
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
	}
	else
	{
		startTime = the_clock::now();
		if (numOfThreads == 1)
		{
			chunkThreads.push_back(std::thread(updateChunk_NoDisplay, 1, 1, std::ref(cellUpdateBarrier), chunkDimension, steps));
		}
		else
		{

			for (int y = 1; y < sizeOfGrid; y += chunkDimension)
			{
				for (int x = 1; x < sizeOfGrid; x += chunkDimension)
				{
					chunkThreads.push_back(std::thread(updateChunk_NoDisplay, x, y, std::ref(cellUpdateBarrier), chunkDimension, steps));
				}
			}

		}
		
	}


	for (int i = 0; i < chunkThreads.size(); i++)
	{
		chunkThreads[i].join();
	}
	the_clock::time_point endTime = the_clock::now();
	if (input == "n" || input == "N")
	{
	
	auto timeTaken = duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
	std::cout << timeTaken << " milliseconds elapsed" << std::endl;
	}
	return 0;
}

