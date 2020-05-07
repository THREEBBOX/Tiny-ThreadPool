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
//