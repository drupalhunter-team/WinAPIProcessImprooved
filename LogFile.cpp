#define _CRT_SECURE_NO_WARNINGS
#include "LogFile.h"

MsgException::MsgException(std::string message){
	_message = message;
}
std::string& MsgException::ShowMessage(){
	return _message;
}


Logging::AbstractLogger::AbstractLogger(std::string name){
	_level = Info;
	_name = name;
}
Logging::AbstractLogger::AbstractLogger(std::string name , Level level){
	_level = level;
	_name = name;
}
void Logging::AbstractLogger::SetLevel(Level level){
	std::lock_guard<std::mutex>locker(_mu);
	_level = level;
}
Logging::Level Logging::AbstractLogger::GetLevel(){
	std::lock_guard<std::mutex>locker(_mu);
	return _level;
}
bool Logging::AbstractLogger::CanLog(Level level){
	return CompareLevels(level, _level) >= 0;
}
int Logging::AbstractLogger::CompareLevels(Level l1, Level l2){
	if (l1 < l2) return -1;
	if (l1 > l2) return 1;
	return 0;
}
std::string Logging::AbstractLogger::ConvertLevelToString(Level level){
	switch (level)
	{
	case Fine:
		return "FINE";
		break;
	case Info:
		return "INFO";
		break;
	case Warning:
		return "WARNING";
		break;
	case Sever:
		return "SEVER";
	}
}

std::string Logging::InsertDate::Format(std::string message){
	std::stringstream ss;
	time_t now = time(0);
	char* dt = ctime(&now);
	ss << ": " << dt << ": " << message;
	return ss.str();
}





Logging::LogFile::LogFile(std::string path, bool append, std::string name) : AbstractLogger(name)
{
	if (append) _f.open(path, std::ios_base::app);
	else _f.open(path);
	if (!(_f.is_open()))
		throw MsgException("File is not opened");
	time_t now = time(0);
	char* dt = ctime(&now);
	_f << std::endl << dt << startMsg << std::endl;
}

Logging::LogFile::LogFile(std::string path, bool append, std::string name, Level level) : AbstractLogger(name, level) {
	if (append) _f.open(path, std::ios_base::app);
	else _f.open(path);
	if (!(_f.is_open()))
		throw MsgException("File is not opened");
	time_t now = time(0);
	char* dt = ctime(&now);
	_f << std::endl << dt << startMsg << std::endl;
}

void Logging::LogFile::LogMessage(Level level, Formatter& format, std::string message){
	std::lock_guard<std::mutex> guard(_mu);
	if (CanLog(level)){
		message = format.Format(message);
		_f << ConvertLevelToString(level) << message << std::endl;
		_f.flush();
	}
}

Logging::LogFile::~LogFile()
{
	std::lock_guard<std::mutex> guard(_mu);
	time_t now = time(0);
	char* dt = ctime(&now);
	_f << dt << closeMsg << std::endl;
	_f.close();
}
