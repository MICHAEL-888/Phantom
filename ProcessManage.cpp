﻿#include "ProcessManage.h"
#include "API.h"
#include <Windows.h>
#include <iostream>
#include <unordered_map>
#include <winternl.h>

API api;

ProcessManage::ProcessManage() {
	RefreshProcessList();
}

ProcessManage::~ProcessManage() {}

void ProcessManage::InitProcessList() {
	// 指向函数写入所请求信息的实际大小的位置的可选指针。 如果该大小小于或等于
	// SystemInformationLength 参数， 该函数会将信息复制到 SystemInformation
	// 缓冲区中;否则，它将返回 NTSTATUS 错误代码，并在 ReturnLength 中
	// 返回接收请求的信息所需的缓冲区大小。

	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;

	// 传入空指针获取缓冲区大小
	NTSTATUS status = api.ZwQuerySystemInformation(SystemProcessInformation,
		nullptr, 0, &m_BufferSize);

	if (m_BufferSize == 0) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to get buffer size\""
			<< std::endl;
		return;
	}

	// 分配内存
	m_BufferSize *= 2;
	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to allocate memory\""
			<< std::endl;
		return;
	}

	// 获取进程信息
	status = api.ZwQuerySystemInformation(SystemProcessInformation, m_Buffer,
		m_BufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to query system "
			"information\""
			<< std::endl;
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
			newProcess.m_threadsNum = processInfo->NumberOfThreads;
			newProcess.m_parrentProcessId = HandleToULong(processInfo->Reserved2);
			//newProcess.m_createTime = *reinterpret_cast<PLARGE_INTEGER>(processInfo + 0x20);
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
		if (m_processList[i].m_processName == L"Registry" && m_processList[i].m_pid <= 200) {
			continue;
		}

		if (m_processList[i].m_processName == L"Memory Compression") {
			continue;
		}

		ULONG m_BufferSize = 0;
		PVOID m_Buffer = nullptr;

		HANDLE hProcess;
		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };
		clientID.UniqueProcess = ULongToHandle(m_processList[i].m_pid);
		NTSTATUS status =
			api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION,
				&ObjectAttributes, &clientID);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to open process\""
				<< std::endl;
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName,
			nullptr, 0, &m_BufferSize);

		if (m_BufferSize == 0) {
			std::cerr
				<< "ProcessManage::InitProcessPath \"Failed to get buffer size\""
				<< std::endl;
			continue;
		}

		m_BufferSize *= 2;
		m_Buffer = malloc(m_BufferSize);
		//memset(&m_Buffer, 0, m_BufferSize);
		if (!m_Buffer) {
			std::cerr
				<< "ProcessManage::InitProcessPath \"Failed to allocate memory\""
				<< std::endl;
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName,
			m_Buffer, m_BufferSize, nullptr);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to query process "
				"information\""
				<< std::endl;
			free(m_Buffer);
			m_Buffer = nullptr;
			continue;
		}

		UNICODE_STRING newProcessInfo = *(PUNICODE_STRING)m_Buffer;
		if (newProcessInfo.Buffer) {
			m_processList[i].m_processPath = newProcessInfo.Buffer;
		}
		free(m_Buffer);
		CloseHandle(hProcess);
	}

	return;
}

void ProcessManage::InitProcessPeb() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].m_processName == L"Registry" && m_processList[i].m_pid <= 200) {
			continue;
		}

		if (m_processList[i].m_processName == L"Memory Compression") {
			continue;
		}

		ULONG m_BufferSize = 0;
		PROCESS_BASIC_INFORMATION m_Buffer = {0};

		HANDLE hProcess;
		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };
		clientID.UniqueProcess = ULongToHandle(m_processList[i].m_pid);
		NTSTATUS status =
			api.ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS,
				&ObjectAttributes, &clientID);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPeb \"Failed to open process\""
				<< std::endl;
			continue;
		}

		m_BufferSize = sizeof(m_Buffer);
		//memset(&m_Buffer, 0, m_BufferSize);

		status = api.ZwQueryInformationProcess(hProcess, ProcessBasicInformation,
			&m_Buffer, m_BufferSize, nullptr);
		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPeb \"Failed to query process "
				"information\""
				<< std::endl;
			continue;
		}


		PROCESS_BASIC_INFORMATION newProcessInfo = m_Buffer;
		if (newProcessInfo.PebBaseAddress) {
			m_processList[i].m_peb = newProcessInfo.PebBaseAddress;
		}
		CloseHandle(hProcess);
	}

	return;

}

