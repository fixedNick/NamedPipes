#include <windows.h>
#include <string>
#include <vector>

using namespace std;

inline int GetInt(HANDLE hPipe)
{
	DWORD dwDone;
	int n;

	ReadFile(hPipe, &n, sizeof(n), &dwDone, nullptr);
	return n;
}

inline void SendInt(HANDLE hPipe, int n, bool bFlush = true)
{
	DWORD dwDone;
	WriteFile(hPipe, &n, sizeof(n), &dwDone, NULL);
	if (bFlush)
		FlushFileBuffers(hPipe);
}

inline string GetString(HANDLE hPipe)
{
	DWORD dwDone;
	int nLength = GetInt(hPipe);

	vector<char> v(nLength);
	ReadFile(hPipe, &v[0], nLength, &dwDone, nullptr);
	return string(&v[0], nLength);
}

inline void SendString(HANDLE hPipe, const string& str)
{
	DWORD dwDone;
	int nLength = str.length();

	SendInt(hPipe, nLength, false);
	WriteFile(hPipe, str.c_str(), nLength, &dwDone, nullptr);
	FlushFileBuffers(hPipe);
}