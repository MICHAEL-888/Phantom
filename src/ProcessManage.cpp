#include "ProcessManage.h"
#include "API.h"
#include <Windows.h>
#include <iostream>
#include <unordered_map>
#include <winternl.h>

static API api;

ProcessManage::ProcessManage() {
	//RefreshProcessList();
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
		//memset(&readBuffer, 0, bufferSize);
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
		PVOID buffer = {0};


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
		//memset(&readBuffer, 0, bufferSize);

		status = api.ZwQueryInformationProcess(hProcess, ProcessWow64Information,
			&buffer, bufferSize, nullptr);
		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::InitProcessPeb \"Failed to query process "
				"information\""
				<< std::endl;
			continue;
		}


		/*PROCESS_BASIC_INFORMATION newProcessInfo = buffer;
		if (newProcessInfo.PebBaseAddress) {
			m_processList[i].setPeb(newProcessInfo.PebBaseAddress);
		}*/
		CloseHandle(hProcess);
	}

	return;
}

void ProcessManage::InitProcessPeb32() {
	for (int i = 0; i < m_processList.size(); i++) {
		if (m_processList[i].getProcessName() == L"Registry" && m_processList[i].getPid() <= 200) {
			continue;
		}

		if (m_processList[i].getProcessName() == L"Memory Compression") {
			continue;
		}

		ULONG bufferSize{};
		PROCESS_BASIC_INFORMATION buffer = { 0 };

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
		//memset(&readBuffer, 0, bufferSize);

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
	InitProcessPeb32();
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

typedef struct _PEB32
{
	BOOLEAN InheritedAddressSpace;
	BOOLEAN ReadImageFileExecOptions;
	BOOLEAN BeingDebugged;
	union
	{
		BOOLEAN BitField;
		struct
		{
			BOOLEAN ImageUsesLargePages : 1;
			BOOLEAN IsProtectedProcess : 1;
			BOOLEAN IsImageDynamicallyRelocated : 1;
			BOOLEAN SkipPatchingUser32Forwarders : 1;
			BOOLEAN IsPackagedProcess : 1;
			BOOLEAN IsAppContainer : 1;
			BOOLEAN IsProtectedProcessLight : 1;
			BOOLEAN IsLongPathAwareProcess : 1;
		};
	};
	ULONG Mutant;

	ULONG ImageBaseAddress;
	ULONG Ldr;
	
} PEB32, * PPEB32;

typedef struct _PEB_LDR_DATA32
{
	ULONG Length;
	BOOLEAN Initialized;
	ULONG SsHandle;
	LIST_ENTRY32 InLoadOrderModuleList;
	LIST_ENTRY32 InMemoryOrderModuleList;
	LIST_ENTRY32 InInitializationOrderModuleList;
	ULONG EntryInProgress;
	BOOLEAN ShutdownInProgress;
	ULONG ShutdownThreadId;
} PEB_LDR_DATA32, * PPEB_LDR_DATA32;

typedef struct _UNICODE_STRING32
{
	USHORT Length;
	USHORT MaximumLength;
	ULONG Buffer;
} UNICODE_STRING32;

typedef struct _LDR_DATA_TABLE_ENTRY32
{
	LIST_ENTRY32 InLoadOrderLinks;
	LIST_ENTRY32 InMemoryOrderLinks;
	union
	{
		LIST_ENTRY32 InInitializationOrderLinks;
		LIST_ENTRY32 InProgressLinks;
	};
	ULONG DllBase;
	ULONG EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING32 FullDllName;
	UNICODE_STRING32 BaseDllName;
} LDR_DATA_TABLE_ENTRY32, * PLDR_DATA_TABLE_ENTRY32;


int ProcessManage::ProcessInfo::InitModuleList(ProcessManage::ProcessInfo& processInfo) {

	bool wow64Flag = false;

	HANDLE hProcess = api.ZwOpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, processInfo.getPid());
	if (!hProcess) {
		std::cerr << "GetModuleList::ZwOpenProcess error" << std::endl;
		return 0;
	}

	PEB peb{};
	SIZE_T readlen{};
	// PVOID test = reinterpret_cast<PVOID>(reinterpret_cast<uintptr_t>(processInfo.getPeb()) + 0x1000);
	NTSTATUS status = api.ZwReadVirtualMemory(hProcess, processInfo.getPeb(), &peb, sizeof(peb), &readlen);

	if (!NT_SUCCESS(status)) {
		std::cerr << "GetModuleList::ZwReadVirtualMemory error" << std::endl;
		CloseHandle(hProcess);
		return 0;
	}

	PEB_LDR_DATA ldr{};
	status = api.ZwReadVirtualMemory(hProcess, peb.Ldr, &ldr, sizeof(ldr), &readlen);
	if (!NT_SUCCESS(status)) {
		std::cerr << "GetModuleList::ZwReadVirtualMemory error" << std::endl;
		CloseHandle(hProcess);
		return 0;
	}

	LIST_ENTRY head, *next{};
	// InLoadOrderLinks
	head = ldr.InMemoryOrderModuleList;
	// 分清楚目标进程地址和当前进程地址，小心出错！！！
	next = (LIST_ENTRY*)((ULONG_PTR)peb.Ldr + FIELD_OFFSET(PEB_LDR_DATA, InMemoryOrderModuleList));

	while (head.Flink != next) {
		static struct ModuleInfo moduleInfo;
		LDR_DATA_TABLE_ENTRY ldrEntry{};

		status = api.ZwReadVirtualMemory(hProcess, (PVOID)((ULONG_PTR)head.Flink - FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks)), &ldrEntry, sizeof(ldrEntry), &readlen);
		if (!NT_SUCCESS(status)) {
			std::cerr << "GetModuleList::ZwReadVirtualMemory error" << std::endl;
			CloseHandle(hProcess);
			return 0;
		}



		if (ldrEntry.FullDllName.Length > 0 && ldrEntry.FullDllName.Buffer != nullptr) {
			auto buffer = std::make_unique<wchar_t[]>(ldrEntry.FullDllName.Length / sizeof(wchar_t) + 1);

			status = api.ZwReadVirtualMemory(hProcess, ldrEntry.FullDllName.Buffer, buffer.get(),
				ldrEntry.FullDllName.Length, &readlen);
			if (NT_SUCCESS(status)) {
				buffer[ldrEntry.FullDllName.Length / sizeof(wchar_t)] = L'\0';
				moduleInfo.m_modulePath = buffer.get();
			}
			else {
				std::cerr << "GetModuleList::ZwReadVirtualMemory error reading string content" << std::endl;
				moduleInfo.m_modulePath = L"";
			}
		}
		else {
			moduleInfo.m_modulePath = L"";
		}

		ProcessManage pm;
		moduleInfo.m_moduleName = pm.DosPathGetFileName(moduleInfo.m_modulePath);
		moduleInfo.m_dllBase = ldrEntry.DllBase;
		moduleInfo.m_imageSize = reinterpret_cast<ULONG>(ldrEntry.Reserved3[1]);
		processInfo.setModuleInfo(moduleInfo);
		head = ldrEntry.InMemoryOrderLinks;
		
		// 此处判断wow64
		if (!wow64Flag && reinterpret_cast<ULONG_PTR>(ldrEntry.DllBase) < 0xFFFFFFFF) {
			wow64Flag = true;
		}
	}

	if (wow64Flag) {
		InitModuleList32(processInfo);
	}

	CloseHandle(hProcess);
	return 0;


}

