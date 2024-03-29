#include <iostream>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <numeric>
#include <queue>
#include <sstream>
#include <iomanip>
#include <cmath>

// create a struct that when given a task saves its values into its variables
struct Task
{
	char id;
	int wcet;
	int period;
};

struct Info
{
	std::vector<Task> tasks;
	float utilization;
	int hPeriod;

	float setNumber;
	std::string output;
};

// get utilization which is sum of all (WCET / PERIOD)
float setUtilization(const std::vector<Task>& tasks)
{
	float util = 0;
	// adds the values and gets the sum using same logic as the hyper period function
	for (const auto& task : tasks)
	{
		util += float(task.wcet) / task.period;
	}
	
	return util;
}

// calculates hyper period given all the tasks
int hyperPeriod(const std::vector<Task>& tasks)
{
	int hPeriod = 1; 
	// since we are passing by reference const allows nothing to be changed and auto does a task for all tasks (vector) and calculates the lcm
	for (const auto& task : tasks)
	{
		hPeriod = std::lcm(hPeriod, task.period);
	}

	return hPeriod;
}

bool compareTasks(const Task a, const Task b)
{
	return a.period < b.period;
}

// scheduling algorithm for the tasks
void* RMS(void* void_ptr)
{
	// cast void pointer to a struct Info
	Info* info_ptr = (Info*)void_ptr;	// cast void_ptr to a struct from Info
	Info activeTasks;

	// calculate the utilization for set of tasks
	info_ptr->utilization = setUtilization(info_ptr->tasks);
	// calculate the hyperperiod for set of tasks
	info_ptr->hPeriod = hyperPeriod(info_ptr->tasks);

	// check if the tasks are schedulable
	info_ptr->setNumber = info_ptr->tasks.size() * (pow(2, float(1) / info_ptr->tasks.size()) - 1);

	if (info_ptr->utilization <= info_ptr->setNumber)
	{
		// execute scheduling algorithm
		for (auto& t : info_ptr->tasks)
		{
			activeTasks.tasks.push_back(t);
		}
		// algorithm loop
		while (true)
		{
			// sort by period
			sort(activeTasks.tasks.begin(), activeTasks.tasks.end(), compareTasks);

			
		}
		/*
		for (int i = 0; i < activeTasks.tasks.size(); i++)
		{
			auto& t = activeTasks.tasks[i];
			info_ptr->output.append(std::to_string(t.period));
			if (i < activeTasks.tasks.size() - 1)
			{
				info_ptr->output.append(" ");
			}
		}
		*/
	}
	else
	{
		// not schedulable
		info_ptr->output = "Task set schedulabilty is unkown";
	}
	
	

	return nullptr;
}


/*
TEST CASES
A 2 10 B 4 15 C 3 30
X 10 25 Y 5 10 Z 1 20
A 1 5 B 5 20 C 1 10 D 1 5
*/
int main()
{
	std::vector<Info> input;
	std::string line = "";

	// read tasks from input (add into a vector then add each line into another vector in Info struct)
	while (getline(std::cin, line))
	{
		if (line == "exit")		// for testing
		{
			break;
		}
		int found = line.find("1");
		if (found != -1)
		{
			std::stringstream parseInput(line);
			Task tempTask;
			std::vector<Task> tasksInput;
			while (parseInput >> tempTask.id >> tempTask.wcet >> tempTask.period)
			{
				tasksInput.push_back(tempTask);		// add each row of tasks into vector
			}
			input.push_back({ tasksInput });		// add each vector into input vector
		}
	}

	// initiliazie number of threads and create thread execution loop
	int nThreads = input.size();
	pthread_t* tid = new pthread_t[nThreads];

	for (int i = 0; i < input.size(); i++)
	{
		
		if (pthread_create(&tid[i], nullptr, RMS, &input.at(i)))
		{
			std::cerr << "Error creating thread" << std::endl;
			return 1;
		}
	}

	for (int i = 0; i < nThreads; i++)
	{
		pthread_join(tid[i], nullptr);
	}

	// start of CPU loop 
	for (int i = 0; i < nThreads; i++)
	{
		std::cout << "CPU " << i + 1 << std::endl;
		// output task information from vector
		std::cout << "Task scheduling information: ";
		for (int j = 0; j < input.at(i).tasks.size(); j++)
		{
			if (j < input.at(i).tasks.size() - 1)
			{
				std::cout << input.at(i).tasks.at(j).id << " (WCET: " << input.at(i).tasks.at(j).wcet << ", Period: " << input.at(i).tasks.at(j).period << "), ";
			}
			else
			{
				std::cout << input.at(i).tasks.at(j).id << " (WCET: " << input.at(i).tasks.at(j).wcet << ", Period: " << input.at(i).tasks.at(j).period << ")";
			}
		}

		// output the utilization for each input
		std::cout << "\nTask set utilization: " << std::setprecision(2) << input.at(i).utilization << std::endl;
		// output the hyperperiod for each input
		std::cout << "Hyperperiod: " << input.at(i).hPeriod << std::endl;
		// output the scheduling algorithm
		if (i < nThreads - 1)
		{
			std::cout << "Rate Monotonic Algorithm execution for CPU" << i + 1 << ":\n" << input.at(i).output << std::endl << std::endl << std::endl;
		}
		else
		{
			std::cout << "Rate Monotonic Algorithm execution for CPU" << i + 1 << ":\n" << input.at(i).output;
		}
		
	}
	// end of CPU thread loop

	std::cout << std::endl << std::endl << "Application Finished";
	
	return 0;
}
