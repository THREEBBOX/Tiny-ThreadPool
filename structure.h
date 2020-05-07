#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>

using namespace std;
#ifndef THREADPOOL_STRUCTURE_H
#define THREADPOOL_STRUCTURE_H

class worker;

class job;

class manager;

class job {

public:

    job() = delete;//禁止赋值为空

    job(void *(*func_pointer)(void *), void *data_pointer) : func(func_pointer),
                                                             data(data_pointer) { result = nullptr; };

    void job_call_back() noexcept;

    ~job() { cout << "job is done" << endl; };
private:
    void *(*func)(void *);//函数指针
    void *data;//函数数据
    void *result;//函数结果
};

class manager {
public:

    manager() = delete;

    explicit manager(int a) noexcept;

    int add_jobs(job *j);

    bool job_empty() const;

    ~manager();

    queue<job *> jobs;
private:
    mutex mtx;
    condition_variable cond;
    vector<worker *> workers;
    const int thread_num;
};

class worker {

public:
    worker() = delete;

    worker(mutex &m, condition_variable &cond, manager *Manager);

    void working(mutex &m, condition_variable &cond);

    void stopWorking();

    ~worker() {
        if (thrd->joinable())
            thrd->join();
        cout << "worker is gone" << endl;
    };

private:
    bool isWorking;
    manager *myManager;
    thread *thrd;
};

worker::worker(mutex &m, condition_variable &cond, manager *Manager) : myManager(Manager), isWorking(true) {
    thrd = new thread(&worker::working, ref(*this), ref(m), ref(cond));
    thrd->detach();
    cout << "thread is working " << endl;
}

void worker::working(mutex &m, condition_variable &cond) {
    while (isWorking) {

        unique_lock<mutex> gurad(m);
        cond.wait(gurad, [this] {
            return !myManager->job_empty();
        });
        if (!isWorking) {
            gurad.unlock();
            break;
        }
        job tmp = *(myManager->jobs.front());
        myManager->jobs.pop();
        gurad.unlock();
        tmp.job_call_back();
    }

}

void worker::stopWorking() {
    isWorking = false;
}


manager::manager(int a) noexcept: thread_num(a) {
    try {
        for (; a > 0; a--) {
            worker *tmp = new worker(mtx, cond, this);
            workers.push_back(tmp);
        }
    } catch (exception &E) {
        cout << E.what() << endl;
        exit(1);
    }
}

int manager::add_jobs(job *j) {
    mtx.lock();
    try {
        jobs.push(j);
    } catch (exception &E) {
        cout << E.what() << endl;
    }
    mtx.unlock();
    cond.notify_one();
}

bool manager::job_empty() const {
    return jobs.empty();
}

manager::~manager() {
    for(auto w:workers){
        w->stopWorking();
    }
    cond.notify_all();
    for(auto w:workers)
        delete w;
}


void job::job_call_back() noexcept {
//    cout << "job call back" << endl;
    try {
        func(data);
    } catch (exception &E) {
        cout << E.what() << endl;
        cout << "something wrong" << endl;
    }
}

#endif //THREADPOOL_STRUCTURE_H