int ProcessManage::ProcessInfo::InitModuleList32(ProcessManage::ProcessInfo& processInfo) {

	HANDLE hProcess = api.ZwOpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, processInfo.getPid());
	if (!hProcess) {
		std::cerr << "GetModuleList::ZwOpenProcess error" << std::endl;
		return 0;
	}

	PEB32 peb{};
	SIZE_T readlen{};

	NTSTATUS status = api.ZwReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(reinterpret_cast<uintptr_t>(processInfo.getPeb()) + 0x1000), &peb, sizeof(peb), &readlen);

	if (!NT_SUCCESS(status)) {
		std::cerr << "GetModuleList::ZwReadVirtualMemory error" << std::endl;
		CloseHandle(hProcess);
		return 0;
	}

	PEB_LDR_DATA32 ldr{};
	status = api.ZwReadVirtualMemory(hProcess, reinterpret_cast<PVOID>(peb.Ldr), &ldr, sizeof(ldr), &readlen);
	if (!NT_SUCCESS(status)) {
		std::cerr << "GetModuleList::ZwReadVirtualMemory error" << std::endl;
		CloseHandle(hProcess);
		return 0;
	}

	LIST_ENTRY32 head, * next{};
	// InLoadOrderLinks
	head = ldr.InMemoryOrderModuleList;
	// 分清楚目标进程地址和当前进程地址，小心出错！！！
	next = (LIST_ENTRY32*)((ULONG)peb.Ldr + FIELD_OFFSET(PEB_LDR_DATA32, InMemoryOrderModuleList));

	while (reinterpret_cast<PVOID>(head.Flink) != next) {
		static struct ModuleInfo moduleInfo;
		LDR_DATA_TABLE_ENTRY32 ldrEntry{};

		status = api.ZwReadVirtualMemory(hProcess, (PVOID)((ULONG)head.Flink - FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, InMemoryOrderLinks)), &ldrEntry, sizeof(ldrEntry), &readlen);
		if (!NT_SUCCESS(status)) {
			std::cerr << "GetModuleList::ZwReadVirtualMemory error" << std::endl;
			CloseHandle(hProcess);
			return 0;
		}



		if (ldrEntry.FullDllName.Length > 0 
			&& reinterpret_cast<PVOID>(ldrEntry.FullDllName.Buffer) != nullptr) {
			auto buffer = std::make_unique<wchar_t[]>(ldrEntry.FullDllName.Length / sizeof(wchar_t) + 1);

			status = api.ZwReadVirtualMemory(hProcess, 
				reinterpret_cast<PVOID>(ldrEntry.FullDllName.Buffer), buffer.get(),
				ldrEntry.FullDllName.Length, &readlen);
			if (NT_SUCCESS(status)) {
				buffer[ldrEntry.FullDllName.Length / sizeof(wchar_t)] = L'\0';
				moduleInfo.m_modulePath = buffer.get();
			}
			else {
				std::cerr << "GetModuleList::ZwReadVirtualMemory error reading string content" << std::endl;
				moduleInfo.m_modulePath = L"";
			}
		}
		else {
			moduleInfo.m_modulePath = L"";
		}

		ProcessManage pm;
		moduleInfo.m_moduleName = pm.DosPathGetFileName(moduleInfo.m_modulePath);
		moduleInfo.m_dllBase = reinterpret_cast<PVOID>(ldrEntry.DllBase);
		moduleInfo.m_imageSize = ldrEntry.SizeOfImage;
		processInfo.setModuleInfo(moduleInfo);
		head = ldrEntry.InMemoryOrderLinks;
	}

}

