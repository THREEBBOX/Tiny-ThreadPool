/*
 * Created by wang xy
 * Date: 2020/5/6
 * description:
 */


#include<iostream>
#include<vector>
#include<string>

using namespace std;

void* function(int  a){
    cout<<a<<endl;
    return reinterpret_cast<void*>(&a);
}
int main(){
    void *(*funcP)(int);
    funcP=function;
    int a=10;
    try {
        void *p=funcP(a);
        cout<<*reinterpret_cast<int*>(p)<<endl;
    } catch (exception &E) {
        printf("somthing wrong");
    };

    return 0;
}
