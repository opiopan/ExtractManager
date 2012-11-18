//----------------------------------------------------------------------
// Serializable.cpp
//   オブジェクトのシリアライズ・でシリアライズ機構
//----------------------------------------------------------------------
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "Serializable.h"

namespace HSL{ namespace Base {

//----------------------------------------------------------------------
// 64bit版ネットワークバイトオーダー変換
//----------------------------------------------------------------------
static inline int64_t hton_int64(int64_t v)
{
	static const char order[] = "\000\000\000\001";
	if (*reinterpret_cast<const int32_t*>(order) == 1){
		// big endian
		return v;
	}else{
		// little endian
		return 
			((v >> 56) & 0xff) |
			((v >> 40) & 0xff00) |
			((v >> 24) & 0xff0000) |
			((v >> 8)  & 0xff000000) |
			((v << 8)  & 0xff00000000) |
			((v << 24) & 0xff0000000000) |
			((v << 40) & 0xff000000000000) |
			((v << 56) & 0xff00000000000000);
	}
}

#define ntoh_int64 hton_int64

//----------------------------------------------------------------------
// OutputStream:
//   シリアライズ先ストリームの基底クラス
//----------------------------------------------------------------------
OutputStream::~OutputStream()
{
    flush();
}

void OutputStream::flush()
{
#ifdef BUFFERED_STREAM
	if (buffered > 0){
		write(buffer, buffered);
		buffered = 0;
	}
#endif
}

OutputStream& OutputStream::operator << (int64_t data)
{
	int64_t ndata = hton_int64(data);
	put(&ndata, sizeof(ndata));
	return *this;
}

OutputStream& OutputStream::operator << (int32_t data)
{
	int32_t ndata = htonl(data);
	put(&ndata, sizeof(ndata));
	return *this;
}

OutputStream& OutputStream::operator << (bool data)
{
	int32_t ndata = htonl(static_cast<int>(data));
	put(&ndata, sizeof(ndata));
	return *this;
}

OutputStream& OutputStream::operator << (const std::string& data)
{
	int32_t size = static_cast<int32_t>(data.length());
	*this << size;
    if (size > 0){
        put(data.data(), size);
    }
	return *this;
}

OutputStream& OutputStream::operator << (const char* data)
{
	int32_t size = static_cast<int32_t>(strlen(data));
	*this << size;
    if (size > 0){
        put(data, size);
    }
	return *this;
}

OutputStream& OutputStream::put(const void* data, int32_t size)
{
#ifdef BUFFERED_STREAM
	for (int32_t pos = 0; size > 0;){
		int32_t remain = sizeof(buffer) - buffered;
		int32_t putSize = size > remain ? remain : size;

	    memcpy(buffer + buffered, 
			   reinterpret_cast<const char*>(data) + pos, putSize);
		buffered += putSize;
		pos += putSize;
		size -= putSize;

		if (buffered >= sizeof(buffer)){
			flush();
		}
	}
#else
	write(data, size);
#endif
	return *this;
}

//----------------------------------------------------------------------
// class InputStream:
//   デシリアライズ元ストリームの基底クラス
//----------------------------------------------------------------------
InputStream::~InputStream()
{
};

InputStream& InputStream::operator >> (int64_t& data)
{
	int64_t ndata;
	get(&ndata, sizeof(ndata));
	data = ntoh_int64(ndata);
	return *this;
}

InputStream& InputStream::operator >> (int32_t& data)
{
	int32_t ndata;
	get(&ndata, sizeof(ndata));
	data = ntohl(ndata);
	return *this;
}

InputStream& InputStream::operator >> (bool& data)
{
	int32_t ndata;
	get(&ndata, sizeof(ndata));
	data = static_cast<bool>(ntohl(ndata));
	return *this;
}

InputStream& InputStream::operator >> (std::string& data)
{
	int32_t size, nsize;
	get(&nsize, sizeof(nsize));
	size = ntohl(nsize);

	data.resize(0);
	data.reserve(size);
	char tmp[128];
	for (int32_t pos = 0; pos < size;){
		int32_t remain = size - pos;
		int32_t append = remain > sizeof(tmp) ? sizeof(tmp) : remain;
		get(tmp, append);
		data.append(tmp, append);
		pos += append;
	}
    
	return *this;
}

InputStream& InputStream::get(void* data, int32_t size)
{
#ifdef BUFFERED_STREAM
	for (int32_t pos = 0; size > 0;){
		if (buffered == current){
			buffered = read(buffer, sizeof(buffer));
			current = 0;
			if (buffered == 0){
				Serializable::InvalidFormatException e;
				throw e;
			}
		}

		int32_t remain = buffered - current;
		int32_t getSize = size > remain ? remain : size;

	    memcpy(reinterpret_cast<char*>(data) + pos,
			   buffer + current, getSize);
		pos += getSize;
		current += getSize;
		size -= getSize;
	}
#else
	int32_t rsize = read(data, size);
	if (rsize < size){
		Serializable::InvalidFormatException e;
		throw e;
	}
#endif
	return *this;
}

//----------------------------------------------------------------------
// class MemoryOutputStream:
//   オンメモリのアウトプットストリーム実装
//----------------------------------------------------------------------
MemoryOutputStream::MemoryOutputStream()
	: data(NULL), max(0), length(0)
{
	max = STREAM_BUFFER_SIZE;
	length = 0;
	data = new char[max];
}

MemoryOutputStream::~MemoryOutputStream()
{
	delete[] data;
}

void MemoryOutputStream::write(const void* data, int32_t size)
{
	if (length + size > max){
		int64_t newmax = (length + size + STREAM_BUFFER_SIZE ) 
			/ STREAM_BUFFER_SIZE * STREAM_BUFFER_SIZE;
		char* newdata = new char[newmax];
		memcpy(newdata, this->data, length);
		delete[] this->data;
		this->data = newdata;
		max = newmax;
	}
	memcpy(this->data + length, data, size);
	length += size;
}

//----------------------------------------------------------------------
// class MemoryInputStream:
//   オンメモリのインプットストリーム実装
//----------------------------------------------------------------------
MemoryInputStream::~MemoryInputStream()
{
}

int32_t MemoryInputStream::read(void* data, int32_t size)
{
	int64_t remain = length - current;
	int64_t rc = remain < size ? remain : size;

	if (remain > 0){
		memcpy(data, reinterpret_cast<const char*>(this->data) + current, rc);
		current += rc;
	}

	return static_cast<int32_t>(rc);
}

//----------------------------------------------------------------------
// class FileOutputStream:
//   ファイルへのアウトプットストリーム実装
//----------------------------------------------------------------------
FileOutputStream::FileOutputStream(const char* path) : stream(NULL)
{
	stream = fopen(path, "w");
	if (stream == NULL){
		Serializable::OpenException e;
		throw e;
	}
}

FileOutputStream::~FileOutputStream()
{
	close();
}

void FileOutputStream::close()
{
	if (stream){
		if (fclose(stream) != 0){
			Serializable::WriteException e;
			throw e;
		}
	}
}

void FileOutputStream::write(const void* data, int32_t size)
{
	if (fwrite(data, size, 1, stream) != 1){
		Serializable::WriteException e;
		throw e;
	}
}

//----------------------------------------------------------------------
// class FileInputStream:
//   ファイルからのインプットストリーム実装
//----------------------------------------------------------------------
FileInputStream::FileInputStream(const char* path) : stream(NULL)
{
	stream = fopen(path, "r");
	if (stream == NULL){
		Serializable::OpenException e;
		throw e;
	}
}

FileInputStream::~FileInputStream()
{
	close();
}

void FileInputStream::close()
{
	if (stream){
		if (fclose(stream) != 0){
			Serializable::ReadException e;
			throw e;
		}
	}
}

int32_t FileInputStream::read(void* data, int32_t size)
{
	size_t rc = fread(data, 1, size, stream);
    /*
	if (rc < 0){
		Serializable::ReadException e;
		throw e;
	}
    */
	
	return static_cast<int32_t>(rc);
}

}} // end of namespace

