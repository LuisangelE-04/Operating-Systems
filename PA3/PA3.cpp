#include <pthread.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <queue>
#include <sstream>
#include <iomanip>
#include <cmath>

struct args
{
    std::string in; // input
    // std::string out;                   // output
    int num;                           // which call it is
    int* next;                         // for calling in next thread
    pthread_mutex_t* input_copy_mutex; // for shared data
    pthread_mutex_t* print_mutex;      // for printing
    pthread_cond_t* condition;         // The condition variable.
};

// node will be the main struct used for each task
struct node
{
    std::string name; // stores the task name
    int wceTime;      // stores the task worst case execution time
    int period;       // stores the task period
    int execLeft;     // stores how many executions this task has left in the period

    node(std::string n, int w, int p, int e) : name(n), wceTime(w), period(p), execLeft(e) {}

    // this helps us decide what priority will be in our pQueue
    bool operator<(const node& other) const
    {
        if (execLeft == 0 && other.execLeft == 0)
            return period > other.period;

        if (execLeft == 0)
            return execLeft < other.execLeft;

        if (other.execLeft == 0)
            return execLeft < other.execLeft;

        if (period == other.period)
            return name > other.name;

        return period > other.period;
    }
};

int gcd(int a, int b) // helper for lcm
{
    while (b)
    {
        a %= b;
        std::swap(a, b);
    }
    return a;
}

int lcm(int a, int b) // lcm using gcd
{
    int temp = gcd(a, b);

    return temp ? (a / temp * b) : 0;
}

// used to calculate hyperPeriod
int calculateHyperPeriod(const std::priority_queue<node>& tasks)
{
    if (tasks.empty())
    {
        return 0;
    }

    int hyperPeriod = tasks.top().period;

    std::priority_queue<node> temp = tasks;
    while (!temp.empty())
    {
        node current = temp.top();
        temp.pop();
        hyperPeriod = lcm(hyperPeriod, current.period);
    }

    return hyperPeriod;
}

// this calculates the expression where "Task set schedulability is unknown"
double calculateExpression(int n)
{
    return n * (std::pow(2.0, 1.0 / n) - 1);
}

// this function takes the string I create throughout the program and outputs it formatted.
std::string convertToTaskSchedule(const std::string& input)
{
    std::ostringstream resultStream;
    char currentChar = '\0';
    int currentCount = 0;

    for (size_t i = 0; i < input.size(); ++i)
    {
        char c = input[i];
        if (c == currentChar)
        {
            currentCount++;
        }
        else
        {
            if (currentCount > 0)
            {
                if (currentChar == 'I')
                {
                    resultStream << "Idle(" << currentCount << "), ";
                }
                else
                {
                    resultStream << currentChar << "(" << currentCount << "), ";
                }
            }
            currentChar = c;
            currentCount = 1;
        }
    }

    // Add the last character count
    if (currentCount > 0)
    {
        if (currentChar == 'I')
        {
            resultStream << "Idle(" << currentCount << ")";
        }
        else
        {
            resultStream << currentChar << "(" << currentCount << ")";
        }
    }

    return resultStream.str();
}

