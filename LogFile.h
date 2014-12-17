#pragma once
#include <string>
#include <fstream>
#include <exception>
#include <mutex>
#include <ctime>
#include <sstream>
#include <atomic>
#pragma warning( disable : 4290 )
class MsgException : public std::exception{
private:
	std::string _message;
public:
	MsgException(std::string message);
	std::string& ShowMessage();
};

namespace Logging{
	enum Level {
		Fine, Info, Warning, Sever
	};

	class Formatter{
	public:
		virtual std::string Format(std::string message)=0;
	};
	class AbstractLogger{
	protected:
		std::mutex _mu;
		Level _level;
		std::string _name;
	public:
		AbstractLogger(std::string name);
		AbstractLogger(std::string name,Level level);
		virtual void LogMessage(Level level, Formatter& format, std::string message)=0;
		void SetLevel(Level level);
		Level GetLevel();
		void SetName(std::string& name);
		std::string GetName();
		bool CanLog(Level level);
		static std::string ConvertLevelToString(Level level);
		static int CompareLevels(Level l1, Level l2);
	};

	class InsertDate : public Formatter{
		std::string Format(std::string message);
	};
	class LogFile : public AbstractLogger
	{
	public:
		LogFile::LogFile(std::string path, bool append, std::string name);
		LogFile::LogFile(std::string path, bool append, std::string name, Level level);
		void LogMessage(Level level, Formatter& format, std::string message);
		~LogFile();
	private:
		std::ofstream _f;
		const char* startMsg = ": Session started";
		const char* closeMsg = ": Session closed";
	};
}




