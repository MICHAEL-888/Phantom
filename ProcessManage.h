#pragma once
#include <Windows.h>
#include <iostream>
#include <winternl.h>
#include <vector>

class ProcessManage {
public:
  ProcessManage();
  ~ProcessManage();
  void InitProcessList();
  void InitProcessPath();

private:
	class ProcessList {
	public: // �޸ķ������η�Ϊ public
		ULONG pid;
		std::wstring processName;
		std::wstring processPath;
	};
	std::vector<ProcessList> processList;

	void NtPathToDos();

};
