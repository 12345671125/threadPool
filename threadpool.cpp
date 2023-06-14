#if 1
#include "threadpool.hpp"

using namespace std;

Task::Task(callback func, void* arg) {   //传入的参数为 一个泛型的函数地址和一个void指针
	this->function = func;
	this->arg = arg;
}

void Task::run() {
	this->function(this->arg);
}

ThreadPool::ThreadPool(int min, int max) : capacity(100) {
	this->minNum = min;
	this->maxNum = max;
	this->busyNum = 0;
	this->liveNum = min;
	this->exitNum = 0;
	this->shutdown = false;

	for (int i = 0; i < min; i++) {
		this->workThreads.push_back(thread(&ThreadPool::worker, this));//实例化工作线程,传入worker函数,和线程池指针
	}

	thread managerT(&ThreadPool::manager, this);
	managerT.detach();
}

void ThreadPool::worker() {  //工作函数
	while (true) {
		unique_lock<mutex> lock(poolMutex_); //使用unique_lock对象来管理线程池poolMutex锁,当向unique_lock对象传入一个锁对象时会自动上锁,每一个工作线程都会执行这个工作函数,创建一个unique_lock对象,争夺poolmutex的锁权
		while (taskQ.empty() && !shutdown) {  //当任务数组为空并且线程池未关闭
			cond_Var.wait(lock); //阻塞当前线程 等待唤醒
		}

		if (shutdown) {    //如果线程池关闭
			lock.unlock();//解锁
			return;
		}
		//如果任务队列不为空且线程池未被关闭，从任务队列中取出一个任务
		Task task = taskQ.front(); 
		taskQ.pop();
		lock.unlock();

		unique_lock<mutex> busyLock(busyMutex_);//使用unique_lock对象来管理线程池busyMutex锁,当向unique_lock对象传入一个锁对象时会自动上锁,每一个工作线程都会执行这个工作函数,创建一个unique_lock对象,争夺busymutex的锁权
		busyNum++;
		busyLock.unlock();

		task.run();//执行任务对象的run()函数,执行任务

		busyLock.lock();//当任务执行完毕,回到这一行,当unique_lock无法获取锁时，其会阻塞，直到其他对象释放锁，才会继续执行。
		busyNum--;
		busyLock.unlock();
	}
}

void ThreadPool::manager() {
	while (!shutdown) { //当线程池未关闭
		this_thread::sleep_for(chrono::seconds(5)); //管理者线程睡5秒

		unique_lock<mutex> lock(poolMutex_);//获取线程池锁权
		int queueSize = taskQ.size();//获取当前任务队列中的任务数量
		int liveNum = this->liveNum;//获取当前线程池中存活的工作线程数量
		lock.unlock();

		unique_lock<mutex> busyLock(busyMutex_);//获取繁忙锁锁权
		int busyNum = this->busyNum;//获取当前繁忙的线程数量
		busyLock.unlock();

		if (queueSize > liveNum && liveNum < maxNum) { //如果当前任务数量大于存活的线程数且存活的线程数小于最大的线程数
			lock.lock();
			int counter = 0;
			for (int i = 0; i < maxNum && counter < 2 && liveNum < maxNum; i++) { //
				if (workThreads[i].joinable() == false) {   //joinable 用来查看该线程是否为可执行线程,如果线程队列中有不可执行线程
					workThreads[i] = thread(&ThreadPool::worker, this); //实例化一个线程来替换不可执行线程
					counter++;
					liveNum++;
				}
			}
			lock.unlock();
		}

		if (busyNum * 2 < liveNum && liveNum > minNum) { //如果繁忙的线程数<存活的线程数的两倍并且存活的线程数大于最小线程数
			lock.lock();
			exitNum = 2;//设置需要摧毁的线程数为2
			lock.unlock();

			cond_Var.notify_all();//唤醒所有正在挂起的线程

			this_thread::sleep_for(chrono::seconds(2));

			lock.lock();
			liveNum = this->liveNum;//获取存活的线程数
			lock.unlock();
			/*以下用来摧毁线程*/
			if (liveNum > minNum) {  //如果存活的线程数大于最小线程数
				lock.lock();
				int counter = 0;
				for (int i = 0; i < workThreads.size() && counter < exitNum; i++) {
					if (workThreads[i].joinable() == true) { //如果当前线程为可执行线程
						workThreads[i].join();//阻塞其他线程等待该线程执行结束?
						counter++;
						liveNum--;
					}
				}
				exitNum -= counter;
				lock.unlock();
			}
		}
	}
}

void ThreadPool::addTask(callback func, void* arg) {
	Task task(func, arg);
	addTask(task);
}
void ThreadPool::addTask(Task task) {
	unique_lock<mutex> lock(poolMutex_);
	while (taskQ.size() == capacity && !shutdown) { cond_Var.wait(lock); }

	if (shutdown) {
		lock.unlock();
		return;
	}

	taskQ.push(task);
	cond_Var.notify_all();
	lock.unlock();
}

int ThreadPool::getBusyNum() { 
	unique_lock<mutex> lock(busyMutex_); 
	int busyNum = this->busyNum;
	lock.unlock(); 
	return busyNum;
}

int ThreadPool::getLiveNum() { 
	unique_lock<mutex> lock(poolMutex_); 
	int liveNum = this->liveNum; 
	lock.unlock(); 
	return liveNum; 
}

ThreadPool::~ThreadPool() {
	shutdown = true;
	cond_Var.notify_all();

	for (int i = 0; i < workThreads.size(); i++) {
		if (workThreads[i].joinable()) {
			workThreads[i].join();
		}
	}
}
#endif
