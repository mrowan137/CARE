#include "care/care.h"
#include <unistd.h>


#define NODE_LOOP_MANAGED_PTR(INDEX, BEGINNODE, ENDNODE) CARE_CHECKED_MANAGED_PTR_LOOP_START(INDEX, BEGINNODE, ENDNODE, node_loop_managed_ptr_check)
#define NODE_LOOP_MANAGED_PTR_END CARE_CHECKED_MANAGED_PTR_LOOP_END(node_loop_managed_ptr_check)
#define real8 double
#define real8_ptr host_device_ptr<real8>


class A
{
public:
    __host__ __device__ A ();
    chai::managed_ptr<A> make_managed ();
    __host__ __device__ virtual ~A ();
};

__host__ __device__ A::A () {}

chai::managed_ptr<A>  A::make_managed () {
    auto toReturn = chai::make_managed<A> ();
    hipDeviceSynchronize();
    return toReturn;
}

__host__ __device__ A::~A () {}

template<typename T>
chai::managed_ptr<T> make_managed_safe (T * obj) {
    auto new_managed = obj != nullptr ? obj->make_managed() : nullptr;
    return chai::dynamic_pointer_cast<T>(new_managed);
}

class B
{
public:
    __host__ __device__ B ();
    
    virtual chai::managed_ptr<B> make_managed () = 0;
    __host__ __device__ virtual ~B ();
    __host__ __device__ virtual void f () = 0;
    
    int _; // commenting this gets around hang..
};

__host__ __device__ B::B () {}
__host__ __device__ B::~B () {}

class Derived1 : public B
{
public:
    Derived1 ();
    __host__ __device__ Derived1 (A * base);
    chai::managed_ptr<B> make_managed ();
    __host__ __device__ virtual ~Derived1 ();
    __host__ __device__ void f ();
    
protected:
    A* base = nullptr;
    
};

__host__ __device__ void Derived1::f () {
    return;
}

Derived1::Derived1 ()
{
    base = nullptr;
}

__host__ __device__ Derived1::Derived1 (A * base_)
    : B(), base(base_) {}

chai::managed_ptr<B> Derived1::make_managed () {
    chai::managed_ptr<A> my_base = make_managed_safe(base) ;
    hipDeviceSynchronize();
    
    auto toReturn = chai::make_managed<Derived1>(chai::unpack(my_base));
    hipDeviceSynchronize();
    
    my_base.free();
    hipDeviceSynchronize();
    
    return toReturn;
}

__host__ __device__ Derived1::~Derived1 ()
{
}

class Derived2 : public B
{
public:
    __host__ __device__ Derived2 ();
    
    chai::managed_ptr<B> make_managed ();
    __host__ __device__ virtual ~Derived2 ();
    __host__ __device__ void f ();
};


__host__ __device__ Derived2::Derived2 ()
    : B() {}

chai::managed_ptr<B> Derived2::make_managed () {
    auto toReturn = chai::make_managed<Derived2> ();
    hipDeviceSynchronize();
    return toReturn;
}

__host__ __device__ Derived2::~Derived2 () {}
__host__ __device__ void Derived2::f () { return; }


int main ()
{

    B* x = new Derived2();
    chai::managed_ptr<B> x_managed_ptr = make_managed_safe(x);
    hipDeviceSynchronize();
    
    NODE_LOOP_MANAGED_PTR(i, 0, 1) {
        (*x_managed_ptr).f();
    } NODE_LOOP_MANAGED_PTR_END
          
    sleep(1.0); // removing sleep would get around the hang
    hipDeviceSynchronize();
    
	x_managed_ptr.free();
    hipDeviceSynchronize();
    delete x;
    
    B* y   = new Derived1();
    chai::managed_ptr<B> y_managed_ptr = make_managed_safe(y);
    hipDeviceSynchronize();
    
	y_managed_ptr.free();
    hipDeviceSynchronize();
    delete y;
    
    return 0;
}
