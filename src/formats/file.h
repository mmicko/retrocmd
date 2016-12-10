#ifndef FILE_DEFINITION_H
#define FILE_DEFINITION_H
#include <memory>
#include <vector>
#include <fstream>

class file_definition
{
public:
	file_definition(std::string path);

	virtual ~file_definition() { if (m_stream) m_stream->close(); }
		
	bool is_open() const { return  m_stream->is_open(); }
	std::string name() const { return m_name; }
	std::string extension() const { return m_extension; }
	uint64_t filesize() const { return m_filesize; }
	std::vector<uint8_t> read(int size) const;
private:
	uint64_t calc_filesize() const;

	std::string m_path;
	std::string m_name;
	std::string m_extension;
	uint64_t    m_filesize;
	std::unique_ptr<std::ifstream> m_stream;
};

#endif // FILE_DEFINITION_H
