#include "ProcessManage.h"
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

	ULONG bufferSize{};

	// 传入空指针获取缓冲区大小
	NTSTATUS status = api.ZwQuerySystemInformation(SystemProcessInformation,
		nullptr, 0, &bufferSize);

	if (bufferSize == 0) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to get buffer size\""
			<< std::endl;
		return;
	}

	// 分配内存
	bufferSize *= 2;
	auto buffer = std::make_unique<BYTE[]>(bufferSize);
	if (!buffer) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to allocate memory\""
			<< std::endl;
		return;
	}

	// 获取进程信息
	status = api.ZwQuerySystemInformation(SystemProcessInformation, buffer.get(),
		bufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::InitProcessList \"Failed to query system "
			"information\""
			<< std::endl;
		return;
	}

	PSYSTEM_PROCESS_INFORMATION processInfo =
		(PSYSTEM_PROCESS_INFORMATION)buffer.get();

	// 遍历进程信息

	while (true) {
		m_processList.emplace_back();
		ProcessInfo& newProcess = m_processList.back();
		if (processInfo->ImageName.Buffer) {
			newProcess.setPid(HandleToULong(processInfo->UniqueProcessId));
			newProcess.setProcessName(processInfo->ImageName.Buffer);
			newProcess.setThreadsNum(processInfo->NumberOfThreads);
			newProcess.setParrentProcessId(HandleToULong(processInfo->Reserved2));
			//newProcess.m_createTime = *reinterpret_cast<PLARGE_INTEGER>(processInfo + 0x20);
		}
		else {
			newProcess.setPid(HandleToULong(processInfo->UniqueProcessId));
			newProcess.setProcessName(L"系统空闲进程");
		}

		if (processInfo->NextEntryOffset == 0) {
			break;
		}

		processInfo = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)processInfo +
			processInfo->NextEntryOffset);
	}
}

void ProcessManage::InitProcessPath() {

	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].getProcessName() == L"Registry" && m_processList[i].getPid() <= 200) {
			continue;
		}

		if (m_processList[i].getProcessName() == L"Memory Compression") {
			continue;
		}

		ULONG bufferSize{};

		HANDLE hProcess;
		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };
		clientID.UniqueProcess = ULongToHandle(m_processList[i].getPid());
		NTSTATUS status =
			api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION,
				&ObjectAttributes, &clientID);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to open process\""
				<< std::endl;
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName,
			nullptr, 0, &bufferSize);

		if (bufferSize == 0) {
			std::cerr
				<< "ProcessManage::InitProcessPath \"Failed to get buffer size\""
				<< std::endl;
			continue;
		}

		bufferSize *= 2;
		auto buffer = std::make_unique<BYTE[]>(bufferSize);
		//memset(&buffer, 0, bufferSize);
		if (!buffer) {
			std::cerr
				<< "ProcessManage::InitProcessPath \"Failed to allocate memory\""
				<< std::endl;
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName,
			buffer.get(), bufferSize, nullptr);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPath \"Failed to query process "
				"information\""
				<< std::endl;
			buffer = nullptr;
			continue;
		}

		UNICODE_STRING newProcessInfo = *reinterpret_cast<PUNICODE_STRING>(buffer.get());
		if (newProcessInfo.Buffer) {
			m_processList[i].setProcessPath(newProcessInfo.Buffer);
		}
		CloseHandle(hProcess);
	}

	return;
}

