#include <cstdio>
#include<iostream>
#include<Windows.h>
#include"threadPool.hpp"
void func(void* arg) {         //指针所存放的信息除了本身记录的首地址外,还包括其类型,编译器通过指针的类型和指针存放的首地址在从内存中读取对应的数据
    int i = *(int*)arg;
    std::cout << "id:"<< std::this_thread::get_id() <<":"<< i << std::endl;
}
int main()
{
    ThreadPool threadPool(5, 10);
    for (int i = 0; i < 20; i++) {
        int* num = new int(10);
        *num += i;
        threadPool.addTask(func, num);
    }
    Sleep(30000);
    threadPool.~ThreadPool();
    return 0;
}