#include <Windows.h>
#include <iostream>
#include <winternl.h>
#pragma once
class ProcessManage {
public:
  ProcessManage();
  ~ProcessManage();
  void InitProcessList();

private:
	class ProcessList {
	public: // ĞŞ¸Ä·ÃÎÊĞŞÊÎ·ûÎª public
		ULONG pid;
		std::wstring processName;
	};
	ProcessList processList;

};