void ProcessManage::InitProcessPeb() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].getProcessName() == L"Registry" && m_processList[i].getPid() <= 200) {
			continue;
		}

		if (m_processList[i].getProcessName() == L"Memory Compression") {
			continue;
		}

		ULONG bufferSize{};
		PROCESS_BASIC_INFORMATION buffer = {0};

		HANDLE hProcess;
		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };
		clientID.UniqueProcess = ULongToHandle(m_processList[i].getPid());
		NTSTATUS status =
			api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION,
				&ObjectAttributes, &clientID);

		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPeb \"Failed to open process\""
				<< std::endl;
			continue;
		}

		bufferSize = sizeof(buffer);
		//memset(&buffer, 0, bufferSize);

		status = api.ZwQueryInformationProcess(hProcess, ProcessBasicInformation,
			&buffer, bufferSize, nullptr);
		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPeb \"Failed to query process "
				"information\""
				<< std::endl;
			continue;
		}


		PROCESS_BASIC_INFORMATION newProcessInfo = buffer;
		if (newProcessInfo.PebBaseAddress) {
			m_processList[i].setPeb(newProcessInfo.PebBaseAddress);
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
			if (p2.getProcessPath().find(p.first) == 0) {
				p2.setProcessPath(p2.getProcessPath().replace(0, p.first.length(), p.second));
			}
		}
	}

	return;
}

std::wstring ProcessManage::PidToDosPath(const ULONG pid) {
	ULONG bufferSize{};

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
	status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, nullptr, 0, &bufferSize);

	if (bufferSize == 0) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to get buffer size\"" << std::endl;
		CloseHandle(hProcess);
		return L"";
	}

	auto buffer = std::make_unique<BYTE[]>(bufferSize);
	if (!buffer) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to allocate memory\"" << std::endl;
		CloseHandle(hProcess);
		return L"";
	}

	status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, buffer.get(), bufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::PidToDosPath \"Failed to query process information\"" << std::endl;
		CloseHandle(hProcess);
		return L"";
	}

	UNICODE_STRING newProcessInfo = *reinterpret_cast<PUNICODE_STRING>(buffer.get());
	if (newProcessInfo.Buffer) {
		CloseHandle(hProcess);
		return newProcessInfo.Buffer;
	}

	CloseHandle(hProcess);
	return L"";

}

bool ProcessManage::IsPidExistedInSystem(ULONG pid) {
	bool result = true;
	HANDLE hProcess = api.ZwOpenProcess(SYNCHRONIZE, FALSE, pid);
	if (hProcess) {
		if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0) { // signal
			result = false;
		}
		DWORD exitCode;
		if (GetExitCodeProcess(hProcess, &exitCode)) {
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
	CloseHandle(hProcess);
	return result;
}

bool ProcessManage::IsPidExistedInList(ULONG pid) {
	bool exsited = false;

	for (const auto& p : m_processList) {
		if (p.getPid() == pid) {
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
					newProcess.setPid(pid);
					newProcess.setProcessName(DosPathGetFileName(path));
					newProcess.setProcessPath(path);
					newProcess.setIsHide(true);
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
	InitProcessPeb();
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
	ULONG isCritical;
	NTSTATUS status = api.ZwQueryInformationProcess(hProcess, ProcessBreakOnTermination, &isCritical, sizeof(isCritical), nullptr);
	if (!NT_SUCCESS(status)) {
		CloseHandle(hProcess);
		std::cerr << "ProcessManage::IsCritical error" << std::endl;
		return ret;
	}
	if (isCritical) {
		ret = true;
	}
	CloseHandle(hProcess);
	return ret;
}

void ProcessManage::InitProcessCritical() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].getProcessName() == L"Registry" && m_processList[i].getPid() <= 200) {
			continue;
		}

		if (m_processList[i].getProcessName() == L"Memory Compression") {
			continue;
		}

		bool isCritical = IsCritical(m_processList[i].getPid());
		m_processList[i].setIsCritical(isCritical);
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
		if (m_processList[i].getParrentProcessId() == 4) {
			parrentProcessName = L"System";
		} else {
			parrentProcessName = GetProcessParrentName(m_processList[i].getParrentProcessId());
		}

		if (parrentProcessName == L"") {
			parrentProcessName = L"(进程已销毁)";
		}
		m_processList[i].setParrentProcessName(parrentProcessName);
	}
	return;
}

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
		BYTE ppl = GetProcessPpl(m_processList[i].getPid());
		m_processList[i].setPpl(ppl);
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

	auto buffer = std::make_unique<BYTE[]>(dwSize);
	GetTokenInformation(hToken, TokenUser, buffer.get(), dwSize, &dwSize);

	PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.get());
	PSID pSid = pTokenUser->User.Sid;

	LPWSTR stringSid{};
	ConvertSidToStringSidW(pSid, &stringSid);
	CloseHandle(hToken);
	CloseHandle(hProcess);

	if (!stringSid) {
		std::cerr << "ProcessManage::GetProcessSid error" << std::endl;
		return L"";
	}

	std::wstring sid(stringSid);
	LocalFree(stringSid);

	return sid;

}

