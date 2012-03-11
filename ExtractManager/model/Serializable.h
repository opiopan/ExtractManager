//----------------------------------------------------------------------
// Serializable.h
//   オブジェクトのシリアライズ・デシリアライズ機構
//----------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <string>
#include <stdio.h>

//#define BUFFERED_STREAM
#define STREAM_BUFFER_SIZE (32*1024)

namespace HSL{ namespace Base {

class OutputStream;
class InputStream;

//----------------------------------------------------------------------
// class Serializable: 
//   シリアライズ可能オブジェクトが実装すべきインタフェース
//----------------------------------------------------------------------
class Serializable{
public:
	class SerializeException{};
    class OpenException: public SerializeException{};
    class WriteException: public SerializeException{};
    class ReadException: public SerializeException{};
    class InvalidFormatException: public SerializeException{};

    virtual void serialize(HSL::Base::OutputStream& s) = 0;
    virtual void deserialize(HSL::Base::InputStream& s) = 0;
};

//----------------------------------------------------------------------
// class OutputStream:
//   シリアライズ先ストリームの基底クラス
//----------------------------------------------------------------------
class OutputStream{
private:
#ifdef BUFFERED_STREAM
    char buffer[STREAM_BUFFER_SIZE];
    int  buffered;
#endif
    
public:
#ifdef BUFFERED_STREAM
    OutputStream():buffered(0){};
#else
    OutputStream(){};
#endif
    virtual ~OutputStream();
    
    void flush();
    
    OutputStream& operator << (int64_t data);
    OutputStream& operator << (int32_t data);
    OutputStream& operator << (bool data);
    OutputStream& operator << (const std::string& data);
	OutputStream& operator << (const char* data);
    OutputStream& put(const void* data, int32_t size);

protected:
    virtual void write(const void* data, int32_t size) = 0;
};

//----------------------------------------------------------------------
// class InputStream:
//   デシリアライズ元ストリームの基底クラス
//----------------------------------------------------------------------
class InputStream{
private:
#ifdef BUFFERED_STREAM
    char buffer[STREAM_BUFFER_SIZE];
    int  buffered;
    int  current;
#endif

public:
#ifdef BUFFERED_STREAM
    InputStream():buffered(0), current(0){};
#else
    InputStream(){};
#endif
    virtual ~InputStream();

    InputStream& operator >> (int64_t& data);
    InputStream& operator >> (int32_t& data);
    InputStream& operator >> (bool& data);
    InputStream& operator >> (std::string& data);
    InputStream& get(void* data, int32_t size);

protected:
    virtual int32_t read(void* data, int32_t size) = 0;
};

//----------------------------------------------------------------------
// class MemoryOutputStream / class MemoryInputStream:
//   オンメモリのストリーム実装
//----------------------------------------------------------------------
class MemoryOutputStream : public HSL::Base::OutputStream{
protected:
    char*   data;
    int64_t max;
    int64_t length;

public:
    MemoryOutputStream();
    virtual ~MemoryOutputStream();
    int64_t getLength(){flush(); return length;};
    void* getData(){flush(); return data;};

protected:
    virtual void write(const void* data, int32_t size);
};

class MemoryInputStream : public HSL::Base::InputStream{
protected:
    const void*   data;
    const int64_t length;
    int64_t       current;

public:
    MemoryInputStream(const void* data, int64_t length) :
	data(data), length(length), current(0){};
    virtual ~MemoryInputStream();

protected:
    virtual int32_t read(void* data, int32_t size);
};

//----------------------------------------------------------------------
// class FileOutputStream / class FileInputStream:
//   ファイルストリーム実装
//----------------------------------------------------------------------
class FileOutputStream : public HSL::Base::OutputStream{
protected:
	FILE* stream;

public:
	FileOutputStream(const char* path);
	virtual ~FileOutputStream();
	void close();

protected:
    virtual void write(const void* data, int32_t size);
};

class FileInputStream : public HSL::Base::InputStream{
protected:
	FILE* stream;

public:
    FileInputStream(const char* path);
    virtual ~FileInputStream();
	void close();

protected:
    virtual int32_t read(void* data, int32_t size);
};

}} // end of namespace

using namespace HSL::Base;
