#ifndef BIG_ATARI_IMAGE_FILE_FORMAT_H
#define BIG_ATARI_IMAGE_FILE_FORMAT_H

#include "images/image.h"

class big_atari_image_file_format : public simple_image_file_format
{
public:
	big_atari_image_file_format() : simple_image_file_format() {}
	virtual ~big_atari_image_file_format() {}

	int detect(file_definition& fd) override { return fd.extension() == "big" && fd.filesize() == 64000 ? 100 : 0; };
	std::string name() override { return "BIG - Atari ST"; }
	void load(file_definition& fd) override { buffer_load(fd, 640, 800); }
};

#endif // BIG_ATARI_IMAGE_FILE_FORMAT_H
