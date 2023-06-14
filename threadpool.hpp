#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<queue>
#include<thread>
#include<vector>
#include<mutex>
#include<condition_variable>
using std::queue;
using std::thread;
using std::vector;
using std::mutex;
using std::condition_variable;
using callback = void(*)(void* arg);//一个泛型的函数指针类型
#endif 

class Task {
public:
	Task(callback func, void* arg);
	void run(); //重新封装了线程任务函数,用来调用任务函数
	
private:
	callback function;//工作函数
	void* arg;//工作函数的参数
};
class ThreadPool {
private:
	//任务队列
	queue<Task> taskQ;
	int capacity;//容量
	thread managerT;//管理者线程->生产者线程
	vector<thread> workThreads;//工作线程->消费者线程
	int minNum;//最小线程数
	int maxNum;//最大线程数
	int busyNum;//满的线程数
	int liveNum;//存活的线程数
	int exitNum;//要退出的线程数
	mutex poolMutex_; //原始poolMutex锁
	mutex busyMutex_;//原始busyMutex锁
	//std::unique_lock<mutex> poolMutex;//线程池锁
	//std::unique_lock<mutex> busyMutex;//busyNum锁
	bool shutdown; //是不是要销毁线程池，销毁为true，不销毁为false
	condition_variable cond_Var;//条件变量，用来传递信号,或者阻塞线程
	void manager();// 管理者线程所执行的函数，用来查看，管理线程
	void worker();//工作者线程所执行的函数
public:
	ThreadPool(int minNum, int maxNum);//线程池初始化
	~ThreadPool();//线程池销毁
	void addTask(callback func, void* arg);//向线程池中添加任务
	void addTask(Task task);//向线程池中添加任务
	int getBusyNum();//获取满的线程数
	int getLiveNum();//获取活的线程数

};