## 1. 线程池的优势
线程过多会带来调度开销，进而影响缓存局部性和整体性能。而线程池维护着多个线程，等待着监督管理者分配可并发执行的任务。因此线程池有以下优势：
1. 重用已存在的线程、降低系统资源消耗、降低线程创建和销毁的开销；
2. 提升响应速度，当有任务到达时，通过复用已存在的线程，无需等待新线程；
3. 方便线程并发数的管控。在高并发场合防止线程过多；
4. 提供更强大的功能，延时定时线程池。
## 2. 线程池的结构分析
### 2.1 线程池的工作流程
很多人会下意识的将线程池与内存池作比较，但是两者对资源的分配方式存在着很大的不同。对于内存池，内存是被动者，当有任务需要内存的时候会从内存池中获取，使用之后再放回内存池中；而线程池中，**线程池中的线程是主动者，他们不断的从队列中获取任务并不断的运行**。这就好比银行大厅，顾客是任务，线程是柜员；当柜员上岗之后如果有顾客，便不断的处理顾客，如果没有变暂时进入空闲状态。**相较于顾客柜员是主动者，他们选择顾客，而不是顾客选择柜员**。对于单个线程，其流程如下：
```mermaid
flowchat
st=>start: 线程被创建
cond=>condition: 有任务
op=>operation: 执行任务
op_wait=>operation: 等待

st->cond
cond(yes)->op
cond(no)->op_wait
op->cond
op_wait->op
```
相较于线程，线程池的工作便十分简单了，其大体流程入下：
```mermaid
flowchat
st=>start: 创建线程池
op_start=>operation: 根据线程数启动线程
op_add=>operation: 添加线程
e=>end: 关闭销毁所有线程

st->op_start->op_add
op_add->e
```
### 2.2线程池的组成（类）
根据上述流程分析，我们可以认识到，线程池程序中需要由至少三个类：
1. 用来管理单个线程的worker工人类；
2. 用来管理整个调度的manager管理者类；
3. 用来传递任务的job任务类；  

