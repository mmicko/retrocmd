#include "file.h"
#include <algorithm>

#if defined(WIN32)
constexpr char PATHSEPCH = '\\';
#else
constexpr char PATHSEPCH = '/';
#endif

std::vector<uint8_t> file_definition::read(int size) const
{
	std::vector<uint8_t> buffer(size);
	m_stream->read(reinterpret_cast<char*>(buffer.data()), size);
	return buffer;
}

file_definition::file_definition(std::string path)
{
	m_path = path;
	m_stream = std::make_unique<std::ifstream>(path, std::ios::binary);		
	if (m_stream->is_open())
	{
		m_filesize = calc_filesize();
		m_extension.clear();
		std::string::size_type idx = path.rfind('.');
		if (idx != std::string::npos)
		{
			m_extension = path.substr(idx + 1);
		}
		std::transform(m_extension.begin(), m_extension.end(), m_extension.begin(), ::tolower);
		m_name = path;
		std::string::size_type pos = path.rfind(PATHSEPCH);
		if (pos != std::string::npos)
		{
			m_name = path.substr(pos + 1);
		}
	}
}

uint64_t file_definition::calc_filesize() const
{
	std::ifstream file(m_path, std::ios::binary);
	file.seekg(0, std::ios::end);
	uint64_t fsize = file.tellg();
	file.close();
	return fsize;
}
