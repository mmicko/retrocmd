#ifndef FILE_FORMAT_H
#define FILE_FORMAT_H

#include "file.h"

class file_format
{
public:
	file_format() {}
	virtual ~file_format() {}

	virtual int detect(file_definition& fd) = 0;
	virtual std::string name() = 0;
};

#endif // FILE_FORMAT_H