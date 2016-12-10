#ifndef PIC_ATARI_IMAGE_FILE_FORMAT_H
#define PIC_ATARI_IMAGE_FILE_FORMAT_H

#include "images/image.h"

class pic_atari_image_file_format : public simple_image_file_format
{
public:
	pic_atari_image_file_format() : simple_image_file_format() {}
	virtual ~pic_atari_image_file_format() {}

	int detect(file_definition& fd) override { return fd.extension()=="pic" && fd.filesize()==32000 ? 100 : 0; };
	std::string name() override { return "PIC - Atari ST"; }
	void load(file_definition& fd) override { buffer_load(fd, 640, 400); }
};

#endif // PIC_ATARI_IMAGE_FILE_FORMAT_H