// here is my function used in multi-threading
void* RMSA(void* x_void_ptr) // RMSA --> Rate Monotonic Scheduling Algorithm
{
    args Boat = *(args*)x_void_ptr;             // Deinitilization
    int localNum = Boat.num;                     // turning shared resource into a local resource
    std::string localString = Boat.in;           // turning shared resource into a local resource
    pthread_mutex_unlock(Boat.input_copy_mutex); // unlock copying semaphore now that we have it all local

    std::priority_queue<node> pq;
    std::vector<node> Ttasks;
    std::istringstream iss(localString);

    // initializing variables
    std::string name;
    std::string output = "";
    int wceTime, period;
    int numTasks = 0;
    double util = 0;
    std::string out;

    // pushing into two queues, one to help me print, one to be priority
    while (iss >> name >> wceTime >> period)
    {
        pq.push(node(name, wceTime, period, wceTime));
        Ttasks.push_back(node(name, wceTime, period, wceTime));
    }

    // printing CPU #
    int hyperPeriod = calculateHyperPeriod(pq);

    out += "CPU " + std::to_string(localNum) + "\n";
    out += "Task scheduling information: ";

    // this for-loop gets the utilization number, as well as line one printing
    for (std::vector<node>::const_iterator it = Ttasks.begin(); it != Ttasks.end(); ++it)
    {
        const node& task = *it;
        numTasks++;
        util = util + (static_cast<double>(task.wceTime) / static_cast<double>(task.period));
        out += task.name + " (WCET: " + std::to_string(task.wceTime) + ", Period: " + std::to_string(task.period);
        if (numTasks < Ttasks.size())
        {
            out += "), "; // Print comma if it's not the last element
        }
        else
        {
            out += ") "; // If it's the last element, don't print comma
        }
    }

    // more printing
    std::stringstream utilStream;
    utilStream << std::fixed << std::setprecision(2) << util;
    out += "\nTask set utilization: " + utilStream.str();

    out += "\nHyperperiod: " + std::to_string(hyperPeriod) + "\n";
    out += "Rate Monotonic Algorithm execution for CPU " + std::to_string(localNum) + ":\n";

    // logic based on utilization and formula given in directions
    if (util > 1)
    {
        out += "The task set is not schedulable\n";
    }
    else if (util > calculateExpression(numTasks))
    {
        out += "Task set schedulability is unknown\n";
    }
    else // find the scheduling diagram
    {
        out += "Scheduling Diagram for CPU " + std::to_string(localNum) + ": ";
        for (int i = 1; i <= hyperPeriod; i++)
        {
            node current = pq.top();

            // if there are still tasks to execute...
            if (current.execLeft > 0)
            {
                pq.pop();

                // adding to my output string which will later be formatted
                output += current.name;

                current.execLeft--;
                pq.push(current);

                std::priority_queue<node> temp;

                // checking if any of the nodes have crossed thier period, in which case...
                while (!pq.empty())
                {
                    node current = pq.top();
                    pq.pop();

                    if (i % current.period == 0 && i != 1)
                    {

                        current.execLeft += current.wceTime; // i need to add executions!
                    }

                    temp.push(current);
                }

                pq = temp;
            }
            else
            {

                output.append("I"); // will be formatted correctly later

                std::priority_queue<node> temp;

                // checking if any of the nodes have crossed thier period, in which case...
                while (!pq.empty())
                {
                    node current = pq.top();
                    pq.pop();

                    if ((i % current.period == 0 && i != 1))
                    {
                        current.execLeft += current.wceTime; // i need to add executions!
                    }

                    temp.push(current);
                }

                pq = temp;
            }
        }
    }
    out += convertToTaskSchedule(output);
    out += "\n\n";

    pthread_mutex_lock(Boat.print_mutex); // second critical section --> check if num = localNum

    while ((*(Boat.next)) != localNum)
    {
        pthread_cond_wait(Boat.condition, Boat.print_mutex); // if not set it to wait until it does
    }

    pthread_mutex_unlock(Boat.print_mutex); // unlock crit section

    std::cout << out; // print accumulated output

    pthread_mutex_lock(Boat.print_mutex); // lock so we can increment the shared resource

    (*Boat.next)++;

    pthread_cond_broadcast(Boat.condition); // wake up other threads since now we are done

    pthread_mutex_unlock(Boat.print_mutex); // unlock semaphore

    return NULL;
}

int main()
{

    struct args x;
    std::vector<std::string> store;
    pthread_mutex_t input_copy_mutex;
    pthread_mutex_init(&input_copy_mutex, NULL); // semaphore for copying shared resources

    pthread_mutex_t print_mutex;
    pthread_mutex_init(&print_mutex, NULL); // semaphore for printing

    static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    static int next = 1;
    x.input_copy_mutex = &input_copy_mutex;
    x.print_mutex = &print_mutex;
    x.condition = &condition;
    x.next = &next;

    std::string input = "";
    int count1 = 0; // allows me to set pthread_t tid[]
    int count2 = 0; // allows me to make threads increment (++) each time

    while (getline(std::cin, input))
    {
        if (input == "exit")
        {
            break;
        }
        count1++;
        store.push_back(input);
    }

    pthread_t tid[count1];

    for (int i = 0; i < count1; i++)
    {
        pthread_mutex_lock(&input_copy_mutex); // Enter first critical section
        x.in = store[i];                       // sending the input
        x.num = count2 + 1;                    // sending which CPU# it will handle

        if (pthread_create(&tid[count2], NULL, RMSA, &x)) // only using one memory address
        {
            std::cerr << "Error creating thread" << std::endl;
            return 1;
        }
        count2++;
    }

    for (int i = 0; i < count1; i++) // joining threads
        pthread_join(tid[i], NULL);

    return 0;
}

