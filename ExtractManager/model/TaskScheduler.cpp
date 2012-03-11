//----------------------------------------------------------------------
// TaskSchedule.cpp
//   タスクスケジューラー
//----------------------------------------------------------------------

#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include "TaskScheduler.h"

TaskScheduler* TaskScheduler::instance = NULL;

//----------------------------------------------------------------------
// 構築・削除（シングルトン）
//----------------------------------------------------------------------
TaskScheduler::TaskScheduler() 
	: isScheduling(false)
	, flagToShutdown(false)
	, current(-1)
    , cvForScheduling(*this)
    , cvForShutdown(*this)
{
}

TaskScheduler::~TaskScheduler()
{
}

TaskScheduler& TaskScheduler::getInstance()
{
	static Lockable l;
    BEGIN_LOCK(&l){
        if (!instance){
            instance = new TaskScheduler;
        }
        return *instance;    
    }END_LOCK
}

//----------------------------------------------------------------------
// オブジェクトシリアライズ
//----------------------------------------------------------------------
void TaskScheduler::serialize(OutputStream& s)
{
	s << static_cast<int64_t>(tasks.size());
	for (std::vector<TaskBasePtr>::iterator i = tasks.begin();
		 i != tasks.end(); i++){
		s << (*i)->getType();
		(*i)->serialize(s);
	}
}

void TaskScheduler::deserialize(InputStream& s)
{
	tasks.clear();
	current = -1;
	int64_t num;
	s >> num;
	for (size_t i = 0; i < num; i++){
		std::string type;
		s >> type;
        TaskBasePtr task = TaskFactory::getInstance().createVacuityObject(type);
        task->deserialize(s);
		tasks.push_back(task);
	}
}

//----------------------------------------------------------------------
// 初期設定ファイルの準備
//----------------------------------------------------------------------
void TaskScheduler::prepareInitialConfiguration()
{
	struct stat buf;
	if (stat(path.c_str(), &buf) != 0 && 
		(errno == ENOENT || errno == ENOTDIR)){
		std::string cmd = "mkdir -p \"`dirname \"";
		cmd.append(path.c_str());
		cmd.append("\"`\"");
		system(cmd.c_str());
		
		persist();
	}
}

//----------------------------------------------------------------------
// 永続化
//----------------------------------------------------------------------
void TaskScheduler::persist()
{
	FileOutputStream s(path.c_str());
	serialize(s);
}


//----------------------------------------------------------------------
// 初期化・終了
//----------------------------------------------------------------------
void TaskScheduler::initialize(const char* path)
{
    BEGIN_LOCK(this){
        this->path = path;
        current = -1;
        
        prepareInitialConfiguration();
        
        FileInputStream s(path);
        deserialize(s);
        
        for (std::vector<TaskBasePtr>::iterator i = tasks.begin();
             i != tasks.end(); i++){
            if ((*i)->getState() == TaskBase::TASK_RUNNING){
                (*i)->downrecover();
            }
        }
    }END_LOCK
}

void TaskScheduler::shutdown()
{
    BEGIN_LOCK(this){
        if (current >= 0){
            tasks[current]->cancel();
        }
        while (isScheduling){
            flagToShutdown = true;
            cvForScheduling.sendSignal();
            cvForShutdown.waitSignal();
        }
        persist();
    }END_LOCK
}

//----------------------------------------------------------------------
// タスク操作
//----------------------------------------------------------------------
TaskBasePtr TaskScheduler::newTask(
	const std::vector<std::string>& paths, const char* password)
{
	return TaskFactory::getInstance().createNewTask(0, paths, password);
}

void TaskScheduler::addTask(TaskBasePtr& task)
{
    BEGIN_LOCK(this){
        task->commit();
        tasks.push_back(task);
        persist();
        cvForScheduling.sendSignal();
    }END_LOCK
}

TaskBasePtr& TaskScheduler::getTask(uint64_t index)
{
    BEGIN_LOCK(this){
        return tasks[index];
    }END_LOCK
}

void TaskScheduler::commit()
{
    BEGIN_LOCK(this){
        for (std::vector<TaskBasePtr>::iterator i = tasks.begin();
             i != tasks.end(); i++){
            (*i)->commit();
        }
        persist();
        cvForScheduling.sendSignal();
    }END_LOCK
}

void TaskScheduler::removeTask(int64_t index)
{
    BEGIN_LOCK(this){
        std::vector<TaskBasePtr>::iterator i = tasks.begin() + index;
        if ((*i)->getState() != TaskBase::TASK_RUNNING){
            tasks.erase(i);
            if (current >= 0){
                current = -1;
                for (int i = 0; i < tasks.size(); i++){
                    if (tasks[i]->getState() == TaskBase::TASK_RUNNING){
                        current = i;
                        break;
                    }
                }
            }
        }
    }END_LOCK
}

//----------------------------------------------------------------------
// タスクスケジュール
//----------------------------------------------------------------------
void TaskScheduler::schedule(NotifyFunc func, void* context)
{
    observer.func = func;
    observer.context = context;

    BEGIN_LOCK(this){
        isScheduling = true;
    }END_LOCK
    
    int index;
    while ((index = getNextTask()) >= 0){
        lastProgress = 0;
        tasks[index]->prepare();
        persist();
        BEGIN_LOCK(this){
            current = index;            
        }END_LOCK
        tasks[index]->run(notifyTaskProgress, this);
        BEGIN_LOCK(this){
            current = -1;
        }END_LOCK
        persist();
        func(-1, context);
    }

    BEGIN_LOCK(this){
        isScheduling = false;
        cvForShutdown.sendSignal();
    }END_LOCK
}

int TaskScheduler::getNextTask()
{
    BEGIN_LOCK(this){
        while (!flagToShutdown){
            for (int i = 0; i < tasks.size(); i++){
                if (tasks[i]->getState() == TaskBase::TASK_PREPARED){
                    return i;
                }
            }
            current = -2;
            observer.func(current, observer.context);
            cvForScheduling.waitSignal();
        }
        
        return -1;
    }END_LOCK
}

//----------------------------------------------------------------------
// タスク進捗通知
//----------------------------------------------------------------------
bool TaskScheduler::notifyTaskProgress(void* context)
{
    TaskScheduler* scheduler = reinterpret_cast<TaskScheduler*>(context);
    LockHandle l(scheduler);
    
    int current = scheduler->current;
    int progress = scheduler->tasks[current]->getProgress();
    if (scheduler->lastProgress != progress){
        scheduler->lastProgress = progress;
        l.unlock();
        
        scheduler->observer.func(current, scheduler->observer.context);
    }
    return true;
}
