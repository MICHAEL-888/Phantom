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
	public:
		ULONG m_pid;
		std::wstring m_processName;
		std::wstring m_processPath;
	};

	std::vector<ProcessList> processList;

	void NtPathToDos();

};
