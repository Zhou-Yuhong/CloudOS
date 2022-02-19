#include <iostream>
#include <thread>
#include <mutex>
#include <thread>
#include <queue>
int n=0;
std::queue<int> q;
std::thread *t1;
std::thread *t2;
std::mutex mtx;
void addToQueue(){
    while(true) {
        mtx.lock();
        n++;
        q.push(n);
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
void removeFromQueue(){
    while(true) {
        mtx.lock();
        int i = q.front();
        q.pop();
        std::cout<<i<<std::endl;
        mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
int main() {
    std::cout<<"hello world"<<std::endl;
    t1 = new std::thread(&addToQueue);
    t2 = new std::thread(&removeFromQueue);
    while (true){

    };
    return 0;
}
