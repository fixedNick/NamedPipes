// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <atlconv.h> // bstr

#pragma region dllMain


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#pragma endregion

using namespace std;
HANDLE hPipe;

BSTR ConvertStringToBStr(char* str)
{
	int wslen = MultiByteToWideChar(CP_ACP, 0, str, strlen(str), 0, 0);
	BSTR bstr = SysAllocStringLen(0, wslen);
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), bstr, wslen);
	// Use bstr here
	SysFreeString(bstr);
	return bstr;
}

/// КОДЫ ОШИБОК/ПОДТВЕРЖДЕНИЙ:
/// -111: Клиент отключается от сервера
/// -222: Не удается подключиться к NamePipe'у
char* SendMessageToServer(string message)
{
	DWORD dwRead;
	int buffSize = 1024;
	char readBuff[1025] = { 0 };
	TransactNamedPipe(
		hPipe,                 // pipe handle 
		(char*)message.c_str(),              // message to server
		(unsigned)strlen(message.c_str()), // message length 
		readBuff,              // buffer to receive reply
		buffSize,  // size of read buffer
		&dwRead,                // bytes read
		NULL);                  // not overlapped 

	return readBuff;
}

// Наш пайп, инициализируется при успешном подключении в методе ConnectToNamedPipe

extern "C"
{
	_declspec(dllexport) int _stdcall ConnectToNamedPipe()
	{
		if (WaitNamedPipe("\\\\.\\pipe\\CommandTransactPipe", 5000)) 
		{
			hPipe = CreateFile("\\\\.\\pipe\\CommandTransactPipe",
				GENERIC_READ | GENERIC_WRITE,
				0,
				nullptr,
				OPEN_EXISTING,
				0,
				NULL);

			DWORD dwMode = PIPE_READMODE_MESSAGE;
			SetNamedPipeHandleState(
				hPipe,    // pipe handle 
				&dwMode,  // new pipe mode 
				NULL,     // don't set maximum bytes 
				NULL);    // don't set maximum time 


			return atoi(SendMessageToServer("get_active_threads_count"));
		}
		else return -222;
	}

	_declspec(dllexport) void _stdcall ClientDisconnect()
	{
		SendMessageToServer("quit");
	}
	 /*В качестве возвращаемого значения необходим BStr, 
	 для этого используем самописный метод конвертации из char* в bstr
	 метод: ConvertStringToBStr(char* str)*/
	_declspec(dllexport) void* _stdcall SendTextThroughNamedPipe(char* fileText, int tidx)
	{
		// Формируем команду для сервера, по придуманному паттерну
		// Общий Паттерн: <команда>:<аргумент1>:<аргументN>
		// Паттер для данной команды: <команда>:<номер потока>:<текст для записи>
		string request = "send_message:" + to_string(tidx) + ":" + fileText;

		// Отправим команду
		char* response = SendMessageToServer(request);

		// Возвращаем клиенту конвертированный в BStr ответ.
		return ConvertStringToBStr(response);
	}

	_declspec(dllexport) int _stdcall StartServerThread(int threadsCount)
	{
		string req = "threads_start:" + to_string(threadsCount);

		// Запускаем N потоков
		char* resp = SendMessageToServer(req);
		//int activeThreadsCount = atoi(resp);

		return atoi(resp);
	}

	_declspec(dllexport) int _stdcall StopServerThread(bool stopServer)
	{
		string req = "thread_stop";
		char* resp = SendMessageToServer(req);
		//int activeThreadsCount = atoi(resp);

		return atoi(resp);
	}
	std::mutex mtx;

	_declspec(dllexport) void __cdecl WriteToFile(int thread_idx, char* str)
	{
		mtx.lock();
		std::ofstream vmdelet_out;                    //создаем поток 
		vmdelet_out.open(std::to_string(thread_idx) + ".txt", std::ios::app);  // открываем файл для записи в конец
		vmdelet_out << str;                        // сама запись
		vmdelet_out.close();                          // закрываем файл
		mtx.unlock();
	}
}