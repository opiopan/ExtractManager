#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include "unrartask.h"
#include "RarArchive.h"

//----------------------------------------------------------------------
// UnrarTreeNode アーカイブ内エレメントの実装
//----------------------------------------------------------------------
void UnrarElement::serialize(OutputStream& s)
{
	s << name;
	s << extractName;
	s << size;
	s << enable;
}

void UnrarElement::deserialize(InputStream& s)
{
	s >> name;
	s >> extractName;
	s >> size;
	s >> enable;
}

//----------------------------------------------------------------------
// UnrarTreeNode アーカイブ内名前空間表現の実装
//----------------------------------------------------------------------
UnrarTreeNodePtr UnrarTreeNode::createRootNode(){
    return UnrarTreeNodePtr(new UnrarTreeNode(NODE_FOLDER, "", NULL));
}

UnrarTreeNodePtr UnrarTreeNode::mergeTree(
	UnrarTreeNodePtr root, const UnrarElement& element, int64_t elementIndex)
{
    // ルートノードが存在しなければ作成する
    if (root.isNull()){
        root = UnrarTreeNodePtr(new UnrarTreeNode(NODE_FOLDER, "", NULL));
    }

    // エレメントのパスを分解
	std::vector<std::string> path;
    int depth = 0;
	unsigned long from = 0;
	unsigned long to = 0;
    while ((to = element.name.find("/", from)) != std::string::npos){
		path.push_back(element.name.substr(from, to - from));
        depth++;
		from = to + 1;
    }
	path.push_back(element.name.substr(from));

    //フォルダノードを順に追加（既に存在する場合はスキップ）
    UnrarTreeNode* current = root.operator ->();
    for (int i = 0; i < depth; i++){
        bool existFolder = false;
        for (int j = 0; j < current->getChildNum(); j++){
            UnrarTreeNode* child = current->getChild(j);
            if (path.at(i).compare(child->getName()) == 0){
                current = child;
                existFolder = true;
                break;
            }
        }
        if (!existFolder){
            UnrarTreeNodePtr folder = 
                UnrarTreeNodePtr(
                    new UnrarTreeNode(NODE_FOLDER, path.at(i).c_str(), current));
            UnrarTreeNodePtr parentRef = 
                UnrarTreeNodePtr(
                new UnrarTreeNode(NODE_PARENT_REF, "..", folder.operator ->()));
            folder->children.push_back(parentRef);
            current->children.push_back(folder);
            current = folder.operator ->();
        }
    }

    //ファイルノードを追加
    UnrarTreeNodePtr file = 
        UnrarTreeNodePtr(
			new UnrarTreeNode(NODE_FILE, path.at(depth).c_str(), current));
    file->elementIndex = elementIndex;
    file->size = element.size;
    current->children.push_back(file);

    return root;
}

UnrarTreeNodePtr UnrarTreeNode::clone(UnrarTreeNode* newParent)
{
    UnrarTreeNodePtr newNode = 
        UnrarTreeNodePtr(new UnrarTreeNode(type, name.c_str(), newParent));

    switch (type){
    case NODE_PARENT_REF:
        break;
    case NODE_FOLDER:
        for (int i = 0; i < children.size(); i++){
            newNode->children.push_back(
				children.at(i)->clone(newNode.operator ->()));
        }
        break;
    case NODE_FILE:
        newNode->elementIndex = elementIndex;
        newNode->size = size;
        break;
    }

    return newNode;
}

void UnrarTreeNode::serialize(OutputStream& s)
{
	s << static_cast<int>(type);
	s << name;
	s << elementIndex;
	s << size;
	int64_t cnum = children.size();
	s << cnum;
	for (int64_t i = 0; i < cnum; i++){
		children.at(i)->serialize(s);
	}
}

void UnrarTreeNode::deserialize(InputStream& s)
{
	int itype;
	s >> itype;
	type = static_cast<NodeType>(itype);
	s >> name;
	s >> elementIndex;
	s >> size;
	int64_t cnum;
	s >> cnum;
	for (int64_t i = 0; i < cnum; i++){
		UnrarTreeNodePtr node(new UnrarTreeNode(this));
		node->deserialize(s);
		children.push_back(node);
	}
}


