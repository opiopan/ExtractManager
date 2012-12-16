#pragma once
#include <vector>
#include <string>
#include "SmartPtr.h"
#include "Lockable.h"
#include "Serializable.h"

//----------------------------------------------------------------------
// タスクオブジェクト抽象クラス定義
//----------------------------------------------------------------------
class TaskBase : public Serializable ,protected Lockable
{
public:
    enum TaskState {TASK_INITIAL=0, TASK_PREPARED, TASK_SUSPENDED, 
                    TASK_RUNNING, TASK_SUCCEED, TASK_ERROR, TASK_WARNING};
    class TaskProgress {
    public:
        const char* phase;
        int64_t     total;
        int64_t     complete;
    };

protected:
	int                 id;
    std::string         name;
    TaskState           state;
    std::string         message;
    std::string         extension1;
    std::string         extension2;
    struct {
        int64_t total;
        int64_t done;
    }                   statistics;
    
    // 永続化対象外
    std::string         phase;

    class MessageString{
    public:
        std::string message;
        std::string extension1;
        std::string extension2;
    };
    
public:
    virtual ~TaskBase();
    virtual void serialize(OutputStream& s);
    virtual void deserialize(InputStream& s);

    virtual const char* getType() = 0;

    virtual void prepare() = 0;
    typedef bool (*NotifyFunc)(void* context);
    virtual void run(NotifyFunc observer, void* context) = 0;
    virtual void cancel() = 0;
    virtual void downrecover() = 0;
    virtual void resume() = 0;

	virtual void commit() = 0;

    int getId(){LockHandle l(this); return id;};
    const char* getName(){LockHandle l(this); return name.c_str();};
    TaskState getState(){LockHandle l(this); return state;};
    TaskProgress& getProgress(TaskProgress& p){
        p.phase = phase.c_str();
        p.total = statistics.total;
        p.complete = statistics.done;
        return p;
    };
    int getProgress(){
        LockHandle l(this);
        return static_cast<int>(statistics.done * 100 / statistics.total);
    };
    
    enum RM_KIND {RM_MAIN, RM_EXT1, RM_EXT2};
    const char* getResultMessage(RM_KIND kind){
		LockHandle l(this);
        if (kind == RM_MAIN){
            return message.c_str();
        }else if (kind == RM_EXT1){
            return extension1.c_str();
        }else{
            return extension2.c_str();
        }
    };

protected:
	TaskBase(int id, const char* name);
};

typedef SmartPtr<TaskBase> TaskBasePtr;

//----------------------------------------------------------------------
// タスクファクトリ
//----------------------------------------------------------------------
class TaskFactory{
public:
    class FactoryMethods{
    public:
        const char* (*getTypeString)();
        bool (*isSupportedFile)(const std::vector<std::string>& files);
        TaskBase* (*newTaskObject)(
			int id, const std::vector<std::string>& files, const char* pass);
        TaskBase* (*newVacuityObject)();
    };

    class NotSupportedFileException{};
	class NeedPasswordException{};
    class OtherException{
	public:
		std::string message;
        std::string extension;
        OtherException(const char* msg, const char* ext):
            message(msg), extension(ext){};
    };

protected:
    static SmartPtr<TaskFactory>	   instance;
    std::vector<const FactoryMethods*> methods;

public:
    ~TaskFactory();

    bool isAcceptableFiles(const std::vector<std::string>& files);
    SmartPtr<TaskBase> createVacuityObject(const std::string& type);
    SmartPtr<TaskBase> createNewTask(
		int id, const std::vector<std::string>& files, const char* pass = NULL);
    static TaskFactory& getInstance();

protected:
    TaskFactory();
};
