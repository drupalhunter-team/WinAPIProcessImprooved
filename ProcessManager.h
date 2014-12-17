#pragma once
#include "LogFile.h"
#include <windows.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#pragma warning( disable : 4290 )

struct ProcessStatus{
public:
	HANDLE handle;
	DWORD id;
	std::wstring cmdline;
	std::string status;
	ProcessStatus(HANDLE h, DWORD i, std::wstring s, std::string st){
		handle = h;
		id = i;
		cmdline = s;
		status = st;
	}
};

class ProcessManager
{
public:
	ProcessManager(std::wstring cmdline) throw(MsgException);
	ProcessManager(std::wstring cmdline,
		std::function<void()> OnProcStart,
		std::function<void()> OnProcCrash,
		std::function<void()> OnProcManuallyStopped,
		std::function<void()> OnProcRestart
		) throw(MsgException);
	ProcessManager(DWORD pid);
	ProcessManager(DWORD pid,
		std::function<void()> OnProcStart,
		std::function<void()> OnProcCrash,
		std::function<void()> OnProcManuallyStopped,
		std::function<void()> OnProcRestart
		) throw(MsgException);
	void StartProcess() throw(MsgException);
	void RestartProcess() throw(MsgException);
	void StopProcess();
	ProcessStatus GetStatus();
	~ProcessManager();
private:
	std::string _app_status;
	bool _trdalive;
	bool _keepthread;
	std::condition_variable _condVar;
	Logging::LogFile *_log;
	Logging::InsertDate format;
	STARTUPINFO _si;
	PROCESS_INFORMATION _pi;
	std::mutex _mu;
	std::mutex _mu2;
	std::thread * _exeTrd;
	std::wstring _cmdline;
	std::function<void()> _OnProcStart;
	std::function<void()> _OnProcCrash;
	std::function<void()> _OnProcManuallyStopped;
	std::function<void()> _OnProcRestart;
	void threadfunc();

	void _startProcess() throw(MsgException);
	void _restartProcess() throw(MsgException);
	void _stopProcess();
};