//----------------------------------------------------------------------
// UnrarTask 構築・削除
//----------------------------------------------------------------------
UnrarTask::UnrarTask(int id, const RarArchive& rar) 
	: TaskBase(id, "")
	, flagToBeDeleted(true)
	, flagToUpdateTimestamp(true)
    , isDirty(false)
{
    // RarArchiveオブジェクトから情報を抽出
    archivePath = rar.getArchivePath();
    comment = rar.getComment();
	password = rar.getPassword();
    for (size_t i = 0; i < rar.getElementNum(); i++){
        elements.push_back(UnrarElement(
            rar.getElement(i).getName(), rar.getElement(i).getSize()));
    }
    for (size_t i = 0; i < rar.getVolumeNum(); i++){
        volumes.push_back(std::string(rar.getVolumeName(i)));
    }
    
    // アーカイブファイルパスをファイル名とディレクトリパスに分解
	unsigned long sep = archivePath.rfind("/", archivePath.size() - 1);
	if (sep != std::string::npos){
		name = archivePath.substr(sep + 1);
		baseDir = archivePath.substr(0, sep);
	}else{
		name = archivePath;
		baseDir = "";
	}

    // 名前空間グラフ作成
    tree = UnrarTreeNode::createRootNode();
    for (int i = 0; i < elements.size(); i++){
        tree = UnrarTreeNode::mergeTree(tree, elements.at(i), i);
    }
}

UnrarTask::UnrarTask() : TaskBase(-1, "")
{
}

UnrarTask::~UnrarTask(void)
{
}

void UnrarTask::serialize(OutputStream& s)
{
    LockHandle l(this);
    
    TaskBase::serialize(s);
    s << archivePath;
    s << comment;
    s << baseDir;
    s << password;
    s << flagToBeDeleted;
    s << flagToUpdateTimestamp;
    int64_t num = volumes.size();
    s << num;
    for (int64_t i = 0; i < num; i++){
        s << volumes.at(i);
    }
    num = elements.size();
    s << num;
    for (int64_t i = 0; i < num; i++){
        elements.at(i).serialize(s);
    }
    if (tree.operator ->()){
        tree->serialize(s);
    }
}

void UnrarTask::deserialize(InputStream& s)
{
    LockHandle l(this);

    TaskBase::deserialize(s);
    s >> archivePath;
    s >> comment;
    s >> baseDir;
    s >> password;
    s >> flagToBeDeleted;
    s >> flagToUpdateTimestamp;
    int64_t num;
    s >> num;
    for (int64_t i = 0; i < num; i++){
        std::string str;
        s >> str;
        volumes.push_back(str);
    }
    s >> num;
    for (int64_t i = 0; i < num; i++){
        UnrarElement e("");
        e.deserialize(s);
        elements.push_back(e);
    }
    tree = UnrarTreeNodePtr(new UnrarTreeNode(NULL));
    tree->deserialize(s);
    
    isDirty = false;
}

//----------------------------------------------------------------------
// タスク種別文字列取得
//----------------------------------------------------------------------
const char* UnrarTask::getType()
{
    return getTypeString();
}

//----------------------------------------------------------------------
// ファクトリ向けメソッド
//----------------------------------------------------------------------
const TaskFactory::FactoryMethods* UnrarTask::getFactoryMethods()
{
    static TaskFactory::FactoryMethods methods = {
        UnrarTask::getTypeString,
        UnrarTask::isSupportedFile,
        UnrarTask::newTaskObject,
        UnrarTask::newVacuityObject
    };

    return &methods;
}

const char* UnrarTask::getTypeString(){
    return "UnRAR task";
}

bool UnrarTask::isSupportedFile(const std::vector<std::string>& files)
{
    if (files.size() == 1 && files[0].length() > 4 ){
		std::string::iterator i = const_cast<std::string&>(files[0]).end();
		i--;
		if (*i != 'r' && *i != 'R'){
			return false;
		}
		i--;
		if (*i != 'a' && *i != 'A'){
			return false;
		}
		i--;
		if (*i != 'r' && *i != 'R'){
			return false;
		}
		i--;
		if (*i != '.'){
			return false;
		}
		return true;
    }
    return false;
}

