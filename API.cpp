#include "API.h"

API::API()
	: m_hNtDll(nullptr), m_ZwQuerySystemInformation(nullptr),
	m_ZwQueryInformationProcess(nullptr) {
	InitModuleHandle();
	InitProcAdress();
}

API::~API() {}

void API::InitModuleHandle() {
	m_hNtDll = GetModuleHandleW(L"ntdll.dll");

	if (!m_hNtDll) {
		std::cerr << "API::InitModuleHandle \"Failed to get ntdll.dll handle\""
			<< std::endl;
		return;
	}

	return;
}

void API::InitProcAdress() {
	m_ZwQuerySystemInformation = (hZwQuerySystemInformation)GetProcAddress(
		m_hNtDll, "ZwQuerySystemInformation");

	if (!m_ZwQuerySystemInformation) {
		std::cerr << "API::InitProcAdress \"Failed to get ZwQuerySystemInformation "
			"address\""
			<< std::endl;
		return;
	}

	m_ZwQueryInformationProcess = (hZwQueryInformationProcess)GetProcAddress(
		m_hNtDll, "ZwQueryInformationProcess");

	if (!m_ZwQueryInformationProcess) {
		std::cerr << "API::InitProcAdress \"Failed to get "
			"ZwQueryInformationProcess address\""
			<< std::endl;
		return;
	}

	m_ZwOpenProcess = (hZwOpenProcess)GetProcAddress(m_hNtDll, "ZwOpenProcess");

	if (!m_ZwOpenProcess) {
		std::cerr << "API::InitProcAdress \"Failed to get "
			"ZwQueryInformationProcess address\""
			<< std::endl
			<< std::flush;
		return;
	}

	m_ZwOpenProcessToken =
		(hZwOpenProcessToken)GetProcAddress(m_hNtDll, "ZwOpenProcessToken");

	if (!m_ZwOpenProcessToken) {
		std::cerr << "API::InitProcAdress \"Failed to get "
			"ZwOpenProcessToken address\""
			<< std::endl
			<< std::flush;
		return;
	}

	m_ZwDuplicateToken =
		(hZwDuplicateToken)GetProcAddress(m_hNtDll, "ZwDuplicateToken");

	if (!m_ZwOpenProcessToken) {
		std::cerr << "API::InitProcAdress \"Failed to get "
			"ZwDuplicateToken address\""
			<< std::endl
			<< std::flush;
		return;
	}

	m_ZwReadVirtualMemory = (hZwReadVirtualMemory)GetProcAddress(m_hNtDll,
		"ZwReadVirtualMemory");

	if (!m_ZwReadVirtualMemory) {
		std::cerr << "API::InitProcAdress \"Failed to get "
			"ZwReadVirtualMemory address\""
			<< std::endl
			<< std::flush;
		return;
	}

	m_ZwTerminateProcess = (hZwTerminateProcess)GetProcAddress(m_hNtDll,
		"ZwTerminateProcess");

	if (!m_ZwTerminateProcess) {
		std::cerr << "API::InitProcAdress \"Failed to get "
			"ZwTerminateProcess address\""
			<< std::endl
			<< std::flush;
		return;
	}

	m_NtQueueApcThreadEx = (hNtQueueApcThreadEx)GetProcAddress(m_hNtDll,
		"NtQueueApcThreadEx");

	if (!m_NtQueueApcThreadEx) {	
		std::cerr << "API::InitProcAdress \"Failed to get "
			"NtQueueApcThreadEx address\""
			<< std::endl
			<< std::flush;
		return;
	}

	return;
}

NTSTATUS API::ZwOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	CLIENT_ID* ClientId) {
	return m_ZwOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes,
		ClientId);
}

HANDLE API::ZwOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle,
	DWORD dwProcessId) {
	HANDLE hProcess;
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, NULL, bInheritHandle, NULL, NULL);
	CLIENT_ID clientID = { 0 };
	clientID.UniqueProcess = ULongToHandle(dwProcessId);
	NTSTATUS status =
		m_ZwOpenProcess(&hProcess, dwDesiredAccess,
			&ObjectAttributes, &clientID);
	if (!NT_SUCCESS(status)) {
		std::cerr << "API::ZwOpenProcess \"Failed to open process\"";
		return 0;
	}

	return hProcess;
}

NTSTATUS API::ZwQueryInformationProcess(
	HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation, ULONG ProcessInformationLength,
	PULONG ReturnLength) {
	return m_ZwQueryInformationProcess(ProcessHandle, ProcessInformationClass,
		ProcessInformation,
		ProcessInformationLength, ReturnLength);
}

NTSTATUS API::ZwQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
	ULONG SystemInformationLength, PULONG ReturnLength) {
	return m_ZwQuerySystemInformation(SystemInformationClass, SystemInformation,
		SystemInformationLength, ReturnLength);
}

NTSTATUS API::ZwOpenProcessToken(HANDLE ProcessHandle,
	ACCESS_MASK DesiredAccess,
	PHANDLE TokenHandle) {
	return m_ZwOpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle);
}

NTSTATUS API::ZwDuplicateToken(HANDLE ExistingTokenHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	BOOLEAN EffectiveOnly, TOKEN_TYPE Type,
	PHANDLE NewTokenHandle) {
	return m_ZwDuplicateToken(ExistingTokenHandle, DesiredAccess,
		ObjectAttributes, EffectiveOnly, Type,
		NewTokenHandle);
}

NTSTATUS API::ZwReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer,
	SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead) {
	return m_ZwReadVirtualMemory(ProcessHandle, BaseAddress, Buffer,
		NumberOfBytesToRead, NumberOfBytesRead);
}

NTSTATUS API::ZwTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus) {
	return m_ZwTerminateProcess(ProcessHandle, ExitStatus);
}

NTSTATUS API::NtQueueApcThreadEx(HANDLE ThreadHandle, HANDLE flag,
	PPS_APC_ROUTINE ApcRoutine, PVOID NormalContext,
	PVOID SystemArgument1, PVOID SystemArgument2) {
	return m_NtQueueApcThreadEx(ThreadHandle, flag, ApcRoutine,
		NormalContext, SystemArgument1, SystemArgument2);
}