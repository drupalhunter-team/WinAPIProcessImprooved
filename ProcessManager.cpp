#include "ProcessManager.h"
#include <sstream>
#include "GetCmdLine.h"

ProcessManager::ProcessManager(std::wstring cmdline)
{
	_trdalive = false;
	_log = NULL;
	_app_status = "Stopped";
	_log = new Logging::LogFile("logfile.txt", false, "Ragnarok",Logging::Fine);
	_cmdline = cmdline;
	std::thread * _exeTrd = NULL;
	_OnProcStart = []  {return; };
	_OnProcCrash = []  {return; };
	_OnProcManuallyStopped = [] { return; };
	_OnProcRestart = [] { return; };
}

ProcessManager::ProcessManager(std::wstring cmdline,
	std::function<void()> OnProcStart,
	std::function<void()> OnProcCrash,
	std::function<void()> OnProcManuallyStopped,
	std::function<void()> OnProcRestart
	) : ProcessManager(cmdline)
{
	_OnProcStart = OnProcStart;
	_OnProcCrash = OnProcCrash;
	_OnProcManuallyStopped = OnProcManuallyStopped;
	_OnProcRestart = OnProcRestart;
}

ProcessManager::ProcessManager(DWORD pid)
{
	_trdalive = false;
	_log = NULL;
	_app_status = "Stopped";
	_log = new Logging::LogFile("logfile.txt", false, "Ragnarok",Logging::Fine);
	_exeTrd = NULL;
	_OnProcStart = []  {return; };
	_OnProcCrash = []  {return; };
	_OnProcManuallyStopped = [] { return; };
	_OnProcRestart = [] { return; };
	ZeroMemory(&_pi, sizeof(_pi));
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (handle == NULL){
		std::string error = "Failed to open process with error ";
		error.append(std::to_string(GetLastError()));
		_log->LogMessage(Logging::Sever,format,error);
	}
	PWSTR cmdline = GetCMDLine(pid, _log);
	_cmdline.append(cmdline);
	_pi.hProcess = handle;
	_pi.dwProcessId = pid;
	_trdalive = true;
	_exeTrd = new std::thread(&ProcessManager::threadfunc, this);
}

ProcessManager::ProcessManager(DWORD pid,
	std::function<void()> OnProcStart,
	std::function<void()> OnProcCrash,
	std::function<void()> OnProcManuallyStopped,
	std::function<void()> OnProcRestart
	)
{
	_trdalive = false;
	_log = NULL;
	_app_status = "Stopped";
	_log = new Logging::LogFile("logfile.txt", false, "Ragnrok", Logging::Fine);
	_exeTrd = NULL;
	_OnProcStart = []  {return; };
	_OnProcCrash = []  {return; };
	_OnProcManuallyStopped = [] { return; };
	ZeroMemory(&_pi, sizeof(_pi));
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (handle == NULL){
		std::string error = "Failed to open process with error ";
		error.append(std::to_string(GetLastError()));
		_log->LogMessage(Logging::Sever,format,error);
	}
	PWSTR cmdline = GetCMDLine(pid, _log);
	_cmdline.append(cmdline);
	_pi.hProcess = handle;
	_pi.dwProcessId = pid;
	_trdalive = true;
	_exeTrd = new std::thread(&ProcessManager::threadfunc, this);
	_OnProcStart = OnProcStart;
	_OnProcCrash = OnProcCrash;
	_OnProcManuallyStopped = OnProcManuallyStopped;
	_OnProcRestart = OnProcRestart;
}

void ProcessManager::StartProcess()
{
	std::unique_lock<std::mutex> guard(_mu);
	if (_trdalive) return;
	_stopProcess();
	_startProcess();
	_trdalive = true;
	_exeTrd = new std::thread(&ProcessManager::threadfunc, this);
	std::thread t(_OnProcStart);
	t.detach();
}

void ProcessManager::RestartProcess(){
	std::unique_lock<std::mutex> guard(_mu);
	_stopProcess();
	_startProcess();
	_trdalive = true;
	_exeTrd = new std::thread(&ProcessManager::threadfunc, this);
	std::thread t(_OnProcRestart);
	t.detach();
}

void ProcessManager::StopProcess(){
	std::unique_lock<std::mutex> guard(_mu);
	_stopProcess();
	std::thread t(_OnProcManuallyStopped);
	t.detach();
}

ProcessStatus ProcessManager::GetStatus(){
	std::lock_guard<std::mutex> guard(_mu);
	return ProcessStatus(_pi.hProcess, _pi.dwProcessId, _cmdline, _app_status);
}


ProcessManager::~ProcessManager()
{
	_stopProcess();
	if (_log)
		delete _log;
	if (_exeTrd != NULL){
		if (_exeTrd->joinable())
			_exeTrd->join();
	}
}



void ProcessManager::_startProcess(){

	//clear memory
	ZeroMemory(&_si, sizeof(_si));
	_si.cb = sizeof(_si);
	ZeroMemory(&_pi, sizeof(_pi));
	wchar_t* cmdLine = const_cast<wchar_t*>(_cmdline.c_str());

	_log->LogMessage(Logging::Info, format ,"Trying to start process");
	if (!CreateProcess(NULL,   // No module name (use command line)
		cmdLine,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&_si,            // Pointer to STARTUPINFO structure
		&_pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		std::ostringstream strStream;
		strStream << "Process couldn't start. Error code " << GetLastError();
		_log->LogMessage(Logging::Warning, format ,strStream.str());
		throw MsgException(strStream.str());
	}
	else {
		_app_status = "Running";
		_log->LogMessage(Logging::Info, format, "Process started succesfully");
	}
}

void ProcessManager::_stopProcess(){
	_log->LogMessage(Logging::Info, format, "Trying to shutdown the process");
	if (_exeTrd != NULL && _trdalive){
		_trdalive = false;
		TerminateProcess(_pi.hProcess, 0);
		_exeTrd->join();
		delete _exeTrd;
		_exeTrd = NULL;
		_log->LogMessage(Logging::Info, format,"Process terminated");
		_app_status = "Stopped";
	}
	else {
		_log->LogMessage(Logging::Info, format, "Process is not running thus it wasn't stopped");
	}
}

void ProcessManager::threadfunc(){
	std::unique_lock<std::mutex> guard(_mu2);
	while (_trdalive){
		WaitForSingleObject(_pi.hProcess, INFINITE);
		if (_trdalive){
			unsigned long exitCode;
			GetExitCodeProcess(_pi.hProcess, &exitCode);
			_app_status = "Restarting";
			CloseHandle(_pi.hProcess);
			CloseHandle(_pi.hThread);
			std::ostringstream str;
			str << "Process terminated itself with code " << exitCode;
			_log->LogMessage(Logging::Info, format, str.str());
			_startProcess();
			std::thread t(_OnProcCrash);
			t.detach();
		}
	}
	CloseHandle(_pi.hProcess);
	CloseHandle(_pi.hThread);
}