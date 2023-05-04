#include <cstdio>
#include<iostream>
#include<Windows.h>
#include"threadPool.hpp"
void func(void* arg) {
    int* num = (int*)arg;
    std::cout << "id：" << std::this_thread::get_id() << *num << std::endl;
    Sleep(1000);
}
int main()
{
    ThreadPool threadPool(5, 10);
    for (int i = 0; i < 2; i++) {
        int* num = new int(10);
        *num += i;
        threadPool.addTask(func, num);
    }
    Sleep(30000);
    threadPool.~ThreadPool();
    return 0;
}