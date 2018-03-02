#include <node.h>
#include <node_buffer.h>
#include <v8.h>
#include <nan.h>

#include <string.h>
#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef  _MSC_VER
	#define MMAP_FILE_WIN
	#include <Windows.h>
	
	const char * GetLastErrorAsString() {
        static char buf[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                buf, sizeof(buf), NULL);
        return buf;
    }
#else
	#include <unistd.h>
	#include <sys/mman.h>
#endif

using namespace v8;

char * create_map(const char * path, size_t size, bool readWrite) {
	char * shared = 0;
	
#ifdef MMAP_FILE_WIN
	HANDLE file = CreateFileA(path, 
		readWrite ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ, // what I want to do with it
		FILE_SHARE_READ | FILE_SHARE_WRITE, // what I want to allow others to do with it
		NULL, // change this to allow child processes to inherit the handle
		OPEN_ALWAYS, // how to proceed if file exists or does not exist: open if exists, create if it doesn't
		FILE_ATTRIBUTE_NORMAL, // any special attributes
		NULL
	);
	if (file == INVALID_HANDLE_VALUE) {
		Nan::ThrowError(GetLastErrorAsString()); 
		return 0;
	}

	
	HANDLE mmap_handle = CreateFileMappingA(file, 
		NULL, // change this to allow child processes to inherit the handle
		readWrite ? PAGE_READWRITE : PAGE_READONLY, // what I want to do with it
		0, size, // size to map
		NULL // name
	); 
	//mmap_handle = OpenFileMappingA(FILE_MAP_READ, FALSE, path);
	if (!mmap_handle) {
		CloseHandle(file);
		Nan::ThrowError(GetLastErrorAsString()); 
		return 0;
	}
	
	shared = (char *)MapViewOfFile(mmap_handle, readWrite ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, size);
	if (!shared) {
		CloseHandle(file);
		CloseHandle(mmap_handle);
		Nan::ThrowError(GetLastErrorAsString()); 
		return 0;
	}
	CloseHandle(file);
	CloseHandle(mmap_handle);	
		
#else
	// create:
	int fd = open(path, readWrite ? O_CREAT | O_RDWR : O_RDONLY, 0666); // 0666 or 0644 or 0600?
	if (fd == -1) {
		Nan::ThrowError("Error opening file for writing");
		return 0;
	}   
	// validate size
	struct stat fileInfo = {0};
	if (fstat(fd, &fileInfo) == -1) {
		Nan::ThrowError("Error getting the file size");
		return 0;
	}
	
	//printf("file %s is size %ji", path, (intmax_t)fileInfo.st_size);
	// stretch / verify size
	if (readWrite) {
		if (fileInfo.st_size < size) {
			if (lseek(fd, size-1, SEEK_SET) == -1 || write(fd, "", 1) == -1) {
				close(fd);
				Nan::ThrowError("Error stretching the file");
				return 0;
			}
			// update size
			if (fstat(fd, &fileInfo) == -1) {
				Nan::ThrowError("Error getting the file size");
				return 0;
			}
		}
	}
	//printf("file %s is size %ji\n", path, (intmax_t)fileInfo.st_size);
	
	if (fileInfo.st_size == 0) {
		Nan::ThrowError("file is empty");
		return 0;
	} else if (fileInfo.st_size < size) {
		Nan::ThrowError("file is too small");
		return 0;
	} 
	
	auto flag = PROT_READ;
	if (readWrite) flag |= PROT_WRITE;			
	shared = (char *)mmap(0, size, flag, MAP_SHARED, fd, 0);
	if (shared == MAP_FAILED) {
		close(fd);
		Nan::ThrowError("mmapping the file");
		return 0;
	}
	close(fd);
#endif
	
	return shared;
}

void bufFreeCallback(char *p, void *hint) {
	//printf("bye %p %d\n", p, size);
#ifdef MMAP_FILE_WIN
	UnmapViewOfFile(p);
#else
	munmap(p, (size_t)hint);
#endif	
}

/*
// sync writes changes to disk
bool sync(void * shared, size_t size) {
	if (!shared) return true;

#ifdef MMAP_FILE_WIN 
		if (!FlushViewOfFile(shared, 0)) {
			console.error(GetLastErrorAsString()); 
			return false;
		}
#else
		if (msync(shared, size), MS_SYNC) == -1) {
			console.error("Could not sync the file to disk");
			return false;
		}
#endif 
	return true;
}
*/

// "buf.txt" [, "r"]
// "buf.txt", "r+"
NAN_METHOD(openSync) {
	
	bool readWrite=false;
	
	// args:
	String::Utf8Value path(info[0]);
	if (strlen(*path) <= 0) {
		Nan::ThrowError("first argument must be a non-empty string for the file path");
	}
	
	size_t size = (size_t)info[1]->Uint32Value();
	if (size <= 0) {
		Nan::ThrowError("second argument must be an integer size in bytes");
	}
	
	String::Utf8Value flags(info[2]);
	if (info.Length() > 1 && strcmp(*flags, "r+") == 0) {
		readWrite = true;
	}
	
	// map here:
	//char * p = new char[size];
	char * p = create_map(*path, size, readWrite);
	if (readWrite) p[0] = 'y';
	
	
	//printf("path %s %d size %d rw %d p %p\n", *path, path.length(), (int)size, readWrite, p);
	
	Nan::MaybeLocal<v8::Object> nanbuf = Nan::NewBuffer(p, size, bufFreeCallback, (void *)size);
	
	info.GetReturnValue().Set(nanbuf.ToLocalChecked());
	//info.GetReturnValue().Set(Nan::New("world").ToLocalChecked());
}

NAN_MODULE_INIT(InitAll) {
	NAN_EXPORT(target, openSync);
}

NODE_MODULE(mmapfile, InitAll)