TaskBase* UnrarTask::newTaskObject(
	int id, const std::vector<std::string>& files, const char*pass)
{
	try{
		RarArchive rar(files[0].c_str(), pass);
		return new UnrarTask(id, rar);
	}catch(RarArchiveExceptionPtr& e){
		if (e->getErrorCode() == RARERROR_NOPASSWORD){
			TaskFactory::NeedPasswordException e2;
			throw e2;
		}else{
			TaskFactory::OtherException e2(e->getErrorString());
			throw e2;
		}
	}
}

TaskBase* UnrarTask::newVacuityObject()
{
    return new UnrarTask();
}

//----------------------------------------------------------------------
// タスク編集
//----------------------------------------------------------------------
void UnrarTask::getProperties(TaskProperties& props)
{
    LockHandle l(this);
    
    props.baseDir = baseDir;
    props.password = password;
    props.flagToBeDeleted = flagToBeDeleted;
    props.flagToUpdateTimestamp = flagToUpdateTimestamp;
    props.tree = tree->clone();
    props.currentNode = props.tree.operator->();
    props.initialTree = UnrarTreeNode::createRootNode();
    for (int64_t i = 0; i < elements.size(); i++){
        props.initialTree = 
            UnrarTreeNode::mergeTree(props.initialTree, elements[i], i);
    }
}

void UnrarTask::setProperties(const TaskProperties& props)
{
    LockHandle l(this);
    
    uncommitedProperties.baseDir = props.baseDir;
    uncommitedProperties.password = props.password;
    uncommitedProperties.flagToBeDeleted = props.flagToBeDeleted;
    uncommitedProperties.flagToUpdateTimestamp = props.flagToUpdateTimestamp;
    uncommitedProperties.tree = props.tree;

    isDirty = true;    
}

void UnrarTask::commit()
{
    LockHandle l(this);

    if (isDirty){
        isDirty = false;

        if (state == TASK_SUSPENDED || state == TASK_RUNNING){
            TaskFactory::OtherException e("Cannot modify a running task");
            throw e;
        }else if (state == TASK_SUCCEED){
            TaskFactory::OtherException e("Cannot modify a finished task");
            throw e;
        }else if (state == TASK_INITIAL){
            state = TASK_PREPARED;
        }
        baseDir = uncommitedProperties.baseDir;
        password = uncommitedProperties.password;
        flagToBeDeleted = uncommitedProperties.flagToBeDeleted;
        flagToUpdateTimestamp = uncommitedProperties.flagToUpdateTimestamp;
        tree = uncommitedProperties.tree;
    }
}

//----------------------------------------------------------------------
// タスク実行準備 (TASK_PREPARED→TASK_RUNNINGに遷移)
//----------------------------------------------------------------------
void UnrarTask::prepare()
{
    LockHandle l(this);
    if (state == TASK_PREPARED){
        state = TASK_RUNNING;
        flagCanceled = false;
        phase = "Preparing to begin task";
    }
}


//----------------------------------------------------------------------
// 名前空間ツリーを要素リストの展開名として反映
//----------------------------------------------------------------------
void UnrarTask::reflectTargetName(UnrarTreeNode* node, const char* base)
{
    if (node->getType() == node->NODE_FILE){
		std::string path = base;
        path.append("/");
        path.append(node->getName());
        UnrarElement& element = elements[node->getElementIndex()];
        element.extractName = path;
        element.enable = true;
        statistics.total += element.size;
    }else if (node->getType() == node->NODE_FOLDER){
		std::string path = base;
        if (node->getName()[0] != '\0'){
            path.append("/");
            path.append(node->getName());
        }
        for (int64_t i = 0; i < node->getChildNum(); i++){
            reflectTargetName(node->getChild(i), path.c_str());
        }
    }
}

