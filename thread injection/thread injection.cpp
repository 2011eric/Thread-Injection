// thread injection.cpp: 定義主控台應用程式的進入點。
//

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <iomanip>
#define _CRT_SECURE_NO_WARNINGSS
#pragma warning(disable : 4996)


typedef int(__stdcall *__MessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);

using namespace std;
int GetPidBySnap() {
	PROCESSENTRY32 empty;
	empty.dwSize = sizeof(empty);
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//Create a snapshot of all the current process.
	char propath[260];
	char exe[260];
	cout << "Please Enter the exename:" << endl;
	cin >> exe;
	DWORD PID;
	if (Process32First(snap, &empty)) {
		while (Process32Next(snap, &empty)) {
			wcstombs(propath, empty.szExeFile, 260);
			if (!strcmp(exe, propath)) {
				cout << "Process Found!" << endl;
				PID = empty.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(snap);
	if (PID == 0) {
		cout << "NotFound";
		return 0;
	}
	else {
		return PID;
	}
}
int GetPidByWindow() {
	char winname[260];
	cin.getline(winname, 260);
	HWND myWindow = FindWindowA(NULL, winname);
	cout << hex << myWindow << endl;
	DWORD PID;
	GetWindowThreadProcessId(myWindow, &PID);
	if (PID == 0) {
		cout << "NotFound";
		return 0;
	}
	else {
		return PID;
	}

}

class codecave {
public:
	char Message[256];
	char Title[256];
	DWORD msgboxAdr;//address of the function


};
DWORD _stdcall RemoteThread(codecave *cData) {
	__MessageBoxA MsgBox = (__MessageBoxA)cData->msgboxAdr;
	MsgBox(NULL, cData->Message, cData->Title, MB_ICONINFORMATION);
	return EXIT_SUCCESS;
}

int main()
{
	//Find PID
	DWORD PID;
	do {
		cout << "(1)Get PID by Window (2)Get PID by exename" << endl;
		int op;
		cin >> op;
		cin.ignore();
		if (op == 1) {
			PID = GetPidByWindow();
		}
		else {
			PID = GetPidBySnap();
		}
	} while (PID == 0);
	cout << "PID found:" << PID << endl;


	//Get address of kernel32.dll
	HINSTANCE k32 = GetModuleHandle(L"kernel32.dll");
	LPVOID funcAdr = GetProcAddress(k32, "GetModuleHandleA");
	if (!funcAdr) {
		funcAdr = GetProcAddress(k32, "GetModuleHandleW");
	}

	//Open process
	cout << "Opening process..." << endl;
	HANDLE pro = OpenProcess(PROCESS_ALL_ACCESS, false, PID);//Open process
	if (pro == INVALID_HANDLE_VALUE) {
		cout << "Failed to open process";
	}

	//Get base address of the process
	DWORD programbase;
	HANDLE thread = CreateRemoteThread(pro, NULL, NULL, (LPTHREAD_START_ROUTINE)funcAdr, NULL, NULL, NULL);
	WaitForSingleObject(thread, INFINITE);
	GetExitCodeThread(thread, &programbase);
	CloseHandle(thread);
	cout << "programbase:" << hex << setw(8) << programbase << endl;
	
	//Get the address of MessageBox
	HINSTANCE u32 = LoadLibraryA("user32.dll");
	LPVOID MsgBoxAdr = GetProcAddress(u32, "MessageBoxA");
	if (!MsgBoxAdr)cout << "Failed to get the address" << "\n";
	FreeLibrary(u32);

	//Prepare the parameter for MessageBox
	codecave cdcv;
	ZeroMemory(&cdcv, sizeof(cdcv));
	strcpy(cdcv.Title, "Test");
	strcpy(cdcv.Message, "Injected!!!!!!!!!!!!!!!!");
	cdcv.msgboxAdr = (DWORD)MsgBoxAdr;

	//Inject function
	LPVOID remoteCave = VirtualAllocEx(pro, NULL, 0x5D, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	cout << "codecave create!!  address:" <<remoteCave << endl;
	DWORD shellcode = (DWORD)&RemoteThread;
	cout << shellcode << hex << endl;
	WriteProcessMemory(pro, remoteCave, (LPCVOID)shellcode, 0x5D, 0);
	
	//Inject data
	codecave* codecaveRemote = (codecave*)VirtualAllocEx(pro, NULL, sizeof(codecave), MEM_COMMIT, PAGE_READWRITE);
	cout << "Data stored!!  address:" << codecaveRemote << endl;
	WriteProcessMemory(pro, codecaveRemote, &cdcv, sizeof(codecave), NULL);

	//Call the function
	HANDLE remote = CreateRemoteThread(pro, 0, 0, (LPTHREAD_START_ROUTINE)remoteCave, (LPVOID)codecaveRemote, 0, 0);
	WaitForSingleObject(remote, INFINITE);
	



	VirtualFreeEx(pro, remoteCave, sizeof(codecave), MEM_RELEASE);
	CloseHandle(pro);
	CloseHandle(remote);
	
	system("pause");
	return 0;
}
