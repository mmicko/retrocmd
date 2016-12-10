#include <vector>
#include <chrono>
#include <algorithm>
#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"
#include "entry/cmd.h"
#include "entry/input.h"
#include <dirent.h> 
#include "imgui/imgui.h"
#include "nanovg/nanovg.h"
#include "images/image.h"

#include <array>
#include <memory>


#if defined(WIN32)
constexpr char PATHSEPCH = '\\';
#else
constexpr char PATHSEPCH = '/';
#include <cstdlib>
#include <cerrno>
#include <memory>
#include <unistd.h>
#endif

#if defined(__GNUC__)
#define ATTR_PRINTF(x,y)        __attribute__((format(printf, x, y)))
#else
#define ATTR_PRINTF(x,y)
#endif

bool get_full_path(std::string &dst, std::string const &path)
{
	try
	{
#if defined(WIN32)
		std::vector<char> path_buffer(MAX_PATH);
		if (::_fullpath(&path_buffer[0], path.c_str(), MAX_PATH))
		{
			dst = &path_buffer[0];
			return true;
		}
		else
		{
			return false;
		}
#else
		std::unique_ptr<char, void(*)(void *)> canonical(::realpath(path.c_str(), nullptr), &std::free);
		if (canonical)
		{
			dst = canonical.get();
			return true;
		}

		std::vector<char> path_buffer(PATH_MAX);
		if (::realpath(path.c_str(), &path_buffer[0]))
		{
			dst = &path_buffer[0];
			return true;
		}
		else if (path[0] == PATHSEPCH)
		{
			dst = path;
			return true;
		}
		else
		{
			while (!::getcwd(&path_buffer[0], path_buffer.size()))
			{
				if (errno != ERANGE)
					return false;
				else
					path_buffer.resize(path_buffer.size() * 2);
			}
			dst.assign(&path_buffer[0]).push_back(PATHSEPCH);
			dst.append(path);
			return true;
		}
#endif
	}
	catch (...)
	{
		return false;
	}
}

typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint_type;
typedef std::chrono::high_resolution_clock clock_type;

struct fileData
{
	std::string name;
	std::string fullpath;
	bool isDirectory;
};

class mainapp : public entry::AppI
{
public:
	mainapp(): m_panel(0), m_width(0), m_height(0), m_debug(0), m_reset(0), m_text_width(0), m_text_height(0), m_nvg(nullptr), m_image_handle(0), m_image_width(0), m_image_height(0), m_screen(0)
	{
	}

	void init(int _argc, char** _argv) override;
	int shutdown() override;
	bool update() override;	

	void updateFolder(std::vector<fileData> &filelist, std::string path);
	void clearBuffer();
	void bufferPrintf(uint16_t x, uint16_t y, uint8_t attrib, char const *format, ...) ATTR_PRINTF(5, 6);
	void bufferAttrib(uint16_t x, uint16_t y, uint8_t attrib, uint16_t w);
	void checkKeyPress();
	void keypressed(entry::Key::Enum key);
	void displayPanel(int num, uint16_t posx, uint16_t width, uint16_t height);
private:
	std::vector<fileData> m_filelist[2];
	std::string m_path[2];
	uint16_t m_selected[2];
	
	uint8_t m_panel;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;

	entry::MouseState m_mouseState;

	uint16_t m_text_width;
	uint16_t m_text_height;

	std::vector<uint8_t> m_buffer;
	char m_temp_buffer[8192];

	bool m_keyState[entry::Key::Count];
	timepoint_type m_keyTimePress[entry::Key::Count];
	clock_type m_clock;
	NVGcontext* m_nvg;

	int m_image_handle;
	uint32_t m_image_width;
	uint32_t m_image_height;

	int m_screen;
};

ENTRY_IMPLEMENT_MAIN(mainapp);