void ProcessManage::InitProcessSid() {
	for (int i = 0; i < m_processList.size(); i++) {
		std::wstring sid = GetProcessSid(m_processList[i].getPid());
		m_processList[i].setSid(sid);
	}
	return;
}

std::wstring ProcessManage::GetSidUserName(std::wstring stringSid) {
	PVOID sid{};
	ConvertStringSidToSidW(stringSid.c_str(), &sid);

	DWORD bufferSize{};
	DWORD bufferSize2{};
	SID_NAME_USE peUse{};
	LookupAccountSidW(nullptr, sid, nullptr, &bufferSize, nullptr, &bufferSize2, &peUse);

	if (!bufferSize) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		LocalFree(sid);
		return L"";
	}

	// LPWSTR userName = (LPWSTR)malloc(bufferSize);
	auto userName = std::make_unique<WCHAR[]>(bufferSize);
	auto userDomain = std::make_unique<WCHAR[]>(bufferSize2);


	BOOL ret = LookupAccountSidW(nullptr, sid, reinterpret_cast<LPWSTR>(userName.get()), 
		&bufferSize,reinterpret_cast<LPWSTR>(userDomain.get()), &bufferSize2, &peUse);
	if (!ret) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		LocalFree(sid);
		return L"";
	}

	LocalFree(sid);
	return reinterpret_cast<LPWSTR>(userName.get());
}

std::wstring ProcessManage::GetSidDomain(std::wstring stringSid) {
	PVOID sid{};
	ConvertStringSidToSidW(stringSid.c_str(), &sid);

	DWORD bufferSize = 0;
	DWORD bufferSize2 = 0;
	SID_NAME_USE peUse{};
	LookupAccountSidW(nullptr, sid, nullptr, &bufferSize, nullptr, &bufferSize2, &peUse);

	if (!bufferSize) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		LocalFree(sid);
		return L"";
	}

	// LPWSTR userName = (LPWSTR)malloc(bufferSize);
	auto userName = std::make_unique<WCHAR[]>(bufferSize);
	auto userDomain = std::make_unique<WCHAR[]>(bufferSize2);


	BOOL ret = LookupAccountSidW(nullptr, sid, userName.get(), &bufferSize, userDomain.get(), &bufferSize2, &peUse);
	if (!ret) {
		std::cerr << "ProcessManage::GetSidUserName error" << std::endl;
		LocalFree(sid);
		return L"";
	}

	LocalFree(sid);
	return userDomain.get();
}

void ProcessManage::InitProcessUserName() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].getProcessName() == L"系统空闲进程" && m_processList[i].getPid() == 0) {
			m_processList[i].setUserName(L"SYSTEM");
		}

		if (m_processList[i].getProcessName() == L"System" && m_processList[i].getPid() == 4) {
			m_processList[i].setUserName(L"SYSTEM");
		}

		if (!m_processList[i].getSid().empty()) {
			std::wstring userName = GetSidUserName(m_processList[i].getSid());
			m_processList[i].setUserName(userName);
		}
	}
	return;
}

void ProcessManage::InitProcessUserDomain() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].getProcessName() == L"系统空闲进程" && m_processList[i].getPid() == 0) {
			m_processList[i].setUserDomain(L"NT AUTHORITY");
		}

		if (m_processList[i].getProcessName() == L"System" && m_processList[i].getPid() == 4) {
			m_processList[i].setUserDomain(L"NT AUTHORITY");
		}

		if (!m_processList[i].getSid().empty()) {
			std::wstring userDomain = GetSidDomain(m_processList[i].getSid());
			m_processList[i].setUserDomain(userDomain);
		}
	}
	return;
}