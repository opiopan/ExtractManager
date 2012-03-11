#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unrar.h>
#include "SmartPtr.h"

namespace HSL{ namespace Util{

//----------------------------------------------------------------------
// 例外
//----------------------------------------------------------------------
class RarArchiveException{
protected:
    int	code;
    RarArchiveException(int c) : code(c){};
public:
    virtual ~RarArchiveException();
    int getErrorCode(){ return code; };
    virtual const char* getErrorString() = 0;
};
typedef SmartPtr<RarArchiveException> RarArchiveExceptionPtr;

class RarOpenException : public RarArchiveException{
public:
    RarOpenException(int c) : RarArchiveException(c){};
    virtual ~RarOpenException();
    virtual const char* getErrorString();
};

class RarProcessException : public RarArchiveException{
public:
    RarProcessException(int c) : RarArchiveException(c){};
    virtual ~RarProcessException();
    virtual const char* getErrorString();
};

class RarReadHeaderException : public RarArchiveException{
public:
    RarReadHeaderException(int c) : RarArchiveException(c){};
    virtual ~RarReadHeaderException();
    virtual const char* getErrorString();
};

class RarOtherException : public RarArchiveException{
protected:
    std::string message;
public:
    RarOtherException(const char* msg, int c = 0) 
	: RarArchiveException(c), message(msg){};
    virtual ~RarOtherException();
    virtual const char* getErrorString();
};

#define RARERROR_NOPASSWORD -99

//----------------------------------------------------------------------
// アーカイブ要素クラス
//----------------------------------------------------------------------
class RarElement{
public:
    virtual ~RarElement();
    virtual const char* getName() const = 0;
    virtual int64_t	getSize() const = 0;
    virtual bool getIgnoreState() const = 0;
    virtual void setIgnore(bool) = 0;
    virtual const char* getExtractName() const = 0;
    virtual void setExtractName(const char*) = 0;
};
typedef SmartPtr<RarElement> RarElementPtr;

//----------------------------------------------------------------------
// Rarアーカイブ抽象化クラス
//----------------------------------------------------------------------
typedef int (*RarProgressNotifyFunc)(const char* volname, int size, void* data);

class RarArchive
{
protected:
    bool                        initializing;
    std::string                 archivePath;
    std::vector<std::string>	volumes;
    std::string                 comment;
    std::vector<RarElementPtr>	elements;
    std::string                 password;
    std::string                 baseDir;
    std::string                 currentVolume;
    RarArchiveExceptionPtr      callbackException;
    RarProgressNotifyFunc       notifyFunc;
    void*                       notifyFuncData;

protected:
    // UnRar.dllの低レベルカプセル化クラス
    class RarHandle : public RAROpenArchiveDataEx{
    protected:
	HANDLE openHandle;
    public:
	RarHandle(RarArchive* arc, unsigned int mode);
	~RarHandle();
	
	int readHeader(RARHeaderDataEx *header);
	int skip();
	int test();
	int extract(const char* destPath = NULL, const char* destName = NULL);
	int close();
    };

public:
    RarArchive(const char* path, const char* pass = NULL);
    virtual ~RarArchive(void);

    virtual void extract();

    inline void setPassword(const char* pass)
    {
	password = pass;
    };

    inline const char* getPassword() const
    {
	return password.c_str();
    };

    inline void setBaseDir(const char* bdir)
    {
	baseDir = bdir;
    };
	
    inline const char* getArchivePath() const
    {
	return archivePath.c_str();
    };
    inline size_t getVolumeNum() const
    {
	return volumes.size();
    };
    inline const char* getVolumeName(size_t index) const
    {
	return volumes.at(index).c_str();
    };
    inline const char* getComment() const
    {
	return comment.c_str();
    };
    inline size_t getElementNum() const
    {
	return elements.size();
    };
    inline RarElement& getElement(size_t index) const
    {
	return *(elements.at(index).operator ->());
    };
    inline void setNotifyFunc(RarProgressNotifyFunc func, void* data)
    {
	notifyFunc = func;
	notifyFuncData = data;
    };

protected:
    int onUnrarEvent(UINT msg, LPARAM P1, LPARAM P2);
    static int CALLBACK callbackProc(UINT msg, LPARAM userData, LPARAM P1, LPARAM P2);
};

};}; // end of namespace

using namespace HSL::Util;
