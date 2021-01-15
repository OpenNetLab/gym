#pragma once
#define atomic_cas(dst, old, new) __sync_bool_compare_and_swap((dst), (old), (new))
#define atomic_lock(ptr)\
while(!atomic_cas(ptr,0,1))
#define atomic_unlock(ptr)\
while(!atomic_cas(ptr,1,0))

namespace ns3{
class Lock{
public:
    virtual void Enter()=0;
    virtual void Leave()=0;
    virtual ~Lock(){}
};

class AtomicLock:public Lock{
public:
    void Enter() override{
        atomic_lock(&lock_);
    }
    void Leave() override{
        atomic_unlock(&lock_);
    }
private:
    int lock_{0};
};

class LockScope{
public:
    explicit LockScope(Lock *lock):lock_(lock){
        lock_->Enter();
    }
    ~LockScope(){
        lock_->Leave();
    }
private:
    Lock *lock_;
};
}