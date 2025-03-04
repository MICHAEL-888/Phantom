﻿#include "ProcessManage.h"
#include <Windows.h>
#include <iostream>
#include <winternl.h>
#include <ntddk.h>

// [ZwQuerySystemInformation 自Windows 8起不再可用。 请改用本主题中列出的备用函数。]
// 必须动态调用，首先声明函数指针
typedef NTSTATUS(WINAPI* hZwQuerySystemInformation)(
	SYSTEM_INFORMATION_CLASS SystemInformationClass, 
	PVOID SystemInformation,
	ULONG SystemInformationLength, 
	PULONG ReturnLength);

//NTSTATUS WINAPI ZwQuerySystemInformation(
//	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
//	_Inout_   PVOID                    SystemInformation,
//	_In_      ULONG                    SystemInformationLength,
//	_Out_opt_ PULONG                   ReturnLength
//);

//typedef struct _SYSTEM_PROCESS_INFORMATION {
//	ULONG NextEntryOffset;
//	ULONG NumberOfThreads;
//	BYTE Reserved1[48];
//	UNICODE_STRING ImageName;
//	KPRIORITY BasePriority;
//	HANDLE UniqueProcessId;
//	PVOID Reserved2;
//	ULONG HandleCount;
//	ULONG SessionId;
//	PVOID Reserved3;
//	SIZE_T PeakVirtualSize;
//	SIZE_T VirtualSize;
//	ULONG Reserved4;
//	SIZE_T PeakWorkingSetSize;
//	SIZE_T WorkingSetSize;
//	PVOID Reserved5;
//	SIZE_T QuotaPagedPoolUsage;
//	PVOID Reserved6;
//	SIZE_T QuotaNonPagedPoolUsage;
//	SIZE_T PagefileUsage;
//	SIZE_T PeakPagefileUsage;
//	SIZE_T PrivatePageCount;
//	LARGE_INTEGER Reserved7[6];
//} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

//如果使用 ZwQueryInformationProcess，请通过 运行时动态链接访问函数。 
//这样，代码就有机会在操作系统中更改或删除函数时正常响应。 但是，签名更改可能无法检测到。
//此函数没有关联的导入库。 必须使用 LoadLibrary 和 GetProcAddress 函数动态链接到 Ntdll.dll。

typedef NTSTATUS(WINAPI* hZwQueryInformationProcess)(
	HANDLE ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation,
	ULONG ProcessInformationLength,
	PULONG ReturnLength
	);


//NTSTATUS WINAPI ZwQueryInformationProcess(
//	_In_      HANDLE           ProcessHandle,
//	_In_      PROCESSINFOCLASS ProcessInformationClass,
//	_Out_     PVOID            ProcessInformation,
//	_In_      ULONG            ProcessInformationLength,
//	_Out_opt_ PULONG           ReturnLength
//);

ProcessManage::ProcessManage() {
	InitProcessList();
	InitProcessPath();
}

ProcessManage::~ProcessManage() {}

void ProcessManage::InitProcessList() {
	// 获取 ntdll.dll 模块句柄
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtDll) {
		std::cerr << "Failed to get ntdll.dll handle" << std::endl;
		return;
	}

	// 获取 ZwQuerySystemInformation 函数地址
	hZwQuerySystemInformation ZwQuerySystemInformation =
		(hZwQuerySystemInformation)GetProcAddress(hNtDll,
			"ZwQuerySystemInformation");
	if (!ZwQuerySystemInformation) {
		std::cerr << "Failed to get ZwQuerySystemInformation address" << std::endl;
		return;
	}

	// 指向函数写入所请求信息的实际大小的位置的可选指针。 如果该大小小于或等于 SystemInformationLength 参数，
	// 该函数会将信息复制到 SystemInformation 缓冲区中;否则，它将返回 NTSTATUS 错误代码，并在 ReturnLength 中
	// 返回接收请求的信息所需的缓冲区大小。

	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;

	// 传入空指针获取缓冲区大小
	NTSTATUS status = ZwQuerySystemInformation(SystemProcessInformation, nullptr,
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
	status = ZwQuerySystemInformation(SystemProcessInformation, m_Buffer,
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
	// 获取 ntdll.dll 模块句柄
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtDll) {
		std::cerr << "Failed to get ntdll.dll handle" << std::endl;
		return;
	}

	// 获取ZwQueryInformationProcess函数地址
	hZwQueryInformationProcess ZwQueryInformationProcess =
		(hZwQueryInformationProcess)GetProcAddress(hNtDll, "ZwQueryInformationProcess");
	if (!ZwQueryInformationProcess) {
		std::cerr << "Failed to get ZwQuerySystemInformation address" << std::endl;
		return;
	}

	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;
	NTSTATUS status = ZwQueryInformationProcess(ULongToHandle(processList[8].pid), ProcessImageFileName, nullptr, 0, &m_BufferSize);

	if (m_BufferSize == 0) {
		std::cerr << "Failed to get buffer size" << std::endl;
		return;
	}

	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "Failed to allocate memory" << std::endl;
		return;
	}

	status = ZwQueryInformationProcess(ULongToHandle(processList[8].pid), ProcessImageFileName, m_Buffer, m_BufferSize, nullptr);

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