std::wstring ProcessManage::DosPathGetFileName(const std::wstring& path) {
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

bool ProcessManage::IsPidExistedInSystem(ULONG pid) {
	bool result = true;
	HANDLE phd = api.ZwOpenProcess(SYNCHRONIZE, FALSE, pid);
	if (phd) {
		if (WaitForSingleObject(phd, 0) == WAIT_OBJECT_0) { // signal
			result = false;
		}
		DWORD exitCode;
		if (GetExitCodeProcess(phd, &exitCode)) {
			if (exitCode != STILL_ACTIVE) {
				result = false;
			}
		}
	}
	else {
		if (GetLastError() == ERROR_INVALID_PARAMETER) { // 87
			result = false;
		}
	}
	CloseHandle(phd);
	return result;
}

bool ProcessManage::IsPidExistedInList(ULONG pid) {
	bool exsited = false;

	for (const auto& p : m_processList) {
		if (p.m_pid == pid) {
			exsited = true;
			break;
		}
	}

	return exsited;
}

bool ProcessManage::DetectHiddenProcessByPid(std::vector<ProcessInfo>& processList) {

	for (ULONG pid = 8; pid < 65536; pid += 4) {	// 此处参考openark设置pid枚举上限
		if (!IsPidExistedInList(pid)) {
			if (IsPidExistedInSystem(pid)) {
				std::wstring path = PidToDosPath(pid);
				if (path != L"") {
					processList.emplace_back();
					ProcessInfo& newProcess = processList.back();
					newProcess.m_pid = pid;
					newProcess.m_processName = DosPathGetFileName(path);
					newProcess.m_processPath = path;
					newProcess.m_isHide = true;
				}
			}
		}
	}

	return true;
}

void ProcessManage::RefreshProcessList() {
	m_processList.clear();
	InitProcessList();
	InitProcessPath();
	InitProcessParrentName();
	// InitProcessPeb();
	InitProcessCritical();
	InitProcessPpl();
	InitProcessSid();
	InitProcessUserName();
	InitProcessUserDomain();
	DetectHiddenProcessByPid(m_processList);
	ListNtPathToDos();
}

bool ProcessManage::IsCritical(ULONG pid) {
	bool ret = false;
	HANDLE hProcess = api.ZwOpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
	ULONG critical;
	NTSTATUS status = api.ZwQueryInformationProcess(hProcess, ProcessBreakOnTermination, &critical, sizeof(critical), nullptr);
	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::IsCritical error" << std::endl;
		return ret;
	}
	if (critical) {
		ret = true;
	}
	CloseHandle(hProcess);
	return ret;
}

void ProcessManage::InitProcessCritical() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].m_processName == L"Registry" && m_processList[i].m_pid <= 200) {
			continue;
		}

		if (m_processList[i].m_processName == L"Memory Compression") {
			continue;
		}

		bool critical = IsCritical(m_processList[i].m_pid);
		m_processList[i].m_isCritical = critical;
	}
	return;
}

std::wstring ProcessManage::GetProcessParrentName(ULONG pid) {
	std::wstring parrentProcessName = DosPathGetFileName(PidToDosPath(pid));
	return parrentProcessName;
}

void ProcessManage::InitProcessParrentName() {
	for (int i = 0; i < m_processList.size(); i++) {


		std::wstring parrentProcessName;
		if (m_processList[i].m_parrentProcessId == 4) {
			parrentProcessName = L"System";
		} else {
			parrentProcessName = GetProcessParrentName(m_processList[i].m_parrentProcessId);
		}

		if (parrentProcessName == L"") {
			parrentProcessName = L"(进程已销毁)";
		}
		m_processList[i].m_parrentProcessName = parrentProcessName;
	}
	return;
}

typedef struct _PS_PROTECTION {
	union {
		UCHAR Level;
		struct {
			UCHAR Type : 3;
			UCHAR Audit : 1;                  // Reserved
			UCHAR Signer : 4;
		};
	};
} PS_PROTECTION, * PPS_PROTECTION;

BYTE ProcessManage::GetProcessPpl(ULONG pid) {
	HANDLE hProcess = api.ZwOpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
	BYTE pplInfo{};
	NTSTATUS status = api.ZwQueryInformationProcess(hProcess, static_cast<PROCESSINFOCLASS>(61), &pplInfo, sizeof(pplInfo), nullptr);
	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::GetProcessPpl error" << std::endl;
	}
	CloseHandle(hProcess);
	return pplInfo;
}

void ProcessManage::InitProcessPpl() {
	for (int i = 0; i < m_processList.size(); i++) {
		BYTE ppl = GetProcessPpl(m_processList[i].m_pid);
		m_processList[i].m_ppl = ppl;
	}
	return;
}

