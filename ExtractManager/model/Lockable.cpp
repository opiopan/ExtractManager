//----------------------------------------------------------------------
// Lockable.cpp: 同期機能の実装
//----------------------------------------------------------------------

#include "Lockable.h"

namespace HSL{ namespace Base {

Lockable::~Lockable()
{
	pthread_mutex_destroy(&mutex);
}
void Lockable::_lockObject()
{
	pthread_mutex_lock(&mutex);
}
void Lockable::_unlockObject()
{
	pthread_mutex_unlock(&mutex);
}

LockableWithCV::~LockableWithCV()
{
	pthread_cond_destroy(&cv);
}
void LockableWithCV::_unlockObject()
{
	if (autoSignal){
		sendSignal();
	}
	Lockable::_unlockObject();
}
void LockableWithCV::waitSignal()
{
	if (autoSignal){
		sendSignal();
	}
	pthread_cond_wait(&cv, &mutex);
}

void LockableWithCV::sendSignal()
{
	pthread_cond_broadcast(&cv);
}

}} // end of namespace
