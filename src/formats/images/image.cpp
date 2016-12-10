#include "image.h"
#include "atarist/pic.h"
#include "atarist/big.h"

std::vector<std::unique_ptr<image_file_format>> get_image_formats()
{
	std::vector<std::unique_ptr<image_file_format>> retval;
	retval.push_back(std::make_unique<pic_atari_image_file_format>());
	retval.push_back(std::make_unique<big_atari_image_file_format>());
	return retval;;
}


void simple_image_file_format::buffer_load(file_definition& fd, uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	std::vector<uint8_t> data = fd.read(m_width / 8 * m_height);
	m_image.resize(m_width * m_height);
	long dst = 0;
	long src = 0;
	for (uint32_t y = 0; y < m_height; y++)
	{
		for (uint32_t x = 0; x < m_width / 8; x++)
		{
			for (int b = 7; b >= 0; b--)
			{
				m_image[dst] = (data[src] >> b) & 1 ? 0x000000ff : 0xffffffff;
				dst++;
			}
			src++;
		}
	}
}
