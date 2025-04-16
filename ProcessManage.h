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
		ULONG m_parrentProcessId{};	// ע��˴�ͨ�����������ֶλ�ȡ������ID��Ҳ��ͨ��ZwQueryInformationProcess������ʽ��ȡ
		std::wstring m_parrentProcessName{};
		LARGE_INTEGER m_createTime{};	// ���ܴ�����
		PPEB m_peb{}; // peb�ṹ�пɼ����̵�����Ϣ���ɻ�ȡ���̼���ģ���б�peb��Ŀ����̵���
		BYTE m_isDebugged{};	// pebҪ�����̣���������
		std::vector<std::wstring> m_module{};	//ͬ����Ҫ��peb����������
		bool m_isCritical{};
		BYTE m_ppl{};	// win8.1�����ϰ汾
		std::wstring m_sid{};	// sid��Ϣ
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

	// ppl����ĵ��ο�
	// https://learn.microsoft.com/zh-cn/windows/win32/procthread/zwqueryinformationprocess
	BYTE GetProcessPpl(ULONG pid);

	std::wstring GetProcessSid(ULONG pid);

	std::wstring GetSidUserName(std::wstring stringSid);

	std::wstring GetSidDomain(std::wstring stringSid);
};