int ProcessManage::ProcessInfo::InitThreadList(ProcessManage::ProcessInfo& processInfo) {
	ULONG pid = processInfo.getPid();

	ULONG bufferSize = 0;
	NTSTATUS status = api.ZwQuerySystemInformation(SystemProcessInformation, nullptr, 0, &bufferSize);

	if (bufferSize == 0) {
		std::cerr << "InitThreadList: Failed to get buffer size" << std::endl;
		return -1;
	}

	bufferSize *= 2;
	auto buffer = std::make_unique<BYTE[]>(bufferSize);
	if (!buffer) {
		std::cerr << "InitThreadList: Failed to allocate memory" << std::endl;
		return -1;
	}

	status = api.ZwQuerySystemInformation(SystemProcessInformation, buffer.get(), bufferSize, nullptr);
	if (!NT_SUCCESS(status)) {
		std::cerr << "InitThreadList: Failed to query system information" << std::endl;
		return -1;
	}

	PSYSTEM_PROCESS_INFORMATION processInfo_ptr = (PSYSTEM_PROCESS_INFORMATION)buffer.get();
	while (true) {
		if (HandleToULong(processInfo_ptr->UniqueProcessId) == pid) {
			PSYSTEM_THREAD_INFORMATION threadInfo =
				(PSYSTEM_THREAD_INFORMATION)((LPBYTE)processInfo_ptr + sizeof(SYSTEM_PROCESS_INFORMATION));

			for (ULONG i = 0; i < processInfo_ptr->NumberOfThreads; i++) {
				ThreadInfokk ti;
				
				ti.m_tid = HandleToULong(threadInfo[i].ClientId.UniqueThread);
				ti.m_startAddress = threadInfo[i].StartAddress;
				ti.m_status = threadInfo[i].ThreadState;
				ti.m_waitReason = threadInfo[i].WaitReason;
				ti.m_priority = threadInfo[i].Priority;
				ti.m_changeCount = threadInfo[i].Reserved3;
				
				processInfo.setThreadInfo(ti);
			}

			break;
		}

		if (processInfo_ptr->NextEntryOffset == 0) {
			break;
		}

		processInfo_ptr = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)processInfo_ptr +
			processInfo_ptr->NextEntryOffset);
	}

	return 0;
}