void mainapp::init(int _argc, char** _argv)
{
	Args args(_argc, _argv);

	m_width  = 1280;
	m_height = 720;
	m_debug  = BGFX_DEBUG_TEXT;
	m_reset  = BGFX_RESET_NONE;
	m_selected[0] = 0;
	m_selected[1] = 0;
	m_panel = 0;

	// Disable commands from BGFX
	cmdShutdown();
	cmdInit();

	bgfx::init(args.m_type, args.m_pciId);
	bgfx::reset(m_width, m_height, m_reset);

	// Enable debug text.
	bgfx::setDebug(m_debug);

	// Set view 0 clear state.
	bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x000000ff
			, 1.0f
			, 0
			);

	for (auto& item : m_keyState)
		item = false;

	get_full_path(m_path[0], ".");
	get_full_path(m_path[1], ".");
	updateFolder(m_filelist[0],m_path[0]);
	updateFolder(m_filelist[1],m_path[1]);
	
	entry::WindowHandle defaultWindow = { 0 };
	setWindowTitle(defaultWindow, "Retro Commander");

	imguiCreate();
	m_nvg = nvgCreate(1, 0);

	if (_argc == 2) {
		m_screen = 1;
		file_definition fd(_argv[1]);
		if (fd.is_open()) {
			for (auto &format : get_image_formats())
			{
				if (format->detect(fd) == 100)
				{
					format->load(fd);
					m_image_width = format->width();
					m_image_height = format->height();
					m_image_handle = nvgCreateImageRGBA(m_nvg, m_image_width, m_image_height, 0, format->data());
					break;
				}
			}
		}
	}
	bgfx::setViewSeq(0, true);
}

int mainapp::shutdown()
{
	if (m_image_handle) nvgDeleteImage(m_nvg, m_image_handle);
	imguiDestroy();

	nvgDelete(m_nvg);
	// Shutdown bgfx.
	bgfx::shutdown();
	return 0;
}


void mainapp::bufferPrintf(uint16_t x, uint16_t y, uint8_t attrib, const char *format, ...)
{
	va_list args;
	/* format the message */
	va_start(args, format);
	uint32_t num = vsnprintf(m_temp_buffer, sizeof(m_temp_buffer), format, args);
	va_end(args);

	if (x < m_text_width && y < m_text_height)
	{
		uint32_t pos = (y*m_text_width + x) * 2;
		for (uint32_t ii = 0, xx = x; ii < num && xx < m_text_width; ++ii, ++xx)
		{
			m_buffer[pos++] = m_temp_buffer[ii];
			m_buffer[pos++] = attrib;
		}
	}
}

void mainapp::bufferAttrib(uint16_t x, uint16_t y, uint8_t attrib, uint16_t w)
{
	if (x < m_text_width && y < m_text_height)
	{
		uint32_t pos = (y*m_text_width + x) * 2  + 1;
		for (uint32_t ii = 0, xx = x; ii < w && xx < m_text_width; ++ii, ++xx)
		{
			m_buffer[pos++] = attrib;
			pos++;
		}
	}
}

void mainapp::updateFolder(std::vector<fileData> &filelist, std::string path)
{
	filelist.clear();
	DIR* dir = opendir(path.c_str());
	if (NULL != dir)
	{
		for (dirent* item = readdir(dir); NULL != item; item = readdir(dir))
		{
			fileData fd;
			fd.name = std::string(item->d_name);
			fd.fullpath = path + PATHSEPCH + fd.name;
			fd.isDirectory = (item->d_type & DT_DIR)!=0;
			if (fd.name!=".") filelist.push_back(fd);
		}
		closedir(dir);
	}
	std::sort(filelist.begin(), filelist.end(), [](fileData a, fileData b) {
        return a.name < b.name;   
    });	
}

void mainapp::clearBuffer()
{
	const bgfx::Stats* stats = bgfx::getStats();
	if (m_text_width != stats->textWidth || m_text_height != stats->textHeight)
	{
		m_text_width = stats->textWidth;
		m_text_height = stats->textHeight;
		m_buffer.resize(m_text_width*m_text_height * 2);		
	}
	std::fill(m_buffer.begin(), m_buffer.end(), 0x00);
}

