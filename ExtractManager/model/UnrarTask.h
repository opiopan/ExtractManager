#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "task.h"
#include "RarArchive.h"

//----------------------------------------------------------------------
// UnrarTreeNode アーカイブ内エレメント
//----------------------------------------------------------------------
class UnrarElement
{
public:
	std::string	name;
	std::string	extractName;
    int64_t		size;
    bool		enable;

public:
    UnrarElement(const char* nm = "", int64_t sz = 0) :
        name(nm), size(sz), enable(false){};
    UnrarElement(const UnrarElement& src) :
        name(src.name), extractName(src.extractName),
        size(src.size), enable(src.enable){};

    virtual void serialize(OutputStream& s);
    virtual void deserialize(InputStream& s);
};

//----------------------------------------------------------------------
// UnrarTreeNode アーカイブ内名前空間表現
//----------------------------------------------------------------------
class UnrarTreeNode;
typedef SmartPtr<UnrarTreeNode> UnrarTreeNodePtr;
class UnrarTreeNode
{
public:
    enum NodeType{NODE_PARENT_REF, NODE_FOLDER, NODE_FILE};
protected:
    NodeType                      type;
	std::string                   name;
    UnrarTreeNode*                parent;
    int64_t                       elementIndex;
    int64_t                       size;
	std::vector<UnrarTreeNodePtr> children;

	typedef std::vector<UnrarTreeNodePtr>::iterator ChildrenIterator;
    
public:
    UnrarTreeNode(UnrarTreeNode* p) : parent(p){};
    static UnrarTreeNodePtr createRootNode();
    static UnrarTreeNodePtr mergeTree(
		UnrarTreeNodePtr root, const UnrarElement& element, 
		int64_t elementIndex);
    UnrarTreeNodePtr clone(UnrarTreeNode* newParent = NULL);

    NodeType getType() const{return type;};
    const char* getName() const{return name.c_str();};
    UnrarTreeNode* getParent() const{return parent;};
    int64_t getChildNum() const{return children.size();};
    UnrarTreeNode* getChild(int64_t index) const{
		return children.at(index).operator ->();};
    int64_t getElementIndex() const{return elementIndex;};
    int64_t getSize() const{return size;};

    void changeName(const char* nm){name = nm;};
    void deleteChild(int64_t index){
		ChildrenIterator i = children.begin() + index;
		children.erase(i);}
    void pullFolder(int64_t index)
    {
        UnrarTreeNodePtr node = children.at(index);
        if (node->type == NODE_FOLDER){
			ChildrenIterator i = children.begin() + index;
            children.erase(i);
            for (i = node->children.begin(); i != node->children.end(); i++){
                UnrarTreeNodePtr child = *i;
                if (child->type != NODE_PARENT_REF){
                    children.push_back(child);
                }
            }
        }
    };

    virtual void serialize(OutputStream& s);
    virtual void deserialize(InputStream& s);

protected:
    UnrarTreeNode(NodeType t, const char* n, UnrarTreeNode* p) :
         type(t), name(n), parent(p){};
};


//----------------------------------------------------------------------
// UnrarTask RARアーカイブの展開タスク
//----------------------------------------------------------------------
class UnrarTask :
    public TaskBase
{
public:
    class TaskProperties{
    public:
        std::string               baseDir;
        std::string               password;
        bool                      flagToBeDeleted;
        bool                      flagToUpdateTimestamp;
        std::vector<UnrarElement> elements;
        UnrarTreeNodePtr          tree;
        UnrarTreeNodePtr          initialTree;
        UnrarTreeNode*            currentNode;
        int32_t                   languageID;
        int32_t                   encodingID;
    };
    
protected:
    std::string               archivePath;
    std::string               comment;
    std::string               baseDir;
    std::string               password;
    bool                      flagToBeDeleted;
    bool                      flagToUpdateTimestamp;
    std::vector<std::string>  volumes;
    std::vector<UnrarElement> elements;
    UnrarTreeNodePtr          tree;
    int32_t                   languageID;
    int32_t                   encodingID;

    //永続化対象外
    bool                      flagCanceled;
    bool                      isDirty;
    TaskProperties            uncommitedProperties;
    class {
    public:
        NotifyFunc func;
        void*      context;
    }                         observer;

public:
    virtual ~UnrarTask(void);

    virtual void serialize(OutputStream& s);
    virtual void deserialize(InputStream& s);
    static const TaskFactory::FactoryMethods* getFactoryMethods();

    virtual const char* getType();

    virtual void prepare();
    virtual void run(NotifyFunc observer, void* context);
    virtual void cancel();
    virtual void downrecover();
    virtual void resume();

    virtual void commit();
    
    void getProperties(TaskProperties&);
    void setProperties(const TaskProperties&);

    const char* getComment(){return comment.c_str();};
    std::vector<std::string>& getVolumes(){return volumes;};
    
    virtual int32_t getSupportedLanguageNum();
    virtual const char* getLanguageName(int32_t lid);
    virtual int32_t getSupportedEncodingNum(int32_t lid);
    virtual const char* getEncodingName(int32_t lid, int32_t eid);
    virtual UnrarTreeNodePtr getTreeWithEncoding(int32_t& lid, int32_t& eid,
                                                 std::vector<UnrarElement>& elm);
    
protected:
    UnrarTask();
    UnrarTask(int id, const RarArchive& rar);

    static const char* getTypeString();
    static bool isSupportedFile(const std::vector<std::string>& files);
    static TaskBase* newTaskObject(
		int id, const std::vector<std::string>& files, const char* pass);
    static TaskBase* newVacuityObject();


    virtual void extract();

    static int progressNotify(const char* volume, int size, void* data);

    void reflectTargetName(UnrarTreeNode*, const char*);
};
