#define _CRT_SECURE_NO_WARNINGS
#ifndef _TEXT_LOG_H
#define _TEXT_LOG_H
#include <string>
#include <fstream>

class FileLog;

extern FileLog *g_filelog;

enum class FileLogFlags
{
	FILELOG_WRITE_DATE_INIT = 1 << 0,
	FILELOG_WRITE_DATE = 1 << 1,
	FILELOG_CLOSE_AFTER_EACH_WRITE = 1 << 2,
	FILELOG_NO_FLAGS = 1 << 3
};

class FileLog
{
public:

	FileLog();
	FileLog(std::string filename);
	FileLog(std::string filename, uint32_t flags);

	~FileLog();

	FileLog& operator=(FileLog& other) = delete;
	FileLog& operator=(FileLog&& other) = delete;

	FileLog& operator<<(std::string str);
	FileLog& operator<<(int value);

	bool InitFile(std::string filename);
	bool ChangeFile(std::string filename);
	void SetFlags(FileLogFlags m_flags);
	bool WriteLog(std::string text);
	void Delete();

private:
	std::ofstream m_file;
	std::string m_filename;
	uint32_t m_flags;
	bool m_fileOpen;
};

#endif _TEXT_LOG_H
