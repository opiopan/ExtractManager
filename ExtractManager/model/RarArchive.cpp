#include <stdlib.h>
#include "RarArchive.h"
#include "SmartPtr.h"

using namespace std;

const char* wstr2str(const wchar_t* src);
const wchar_t* str2wstr(const char* src);

//----------------------------------------------------------------------
// wchar_t* <-> char* 変換
//----------------------------------------------------------------------
const char* wstr2str(const wchar_t* src)
{
    static char dest[8192 + 1];

    wcstombs(dest, src, sizeof(dest));
    return dest;
}

const wchar_t* str2wstr(const char* src)
{
    static wchar_t dest[1024 + 1];

    mbstowcs(dest, src, sizeof(dest) / sizeof(wchar_t));
    return dest;
}


//----------------------------------------------------------------------
// 例外
//----------------------------------------------------------------------
RarArchiveException::~RarArchiveException(){}
const char* RarArchiveException::getAdditionalString(){return "";}

RarOpenException::~RarOpenException(){}
const char* RarOpenException::getErrorString()
{
    switch(code){
    case ERAR_NO_MEMORY:
        return "Not enough memory to initialize data structures";
    case ERAR_BAD_DATA:
        return "Archive header broken";
    case ERAR_UNKNOWN_FORMAT:
        return "Unknown encryption used for archive headers";
    case ERAR_EOPEN:
        return "File open error";
    }
    return "";
}

RarProcessException::~RarProcessException(){}
const char* RarProcessException::getErrorString()
{
    switch(code){
    case ERAR_UNKNOWN_FORMAT:
        return "Unknown archive format";
    case ERAR_BAD_ARCHIVE:
        return "Bad volume";
    case ERAR_ECREATE:
        return "File create error";
    case ERAR_EOPEN:
        return "Volume open error";
    case ERAR_ECLOSE:
        return "File close error";
    case ERAR_EREAD:
        return "Read error";
    case ERAR_EWRITE:
        return "Write error";
    case ERAR_BAD_DATA:
        return "CRC error";
    case ERAR_UNKNOWN:
        return "Unknown error";
    case ERAR_MISSING_PASSWORD:
        return "Password for encrypted file is not specified";
    }
    return "";
}

RarReadHeaderException::~RarReadHeaderException(){}
const char* RarReadHeaderException::getErrorString()
{
    switch(code){
    case ERAR_END_ARCHIVE:
        return "End of archive";
    case ERAR_BAD_DATA:
        return "File header broken";
    }
    return "";
}

RarOtherException::~RarOtherException(){}
const char* RarOtherException::getErrorString()
{
    return message.c_str();
}
const char* RarOtherException::getAdditionalString()
{
    return extension.c_str();
}


//----------------------------------------------------------------------
// UnRar.dllの低レベルカプセル化クラス
//----------------------------------------------------------------------
static const int64_t COMMENT_BUF_SIZE = 1024 * 1024;
RarArchive::RarHandle::RarHandle(RarArchive* arc, unsigned int mode) :
    openHandle(NULL)
{
    //　オープンパラメタ設定
    RAROpenArchiveDataEx* param = static_cast<RAROpenArchiveDataEx*>(this);
    memset(param, 0, sizeof(*param));
    ArcName	= const_cast<char*>(arc->getArchivePath());
    OpenMode	= mode;
    CmtBufSize	= COMMENT_BUF_SIZE;
    CmtBuf		= new char[CmtBufSize];
    memset(CmtBuf, 0, CmtBufSize);
    Callback	= RarArchive::callbackProc;
    UserData	= reinterpret_cast<LPARAM>(arc);
    
    //　アーカイブをオープン
    openHandle = RAROpenArchiveEx(this);
}

RarArchive::RarHandle::~RarHandle()
{
    close();
    delete[] CmtBuf;
}

int RarArchive::RarHandle::readHeader(RARHeaderDataEx *header)
{
    return RARReadHeaderEx(openHandle, header);
}

