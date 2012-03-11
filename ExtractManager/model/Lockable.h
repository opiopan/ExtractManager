#pragma once
#include <new>
#include <pthread.h>

namespace HSL{ namespace Base {

//----------------------------------------------------------------------
// Lockable: 同期機能実装クラス
//----------------------------------------------------------------------
class Lockable{
protected:
    pthread_mutex_t mutex;
public:
    Lockable(){
        if (pthread_mutex_init(&mutex, NULL)){
            std::bad_alloc e;
            throw e;
        }
    };
    virtual ~Lockable();
    virtual void _lockObject();
    virtual void _unlockObject();
    
    pthread_mutex_t* getMutex(){return &mutex;}; 
};

//----------------------------------------------------------------------
// LockableWithCV: condition variable付き同期機能
//----------------------------------------------------------------------
class LockableWithCV : public Lockable{
private:
	bool			autoSignal;
	pthread_cond_t	cv;
public:
	LockableWithCV(bool autoSignal = true) 
		: autoSignal(autoSignal) {pthread_cond_init(&cv, NULL);};
    virtual ~LockableWithCV();
    virtual void _unlockObject();
	virtual void waitSignal();
	virtual void sendSignal();
};

//----------------------------------------------------------------------
// ConditionVariable: 条件変数
//----------------------------------------------------------------------
class ConditionVariable {
private:
    Lockable& lock;
    pthread_cond_t cv;
public:
    ConditionVariable(Lockable& l):lock(l){pthread_cond_init(&cv, NULL);};
    ~ConditionVariable(){};
    void sendSignal(){pthread_cond_broadcast(&cv);};
    void waitSignal(){pthread_cond_wait(&cv, lock.getMutex());};
};

//----------------------------------------------------------------------
// LockHandle: 排他区間制御クラス
//----------------------------------------------------------------------
class LockHandle {
protected:
    Lockable*   lock;
    bool        bLocked;
public:
    LockHandle(Lockable* l) : lock(l), bLocked(true) {lock->_lockObject();};
    ~LockHandle(){if(bLocked)lock->_unlockObject();};

    void unlock(){lock->_unlockObject();bLocked = false;};
};

//----------------------------------------------------------------------
// 排他区間構文
//----------------------------------------------------------------------
#define BEGIN_LOCK(obj) {LockHandle _l(obj);
#define END_LOCK        }

}} // end of namespace

using namespace HSL::Base;
