#include "ProcessManage.h"
#include <Windows.h>
#include <iostream>
#include <winternl.h>
#include "API.h"

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
	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;
	NTSTATUS status = api.ZwQueryInformationProcess(ULongToHandle(processList[8].pid), ProcessImageFileName, nullptr, 0, &m_BufferSize);

	if (m_BufferSize == 0) {
		std::cerr << "Failed to get buffer size" << std::endl;
		return;
	}

	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "Failed to allocate memory" << std::endl;
		return;
	}

	status = api.ZwQueryInformationProcess(ULongToHandle(processList[8].pid), ProcessImageFileName, m_Buffer, m_BufferSize, nullptr);

	if (!NT_SUCCESS(status)) {
		std::cerr << "Failed to query process information" << std::endl;
		free(m_Buffer);
		m_Buffer = nullptr;
		return;
	}

	UNICODE_STRING newProcessInfo = *(PUNICODE_STRING)m_Buffer;
	processList[8].processPath = newProcessInfo.Buffer;
	std::wcout << processList[8].processPath << std::endl;

	return;
}
