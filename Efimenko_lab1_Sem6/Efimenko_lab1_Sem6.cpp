#include "pch.h"
#include "framework.h"
#include "Efimenko_lab1_Sem6.h"
#include "Efimenko_Thread_Struct.h"
#include "counter.h"
#include "Pipes.h"
#include <thread>
#include <vector>
#include <string>
#include <mutex>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

// Метод для вывода на консоль, но использующий lock_guard(распространяется на промежуток от '{' и до '}'
// Чтобы серверная консолька красиво выводила текст
mutex consoleMtx;
void WriteServerConsole(const char* text)
{
	lock_guard<mutex> guard(consoleMtx);
	cout << text << endl;
}
// thread_start:5
// thread_stop
// send_message:-1:hello, world it's message
vector<string> Split(string input, char separator)
{
	vector<string> res;
	string sSubStr = "";
	for (int i = 0; i < input.length(); i++)
	{
		if (input[i] == separator)
		{
			res.push_back(sSubStr);
			sSubStr = "";
			continue;
		}
		sSubStr += input[i];
	}

	if (sSubStr.length() > 0)
		res.push_back(sSubStr);
	return res;
}

CWinApp theApp;


struct Command
{
	enum CmdCode
	{
		StartThreads,
		StopThread,
		SendMsgToThread
	};

public:
	CmdCode code;
	vector<string> args;
};

HANDLE hSendCompleteEvent = CreateEvent(nullptr, FALSE, FALSE, "hSendCompleteEvent");

Command GetCommand(string commandAsString, bool showInfo = true)
{
	vector<string> splitted = Split(commandAsString, ':');
	// Показываем информацию о пришедшей команде на сервере
	if (showInfo)
	{
		string cmdName = "cmd name: " + splitted[0];
		WriteServerConsole(cmdName.c_str());
		for (int i = 1; i < splitted.size(); i++)
		{
			string cmdArg = "arg " + to_string(i) + ": " + splitted[i];
			WriteServerConsole(cmdArg.c_str());
		}
	}

	const char* cmd_name = splitted[0].c_str();

	///////////////////////////////////////////////////////
	///////     ПОЛУЧАЕМ ИНФОРМАЦИЮ О КОМАНДЕ     /////////
	Command cmd;
	if (strcmp(cmd_name, "threads_start") == 0)
	{
		cmd.code = Command::CmdCode::StartThreads;
		WriteServerConsole("cmd type setted to StartThreads");
	}
	else if (strcmp(cmd_name, "thread_stop") == 0)
	{
		cmd.code = Command::CmdCode::StopThread;
		WriteServerConsole("cmd type setted to StopThread");
	}
	else if (strcmp(cmd_name, "send_message") == 0) {
		cmd.code = Command::CmdCode::SendMsgToThread;
		WriteServerConsole("cmd type setted to SendMsgToThread");
	}

	for (int i = 1; i < splitted.size(); i++) // 0 индекс занимает название команды
		cmd.args.push_back(splitted[i]);
	///////////////////////////////////////////////////////
	///////////////////////////////////////////////////////

	string reqInfo = "client received request: ";
	reqInfo.append(commandAsString);
	WriteServerConsole(reqInfo.c_str());

	return cmd;
}
int RunCommand(Command cmd)
{
	string sActiveThreads;
	string resp;
	switch (cmd.code)
	{

	case Command::CmdCode::StartThreads:
		// cmd.args[0] - Количество потоков для запуска
		if (cmd.args.size() < 1)
		{
			string err = "[Server Error] StartThreads command need at least 1 argument.";
			WriteServerConsole(err.c_str());
			break;
		}

		// Запускаем N потоков
		Efimenko_Thread_Struct::CreateNewThreads(atoi(cmd.args[0].c_str()));
		sActiveThreads = to_string(Efimenko_Thread_Struct::GetThreadsCount());
		//

		resp = "Response to client is: " + sActiveThreads;
		WriteServerConsole(resp.c_str());

		return atoi(sActiveThreads.c_str());

	case Command::CmdCode::StopThread:
		if (Efimenko_Thread_Struct::GetThreadsCount() == 0) 
		{
			WriteServerConsole(to_string(0).c_str());
			return 0;
		}

		Efimenko_Thread_Struct::StopLastThread();

		sActiveThreads = to_string(Efimenko_Thread_Struct::GetThreadsCount());

		resp = "Response to client is: " + sActiveThreads;
		WriteServerConsole(resp.c_str());

		return atoi(sActiveThreads.c_str());

	case Command::CmdCode::SendMsgToThread:
		// cmd.args[0] - Номер потока, который будет записывать в файл. [-1] - все потоки
		// cmd.args[1] - Текст для записи в поток(-и) 
		if (cmd.args.size() < 2)
		{
			string err = "[Server Error] SendMessage command need at least 2 arguments.";
			WriteServerConsole(err.c_str());
			return err.length();
		}
		WriteServerConsole("cmd started execution");
		int threadId = atoi(cmd.args[0].c_str());
		if (threadId == -1) // Все потоки
		{
			WriteServerConsole("choosen all threads");
			counter::ThreadsNeedToCompleteCount = Efimenko_Thread_Struct::GetThreadsCount();
			Efimenko_Thread_Struct::SendDataToAllThreads((char*)cmd.args[1].c_str());
		}
		else
		{
			WriteServerConsole("choosen single thread");
			counter::ThreadsNeedToCompleteCount = 1;
			Efimenko_Thread_Struct::SendDataToSingleThread(threadId, (char*)cmd.args[1].c_str());
		}

		WaitForSingleObject(hSendCompleteEvent, INFINITE);

		return cmd.args[1].length();
	}
}

void ProcessClient(HANDLE hPipe)
{
	while (true)
	{
		string s = GetString(hPipe);
		if (s == "quit")
		{
			cout << "Client closed the connection" << endl;
			break;
		}
		else if (s == "get_active_threads_count")
		{
			SendInt(hPipe, Efimenko_Thread_Struct::GetThreadsCount());
			continue;
		}
		cout << s << endl;

		Command cmd = GetCommand(s);
		int response = RunCommand(cmd);
		SendInt(hPipe, response);
	}
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
}


void start()
{
	// CreateNamedPipe
	while (true) {
		HANDLE hPipe = CreateNamedPipe("\\\\.\\pipe\\CommandTransactPipe",
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			1024,
			1024,
			0,
			nullptr);
		auto bConnected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (bConnected)
			cout << "New client connected!" << endl;
		else 
		{
			CloseHandle(hPipe);
			continue;
		}
		thread t(ProcessClient, hPipe);
		t.detach();
	}

}

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule != nullptr)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: code your application's behavior here.
			wprintf(L"Fatal Error: MFC initialization failed\n");
			nRetCode = 1;
		}
		else start();
	}
	else
	{
		// TODO: change error code to suit your needs
		wprintf(L"Fatal Error: GetModuleHandle failed\n");
		nRetCode = 1;
	}

	return nRetCode;
}