int ProcessManage::ForceTerminateProcessbyZwApi(ULONG pid) {
	HANDLE hProcess = api.ZwOpenProcess(PROCESS_TERMINATE, FALSE, pid);
	NTSTATUS status = api.ZwTerminateProcess(hProcess, 0);
	if (!NT_SUCCESS(status)) {
		std::cerr << "ProcessManage::ForceTerminateProcessbyZwApi error" << std::endl;
		CloseHandle(hProcess);
		return -1;
	}
	CloseHandle(hProcess);
	return 0;

}

BOOL RunKill(ULONG pid) {
	ULONG ulPid = pid;
	char str[] = "boooom";

	HANDLE hProcess = api.ZwOpenProcess(PROCESS_ALL_ACCESS, false, ulPid);
	if (!hProcess) {
		return FALSE;
	}

	int nSize = strlen(str);
	LPVOID pDllAddr = VirtualAllocEx(hProcess, NULL, nSize, MEM_COMMIT, PAGE_READWRITE);
	SIZE_T dwWrittenSize = 0;
	WriteProcessMemory(hProcess, pDllAddr, str, nSize, &dwWrittenSize);

	// 获取进程所有线程
	ProcessManage pm;
	std::vector<struct ThreadInfokk> ti{};
	for (const auto& p : pm.GetProcessList()) {
		if (p.getPid() == pid) {
			ti = p.getThreadInfo();
		}
	}

	HANDLE hThread;

	for (const auto& p : ti) {
		hThread = OpenThread(THREAD_ALL_ACCESS, false, p.m_tid);

#if defined(_M_ARM64)
		CONTEXT context;
		context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, &context);
		context.Pc = (DWORD64)pDllAddr;

#elif defined(_M_X64)
		typedef struct DECLSPEC_ALIGN(16) _CONTEXT {
			DWORD64 P1Home;          /* 000 */
			DWORD64 P2Home;          /* 008 */
			DWORD64 P3Home;          /* 010 */
			DWORD64 P4Home;          /* 018 */
			DWORD64 P5Home;          /* 020 */
			DWORD64 P6Home;          /* 028 */

			/* Control flags */
			DWORD ContextFlags;      /* 030 */
			DWORD MxCsr;             /* 034 */

			/* Segment */
			WORD SegCs;              /* 038 */
			WORD SegDs;              /* 03a */
			WORD SegEs;              /* 03c */
			WORD SegFs;              /* 03e */
			WORD SegGs;              /* 040 */
			WORD SegSs;              /* 042 */
			DWORD EFlags;            /* 044 */

			/* Debug */
			DWORD64 Dr0;             /* 048 */
			DWORD64 Dr1;             /* 050 */
			DWORD64 Dr2;             /* 058 */
			DWORD64 Dr3;             /* 060 */
			DWORD64 Dr6;             /* 068 */
			DWORD64 Dr7;             /* 070 */

			/* Integer */
			DWORD64 Rax;             /* 078 */
			DWORD64 Rcx;             /* 080 */
			DWORD64 Rdx;             /* 088 */
			DWORD64 Rbx;             /* 090 */
			DWORD64 Rsp;             /* 098 */
			DWORD64 Rbp;             /* 0a0 */
			DWORD64 Rsi;             /* 0a8 */
			DWORD64 Rdi;             /* 0b0 */
			DWORD64 R8;              /* 0b8 */
			DWORD64 R9;              /* 0c0 */
			DWORD64 R10;             /* 0c8 */
			DWORD64 R11;             /* 0d0 */
			DWORD64 R12;             /* 0d8 */
			DWORD64 R13;             /* 0e0 */
			DWORD64 R14;             /* 0e8 */
			DWORD64 R15;             /* 0f0 */

			/* Counter */
			DWORD64 Rip;             /* 0f8 */

			/* Floating point */
			union {
				XMM_SAVE_AREA32 FltSave;  /* 100 */
				struct {
					M128A Header[2];      /* 100 */
					M128A Legacy[8];      /* 120 */
					M128A Xmm0;           /* 1a0 */
					M128A Xmm1;           /* 1b0 */
					M128A Xmm2;           /* 1c0 */
					M128A Xmm3;           /* 1d0 */
					M128A Xmm4;           /* 1e0 */
					M128A Xmm5;           /* 1f0 */
					M128A Xmm6;           /* 200 */
					M128A Xmm7;           /* 210 */
					M128A Xmm8;           /* 220 */
					M128A Xmm9;           /* 230 */
					M128A Xmm10;          /* 240 */
					M128A Xmm11;          /* 250 */
					M128A Xmm12;          /* 260 */
					M128A Xmm13;          /* 270 */
					M128A Xmm14;          /* 280 */
					M128A Xmm15;          /* 290 */
				} DUMMYSTRUCTNAME;
			} DUMMYUNIONNAME;

			/* Vector */
			M128A VectorRegister[26];     /* 300 */
			DWORD64 VectorControl;        /* 4a0 */

			/* Debug control */
			DWORD64 DebugControl;         /* 4a8 */
			DWORD64 LastBranchToRip;      /* 4b0 */
			DWORD64 LastBranchFromRip;    /* 4b8 */
			DWORD64 LastExceptionToRip;   /* 4c0 */
			DWORD64 LastExceptionFromRip; /* 4c8 */
		} CONTEXT;

		CONTEXT context;
		context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, reinterpret_cast<LPCONTEXT>(&context));
		context.Rip = (DWORD64)pDllAddr;
