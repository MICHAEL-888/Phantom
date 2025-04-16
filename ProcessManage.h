#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <winternl.h>
#include <sddl.h>



class ProcessManage {

private:
	class ProcessInfo {
	public:

		ULONG m_pid{};
		std::wstring m_processName{};
		std::wstring m_processPath{};
		bool m_isHide = false;
		ULONG m_threadsNum{};
		ULONG m_parrentProcessId{};	// 注意此处通过解析保留字段获取父进程ID，也可通过ZwQueryInformationProcess公开方式获取
		std::wstring m_parrentProcessName{};
		LARGE_INTEGER m_createTime{};	// 功能待完善
		PPEB m_peb{}; // peb结构中可检测进程调试信息，可获取进程加载模块列表，peb在目标进程当中
		BYTE m_isDebugged{};	// peb要读进程，后续完善
		std::vector<std::wstring> m_module{};	//同样需要读peb，后续完善
		bool m_isCritical{};
		BYTE m_ppl{};	// win8.1及以上版本
		std::wstring m_sid{};	// sid信息
		std::wstring m_userDomain{};
		std::wstring m_userName{};
	};

	std::vector<ProcessInfo> m_processList;

	void InitProcessList();
	void InitProcessPath();
	void InitProcessPeb();
	std::wstring DosPathGetFileName(const std::wstring& path);
	void ListNtPathToDos();
	std::wstring PidToDosPath(const ULONG pid);
	bool IsPidExistedInSystem(ULONG pid);
	bool IsPidExistedInList(ULONG pid);
	bool DetectHiddenProcessByPid(std::vector<ProcessInfo>& hidenProcess);
	void InitProcessCritical();
	void InitProcessParrentName();
	void InitProcessPpl();
	void InitProcessSid();
	void InitProcessUserName();
	void InitProcessUserDomain();

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

	void RefreshProcessList();

	bool IsCritical(ULONG pid);

	std::wstring GetProcessParrentName(ULONG pid);

	// ppl相关文档参考
	// https://learn.microsoft.com/zh-cn/windows/win32/procthread/zwqueryinformationprocess
	BYTE GetProcessPpl(ULONG pid);

	std::wstring GetProcessSid(ULONG pid);

	std::wstring GetSidUserName(std::wstring stringSid);

	std::wstring GetSidDomain(std::wstring stringSid);
};
