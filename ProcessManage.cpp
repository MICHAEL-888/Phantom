#include "ProcessManage.h"
#include <Windows.h>
#include <iostream>
#include <winternl.h>
#include "API.h"
#include <unordered_map>

API api;

ProcessManage::ProcessManage() {
	InitProcessList();
	InitProcessPath();
	DetectHiddenProcessByPid();
	ListNtPathToDos();
}

ProcessManage::~ProcessManage() {}

void ProcessManage::InitProcessList() {
	// 指向函数写入所请求信息的实际大小的位置的可选指针。 如果该大小小于或等于 SystemInformationLength 参数，
	// 该函数会将信息复制到 SystemInformation 缓冲区中;否则，它将返回 NTSTATUS 错误代码，并在 ReturnLength 中
	// 返回接收请求的信息所需的缓冲区大小。

	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;

	// 传入空指针获取缓冲区大小
	NTSTATUS status = api.ZwQuerySystemInformation(SystemProcessInformation, nullptr,
		0, &m_BufferSize);

	if (m_BufferSize == 0) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to get buffer size\"" << std::endl;
		return;
	}

	// 分配内存
	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to allocate memory\"" << std::endl;
		return;
	}

	// 获取进程信息
	status = api.ZwQuerySystemInformation(SystemProcessInformation, m_Buffer,
		m_BufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to query system information\"" << std::endl;
		free(m_Buffer);
		m_Buffer = nullptr;
		return;
	}

	PSYSTEM_PROCESS_INFORMATION processInfo =
		(PSYSTEM_PROCESS_INFORMATION)m_Buffer;


	// 遍历进程信息

	while (true) {
		m_processList.emplace_back();
		ProcessInfo& newProcess = m_processList.back();
		if (processInfo->ImageName.Buffer) {
			newProcess.m_pid = HandleToULong(processInfo->UniqueProcessId);
			newProcess.m_processName = processInfo->ImageName.Buffer;
		}
		else {
			newProcess.m_pid = HandleToULong(processInfo->UniqueProcessId);
			newProcess.m_processName = L"系统空闲进程";
		}

		if (processInfo->NextEntryOffset == 0) {
			break;
		}

		processInfo = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)processInfo +
			processInfo->NextEntryOffset);
	}

	if (m_Buffer) {
		free(m_Buffer);
		m_Buffer = nullptr;
	}

}

void ProcessManage::InitProcessPath() {

	for (int i = 0; i < m_processList.size(); i++) {
		ULONG m_BufferSize = 0;
		PVOID m_Buffer = nullptr;

		HANDLE hProcess;
		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };
		clientID.UniqueProcess = ULongToHandle(m_processList[i].m_pid);
		NTSTATUS status = api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &ObjectAttributes, &clientID);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to open process\"" << std::endl;
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, nullptr, 0, &m_BufferSize);

		if (m_BufferSize == 0) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to get buffer size\"" << std::endl;
			CloseHandle(hProcess);
			continue;
		}

		m_Buffer = malloc(m_BufferSize);
		if (!m_Buffer) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to allocate memory\"" << std::endl;
			CloseHandle(hProcess);
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, m_Buffer, m_BufferSize, nullptr);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to query process information\"" << std::endl;
			free(m_Buffer);
			m_Buffer = nullptr;
			CloseHandle(hProcess);
			continue;
		}

		UNICODE_STRING newProcessInfo = *(PUNICODE_STRING)m_Buffer;
		if (newProcessInfo.Buffer) {
			m_processList[i].m_processPath = newProcessInfo.Buffer;
		}

		CloseHandle(hProcess);
		

	}

	return;
}

std::wstring ProcessManage::DosPathToName(const std::wstring& path) {
	return path.substr(path.find_last_of(L"\\/") + 1);
}

void ProcessManage::ListNtPathToDos() {
	std::unordered_map<std::wstring, std::wstring> driverList;
	WCHAR driver = L'A';
	do {
		std::wstring tmp;
		tmp.push_back(driver);
		tmp.push_back(L':');
		LPWSTR lpDeviceName = tmp.data();
		WCHAR lpTargetPath[1000] = { 0 };
		DWORD ucchMax = 1000;
		DWORD status = QueryDosDeviceW(lpDeviceName, lpTargetPath, ucchMax);
		if (status) {
				driverList.emplace(lpTargetPath, tmp);
		}
		
		driver++;
	} while (driver != L'Z');

	for (const auto& p : driverList) {
		for (auto& p2 : m_processList) {
			if (p2.m_processPath.find(p.first) == 0) {
				p2.m_processPath.replace(0, p.first.length(), p.second);
			}
		}
	}

	return;
	
}

std::wstring ProcessManage::PidToDosPath(const ULONG pid) {
	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;

	HANDLE hProcess;
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
	CLIENT_ID clientID = { 0 };
	clientID.UniqueProcess = ULongToHandle(pid);
	NTSTATUS status = api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &ObjectAttributes, &clientID);

	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to open process\"" << std::endl;
		return L"";
	}

	status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, nullptr, 0, &m_BufferSize);

	if (m_BufferSize == 0) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to get buffer size\"" << std::endl;
		return L"";
	}

	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to allocate memory\"" << std::endl;
		return L"";
	}

	status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, m_Buffer, m_BufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to query process information\"" << std::endl;
		free(m_Buffer);
		m_Buffer = nullptr;
		return L"";
	}

	UNICODE_STRING newProcessInfo = *(PUNICODE_STRING)m_Buffer;
	if (newProcessInfo.Buffer) {
		CloseHandle(hProcess);
		return newProcessInfo.Buffer;
	}

	CloseHandle(hProcess);
	return L"";
	
}

bool ProcessManage::IsPidExistedInSystem(const ULONG pid) {
	bool result = true;
	HANDLE phd = api.ZwOpenProcess(SYNCHRONIZE, FALSE, pid);
	if (phd) {
		if (WaitForSingleObject(phd, 0) == WAIT_OBJECT_0)	//signal
			result = false;
		CloseHandle(phd);
	}
	else {
		if (GetLastError() == ERROR_INVALID_PARAMETER)	//87
			result = false;
	}
	return result;
}

bool ProcessManage::IsPidExistedInList(const ULONG pid) {
	bool exsited = false;
	
	for (const auto& p : m_processList) {
		if (p.m_pid == pid) {
			exsited = true;
			break;
		}
	}

	return exsited;
}

std::vector<ProcessManage::ProcessInfo> ProcessManage::DetectHiddenProcessByPid() {
	std::vector<ProcessInfo> hidenProcess;

	for (ULONG pid = 8; pid < 65536; pid += 4) {	// 此处参考openark设置pid枚举上限
		if (!IsPidExistedInList(pid)) {
			if (IsPidExistedInSystem(pid)) {
				std::wstring path = PidToDosPath(pid);
				if (path != L"") {
					hidenProcess.emplace_back();
					ProcessInfo& newProcess = hidenProcess.back();
					newProcess.m_pid = pid;
					newProcess.m_processName = DosPathToName(path);
					newProcess.m_processPath = path;
					newProcess.m_isHide = true;
				}
			}
		}
	}

	return hidenProcess;
}