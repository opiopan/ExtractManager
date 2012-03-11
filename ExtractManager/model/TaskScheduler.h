//----------------------------------------------------------------------
// TaskSchedule.h
//   タスクスケジューラー
//----------------------------------------------------------------------
#pragma once

#include <vector>
#include <string>
#include "Serializable.h"
#include "Task.h"

class TaskScheduler : public Lockable, protected Serializable {
public:
    typedef void (*NotifyFunc)(int taskIndex, void* context);

protected:
	static TaskScheduler*		instance;
	std::string					path;
	std::vector<TaskBasePtr>	tasks;
	int							current;
    int                         lastProgress;
    ConditionVariable           cvForScheduling;
	bool						isScheduling;
    ConditionVariable           cvForShutdown;
	bool						flagToShutdown;
	
    class{
    public:
        NotifyFunc  func;
        void*       context;
    }                           observer;

protected:
	TaskScheduler();
public:
    virtual ~TaskScheduler();

	static TaskScheduler& getInstance();

	void initialize(const char* path);
	void shutdown();
    
	void schedule(NotifyFunc observer, void* context);

	TaskBasePtr newTask(
		const std::vector<std::string>& paths, const char* password = NULL);
	void addTask(TaskBasePtr& task)	;
	void removeTask(int64_t index);

	void commit();
    
    uint64_t getTaskNum(){return tasks.size();};
    TaskBasePtr& getTask(uint64_t index);
    
    int getCurrent(){BEGIN_LOCK(this){return current;}END_LOCK};

    void persist();
    
protected:
	virtual void serialize(OutputStream& s);
	virtual void deserialize(InputStream& s);

	void prepareInitialConfiguration();
    
    int getNextTask();
    static bool notifyTaskProgress(void* context);
};