其中job类用传递任务，我们可以通过函数指针进行实现，形参使用void指针传入，而结果使用void指针传出。但考虑到方便，我们暂时不考虑返回值。job类的代码如下：
```c
class job {

public:

    job() = delete;//禁止赋值为空

    job(void *(*func_pointer)(void *), void *data_pointer) : func(func_pointer),
                                                             data(data_pointer) { result = nullptr; };

    void job_call_back() noexcept;//回到回调函数执行job中的函数

    ~job() { cout << "job is done" << endl; };
    
private:
    void *(*func)(void *);//函数指针
    void *data;//函数数据
    void *result;//函数结果
};
```
worker类负责单个线程的管理，他主要负责单个线程的启动、销毁、和同步，是最核心的类。
```c
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
    bool isWorking;//是否工作，避免已经取走的任务因为析构而未被执行
    manager *myManager;//指向自己的经理
    thread *thrd;//指向实际控制的线程
};
```
**worker**需要了解自己的manager（也就是管理线程），这样方便其从任务队列中获得任务；同时工作状态isWorking可以帮助我们关闭线程。这部分将在后面具体细说。
**manager**负责总体的协调，负责提供condition_variable，mutex等在内的互斥量，同时负责job队列、worker队列的管理，
```c
class manager {
public:

    manager() = delete;//禁止空值初始化

    explicit manager(int a) noexcept;//初始化

    int add_jobs(job *j);//添加任务

    bool job_empty() const;

    ~manager();

    queue<job *> jobs;
private:
    mutex mtx;
    condition_variable cond;
    vector<worker *> workers;
    const int thread_num;
};
```
## 3.C++实现线程池
### 3.1 并发处理
作为多线程程序，最重要的便是需要考虑并发处理问题。程序的并发操作主要集中在两个方面
1. 从任务队列中获得任务；
2. 任务队列添加和处理所产生的的**读者-写者问题**
相关代码如下：
```c
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
/*
 * 添加任务
 */
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
```
我们使用condition_variable来控制队列的PV操作，保证任务队列不会被重复执行、保证队列为空时个线程阻塞。当任务添加时，我们每次唤醒一个进程来处理问题
### 3.2 异常处理
整个线程池中，最不可控的环节便是用户的函数，一旦因为用户的操作失误，造成函数发生错误。我们不能指望用户保证他的程序不会出错，因此，我们需要保证job类中的**job_call_back**做到noexcept。
```c
void job::job_call_back() noexcept {
//    cout << "job call back" << endl;
    try {
        func(data);
    } catch (exception &E) {
        cout << E.what() << endl;
        cout << "something wrong" << endl;
    }
}
```
其次，我们不希望因为空间或是其他原因造成线程的创建失败。如果程序主体便是线程池，在创建失败时，应该留有一定的空间保存相关的数据，因此manager的构造函数应该使用try-catch进行异常处理
```c
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
```
### 3.3 创建和销毁
我们关心的主要是4个方面
1. manager的创建
2. manager的销毁
3. worker的创建
4. worker的销毁
#### manager的创建
比较简单，依照数量启动线程，并将worker放入vector容器中保存。
#### worker的创建
```c
worker::worker(mutex &m, condition_variable &cond, manager *Manager) : myManager(Manager), isWorking(true) {
    thrd = new thread(&worker::working, ref(*this), ref(m), ref(cond));
    thrd->detach();
    cout << "thread is working " << endl;
}
```
worker的重点在于，我们需要保证传入的是引用，即使是*this我们也需要添加ref，保证线程中使用的是worker的引用而不是复制。一旦线程中使用的是复制，就会造成manager无法有效的管理thread。其次，thread需要使用detach模式运行，否则manager的创建会阻塞在第一个线程（job队列为空）。
#### worker的销毁
```c
    ~worker() {
        if (thrd->joinable())
            thrd->join();
        cout << "worker is gone" << endl;
    };
```
如果有的worker已经获得了任务，那么便需要保证任务完成，否则任务既从队列中移除，又没有执行便会造成较大的损失。
#### manager的销毁
```c
manager::~manager() {
    for(auto w:workers){
        w->stopWorking();
    }
    cond.notify_all();
    for(auto w:workers)
        delete w;
}
```
我们首先要让所有workers停止工作，将isWorking设为false。对于没有任务的worker，notify_all让其跳出循环，对于有任务的队列，他们无法获取新的任务，但可以通过join让已经移除的任务执行完毕。
## 4. 测试
主函数如下
```c
#include <iostream>
#include "structure.h"
using namespace std;
void* function(void *data){
    int num= *reinterpret_cast<int*>(data);
    cout<<"NUM "<<num<<"  "<<endl;
//    cout<<this_thread::get_id()<<endl;
    chrono::seconds dura(1);
    this_thread::sleep_for(dura);
    return data;
}

int main() {
    manager A(3);
    int tmp[50];
    for(int i=0;i<50;i++){
        tmp[i]=i;
        void *data= reinterpret_cast<void *>(&tmp[i]);
        job *j=new job(function,data);
        A.add_jobs(j);
    }
    chrono::seconds dura(10);
    this_thread::sleep_for(dura);
    return 0;
}
```
结果如下
```
thread is working
 thread is working
//thread is working
//        NUM 0
//NUM NUM 12
//job is done
//        NUM 3
//job is done
//        job is done
//NUM 4NUM   5
//job is done
//        NUM 6
//job is done
//        job is done
//NUM 7NUM   8
//job is done
//        NUM 9
//job is done
//        NUM 10
//job is done
//        NUM 11
//job is done
//        NUM 12
//job is donejob is done
//        NUM NUM 1314
//job is done
//        job is donejob is done
//NUM 15NUM NUM   1617
//job is donejob is donejob is done
//        NUM NUM NUM 181920
//job is donejob is donejob is done
//        NUM NUM NUM 212223
//job is donejob is done
//        job is done
//NUM NUM 2425NUM     26
//job is donejob is done
//        job is done
//NUM NUM 2728NUM     29
//worker is gone
//        worker is gone
//worker is gone
```
我们可以看到函数能够正常打印，并且能够提前停止。如果去掉函数中的注释就可以发现，函数使用的thread_id只有3个。

## 代码连接
[csdn实现小型线程池](https://blog.csdn.net/weixin_43594564/article/details/105970154)

[完整代码连接(求star)](https://github.com/THREEBBOX/Tiny-ThreadPool)
## 参考视频
[手撕线程池](https://www.bilibili.com/video/BV1Ez411b7cc)
[C++多线程](https://www.bilibili.com/video/BV1Yb411L7ak?p=15)