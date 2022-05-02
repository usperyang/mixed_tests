#include <queue>
#include <deque>
#include <mutex>
#include <iostream>
#include <thread>
#include <string>
#include <fstream>
#include "stdlib.h"
#include "stdio.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <condition_variable>

using namespace std;

#define MAX_FID_NUM 10

typedef struct {
    int timeStamp;
    int sendPid;
    int revPid;
} TRACE;

using QUEUE = deque<int>;

typedef enum {
    LOW,
    NORMAL,
    HIGH
} MsgLevel;

QUEUE Queue[MAX_FID_NUM];
mutex Mutex[MAX_FID_NUM];
condition_variable Cond[MAX_FID_NUM];

std::string GetCurrentTime() {
    std::string defaultTime = "19700101000000000";
    try
    {
        struct timeval curTime;
        gettimeofday(&curTime, NULL);
        int milli = curTime.tv_usec / 1000;
        char buffer[80] = {0};
        struct tm nowTime;
        localtime_r(&curTime.tv_sec, &nowTime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S.", &nowTime);
        char currentTime[84] = {0};
        snprintf(currentTime, sizeof(currentTime), "%s%03d", buffer, milli);
        return currentTime;
    }
    catch (const std::exception& e)
    {
        return defaultTime;
    }
    catch (...)
    {
        return defaultTime;
    }
}

int GetRandomNum(int low, int high)
{
    return (rand() % (high - low)) + low;
}

void QueuePush(unsigned int FID, int msg, MsgLevel level)
{
    if (FID >= MAX_FID_NUM) {
        return;
    }
    unique_lock<mutex> locker(Mutex[FID]);
    if (Queue[FID].size() > 10000) {
        printf("Queue size over 10000, size=<%ld>\n", Queue[FID].size());
    }
    if (level == HIGH) {
        Queue[FID].push_front(msg);
    } else {
        Queue[FID].push_back(msg);
    }
    locker.unlock();
    Cond[FID].notify_one();
}

void QueuePop(unsigned int FID)
{
    unique_lock<mutex> locker(Mutex[FID]);
    while (Queue[FID].empty())
    {
        if (Cond[FID].wait_for(locker, std::chrono::microseconds(10)) == std::cv_status::timeout) {
            return;
        }
    }
    Queue[FID].pop_front();
    locker.unlock();
}

void PushMsg()
{
    while (true)
    {
        QueuePush(GetRandomNum(0, 12), 2, (MsgLevel)GetRandomNum(0, 3));
    }
}

void PopMsg()
{
    while (true)
    {
        QueuePop(GetRandomNum(0, MAX_FID_NUM));
    }   
}

void DoubleThreadTest()
{
    std::thread pushThread(&PushMsg);
    std::thread popThread(&PopMsg);
    pushThread.detach();
    popThread.detach();
}

int main()
{
    DoubleThreadTest();
    while (true)
    {
        sleep(1);
    }
    return 0;
}

// g++ -g -Wall -std=c++11 thread_mutex_test.cpp -lpthread