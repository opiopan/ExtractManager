#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "UnzipTask.h"

//----------------------------------------------------------------------
// 構築・削除
//----------------------------------------------------------------------
UnzipTask::UnzipTask()
: UnrarTask()
{
}

UnzipTask::UnzipTask(int id, const char* path, const char* pass)
{
	flagToBeDeleted = true;
	flagToUpdateTimestamp = true;
    isDirty = false;
    this->id = id;
    archivePath = path;
    comment = "";
    if (pass){
        password = pass;
    };

    // ZIPアーカイブ内ファイル一欄取得コマンド文字列生成
    char escapedPath[1024];
    escapeShellString(path, escapedPath, sizeof(escapedPath));
    std::string cmd = "zipinfo ";
    cmd.append(escapedPath);
    cmd.append(" | egrep -v '^[Ad0-9]' | while read a b c d e f g h i;"
               "do printf '%.16x:%s\\n' \"$d\" \"$i\";done;"
               "zipinfo ");
    cmd.append(escapedPath);
    cmd.append(" 2>/dev/null >/dev/null");
    
    // ZIPアーカイブ内ファイル一欄取得コマンド実行
    FILE* in = popen(cmd.c_str(), "r");
    if (!in){
        TaskFactory::OtherException e(strerror(errno));
        throw e;
    }
    
    // ファイルリスト生成
    char line[2048];
    while (fgets(line, sizeof(line), in)){
        size_t length = strlen(line);
        if (line[length - 1] == '\n'){
            line[length - 1] = 0;
        }
        line[16] = 0;
        long size;
        sscanf(line, "%16lx", &size);
        elements.push_back(UnrarElement(line + 17, size));
    }

    // ZIPアーカイブ内ファイル一欄取得コマンド終了コードチェック
    int rc = pclose(in);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0){
        TaskFactory::OtherException e("an error occurred while "
                                      "reading archive file");
        throw e;        
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

UnzipTask::~UnzipTask(void)
{
}

//----------------------------------------------------------------------
// タスク種別文字列取得
//----------------------------------------------------------------------
const char* UnzipTask::getType()
{
    return getTypeString();
}

//----------------------------------------------------------------------
// 外部プログラム実行用部品
//----------------------------------------------------------------------
void UnzipTask::escapeShellString(const char* src, char* dest, int length)
{
    int i = 0;
    for (; *src && i < length - 1; i++, src++){
        if (*src == ' ' || *src == '(' || *src == ')' || *src == '&' || 
            *src == '\\'){
            if (i + 1 >= length - 1){
                break;
            }
            dest[i++] = '\\';
        }
        dest[i] = *src;
    }
    dest[i] = 0;
}

//----------------------------------------------------------------------
// ファクトリ向けメソッド
//----------------------------------------------------------------------
const TaskFactory::FactoryMethods* UnzipTask::getFactoryMethods()
{
	static TaskFactory::FactoryMethods methods = {
        UnzipTask::getTypeString,
        UnzipTask::isSupportedFile,
        UnzipTask::newTaskObject,
        UnzipTask::newVacuityObject
    };

	return &methods;
}

const char* UnzipTask::getTypeString()
{
	return "UnZIP task";
}

bool UnzipTask::isSupportedFile(const std::vector<std::string>& files)
{
    if (files.size() == 1 && files[0].length() > 4 ){
		std::string::iterator i = const_cast<std::string&>(files[0]).end();
		i--;
		if (*i != 'p' && *i != 'P'){
			return false;
		}
		i--;
		if (*i != 'i' && *i != 'I'){
			return false;
		}
		i--;
		if (*i != 'z' && *i != 'Z'){
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

TaskBase* UnzipTask::newTaskObject(
	int id, const std::vector<std::string>& files, const char* pass)
{
	return new UnzipTask(id, files[0].c_str(), pass);
}

TaskBase* UnzipTask::newVacuityObject()
{
	return new UnzipTask();
}
