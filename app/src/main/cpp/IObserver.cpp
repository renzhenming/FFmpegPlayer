#include "IObserver.h"

//主体函数 添加观察者
void IObserver::AddObserver(IObserver *obs) {
    if (!obs) return;
    mutex.lock();
    observers.push_back(obs);
    mutex.unlock();
}

//通知所有观察者
void IObserver::Notify(XData data) {
    mutex.lock();
    int size = observers.size();
    for(int i = 0 ;i<size;i++){
        observers[i]->Update(data);
    }
    mutex.unlock();
}