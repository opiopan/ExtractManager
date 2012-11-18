#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "UnzipTask.h"

//----------------------------------------------------------------------
// unzipコマンドディレクトリ設定
//----------------------------------------------------------------------
std::string UnzipTask::binDir("/bin/");

void UnzipTask::setBinDir(const char* path)
{
    binDir = path;
    if (binDir.at(binDir.length() - 1) != '/'){
        binDir.append("/");
    }
}

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
    volumes.push_back(path);
    comment = "";
    if (pass){
        password = pass;
    };

    // アーカイブ内ファイル名のエンコーディングを特定
    languageID = -1;
    encodingID = -1;
    detectEncoding(path, languageID, encodingID);
    
    // ファイルリスト生成 & 名前空間グラフ作成
    tree = createTree(path, languageID, encodingID, elements);

    // アーカイブファイルパスをファイル名とディレクトリパスに分解
	unsigned long sep = archivePath.rfind("/", archivePath.size() - 1);
	if (sep != std::string::npos){
		name = archivePath.substr(sep + 1);
		baseDir = archivePath.substr(0, sep);
	}else{
		name = archivePath;
		baseDir = "";
	}
}

UnzipTask::~UnzipTask(void)
{
}

//----------------------------------------------------------------------
// ファイル名 I18N対応機能
//----------------------------------------------------------------------
struct Encoding{
    const char* name;
    const char* option;
};

#define ENCODINGENTRY(a) sizeof(a) / sizeof(Encoding), a

static Encoding japaneseEncodings[] = {
    "Shift JIS", "-OSJIS ",
    "EUC Japanese", "-OEUCJP "
};

static Encoding chineseEncodings[] = {
    "BIG5", "-OBIG5 ",
    "GB18030", "-OGB18030 ",
    "GBK", "-OGBK "
};

static Encoding universalEncodings[] = {
    "UTF8", "-OUTF8 "
};

static struct EncodingList{
    const char* name;
    int         num;
    Encoding*   encodings;
} allEncodings[] = {
    "Auto detect", 0, NULL,
    "Japanese", ENCODINGENTRY(japaneseEncodings),
    "Chinese", ENCODINGENTRY(chineseEncodings),
    "Universal (UTF)", ENCODINGENTRY(universalEncodings),
    NULL, -1, NULL
};

int32_t UnzipTask::getSupportedLanguageNum()
{
    return sizeof(allEncodings) / sizeof(allEncodings[0]) - 1;
}

const char* UnzipTask::getLanguageName(int32_t lid)
{
    return allEncodings[lid].name;
}

int32_t UnzipTask::getSupportedEncodingNum(int32_t lid)
{
    return allEncodings[lid].num;
}

const char* UnzipTask::getEncodingName(int32_t lid, int32_t eid)
{
    return allEncodings[lid].encodings[eid].name;
}

UnrarTreeNodePtr UnzipTask::getTreeWithEncoding(int32_t& lid, int32_t& eid,
                                                std::vector<UnrarElement>& elm)
{
    if (lid <= 0 || eid < 0){
        if (!detectEncoding(archivePath.c_str(), lid, eid)){
            TaskFactory::OtherException e("Specified endcoding is not recognized");
            throw e;
        }
    }
    return createTree(archivePath.c_str(), lid, eid, elm);
}

bool UnzipTask::detectEncoding(const char* path, int32_t& lid, int32_t& eid)
{
    // アーカイブ内ファイル名のエンコーディングを特定
    int32_t begin = lid <= 0 ? 1 : lid;
    int32_t end = lid <= 0 ? sizeof(allEncodings) / sizeof(allEncodings[0]) - 1: lid + 1;
    for (lid = begin; lid < end; lid++){
        int32_t ebegin = eid < 0 ? 0 : eid;
        int32_t eend = eid < 0 ? allEncodings[lid].num : eid + 1;
        for (eid = ebegin; eid < eend; eid++){
            char escapedPath[1024];
            escapeShellString(path, escapedPath, sizeof(escapedPath));
            std::string cmd = "LANG=ja_JP.UTF-8 ";
            cmd.append(binDir);
            cmd.append("zipinfo ");
            cmd.append(allEncodings[lid].encodings[eid].option);
            cmd.append(escapedPath);
            cmd.append(" | /usr/bin/iconv -f UTF8 -t UTF8 >/dev/null 2>/dev/null");
            if (system(cmd.c_str()) == 0){
                return true;
            }
        }
        eid = -1;
    }
    
    return false;
}