/*
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

struct Info
{
    std::string input;
    //std::string output;
    int num;
    //double utilization;
    //int hyperPeriod;
    //double setNum;

    int* count;
    pthread_mutex_t* inputMutex;
    pthread_mutex_t* printMutex;
    pthread_cond_t* condition;
};

struct Task
{
    char id;
    int wcet;
    int period;
    int execLeft;

    // constructor
    //Task(char _id, int _wcet, int _period, int _execLeft) : id(_id), wcet(_wcet), period(_period), execLeft(_execLeft) {}

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

double calculateExpression(int n)
{
    return n * (pow(2, double(1) / n) + 1);
}

void* RMS(void* void_ptr)
{
    Info Boat = *(Info*)void_ptr;
    int localNum = Boat.num;                     
    std::string localString = Boat.input;           
    pthread_mutex_unlock(Boat.inputMutex); 

    std::priority_queue<Task> pq;
    std::vector<Task> Ttasks;
    std::stringstream iss(localString);

    // initializing variables
    std::string name;
    std::string output = "";
    int wceTime, period;
    int numTasks = 0;
    double util = 0;
    std::string out;

    // pushing into two queues, one to help me print, one to be priority
    Task tempT;
    while (iss >> tempT.id >> tempT.wcet >> tempT.period)
    {
        pq.push(tempT);
        Ttasks.push_back(tempT);
    }

    // printing CPU #
    int hp = hyperPeriod(Ttasks);

    out += "CPU " + std::to_string(localNum) + "\n";
    out += "Task scheduling information: ";

    // this for-loop gets the utilization number, as well as line one printing
    for (std::vector<Task>::const_iterator it = Ttasks.begin(); it != Ttasks.end(); ++it)
    {
        const Task& task = *it;
        numTasks++;
        util = util + (static_cast<double>(task.wcet) / static_cast<double>(task.period));
        out += task.id + " (WCET: " + std::to_string(task.wcet) + ", Period: " + std::to_string(task.period);
        if (numTasks < Ttasks.size())
        {
            out += "), ";
        }
        else
        {
            out += ") "; 
        }
    }

    // more printing
    std::stringstream utilStream;
    utilStream << std::fixed << std::setprecision(2) << util;
    out += "\nTask set utilization: " + utilStream.str();

    out += "\nHyperperiod: " + std::to_string(hp) + "\n";
    out += "Rate Monotonic Algorithm execution for CPU " + std::to_string(localNum) + ":\n";

    // logic based on utilization and formula given in directions
    if (util > 1)
    {
        out += "The task set is not schedulable\n";
    }
    else if (util > calculateExpression(numTasks))
    {
        out += "Task set schedulability is unknown\n";
    }
    else // find the scheduling diagram
    {
        out += "Scheduling Diagram for CPU " + std::to_string(localNum) + ": ";
        for (int i = 1; i <= hp; i++)
        {
            Task current = pq.top();

            // if there are still tasks to execute...
            if (current.execLeft > 0)
            {
                pq.pop();

                // adding to my output string which will later be formatted
                output += current.id;

                current.execLeft--;
                pq.push(current);

                std::priority_queue<Task> temp;

                // checking if any of the nodes have crossed thier period, in which case...
                while (!pq.empty())
                {
                    Task current = pq.top();
                    pq.pop();

                    if (i % current.period == 0 && i != 1)
                    {

                        current.execLeft += current.wcet; // i need to add executions!
                    }

                    temp.push(current);
                }

                pq = temp;
            }
            else
            {

                output.append("I"); // will be formatted correctly later

                std::priority_queue<Task> temp;

                // checking if any of the nodes have crossed thier period, in which case...
                while (!pq.empty())
                {
                    Task current = pq.top();
                    pq.pop();

                    if ((i % current.period == 0 && i != 1))
                    {
                        current.execLeft += current.wcet; // i need to add executions!
                    }

                    temp.push(current);
                }

                pq = temp;
            }
        }
    }
    out += convertToDiagram(output);
    out += "\n\n";

    pthread_mutex_lock(Boat.printMutex); // second critical section --> check if num = localNum

    while ((*(Boat.count)) != localNum)
    {
        pthread_cond_wait(Boat.condition, Boat.printMutex); // if not set it to wait until it does
    }

    pthread_mutex_unlock(Boat.printMutex); // unlock crit section

    std::cout << out; // print accumulated output

    pthread_mutex_lock(Boat.printMutex); // lock so we can increment the shared resource

    (*Boat.count)++;

    pthread_cond_broadcast(Boat.condition); // wake up other threads since now we are done

    pthread_mutex_unlock(Boat.printMutex); // unlock semaphore

    return NULL;

    /*
    Info arg = *((Info*)void_ptr);           // dereference pointer
    // shared memory
    int CPUn = arg.CPUnum;
    std::string data = arg.input;
    
    // first critical section
    pthread_mutex_unlock(arg.inputMutex);

    // run algorithm
    std::priority_queue<Task> Q;
    std::vector<Task> tasks;
    std::stringstream parseInput;
    std::string output = "";
    std::stringstream utilS;

    // store data
    Task tempTask;
    while (parseInput >> tempTask.id >> tempTask.wcet >> tempTask.period)
    {
        tempTask.execLeft = tempTask.wcet;
        Q.push(tempTask);
        tasks.push_back(tempTask);
    }

    output += "CPU " + std::to_string(CPUn);
    output += "\nTask scheduling information: ";

    // calcualate utilization
    double util;
    util = setUtilization(tasks);
    // calculate
    int hp;
    hp = hyperPeriod(tasks);
    // check is tasks are schedulable
    double setNum;
    setNum = tasks.size() * (pow(2, float(1) / tasks.size()) - 1);

    for (int i = 0; i < tasks.size(); i++)
    {
        if (i < tasks.size() - 1)
        {
            output += std::to_string(tasks.at(i).id) + " (WCET: " + std::to_string(tasks.at(i).wcet) + ", Period: " + std::to_string(tasks.at(i).period) + "), ";
        }
        else
        {
            output += std::to_string(tasks.at(i).id) + " (WCET: " + std::to_string(tasks.at(i).wcet) + ", Period: " + std::to_string(tasks.at(i).period) + ")\n";
            utilS << std::fixed << std::setprecision(2) << util;
            output += "Task set utilzation: " + std::to_string(util) + utilS.str() + "\n";
            output += "Hyperperiod: " + std::to_string(hp) + "\n";
        }
    }


    pthread_mutex_lock(arg.printMutex);
    while ((*(arg.count)) != CPUn)
    {
        pthread_cond_wait(arg.condition, arg.printMutex);
    }

    pthread_mutex_unlock(arg.printMutex);

    std::cout << output;

    pthread_mutex_lock(arg.printMutex);

    (*arg.count)++;

    pthread_cond_broadcast(arg.condition);

    pthread_mutex_unlock(arg.printMutex);
    
    return nullptr;
    
}

int main()
{
    struct Info data;
    std::vector<std::string> si;

    pthread_mutex_t inputMutex;
    pthread_mutex_init(&inputMutex, NULL);

    pthread_mutex_t printMutex;
    pthread_mutex_init(&printMutex, NULL);

    static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    static int count = 1;

    data.inputMutex = &inputMutex;
    data.printMutex = &printMutex;
    data.condition = &condition;
    data.count = &count;

    std::string line = "";
    int nThreads = 0;

    while (getline(std::cin, line))
    {
        if (line == "exit")
        {
            break;
        }
        int found = line.find("1");
        if (found != -1)
        {
            si.push_back(line);
        }
    }
    nThreads = si.size();
    pthread_t tid[nThreads];

    for (int i = 0; i < nThreads; i++)
    {
        pthread_mutex_lock(&inputMutex);
        data.input = si.at(i);
        data.num = i + 1;
        if (pthread_create(&tid[i], nullptr, RMS, &si))
        {
            std::cerr << "Error cerating therad" << std::endl;
            return 1;
        }
    }

    for (int i = 0; i < nThreads; i++)
    {
        pthread_join(tid[i], NULL);
    }

    return 0;
}*/