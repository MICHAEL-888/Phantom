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
	class ProcessInfo {
	public:
		ULONG m_pid;
		std::wstring m_processName;
		std::wstring m_processPath;
		bool m_isHide = false;
	};

	std::vector<ProcessInfo> m_processList;

	void InitProcessList();
	void InitProcessPath();
	std::wstring DosPathToName(const std::wstring& path);
	void ListNtPathToDos();
	std::wstring PidToDosPath(const ULONG pid);
	bool IsPidExistedInSystem(const ULONG pid);
	bool IsPidExistedInList(const ULONG pid);
	std::vector<ProcessInfo> DetectHiddenProcessByPid();


};
