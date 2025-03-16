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
		std::cerr << "Failed to get buffer size" << std::endl;
		return;
	}

	// 分配内存
	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "Failed to allocate memory" << std::endl;
		return;
	}

	// 获取进程信息
	status = api.ZwQuerySystemInformation(SystemProcessInformation, m_Buffer,
		m_BufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "Failed to query system information" << std::endl;
		free(m_Buffer);
		m_Buffer = nullptr;
		return;
	}

	PSYSTEM_PROCESS_INFORMATION processInfo =
		(PSYSTEM_PROCESS_INFORMATION)m_Buffer;


	// 遍历进程信息

	while (true) {
		processList.emplace_back();
		ProcessList& newProcess = processList.back();
		if (processInfo->ImageName.Buffer) {
			newProcess.pid = HandleToULong(processInfo->UniqueProcessId);
			newProcess.processName = processInfo->ImageName.Buffer;
		}
		else {
			newProcess.pid = HandleToULong(processInfo->UniqueProcessId);
			newProcess.processName = L"系统空闲进程";
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

	for (int i = 0; i < processList.size(); i++) {
		ULONG m_BufferSize = 0;
		PVOID m_Buffer = nullptr;

		HANDLE hProcess;
		OBJECT_ATTRIBUTES ObjectAttributes;
		InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
		CLIENT_ID clientID = { 0 };
		clientID.UniqueProcess = ULongToHandle(processList[i].pid);
		NTSTATUS status = api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &ObjectAttributes, &clientID);

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, nullptr, 0, &m_BufferSize);

		if (m_BufferSize == 0) {
			std::cerr << "Failed to get buffer size" << std::endl;
			continue;
		}

		m_Buffer = malloc(m_BufferSize);
		if (!m_Buffer) {
			std::cerr << "Failed to allocate memory" << std::endl;
			continue;
		}

		status = api.ZwQueryInformationProcess(hProcess, ProcessImageFileName, m_Buffer, m_BufferSize, nullptr);

		if (!NT_SUCCESS(status)) {
			std::cerr << "Failed to query process information" << std::endl;
			free(m_Buffer);
			m_Buffer = nullptr;
			continue;
		}

		UNICODE_STRING newProcessInfo = *(PUNICODE_STRING)m_Buffer;
		if (newProcessInfo.Buffer) {
			processList[i].processPath = newProcessInfo.Buffer;
		}
		

	}
	NtPathToDos();

	return;
}

void ProcessManage::NtPathToDos() {
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
		for (auto& p2 : processList) {
			if (p2.processPath.find(p.first) == 0) {
				p2.processPath.replace(0, p.first.length(), p.second);
			}
		}
	}

	return;
	
}