UnrarTreeNodePtr UnzipTask::createTree(const char* path, int32_t& lid, int32_t& eid,
                                       std::vector<UnrarElement>& elm)
{
    // ZIPアーカイブ内ファイル一欄取得コマンド文字列生成
    char escapedPath[1024];
    escapeShellString(path, escapedPath, sizeof(escapedPath));
    std::string cmd = "LANG=ja_JP.UTF-8 ";
    cmd.append(binDir);
    cmd.append("zipinfo ");
    cmd.append(allEncodings[lid].encodings[eid].option);
    cmd.append(escapedPath);
    cmd.append(" | /usr/bin/egrep -v '^[Ad0-9]' | /usr/bin/egrep ^- | "
               "while read a b c d e f g h i;"
               "do printf '%.16x:%s\\n' \"$d\" \"$i\";done;"
               "/usr/bin/zipinfo ");
    cmd.append(escapedPath);
    cmd.append(" 2>/dev/null >/dev/null");
    
    // ZIPアーカイブ内ファイル一欄取得コマンド実行
    FILE* in = popen(cmd.c_str(), "r");
    if (!in){
        TaskFactory::OtherException e(strerror(errno));
        throw e;
    }
    
    // ファイルリスト生成
    elm.clear();
    char line[2048];
    while (fgets(line, sizeof(line), in)){
        size_t length = strlen(line);
        if (line[length - 1] == '\n'){
            line[length - 1] = 0;
        }
        line[16] = 0;
        long size;
        sscanf(line, "%16lx", &size);
        elm.push_back(UnrarElement(line + 17, size));
    }
    
    // ZIPアーカイブ内ファイル一欄取得コマンド終了コードチェック
    int rc = pclose(in);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0){
        TaskFactory::OtherException e("An error occurred while "
                                      "reading archive file");
        throw e;
    }
    
    // 名前空間グラフ作成
    UnrarTreeNodePtr newTree = UnrarTreeNode::createRootNode();
    for (int i = 0; i < elm.size(); i++){
        newTree = UnrarTreeNode::mergeTree(newTree, elm.at(i), i);
    }
    
    return newTree;
}

//----------------------------------------------------------------------
// 展開
//----------------------------------------------------------------------
void UnzipTask::extract()
{
    // 展開先ファイルの存在チェック
    for (int64_t i = 0; i < elements.size(); i++){
        UnrarElement& element = elements[i];
        struct stat statbuf;
        if (stat(element.extractName.c_str(), &statbuf) == 0){
            std::string msg("A file to extract has already exist: ");
            msg.append(element.extractName);
            throw msg;
        }
    }

    // 展開
    for (int64_t i = 0; i < elements.size(); i++){
        UnrarElement& element = elements[i];
        if (element.enable){
            extractFile(element.name.c_str(), 
                        element.extractName.c_str(), 
                        element.size);
        }
    }
}

void UnzipTask::extractFile(const char* aname, const char* ename, int64_t size)
{
    // ディレクトリの再帰的作成
    char escapedOut[1024];
    escapeShellString(ename, escapedOut, sizeof(escapedOut));
    std::string cmd = "mkdir -p \"`dirname ";
    cmd.append(escapedOut);
    cmd.append("`\"");
    system(cmd.c_str());

    
    // 出力先ファイルオープン
    FILE* out = fopen(ename, "w");
    if (!out){
        std::string msg(strerror(errno));
        msg.append(": ");
        msg.append(ename);
        throw msg;
    }
    
    // unzipコマンド文字列生成
    char escapedElement[1024];
    escapeShellString(aname, escapedElement, sizeof(escapedElement));
    char escapedArchive[1024];
    escapeShellString(archivePath.c_str(), escapedArchive, sizeof(escapedArchive));
    cmd = "LANG=ja_JP.UTF-8 ";
    cmd.append(binDir);
    cmd.append("unzip -p ");
    cmd.append(allEncodings[languageID].encodings[encodingID].option);
    cmd.append(escapedArchive);
    cmd.append(" ");
    cmd.append(escapedElement);
    
    // unzipコマンド起動
    FILE* in = popen(cmd.c_str(), "r");
    if (!in){
        fclose(out);
        std::string msg(strerror(errno));
        throw msg;
    }
    
    // アーカイブデータ read & write
    static char buf[1024 * 512];
    int64_t extracted = 0;
    int64_t flagment = 0;
    while ((flagment = fread(buf, 1, sizeof(buf), in)) > 0){
        if (fwrite(buf, flagment, 1, out) != 1){
            fclose(out);
            pclose(in);
            std::string msg = "Could not write to a file: ";
            msg.append(ename);
            throw msg;
        }
        extracted += flagment;
        
        BEGIN_LOCK(this){
            statistics.done += flagment;
            if (flagCanceled){
                fclose(out);
                pclose(in);
                std::string msg = "Task has been aborted.";
                throw msg;
            }
        }END_LOCK
        
        if (observer.func){
             observer.func(observer.context);
        }
    }
    
    // クリーンアップ
    if (fclose(out) != 0){
        pclose(in);
        std::string msg = "Could not write to a file: ";
        msg.append(ename);
        throw msg;
    }
    int rc = pclose(in);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0 || extracted != size){
        std::string msg = "An error occurred while extracting a file: ";
        msg.append(aname);
        throw msg;
    }
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