int RarArchive::RarHandle::skip()
{
    return RARProcessFileW(openHandle, RAR_SKIP, NULL, NULL);
}

int RarArchive::RarHandle::test()
{
    return RARProcessFileW(openHandle, RAR_TEST, NULL, NULL);
}

int RarArchive::RarHandle::extract(const char* destPath, const char* destName)
{
    return RARProcessFile(
	openHandle, RAR_EXTRACT, 
	const_cast<char*>(destPath), const_cast<char*>(destName));
}


int RarArchive::RarHandle::close()
{
    int rc = 0;
    if (openHandle){
	rc = RARCloseArchive(openHandle);
	openHandle = NULL;
    }
    return rc;
}


//----------------------------------------------------------------------
// アーカイブクラス要素実装
//----------------------------------------------------------------------
namespace HSL{ namespace Util{
class RarElementImp : public RarElement{
protected:
    std::string	name;
    int64_t			size;
    bool			isIgnore;
    bool			isUpdateTimestamp;
    std::string	extractName;
public:
    RarElementImp(const char* nm, int64_t sz) :
	name(nm), size(sz), isIgnore(false){};
    virtual ~RarElementImp();
    virtual const char* getName() const;
    virtual int64_t getSize() const;
    virtual bool getIgnoreState() const;
    virtual void setIgnore(bool);
    virtual const char* getExtractName() const;
    virtual void setExtractName(const char*);

};

RarElement::~RarElement(){}

RarElementImp::~RarElementImp(){}

const char* RarElementImp::getName() const
{
    return name.c_str();
}

int64_t RarElementImp::getSize() const
{
    return size;
}

bool RarElementImp::getIgnoreState() const
{
    return isIgnore;
}

void RarElementImp::setIgnore(bool ignore)
{
    isIgnore = ignore;
}

const char* RarElementImp::getExtractName() const
{
    return extractName.c_str();
}

void RarElementImp::setExtractName(const char* ename)
{
    extractName = ename;
}

}} // end of namespace

//----------------------------------------------------------------------
// コンストラクタ・デストラクタ
//----------------------------------------------------------------------
RarArchive::RarArchive(const char* path, const char* pass) :
    initializing(true), 
    archivePath(path), 
    notifyFunc(NULL)
{
    if (pass){
	password = pass;
    }
    volumes.insert(volumes.end(), archivePath);
    RarHandle rar(this, RAR_OM_LIST);
    if (rar.OpenResult == 0){
	if (rar.CmtState == 1){
	    //CComBSTR cmt(rar.CmtSize, rar.CmtBuf);
	    //comment = cmt.m_str;
	    comment.assign(rar.CmtBuf, rar.CmtSize);
	}
	RARHeaderDataEx header;
	memset(&header, 0, sizeof(header));
	int rc = 0;
	while ((rc = rar.readHeader(&header)) == 0){
	    int64_t fileSize = 
		(static_cast<int64_t>(header.UnpSizeHigh) << 32) + 
		static_cast<int64_t>(header.UnpSize);
	    if (fileSize != 0){
		RarElementPtr eptr(
		    new RarElementImp(
			wstr2str(header.FileNameW), 
			fileSize));
		elements.insert(elements.end(), eptr);
	    }
	    callbackException.setNull();
	    rc  = rar.skip();
	    if (rc != 0){
		throw callbackException.isNull() ? 
		    RarArchiveExceptionPtr(new RarProcessException(rc)) :
		    callbackException;
	    }
	}
	if (rc != ERAR_END_ARCHIVE){
	    throw callbackException.isNull() ? 
		RarArchiveExceptionPtr(new RarReadHeaderException(rc)) :
		callbackException;
	}
    }else{
	throw callbackException.isNull() ? 
	    RarArchiveExceptionPtr(new RarOpenException(rar.OpenResult)) :
	    callbackException;
    }
    
    initializing = false;
    
}

RarArchive::~RarArchive(void){
}

