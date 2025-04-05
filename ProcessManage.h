#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <winternl.h>

class ProcessManage {
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
	std::wstring DosPathGetFileName(const std::wstring& path);
	void ListNtPathToDos();
	std::wstring PidToDosPath(const ULONG pid);
	bool IsPidExistedInSystem(ULONG pid);
	bool IsPidExistedInList(ULONG pid);
	bool DetectHiddenProcessByPid(std::vector<ProcessInfo>& hidenProcess);

public:
	ProcessManage();
	~ProcessManage();
	const std::vector<ProcessManage::ProcessInfo>& GetProcessList() const {
		return m_processList;
	}

	const int GetProcessCount() const {
		return m_processList.size();
	}

	const int GetProcessHiddenCount() const {
		int cnt = 0;
		for (const auto& p : m_processList) {
			if (p.m_isHide) {
				cnt++;
			}
		}

		return cnt;
	}


};
