#include "file_log.h"
#include <chrono>
#include <ctime>
#include <iomanip>

FileLog *g_filelog = nullptr;

FileLog::FileLog() : m_flags(0), m_fileOpen(false)
{
	g_filelog = this;
}

FileLog::FileLog(std::string filename) : m_flags(0), m_fileOpen(false)
{
	g_filelog = this;
	m_fileOpen = this->InitFile(filename);
}

FileLog::FileLog(std::string filename, uint32_t flags) : m_flags(0), m_fileOpen(false)
{
	g_filelog = this;
	m_flags = flags;

	m_fileOpen = this->InitFile(filename);
}

FileLog& FileLog::operator<<(std::string str) 
{
	if (m_fileOpen) {

		this->WriteLog(str);
	}
	return *this;
}

FileLog& FileLog::operator<<(int value)
{
	if (m_fileOpen) {

		this->WriteLog(std::to_string(value));
	}
	return *this;
}

FileLog::~FileLog()
{
	if (m_fileOpen) {
		m_file.close();
	}
	g_filelog = nullptr;
}

bool FileLog::InitFile(std::string filename)  
{
	if (filename.empty()) {
		return false;
	}

	m_filename = filename;
	m_file.open(m_filename, std::ios_base::app);
	if (!m_file.is_open()) {
		return false;
	}

	m_fileOpen = true;

	// init file by writing time/date
	if (m_flags & (int)FileLogFlags::FILELOG_WRITE_DATE_INIT) {

		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		m_file << "File log initialised at " << std::put_time(std::localtime(&in_time_t), "%d-%m-%y %X") << "\n\n";
	}

	if (m_flags & (int)FileLogFlags::FILELOG_CLOSE_AFTER_EACH_WRITE) {

		m_file.close();
		m_fileOpen = false;
	}

	return true;
}

bool FileLog::WriteLog(std::string text)
{
	if(!m_fileOpen) {

		m_file.open(m_filename, std::ios_base::app);

		if (!m_file.is_open()) {
			return false;
		}
	}
	m_fileOpen = true;
		
	// log text along with time and date
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	if (m_flags & (int)FileLogFlags::FILELOG_WRITE_DATE) {
		m_file << "Text logged at: " << std::put_time(std::localtime(&in_time_t), "%d-%m-%y %X") << "\n";
	}

	m_file << text;
	
	if (m_flags & (int)FileLogFlags::FILELOG_CLOSE_AFTER_EACH_WRITE) {

		m_file.close();
		m_fileOpen = false;
	}
	return true;
}

void FileLog::Delete()
{
	remove(m_filename.c_str());
}