#else defined(_M_IX86)
		typedef struct _CONTEXT
		{
			DWORD   ContextFlags;

			/* These are selected by CONTEXT_DEBUG_REGISTERS */
			DWORD   Dr0;
			DWORD   Dr1;
			DWORD   Dr2;
			DWORD   Dr3;
			DWORD   Dr6;
			DWORD   Dr7;

			/* These are selected by CONTEXT_FLOATING_POINT */
			FLOATING_SAVE_AREA FloatSave;

			/* These are selected by CONTEXT_SEGMENTS */
			DWORD   SegGs;
			DWORD   SegFs;
			DWORD   SegEs;
			DWORD   SegDs;

			/* These are selected by CONTEXT_INTEGER */
			DWORD   Edi;
			DWORD   Esi;
			DWORD   Ebx;
			DWORD   Edx;
			DWORD   Ecx;
			DWORD   Eax;

			/* These are selected by CONTEXT_CONTROL */
			DWORD   Ebp;
			DWORD   Eip;
			DWORD   SegCs;
			DWORD   EFlags;
			DWORD   Esp;
			DWORD   SegSs;

			BYTE    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
		} CONTEXT;

		CONTEXT context;
		context.ContextFlags = CONTEXT_CONTROL;
		GetThreadContext(hThread, reinterpret_cast<LPCONTEXT>(&context));
		context.Eip = (DWORD)pDllAddr;
#endif
        SetThreadContext(hThread, reinterpret_cast<PCONTEXT>(&context));
		ResumeThread(hThread);
	}

	CloseHandle(hThread);
	CloseHandle(hProcess);
}

int ProcessManage::ForceTerminateProcessbyApc(ULONG pid) {
	ProcessManage pm;
	pm.InitProcessList();
	ProcessInfo pi;
	for (auto& p : pm.GetProcessList()) {
		if (p.getPid() == pid) {
			pi = p;
			pi.InitThreadList(pi);
		}
	}
	
	for (const auto& p : pi.getThreadInfo()) {
		HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, false, p.m_tid);
		if (!hThread) {
			std::cerr << "ProcessManage::ForceTerminateProcessbyApc OpenThread error" << std::endl;
			return -1;
		}

		NTSTATUS status = api.NtQueueApcThreadEx(hThread, ULongToHandle(1), (PPS_APC_ROUTINE)RunKill, &pid, &pid, &pid);
		if (!NT_SUCCESS(status)) {
			std::cerr << "ProcessManage::ForceTerminateProcessbyApc error" << std::endl;
			CloseHandle(hThread);
			return -1;
		}

		CloseHandle(hThread);
	}
	
	return 0;
}

