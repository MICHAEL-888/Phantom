#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <winternl.h>
#include <sddl.h>

struct ModuleInfo {
	std::wstring m_moduleName{};
	PVOID m_dllBase{};
	ULONG m_imageSize{};
	std::wstring m_modulePath{};

};

class ProcessManage {
public:
	class ProcessInfo {
	private:

		ULONG m_pid{};
		std::wstring m_processName{};
		std::wstring m_processPath{};
		bool m_isHide = false;
		ULONG m_threadsNum{};
		ULONG m_parrentProcessId{};	// 注意此处通过解析保留字段获取父进程ID，也可通过ZwQueryInformationProcess公开方式获取
		std::wstring m_parrentProcessName{};
		LARGE_INTEGER m_createTime{};	// 功能待完善
		PPEB m_peb{}; // peb结构中可检测进程调试信息，可获取进程加载模块列表，peb在目标进程当中
		PPEB m_peb32{};
		BYTE m_isDebugged{};	// peb要读进程，后续完善
		std::vector<std::wstring> m_module{};	//同样需要读peb，后续完善
		bool m_isCritical{};
		BYTE m_ppl{};	// win8.1及以上版本
		std::wstring m_sid{};	// sid信息
		std::wstring m_userDomain{};
		std::wstring m_userName{};
		std::vector<struct ModuleInfo> m_moduleInfo{};

	public:
		ULONG getPid() const { return m_pid; }
		std::wstring getProcessName() const { return m_processName; }
		std::wstring getProcessPath() const { return m_processPath; }
		bool getIsHide() const { return m_isHide; }
		ULONG getThreadsNum() const { return m_threadsNum; }
		ULONG getParrentProcessId() const { return m_parrentProcessId; }
		std::wstring getParrentProcessName() const { return m_parrentProcessName; }
		LARGE_INTEGER getCreateTime() const { return m_createTime; }
		PPEB getPeb() const { return m_peb; }
		BYTE getIsDebugged() const { return m_isDebugged; }
		std::vector<std::wstring> getModule() const { return m_module; }
		bool getIsCritical() const { return m_isCritical; }
		BYTE getPpl() const { return m_ppl; }
		std::wstring getSid() const { return m_sid; }
		std::wstring getUserDomain() const { return m_userDomain; }
		std::wstring getUserName() const { return m_userName; }
		std::vector<ModuleInfo> getModuleInfo() const { return m_moduleInfo; }

		void setPid(const int& m_pid) { this->m_pid = m_pid; }
		void setProcessName(const std::wstring& m_processName) { this->m_processName = m_processName; }
		void setProcessPath(const std::wstring& m_processPath) { this->m_processPath = m_processPath; }
		void setIsHide(const bool& m_isHide) { this->m_isHide = m_isHide; }
		void setThreadsNum(const int& m_threadsNum) { this->m_threadsNum = m_threadsNum; }
		void setParrentProcessId(const int& m_parrentProcessId) { this->m_parrentProcessId = m_parrentProcessId; }
		void setParrentProcessName(const std::wstring& m_parrentProcessName) { this->m_parrentProcessName = m_parrentProcessName; }
		void setCreateTime(const LARGE_INTEGER& m_createTime) { this->m_createTime = m_createTime; }
		void setPeb(const PPEB& m_peb) { this->m_peb = m_peb; }
		void setIsDebugged(const BYTE& m_isDebugged) { this->m_isDebugged = m_isDebugged; }
		void setModule(const std::vector<std::wstring>& m_module) { this->m_module = m_module; }
		void setIsCritical(const bool& m_isCritical) { this->m_isCritical = m_isCritical; }
		void setPpl(const BYTE& m_ppl) { this->m_ppl = m_ppl; }
		void setSid(const std::wstring& m_sid) { this->m_sid = m_sid; }
		void setUserDomain(const std::wstring& m_userDomain) { this->m_userDomain = m_userDomain; }
		void setUserName(const std::wstring& m_userName) { this->m_userName = m_userName; }
		void setModuleInfo(const ModuleInfo& m_moduleInfo) { this->m_moduleInfo.emplace_back(m_moduleInfo); }

		int InitModuleList(ProcessManage::ProcessInfo& processInfo);
	};

private:
	

	std::vector<ProcessInfo> m_processList;

	void InitProcessList();
	void InitProcessPath();
	void InitProcessPeb();
	void InitProcessPeb32();
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
	std::vector<ProcessInfo> getProcessList() const { return m_processList; }


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
			if (p.getIsHide()) {
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
