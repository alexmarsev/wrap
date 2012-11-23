#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <process.h>

#define MAX_ARGLINE 32768

int _tmain(int argc, _TCHAR* argv[]) {
	TCHAR exe[MAX_PATH], ini[MAX_PATH], path[MAX_PATH], argline[MAX_ARGLINE], escpath[MAX_PATH], *div;
	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi;
	int res = 1;

	GetModuleFileName(NULL, ini, MAX_PATH);
	div = _tcsrchr(ini, '\\');
	div++;
	_tcscpy_s(exe, MAX_PATH, div);
	*div = '\0';
	_tcscat_s(ini, MAX_PATH, _T("wrap.ini"));

	if (GetPrivateProfileString(_T("wrap"), exe, NULL, path, MAX_PATH, ini)) {
		escpath[0] = '\0';
		div = _tcschr(path, ' ');
		if (div) _tcscat_s(escpath, MAX_PATH, _T("\""));
		_tcscat_s(escpath, MAX_PATH, path);
		if (div) _tcscat_s(escpath, MAX_PATH, _T("\""));

		argline[0] = '\0';
		_tcscat_s(argline, MAX_ARGLINE, escpath);
		for (int i = 1; i < argc; i++) {
			_tcscat_s(argline, MAX_ARGLINE, _T(" "));
			div = _tcschr(argv[i], ' ');
			if (div) _tcscat_s(argline, MAX_ARGLINE, _T("\""));
			_tcscat_s(argline, MAX_ARGLINE, argv[i]);
			if (div) _tcscat_s(argline, MAX_ARGLINE, _T("\""));
		}	

		_ftprintf(stderr, _T("%s -> %s\n"), exe, path);
		CreateProcess(escpath, argline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		CloseHandle(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, (LPDWORD)&res);
		CloseHandle(pi.hProcess);
	} else {
		_ftprintf(stderr, _T("%s -> undefined\n"), exe);
	}
	return res;
}

struct threadinfo {
	int argc;
	_TCHAR** argv;
	HWND hwnd;
};

DWORD WINAPI WrapwThreadProc(LPVOID lp) {
	DWORD res;
	threadinfo *ti = (threadinfo*) lp;
	res = _tmain(ti->argc, ti->argv);
	SendMessage(ti->hwnd, WM_PARENTNOTIFY, 0, 0);
	return res;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_PARENTNOTIFY:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

int WINAPI _tWinMain(HINSTANCE hinst, HINSTANCE hinstprev, _TCHAR* cmdline, int cmdshow) {
	TCHAR classname[] = TEXT("WRAPW_CLASS");
	WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
	HANDLE hthread;
	threadinfo ti;
	MSG msg;
	DWORD res = 1;
	
	wcex.hInstance = hinst;
	wcex.lpszClassName = classname;
	wcex.lpfnWndProc = WndProc;

	if (RegisterClassEx(&wcex)) {
		if (ti.hwnd = CreateWindowEx(0, classname, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL)) {
			if (ti.argv = CommandLineToArgvW(cmdline, &ti.argc)) {
				if (hthread = CreateThread(NULL, 0, &WrapwThreadProc, &ti, 0, NULL))
					while (GetMessage (&msg, NULL, 0, 0)) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					WaitForSingleObject(hthread, 1000);
					GetExitCodeThread(hthread, &res);
					if (res == STILL_ACTIVE) res = 1;
					CloseHandle(hthread);
			}
		}
	}
	return res;
}
