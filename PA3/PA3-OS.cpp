#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <numeric>
#include <queue>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <vector>


struct Task
{
    char id;
    int wcet;
    int period;
    int execLeft;

    // operator overload for algorithm scheduling
    bool operator<(const Task& other) const
    {
        if (execLeft == 0 && other.execLeft == 0)
        {
            return period > other.period;
        }
        if (execLeft == 0)
        {
            return execLeft < other.execLeft;
        }
        if (other.execLeft == 0)
        {
            return execLeft < other.execLeft;
        }
        if (period == other.period)
        {
            return id > other.id;
        }
        return period > other.period;
    }
};

struct Info
{
    std::vector<Task> tasks;
    int CPUnum;
    double utilization;
    int hyperPeriod;
    double setNum;
    std::string output;
};

// calculates utilization for each set of tasks
double setUtilization(const std::vector<Task>& tasks)
{
    double util = 0;
    // gets the sum of WCET / period
    for (const auto& task : tasks)
    {
        util += float(task.wcet) / task.period;
    }
    return util;
}

// calculates hyperperiod for each set of tasks
int hyperPeriod(const std::vector<Task>& tasks)
{
    int hPeriod = 1;
    for (const auto& task : tasks)
    {
        hPeriod = std::lcm(hPeriod, task.period);
    }
    return hPeriod;
}

// sorts the vector of tasks based on the period in ascending order
bool tasksPriority(const Task a, const Task b)
{
    return a.period < b.period;
}

std::string convertToDiagram(std::string in)
{
    std::stringstream result;
    char currChar = '\0';
    int currCount = 0;

    for (size_t i = 0; i < in.size(); ++i)
    {
        char c = in[i];
        if (c == currChar)
        {
            currCount++;
        }
        else
        {
            if (currCount > 0)
            {
                if (currChar == 'I')
                {
                    result << "Idle(" << currCount << "), ";
                }
                else
                {
                    result << currChar << "(" << currCount << "), ";
                }
            }

            currChar = c;
            currCount = 1;
        }
    }

    // last character
    if (currCount > 0)
    {
        if (currChar == 'I')
        {
            result << "Idle(" << currCount << ")";
        }
        else
        {
            result << currChar << "(" << currCount << ")";
        }
    }

    return result.str();

}

bool compareTasks(const Task a, const Task b)
{
    return a.period < b.period;
}

void* RMS(void* void_ptr)
{
    // cast void pointer to a struct of type Info
    Info* infoPtr = (Info*)void_ptr;
    // queue for algorithm
    std::priority_queue<Task> Q;
    for (int i = 0; i < infoPtr->tasks.size(); i++)
    {
        Q.push(infoPtr->tasks.at(i));
    }


    // calculate the utilization for the set of tasks
    infoPtr->utilization = setUtilization(infoPtr->tasks);
    // calculate the hyperperiod for the set of tasks
    infoPtr->hyperPeriod = hyperPeriod(infoPtr->tasks);

    // check if the tasks are schedulable
    infoPtr->setNum = infoPtr->tasks.size() * (pow(2, float(1) / infoPtr->tasks.size()) - 1);

    infoPtr->output = "";
    std::string line = "";
    if (infoPtr->utilization > 1)
    {
        infoPtr->output += "The task set is not schedulable";
    }
    else if (infoPtr->utilization <= infoPtr->setNum)
    {
        // execute algorithm
        infoPtr->output += "Scheduling Diagram for CPU " + std::to_string(infoPtr->CPUnum) + ": ";

        for (int i = 1; i <= infoPtr->hyperPeriod; i++)
        {
            Task curr = Q.top();

            // if there are tasks left to execute
            if (curr.execLeft > 0)
            {
                Q.pop();

                // add to output string
                line += std::string(1, curr.id);

                curr.execLeft--;
                Q.push(curr);

                std::priority_queue<Task> temp;

                // check if Task crossed their time period to reset
                while (!Q.empty())
                {
                    Task curr = Q.top();
                    Q.pop();

                    if (i % curr.period == 0 && i != 1)
                    {
                        curr.execLeft += curr.wcet;
                    }

                    temp.push(curr);
                }

                Q = temp;
            }
            else
            {
                line.append("I");
                std::priority_queue<Task> temp;

                while (!Q.empty())
                {
                    Task curr = Q.top();
                    Q.pop();

                    if (i % curr.period == 0 && i != 1)
                    {
                        curr.execLeft += curr.wcet;
                    }

                    temp.push(curr);
                }

                Q = temp;
            }
        }

        infoPtr->output += convertToDiagram(line);
    }
    else
    {
        infoPtr->output += "Task set schedulability is unknown";
    }

    return NULL;
}

/*

A 2 10 B 4 15 C 3 30
X 10 25 Y 5 10 Z 1 20
A 1 5 B 5 20 c 1 10 D 1 5

*/


int main()
{
    std::vector<Info> input;
    std::string line = "";

    // read tasks from input
    while (getline(std::cin, line))
    {
        if (line == "exit")
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
                tempTask.execLeft = tempTask.wcet;
                tasksInput.push_back(tempTask);
            }
            input.push_back({ tasksInput });
        }
    }
    // intialize number of threads
    int nThreads = input.size();
    pthread_t* tid = new pthread_t[nThreads];

    for (int i = 0; i < input.size(); i++)
    {
        input.at(i).CPUnum = i + 1;
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

    // CPU loop
    for (int i = 0; i < nThreads; i++)
    {
        std::cout << "CPU " << i + 1 << std::endl;
        // output task information
        std::cout << "Task scheduling information: ";
        for (int j = 0; j < input.at(i).tasks.size(); j++)
        {
            if (j < input.at(i).tasks.size() - 1)
            {
                std::cout << input.at(i).tasks.at(j).id << " (WCET: " << input.at(i).tasks.at(j).wcet << ", Period: " << input.at(i).tasks.at(j).period << "), ";
            }
            else
            {
                std::cout << input.at(i).tasks.at(j).id << " (WCET: " << input.at(i).tasks.at(j).wcet << ", Period: " << input.at(i).tasks.at(j).period << ")\n";

                std::cout << "Task set utilization: " << std::fixed << std::setprecision(2) << input.at(i).utilization << std::endl;
                std::cout << "Hyperperiod: " << input.at(i).hyperPeriod << std::endl;
                if (i < nThreads - 1)
                {
                    std::cout << "Rate Monotonic Algorithm execution for CPU" << i + 1 << ": \n" << input.at(i).output << "\n\n\n";
                }
                else
                {
                    std::cout << "Rate Monotonic Algorithm execution for CPU" << i + 1 << ": \n" << input.at(i).output;
                }
            }
        }
    }
    //std::cout << "\nFinished Program";

    return 0;
}