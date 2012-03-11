#include "task.h"
#include "UnrarTask.h"
#include "UnzipTask.h"

//----------------------------------------------------------------------
// タスク基底クラスの実装
//----------------------------------------------------------------------
TaskBase::TaskBase(int id, const char* nm) : 
    id(id), name(nm), state(TASK_INITIAL), message()
{
    statistics.total = 1; // ゼロ除算例外を発生させないための仮の値
    statistics.done = 0;
    phase = "Preparing to begin task";
}

TaskBase::~TaskBase(void)
{
}

void TaskBase::serialize(OutputStream& s)
{
	s << id;
	s << name;
	s << static_cast<int>(state);
	s << statistics.total;
	s << statistics.done;
	s << message;
}

void TaskBase::deserialize(InputStream& s)
{
	s >> id;
	s >> name;
	int st;
	s >> st;
	state = static_cast<TaskState>(st);
	s >> statistics.total;
    s >> statistics.done;
	s >> message;
    
    if (state == TASK_SUCCEED || state == TASK_ERROR){
        phase = "Processing to end task";
    }
}


//----------------------------------------------------------------------
// タスクファクトリの実装
//----------------------------------------------------------------------
SmartPtr<TaskFactory> TaskFactory::instance(NULL);

TaskFactory::TaskFactory()
{
    methods.insert(methods.end(), UnrarTask::getFactoryMethods());
    methods.insert(methods.end(), UnzipTask::getFactoryMethods());
}

TaskFactory::~TaskFactory()
{
}

TaskFactory& TaskFactory::getInstance()
{
    if (instance.isNull()){
        instance = SmartPtr<TaskFactory>(new TaskFactory);
    }
    return *instance.operator ->();
}

bool TaskFactory::isAcceptableFiles(const std::vector<std::string>& files)
{
    for (size_t i = 0; i < methods.size(); i++){
        if (methods[i]->isSupportedFile(files)){
            return true;
        }
    }
    return false;
}

SmartPtr<TaskBase> TaskFactory::createVacuityObject(const std::string& type)
{
    for (size_t i = 0; i < methods.size(); i++){
        if (type.compare(methods[i]->getTypeString()) == 0){
            return SmartPtr<TaskBase>(methods[i]->newVacuityObject());
        }
    }
    return SmartPtr<TaskBase>(NULL);
}

SmartPtr<TaskBase> TaskFactory::createNewTask(
	int id, const std::vector<std::string>& files, const char* password)
{
    for (size_t i = 0; i < methods.size(); i++){
        if (methods[i]->isSupportedFile(files)){
            SmartPtr<TaskBase> 
				task(methods[i]->newTaskObject(id, files, password));
            return task;
        }
    }

    throw NotSupportedFileException();
}