//----------------------------------------------------------------------
// タスク実行
//----------------------------------------------------------------------
void UnrarTask::run(NotifyFunc func, void* context)
{    
    try{
        observer.func = func;
        observer.context = context;
        
        // 展開対象・展開名の決定
        for (int64_t i = 0; i < elements.size(); i++){
            elements[i].enable = false;
        }
        BEGIN_LOCK(this){
            statistics.total = 0;
            statistics.done = 0;
            reflectTargetName(tree.operator ->(), baseDir.c_str());            
        }END_LOCK

        // 展開
        extract();

        // タイムスタンプ更新
        BEGIN_LOCK(this){
            phase = "Modifying timestamp of extracted files";
        }END_LOCK
        if (flagToUpdateTimestamp){
            for (int64_t i = 0; i < elements.size(); i++){
                UnrarElement& element = elements[i];
                if (element.enable){
					int rc = utimes(element.extractName.c_str(), NULL);
					if (rc != 0){
						std::string msg("Cannot modify time of a file: ");
						msg.append(element.extractName.c_str());
						throw msg;
					}
                }
            }
        }

        // アーカイブ削除
        BEGIN_LOCK(this){
            phase = "Deleting archive files";
        }END_LOCK
        if (this->flagToBeDeleted){
            for (int64_t i = 0; i < volumes.size(); i++){
				if (unlink(volumes[i].c_str()) != 0){
					std::string msg("Cannot remove an archive file: ");
					msg.append(volumes[i].c_str());
                    throw msg;
                }
            }
        }


        //タスク完了
        BEGIN_LOCK(this){
            state = TASK_SUCCEED;
        }END_LOCK
    }catch (RarArchiveExceptionPtr& e){
        LockHandle l(this);
        message = "An error occurred while extracting a archive: ";
        message.append(e->getErrorString());
        state = TASK_ERROR;
    }catch (std::string& s){
        LockHandle l(this);
        message = s;
        state = TASK_ERROR;
    }
}

void UnrarTask::extract()
{
    // 展開オプション設定＆展開先ファイルの存在チェック
    RarArchive rar(this->archivePath.c_str(), password.c_str());
    rar.setBaseDir("");
    rar.setNotifyFunc(UnrarTask::progressNotify, this);
    if (elements.size() != rar.getElementNum()){
        std::string msg("Archive file might be changed.");
        throw msg;
    }
    for (int64_t i = 0; i < elements.size(); i++){
        UnrarElement& element = elements[i];
        struct stat statbuf;
        if (stat(element.extractName.c_str(), &statbuf) == 0){
            std::string msg("A file to extract has already exist: ");
            msg.append(element.extractName);
            throw msg;
        }
        if (element.name != rar.getElement(i).getName()){
            std::string msg("Archive file might be changed.");
            throw msg;
        }
        rar.getElement(i).setExtractName(element.extractName.c_str());
        rar.getElement(i).setIgnore(!element.enable);
    }
    
    // 展開
    rar.extract();
}

//----------------------------------------------------------------------
// タスクキャンセル指示
//----------------------------------------------------------------------
void UnrarTask::cancel()
{
    LockHandle l(this);
    if (state == TASK_RUNNING){
        flagCanceled = true;
    }else if (state == TASK_PREPARED){
        state = TASK_SUSPENDED;
    }
}

//----------------------------------------------------------------------
// ダウンリカバリ
//----------------------------------------------------------------------
void UnrarTask::downrecover()
{
    LockHandle l(this);
    state = TASK_ERROR;
    message = "Task has been aborted.";
    //progress = 0;
    flagCanceled = false;
}

//----------------------------------------------------------------------
// タスク再開
//----------------------------------------------------------------------
void UnrarTask::resume()
{
    LockHandle l(this);
    if (state != TASK_RUNNING && state != TASK_SUCCEED){
        state = TASK_PREPARED;
        message = "";
        statistics.done = 0;
        flagCanceled = false;
    }
}

//----------------------------------------------------------------------
// RAR展開処結果の通知
//----------------------------------------------------------------------
int UnrarTask::progressNotify(const char* volume, int size, void* data)
{
    UnrarTask* task = reinterpret_cast<UnrarTask*>(data);
    
    {
        LockHandle l(task);

        task->phase = "Extracting from ";
        int base = 0;
        for (int i = 0; volume[i]; i++){
            if (volume[i] == '/'){
                base = i + 1;
            }
        }
        
        task->phase.append(volume + base);
        task->statistics.done += size;

        if (task->flagCanceled){
            return -1;
        }
    
    }

    if (task->observer.func && ! task->observer.func(task->observer.context)){
        return -1;
    }
    
    return 0;
}
