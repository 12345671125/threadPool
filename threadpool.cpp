#include"threadPool.hpp"
#include<iostream>
#include<cstring>
#include <Windows.h>//window系统调用头文件
const int NUMBER = 2;
#define CAPACITY 100;
Task::Task(callback func, void* arg) {
	this->function = func;
	this->arg = arg;
}
ThreadPool::ThreadPool(int min, int max) {    //创建线程池
	try
	{
		do {
			this->capacity = CAPACITY;
			this->busyMutex = std::unique_lock<mutex>(this->busyMutex_);//让unique_lock对象来管理原始busyMutex锁,当unique_lock对象实例化时会默认给其管理的锁上锁
			this->poolMutex = std::unique_lock<mutex>(this->poolMutex_);//让unique_lock对象来管理原始poolMutex锁，当unique_lock对象实例化时会默认给其管理的锁上锁
			this->poolMutex.unlock(); //当unique_lock对象实例化时会默认给其管理的锁上锁，所以要先解锁
			this->busyMutex.unlock();//当unique_lock对象实例化时会默认给其管理的锁上锁，所以要先解锁
			this->minNum = min;//初始化线程最小线程数
			this->maxNum = max;//初始化最大线程数
			this->busyNum = 0;//初始化忙的线程数为0
			this->liveNum = min;//初始化存活的线程数为0
			this->exitNum = 0;//初始化需要销毁的线程数为0
			this->shutdown = false;//初始化是否需要摧毁线程池为false
			this->managerT = thread(this->manager, this); //创建管理者线程
			//创建工作者线程
			for (int i = 0; i < this->minNum; i++) {
				//this->workThreads[i] = thread(this->worker, this);
				this->workThreads.push_back(thread(this->worker, this));
			}
		} while (0);

	}
	catch (const std::exception& error)
	{
		std::cout << error.what() << std::endl;
	}

}
void ThreadPool::worker(void* arg) {
	std::cout << std::this_thread::get_id() << std::endl;
	ThreadPool* pool = (ThreadPool*)arg; //工作线程需要读取线程池里的任务，需要传入线程池的指针来读取
	while (1)//每个工作不停的从工作队列中读取任务来执行，这样工作队列就是一个共享资源，需要加锁
	{
		while (pool->poolMutex.try_lock() == false); //尝试上锁，如果争夺不到锁，就一直忙等
		while (pool->taskQ.empty() && !pool->shutdown) {  //首先判断任务队列是否为空
			pool->cond_Var.wait(pool->poolMutex);//如果任务队列为空，阻塞当前线程
			if (pool->exitNum > 0) {
				pool->exitNum--;
				pool->poolMutex.unlock();
				return; //让工作线程自杀
			}
		}
		if (pool->shutdown) { //如果线程池被关闭
			pool->poolMutex.unlock();//打开线程锁，防止死锁
			return;//提前结束工作函数，退出线程
		}
		//如果任务队列不为空且线程池未被关闭，从任务队列中取出一个任务
		Task task = pool->taskQ.front();//从线程池中取出一个任务
		pool->taskQ.pop();//将线程池中第一个任务弹出
		pool->cond_Var.notify_all();//唤醒其他线程
		pool->poolMutex.unlock();
		while (pool->busyMutex.try_lock() == false); //因为要更改线程池中的共享资源，所以要加锁
		std::cout << std::this_thread::get_id() << "：working...." << std::endl;
		task.function(task.arg);//执行任务函数
		pool->busyNum++;
		pool->busyMutex.unlock();
		while (pool->busyMutex.try_lock() == false); //因为要更改线程池中的共享资源，所以要加锁
		std::cout << std::this_thread::get_id() << "：ending...." << std::endl;
		pool->busyNum--;
		pool->busyMutex.unlock();
	}
}
void ThreadPool::manager(void* arg) {
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown) {
		Sleep(5000);//将进程挂起5秒
		//读取线程池中的任务数量和当前线程的数量
		while (pool->poolMutex.try_lock() == false);//尝试获取锁权，如果获取不到就忙等
		int queueSize = pool->taskQ.size();
		int liveNum = pool->liveNum;
		pool->poolMutex.unlock();

		//取出忙的线程数量
		while (pool->busyMutex.try_lock() == false);//尝试获取锁权，如果获取不到就忙等
		int busyNum = pool->busyNum;
		pool->busyMutex.unlock();
		//添加线程
		//当当前任务个数>存活的线程个数 && 存活的线程数<最大的线程数
		if (queueSize > liveNum && liveNum < pool->maxNum) {
			int counter = 0;
			while (pool->poolMutex.try_lock() == false);//尝试获取锁权，如果获取不到就忙等
			for (int i = 0; i < pool->maxNum && counter < NUMBER && pool->liveNum < pool->maxNum; i++) {
				pool->workThreads.push_back(thread(pool->worker, pool));//向线程组中添加线程
				counter++;
				pool->liveNum++;

			}
			pool->poolMutex.unlock();
		}
		//销毁线程
		//忙的线程*2 <存活的线程数&&存活的线程数>最小线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum) {
			while (pool->poolMutex.try_lock() == false);//尝试上锁，如果争夺不到锁，就一直忙等
			pool->exitNum = NUMBER;
			pool->poolMutex.unlock();
			//让阻塞的工作线程自杀
			for (int i = 0; i < NUMBER; i++) {
				pool->cond_Var.notify_all();//唤醒阻塞的线程
			}

		}
	}
}
void ThreadPool::addTask(callback func, void* arg) {
	while (this->poolMutex.try_lock() == false);//尝试上锁，如果争夺不到锁，就一直忙等
	while (this->taskQ.size() == this->capacity && !this->shutdown) {
		this->cond_Var.wait(this->poolMutex);//阻塞生产者线程
	}
	if (this->shutdown) {
		this->poolMutex.unlock();
		return;
	}
	//添加任务
	Task task(func, arg);
	this->taskQ.push(task);
	this->cond_Var.notify_all();
	this->poolMutex.unlock();
}
void ThreadPool::addTask(Task task) {
	while (this->poolMutex.try_lock() == false);
	while (this->taskQ.size() == this->capacity && !this->shutdown) {
		this->cond_Var.wait(this->poolMutex);//阻塞生产者线程
	}
	if (this->shutdown) {
		this->poolMutex.unlock();
		return;
	}
	//添加任务
	this->taskQ.push(task);
	this->cond_Var.notify_all();
	this->poolMutex.unlock();
}
int ThreadPool::getBusyNum()
{
	return this->busyNum;
}
int ThreadPool::getLiveNum()
{
	return this->liveNum;
}
ThreadPool::~ThreadPool() {
	this->shutdown = true;
	this->managerT.join();
	this->cond_Var.notify_all();
}