void mainapp::keypressed(entry::Key::Enum key)
{
	if (key == entry::Key::Up) {
		if (m_selected[m_panel] > 0) m_selected[m_panel]--;
	}
	if (key == entry::Key::Down) {
		if (m_selected[m_panel] < m_filelist[m_panel].size()-1 && m_filelist[m_panel].size()> 0) m_selected[m_panel]++;
	}
	if (key == entry::Key::Home) {
		m_selected[m_panel] = 0;
	}
	if (key == entry::Key::End) {
		if (m_filelist[m_panel].size() > 0) m_selected[m_panel] = uint16_t(m_filelist[m_panel].size() - 1); else m_selected[m_panel] = 0;
	}
	if (key == entry::Key::PageUp) {
		if (m_selected[m_panel] > 10) m_selected[m_panel] -= 10; else m_selected[m_panel] = 0;
	}
	if (key == entry::Key::PageDown) {
		m_selected[m_panel] += 10;
		if (m_filelist[m_panel].empty()) m_selected[m_panel] = 0;
		if (m_selected[m_panel] > m_filelist[m_panel].size()-1) m_selected[m_panel] = uint16_t(m_filelist[m_panel].size() - 1);
	}
	if (key == entry::Key::Return) {
		if (m_filelist[m_panel].size()> 0 && m_filelist[m_panel][m_selected[m_panel]].isDirectory)
		{
			get_full_path(m_path[m_panel], m_filelist[m_panel][m_selected[m_panel]].fullpath);
			updateFolder(m_filelist[m_panel],m_path[m_panel]);
			
			m_selected[m_panel] = 0;
		}
	}
	if (key == entry::Key::Tab) {
		m_panel = m_panel == 1 ? 0 : 1;
	}
}

void mainapp::checkKeyPress()
{
	for (int32_t ii = 0; ii < int32_t(entry::Key::Count); ++ii)
	{
		bool oldpressed = m_keyState[ii];
		bool pressed = inputGetKeyState(entry::Key::Enum(ii));
		if (oldpressed == false && pressed == true) { // Key pressed
			m_keyTimePress[ii] = m_clock.now();
			m_keyState[ii] = true;
		}
		if (oldpressed == true && pressed == true) { // Key being pressed
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(m_clock.now() - m_keyTimePress[ii]);
			if (elapsed.count() > 150) { // 150ms
				m_keyState[ii] = false;
				keypressed(entry::Key::Enum(ii));
			}
		}
		if (oldpressed == true && pressed == false) { // Key released
			m_keyState[ii] = false;
			keypressed(entry::Key::Enum(ii));
		}
	}
}

void mainapp::displayPanel(int num, uint16_t posx, uint16_t width, uint16_t height)
{
	int posy = 1;
	bufferPrintf(posx, 0, 0x0f, "%s", m_path[num].c_str());
	int startndx = 0;
	int endndx = std::min(uint16_t(m_filelist[num].size()),height);
	if (m_selected[num] > height) {
		startndx = m_selected[num] - height;
		endndx -= height;
	}	
	for (int i=startndx;i<endndx;i++)
	{
		auto entry = m_filelist[num][i];
		bufferPrintf(posx, posy, 0x0f, "%s", entry.name.c_str());
		if (entry.isDirectory)
			bufferPrintf(posx+ width - 7, posy, 0x03, "<dir>");
		posy++;
	}
	if (m_panel == num) bufferAttrib(posx, m_selected[num] + 1, 0x40, width);
}

bool mainapp::update()
{
	checkKeyPress();
	if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
	{
		bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height) );
		bgfx::touch(0);
		switch(m_screen)
		{
		default:
		case 0:
			clearBuffer();
			displayPanel(0, 0, m_text_width / 2, m_text_height - 2);
			displayPanel(1, m_text_width / 2, m_text_width / 2, m_text_height - 2);
			bgfx::dbgTextImage(0, 0, m_text_width, m_text_height, m_buffer.data(), m_text_width * 2);
			break;
		case 1:
			nvgBeginFrame(m_nvg, m_width, m_height, 1.0f);

			struct NVGpaint  imgPaint = nvgImagePattern(m_nvg, 0, 0, float(m_image_width), float(m_image_height), 0.0f / 180.0f*NVG_PI, m_image_handle, 1.0f);
			nvgBeginPath(m_nvg);
			nvgRect(m_nvg, 0, 0, float(m_image_width), float(m_image_height));
			nvgFillPaint(m_nvg, imgPaint);
			nvgFill(m_nvg);
			nvgEndFrame(m_nvg);
			break;
		}								
		bgfx::frame();
		return true;
	}

	return false;
}
