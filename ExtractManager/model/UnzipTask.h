#pragma once
#include "UnrarTask.h"

//----------------------------------------------------------------------
// UnzipTask ZIPアーカイブの展開タスク
//   タスクが保持する属性、編集用GUIはUnrarTaskと共通
//   UnrarTaskの派生クラスとして定義し、アーカイブファイル情報抽出部
//   および、各ファイルデータ抽出処理のみを、固有処理として実装
//----------------------------------------------------------------------
class UnzipTask : public UnrarTask
{
protected:
    static std::string binDir;
    
public:
    virtual ~UnzipTask(void);

	static const TaskFactory::FactoryMethods* getFactoryMethods();
    static void setBinDir(const char* path);

    virtual int32_t getSupportedLanguageNum();
    virtual const char* getLanguageName(int32_t lid);
    virtual int32_t getSupportedEncodingNum(int32_t lid);
    virtual const char* getEncodingName(int32_t lid, int32_t eid);
    virtual UnrarTreeNodePtr getTreeWithEncoding(int32_t& lid, int32_t& eid,
                                                 std::vector<UnrarElement> &elm);

protected:
    UnzipTask();
    UnzipTask(int id, const char* path, const char* pass);

    // 展開
    virtual void extract();
    virtual void extractFile(const char* aname, const char* ename, int64_t size);
    
    // タイプ文字列返却
    virtual const char* getType();
    
	// ファクトリ向けメソッド
    static const char* getTypeString();
    static bool isSupportedFile(const std::vector<std::string>& files);
    static TaskBase* newTaskObject(
		int id, const std::vector<std::string>& files, const char* pass);
    static TaskBase* newVacuityObject();

    // I18N向け
    bool detectEncoding(const char* path, int32_t& lid, int32_t& eid);
    UnrarTreeNodePtr createTree(const char* path, int32_t& lid, int32_t& eid,
                                std::vector<UnrarElement>& elm);

    // 外部プログラム実行用部品
    void escapeShellString(const char* src, char* dest, int length);
};
