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

    explicit manager(int a);

    int add_jobs(job *j);

    bool job_empty() const;

    ~manager() = default;

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

    ~worker() = default;

private:
    bool isWorking;
    manager *myManager;
    thread *thrd;
};

worker::worker(mutex &m, condition_variable &cond, manager *Manager) : myManager(Manager), isWorking(true) {
    thrd = new thread(&worker::working, *this, ref(m), ref(cond));
    thrd->detach();
}

void worker::working(mutex &m, condition_variable &cond) {
    cout << "working "  <<this_thread::get_id()<< endl;//this_thread::get_id()
    while (isWorking) {

        unique_lock<mutex> gurad(m);
        cond.wait(gurad, [this] {
            return !myManager->job_empty();
        });
        if(!isWorking){
            gurad.unlock();
            break;
        }
        job tmp = *(myManager->jobs.front());
        myManager->jobs.pop();
        cout<<myManager->jobs.size()<<endl;
        gurad.unlock();
        tmp.job_call_back();
    }

}


manager::manager(int a) : thread_num(a) {
    try {
        for (; a > 0; a--) {
            worker *p = new worker(mtx, cond, this);
            workers.push_back(p);
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


void job::job_call_back() noexcept {
    cout<<"job call back"<<endl;
    try {
        func(data);
    } catch (exception &E) {
        cout << E.what() << endl;
        cout << "something wrong" << endl;
    }
}

#endif //THREADPOOL_STRUCTURE_H