//----------------------------------------------------------------------
// アーカイブ展開
//----------------------------------------------------------------------
void RarArchive::extract()
{
    RarHandle rar(this, RAR_OM_EXTRACT);
    if (rar.OpenResult == 0){
	RARHeaderDataEx header;
	memset(&header, 0, sizeof(header));
	int rc = 0;
	while ((rc = rar.readHeader(&header)) == 0){
	    bool skip = false;
	    RarElementPtr eptr(NULL);
	    
	    // サイズ0のエントリはスキップ
	    int64_t fileSize = 
		(static_cast<int64_t>(header.UnpSizeHigh) << 32) + 
		static_cast<int64_t>(header.UnpSize);
	    if (fileSize == 0){
		skip = true;
	    }else{
		// 対応するエントリオブジェクトを特定
		for (size_t i = 0; i < elements.size(); i++){
		    if (strcmp(elements[i]->getName(), 
			       wstr2str(header.FileNameW)) == 0){	
			eptr = elements[i];
			break;
		    }
		}
		if (eptr.isNull()){
		    throw RarArchiveExceptionPtr(
			new RarOtherException(
			    "Archive file might be corrupted.", ""));
		}
		if (eptr->getIgnoreState()){
		    skip = true;
		}
	    }
	    
	    // 展開
	    callbackException.setNull();
        currentVolume = volumes[0];
	    if (skip){
		rc = rar.skip();
	    }else{
		rc  = rar.extract(NULL, eptr->getExtractName());
	    }
	    if (rc != 0 || !callbackException.isNull()){
		throw callbackException.isNull() ? 
		    RarArchiveExceptionPtr(new RarProcessException(rc)) :
		    callbackException;
	    }
	}
	if (rc != ERAR_END_ARCHIVE){
	    throw callbackException.isNull() ? 
		RarArchiveExceptionPtr(new RarReadHeaderException(rc)) :
		callbackException;
	}
    }else{
	throw RarArchiveExceptionPtr(new RarOpenException(rar.OpenResult));
    }
}

//----------------------------------------------------------------------
// UnRar.dll イベント処理メソッド
//----------------------------------------------------------------------
int RarArchive::onUnrarEvent(UINT msg, LPARAM P1, LPARAM P2)
{
    switch (msg){
    case UCM_CHANGEVOLUME: {
        const char* nextvol= reinterpret_cast<const char*>(P1);
        currentVolume = nextvol;
        if (P2 == RAR_VOL_ASK){
            const char* msg = "A part of the archive volumes is not found: ";
            callbackException =
                RarArchiveExceptionPtr(new RarOtherException(msg, nextvol));
            return -1;
        }else if (P2 == RAR_VOL_NOTIFY){
            volumes.insert(volumes.end(), string(nextvol));
            return 0;
        }
    }
    case UCM_PROCESSDATA:
        if (notifyFunc){
            int rc = notifyFunc(currentVolume.c_str(), static_cast<int>(P2), 
								notifyFuncData);
            if (rc != 0){
                const char* msg = "Extracting has been canceled.";
                callbackException =
                    RarArchiveExceptionPtr(new RarOtherException(msg, ""));
            }
            return rc;
        }
        return 0;
    case UCM_NEEDPASSWORD:
        if (password.length() == 0){
            const char* msg = "Password is nessesary";
            callbackException =
            RarArchiveExceptionPtr(
                new RarOtherException(msg, "", RARERROR_NOPASSWORD));
            return -1;
        }
        char* buf = reinterpret_cast<char*>(P1);
        strncpy(buf, password.c_str(), P2);
        return 0;
    }
    return 0;
}

//----------------------------------------------------------------------
// UnRar.dll向けコールバック関数
//----------------------------------------------------------------------
int CALLBACK RarArchive::callbackProc(UINT msg, LPARAM userData,
				      LPARAM P1, LPARAM P2)
{
    RarArchive* own = reinterpret_cast<RarArchive*>(userData);
    
    try{
        return own->onUnrarEvent(msg, P1, P2);
    }catch (std::bad_alloc){
        own->callbackException = 
            RarArchiveExceptionPtr(new RarOtherException("Not enough memory", ""));
        return -1;
    }
}

