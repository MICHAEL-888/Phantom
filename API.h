﻿#pragma once
#include <Windows.h>
#include <iostream>
#include <winternl.h>

// [ZwQuerySystemInformation 自Windows 8起不再可用。
// 请改用本主题中列出的备用函数。] 必须动态调用，首先声明函数指针
typedef NTSTATUS(WINAPI* hZwQuerySystemInformation)(
	SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
	ULONG SystemInformationLength, PULONG ReturnLength);

// NTSTATUS WINAPI ZwQuerySystemInformation(
//	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
//	_Inout_   PVOID                    SystemInformation,
//	_In_      ULONG                    SystemInformationLength,
//	_Out_opt_ PULONG                   ReturnLength
//);

// typedef struct _SYSTEM_PROCESS_INFORMATION {
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
// } SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

// 如果使用 ZwQueryInformationProcess，请通过 运行时动态链接访问函数。
// 这样，代码就有机会在操作系统中更改或删除函数时正常响应。
// 但是，签名更改可能无法检测到。 此函数没有关联的导入库。 必须使用 LoadLibrary
// 和 GetProcAddress 函数动态链接到 Ntdll.dll。
typedef NTSTATUS(WINAPI* hZwQueryInformationProcess)(
	HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation, ULONG ProcessInformationLength,
	PULONG ReturnLength);

// NTSTATUS WINAPI ZwQueryInformationProcess(
//	_In_      HANDLE           ProcessHandle,
//	_In_      PROCESSINFOCLASS ProcessInformationClass,
//	_Out_     PVOID            ProcessInformation,
//	_In_      ULONG            ProcessInformationLength,
//	_Out_opt_ PULONG           ReturnLength
//);

// msdn文档中第四个参数为PCLIENT_ID ClientId，此处手动调整一下。

typedef NTSTATUS(WINAPI* hZwOpenProcess)(PHANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	CLIENT_ID* ClientId);

typedef NTSTATUS(WINAPI* hZwOpenProcessToken)(HANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	PHANDLE TokenHandle);

typedef NTSTATUS(NTAPI* hZwDuplicateToken)(HANDLE ExistingTokenHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	BOOLEAN EffectiveOnly,
	TOKEN_TYPE Type,
	PHANDLE NewTokenHandle);

typedef NTSTATUS (NTAPI* hZwReadVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer,
	SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead);

typedef NTSTATUS(NTAPI* hZwTerminateProcess)(HANDLE ProcessHandle, NTSTATUS ExitStatus);

typedef _Function_class_(PS_APC_ROUTINE)
VOID NTAPI PS_APC_ROUTINE(
	PVOID ApcArgument1,
	PVOID ApcArgument2,
	PVOID ApcArgument3
);

typedef PS_APC_ROUTINE* PPS_APC_ROUTINE;

typedef NTSTATUS(NTAPI* hNtQueueApcThreadEx)(
	HANDLE ThreadHandle,                        //需要插入的线程句柄
	HANDLE flag,                                //0：用户普通APC  1：用户特殊APC  其余值也为普通APC
	PPS_APC_ROUTINE ApcRoutine,                        //需要执行的APC函数指针
	PVOID NormalContext,                //需要执行的APC函数的第零个参数
	PVOID SystemArgument1,        //需要执行的APC函数的第一个参数
	PVOID SystemArgument2);        //需要执行的APC函数的第二个参数

class API {
public:
	API();
	~API();
	void InitModuleHandle();
	void InitProcAdress();

	NTSTATUS ZwOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		CLIENT_ID* ClientId);
	HANDLE
		ZwOpenProcess(DWORD dwDesiredAccess,
			BOOL bInheritHandle, // 第二个参数没用，为了对齐OpenProcess格式
			DWORD dwProcessId);

	NTSTATUS ZwQueryInformationProcess(HANDLE ProcessHandle,
		PROCESSINFOCLASS ProcessInformationClass,
		PVOID ProcessInformation,
		ULONG ProcessInformationLength,
		PULONG ReturnLength);

	NTSTATUS
		ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
			PVOID SystemInformation,
			ULONG SystemInformationLength, PULONG ReturnLength);

	NTSTATUS ZwOpenProcessToken(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess,
		PHANDLE TokenHandle);

	NTSTATUS ZwDuplicateToken(HANDLE ExistingTokenHandle,
		ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		BOOLEAN EffectiveOnly, TOKEN_TYPE Type,
		PHANDLE NewTokenHandle);

	NTSTATUS ZwReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer,
		SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead);

	NTSTATUS ZwTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

	NTSTATUS NtQueueApcThreadEx(
		HANDLE ThreadHandle,
		HANDLE ReserveHandle,
		PPS_APC_ROUTINE ApcRoutine,
		PVOID ApcArgument1,
		PVOID ApcArgument2,
		PVOID ApcArgument3
	);

private:
	HMODULE m_hNtDll;

	hZwQuerySystemInformation m_ZwQuerySystemInformation;
	hZwQueryInformationProcess m_ZwQueryInformationProcess;
	hZwOpenProcess m_ZwOpenProcess;
	hZwOpenProcessToken m_ZwOpenProcessToken;
	hZwDuplicateToken m_ZwDuplicateToken;
	hZwReadVirtualMemory m_ZwReadVirtualMemory;
	hZwTerminateProcess m_ZwTerminateProcess;
	hNtQueueApcThreadEx m_NtQueueApcThreadEx;
};
