#ifndef COUNTER_H
#define COUNTER_H

#include <condition_variable>
#include <thread>

/// <summary>
/// Класс для контроля завершения всех потоков
/// </summary>
class counter
{
public:
	static int ThreadsAlreadyCompletedCount;
	static int ThreadsNeedToCompleteCount;
	static bool isAllThreadsCompleteRead();
	static void Reset();
};

#endif