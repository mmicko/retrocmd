#ifndef IMAGE_FILE_FORMAT_H
#define IMAGE_FILE_FORMAT_H

#include "format.h"

class image_file_format : public file_format
{
public:
	image_file_format() : file_format() {}
	virtual ~image_file_format() {}

	virtual uint32_t width() = 0;
	virtual uint32_t height() = 0;
	virtual uint8_t  bit_planes() = 0;
	virtual void load(file_definition& fd) = 0;

	uint8_t* data() { return  reinterpret_cast<uint8_t*>(m_image.data()); }
protected:
	std::vector<uint32_t> m_image;
};

class simple_image_file_format : public image_file_format
{
public:
	simple_image_file_format() : image_file_format(), m_width(0), m_height(0) { }

	virtual ~simple_image_file_format() {}

	void buffer_load(file_definition& fd, uint32_t width, uint32_t height);
	uint32_t width() override { return m_width; };
	uint32_t height() override { return m_height; };
	uint8_t  bit_planes() override { return 1; };
protected:
	uint32_t m_width;
	uint32_t m_height;
};


std::vector<std::unique_ptr<image_file_format>> get_image_formats();

#endif // IMAGE_FILE_FORMAT_H