std::wstring ProcessManage::GetProcessSid(ULONG pid) {
	HANDLE hProcess = api.ZwOpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
	if (!hProcess) {
		std::cerr << "ProcessManage::GetProcessSid error" << std::endl;
		return L"";
	}

	HANDLE hToken{};
	NTSTATUS status = api.ZwOpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::GetProcessSid error" << std::endl;
		CloseHandle(hProcess);
		return L"";
	}

	DWORD dwSize = 0;
	GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwSize);
	if (!dwSize) {
		std::cerr << "ProcessManage::GetProcessSid error" << std::endl;
		CloseHandle(hToken);
		CloseHandle(hProcess);
		return L"";
	}

	// auto m_buffer = std::make_unique<BYTE[]>(dwSize);
	LPVOID m_buffer = malloc(dwSize);
	GetTokenInformation(hToken, TokenUser, m_buffer, dwSize, &dwSize);

	PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(m_buffer);
	PSID pSid = pTokenUser->User.Sid;

	LPWSTR stringSid = nullptr;
	ConvertSidToStringSidW(pSid, &stringSid);
	CloseHandle(hToken);
	CloseHandle(hProcess);

	if (!stringSid) {
		std::cerr << "ProcessManage::GetProcessSid error" << std::endl;
		return L"";
	}

	std::wstring sid(stringSid);
	free(m_buffer);
	LocalFree(stringSid);

	return sid;

}

void ProcessManage::InitProcessSid() {
	for (int i = 0; i < m_processList.size(); i++) {
		std::wstring sid = GetProcessSid(m_processList[i].m_pid);
		m_processList[i].m_sid = sid;
	}
	return;
}

std::wstring ProcessManage::GetSidUserName(std::wstring stringSid) {
	PVOID sid = nullptr;
	ConvertStringSidToSidW(stringSid.c_str(), &sid);

	DWORD bufferSize = 0;
	DWORD bufferSize2 = 0;
	SID_NAME_USE peUse{};
	LookupAccountSidW(nullptr, sid, nullptr, &bufferSize, nullptr, &bufferSize2, &peUse);

	if (!bufferSize) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		return L"";
	}

	// LPWSTR userName = (LPWSTR)malloc(bufferSize);
	auto userName = std::make_unique<WCHAR[]>(bufferSize);
	auto userDomain = std::make_unique<WCHAR[]>(bufferSize2);


	BOOL ret = LookupAccountSidW(nullptr, sid, userName.get(), &bufferSize, userDomain.get(), &bufferSize2, &peUse);
	if (!ret) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		return L"";
	}

	return userName.get();
}

std::wstring ProcessManage::GetSidDomain(std::wstring stringSid) {
	PVOID sid = nullptr;
	ConvertStringSidToSidW(stringSid.c_str(), &sid);

	DWORD bufferSize = 0;
	DWORD bufferSize2 = 0;
	SID_NAME_USE peUse{};
	LookupAccountSidW(nullptr, sid, nullptr, &bufferSize, nullptr, &bufferSize2, &peUse);

	if (!bufferSize) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		return L"";
	}

	// LPWSTR userName = (LPWSTR)malloc(bufferSize);
	auto userName = std::make_unique<WCHAR[]>(bufferSize);
	auto userDomain = std::make_unique<WCHAR[]>(bufferSize2);


	BOOL ret = LookupAccountSidW(nullptr, sid, userName.get(), &bufferSize, userDomain.get(), &bufferSize2, &peUse);
	if (!ret) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		return L"";
	}

	return userDomain.get();
}

void ProcessManage::InitProcessUserName() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].m_processName == L"系统空闲进程" && m_processList[i].m_pid == 0) {
			m_processList[i].m_userName = L"SYSTEM";
		}

		if (m_processList[i].m_processName == L"System" && m_processList[i].m_pid == 4) {
			m_processList[i].m_userName = L"SYSTEM";
		}

		if (!m_processList[i].m_sid.empty()) {
			std::wstring userName = GetSidUserName(m_processList[i].m_sid);
			m_processList[i].m_userName = userName;
		}
	}
	return;
}

void ProcessManage::InitProcessUserDomain() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].m_processName == L"系统空闲进程" && m_processList[i].m_pid == 0) {
			m_processList[i].m_userDomain = L"NT AUTHORITY";
		}

		if (m_processList[i].m_processName == L"System" && m_processList[i].m_pid == 4) {
			m_processList[i].m_userDomain = L"NT AUTHORITY";
		}

		if (!m_processList[i].m_sid.empty()) {
			std::wstring userDomain = GetSidDomain(m_processList[i].m_sid);
			m_processList[i].m_userDomain = userDomain;
		}
	}
	return;
}