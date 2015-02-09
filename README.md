MPIRPC
======

`MPIRPC` is a C++11 library used to invoke functions and member functions on remote MPI nodes.

## Instalation

    cd path/to/MPIRPC/src
    mkdir build
    cd build
    cmake ..
    make
    sudo make install
    
### Installing on ARCHER ###

If you are using hapbin on the [ARCHER UK National HPC Service](http://www.archer.ac.uk/), follow these steps:

    cd path/to/MPIRPC/build
    . build.archer.sh
    
MPIRPC will be installed into `$HOME/install/`.

## Example

The following is an example of the usage and features of MPIRPC:

```C++
#include <mpirpc/manager.hpp>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>

mpirpc::FunctionHandle doneFunc;
mpirpc::FunctionHandle fooFunc;
mpirpc::Manager *manager;

struct MyType {
    int a;
    int b;
}

ParameterStream& operator<<(ParameterStream& out, const MyType& t) {
    out << t.a << t.b;
    return out;
}

ParameterStream& operator>>(ParameterStream& in, MyType& t) {
    in >> t.a >> t.b;
    return in;
}

void processStep() {
    //Process range
    MyType res;
    //...
    int left = (manager->rank() == 0)? manager->numProcs() - 1 : manager->rank() - 1;
    int right = (manager->rank() == manager->numProcs() - 1)? 0 : manager->rank() + 1;
    manager->invokeFunction(left, dataLeft, true);
    manager->invokeFunction(right, dataRight, false);
    mpirpc::ObjectWrapperBase *vecLeft = manager->getObjectOfType<std::vector<int>>(left);
    mpirpc::ObjectWrapperBase *vecRight = manager->getObjectOfType<std::vector<int>>(right);
    manager->invokeFunction(vecLeft, &std::vector::push_back, res);
    manaber->invokeFunction(vecRight, &std::vector::push_back, res);
    manager->invokeFunction(fooFunc);
}

void updateFromNeighbor(const std::vector<int>& halo, bool left) {
    //Update local data using `halo` data from neighbor
}

bool foo(int rank) { ... }

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    std::chrono::milliseconds sleepTime(50);
    manager = new mpirpc::Manager();
    int prosToGo = manager->numProcs();
    manager->registerFunction(updateNeighbor);
    fooFunc = manager->registerFunction(std::bind(foo, manager->rank()));
    doneFunc = manager->registerLambda([&]() {
        --procsToGo;
    });
    std::vector<MyType> vec;
    manager->registerType<std::vector<int>>()
    manager->registerObject(&vec);
    manager->registerFunction(&std::vector::push_back);
    /**
        * MPI_Issend() in Manager::registerObject does not necessarily notify other processes 
        * that a send is ready before the barrier. Therefore, we must loop until the sends and 
        * recieves are complete.
        */
    while (manager->getObjectsOfType<std::vector<int>>().size() < manager->numProcs() 
            || manager->queueSize() > 0)
    {
        manager->checkMessages();
        std::this_thread::sleep_for(sleepTime);
    }
    manager.barrier();
    for (int i = 0; i < 100; ++i) {
        processStep();
        manager->checkMessages();
    }
    manager->invokeFunction(0, done);
    while(manager->checkMessages())
    {
        if (procsToGo == 0)
            manager->shutdown();
        std::this_thread::sleep_for(sleepTime);
    }
    MPI_Finalize();
}
```