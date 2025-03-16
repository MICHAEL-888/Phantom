#pragma once
#include <Windows.h>
#include <iostream>
#include <winternl.h>
#include <vector>

class ProcessManage {
public:
  ProcessManage();
  ~ProcessManage();
  

private:
	class ProcessList {
	public:
		ULONG m_pid;
		std::wstring m_processName;
		std::wstring m_processPath;
	};

	std::vector<ProcessList> m_processList;

	void InitProcessList();
	void InitProcessPath();
	void NtPathToDos();
	bool IsPidExistedInSystem(ULONG pid);
	bool IsPidExistedInList(ULONG pid);
	void DetectHiddenProcessByPid();

};
