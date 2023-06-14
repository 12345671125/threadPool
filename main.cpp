#include <cstdio>
#include<iostream>
#include<Windows.h>
#include"threadPool.hpp"
void func(void* arg) {         //ָ������ŵ���Ϣ���˱����¼���׵�ַ��,������������,������ͨ��ָ������ͺ�ָ���ŵ��׵�ַ�ڴ��ڴ��ж�ȡ��Ӧ������
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