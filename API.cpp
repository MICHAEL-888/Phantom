#include "API.h"

API::API()
{
	m_hNtDll = nullptr;
	m_ZwQuerySystemInformation = nullptr;
	m_ZwQueryInformationProcess = nullptr;
	InitModuleHandle();
	InitProcAdress();
}

API::~API() {}

void API::InitModuleHandle()
{
	m_hNtDll = GetModuleHandleW(L"ntdll.dll");

	if (!m_hNtDll) {
		std::cerr << "API::InitModuleHandle \"Failed to get ntdll.dll handle\"" << std::endl;
		return;
	}

	return;

}

void API::InitProcAdress()
{
	m_ZwQuerySystemInformation = 
		(hZwQuerySystemInformation)GetProcAddress(m_hNtDll, "ZwQuerySystemInformation");

	if (!m_ZwQuerySystemInformation) {
		std::cerr << "API::InitProcAdress \"Failed to get ZwQuerySystemInformation address\"" << std::endl;
		return;
	}

	m_ZwQueryInformationProcess =
		(hZwQueryInformationProcess)GetProcAddress(m_hNtDll, "ZwQueryInformationProcess");

	if (!m_ZwQueryInformationProcess) {
		std::cerr << "API::InitProcAdress \"Failed to get ZwQueryInformationProcess address\"" << std::endl;
		return;
	}

	m_ZwOpenProcess = 
		(hZwOpenProcess)GetProcAddress(m_hNtDll, "ZwOpenProcess");

	if (!m_ZwOpenProcess) {
		std::cerr << "API::InitProcAdress \"Failed to get ZwQueryInformationProcess address\"" << std::endl << std::flush;
		return;
	}

	return;
}

NTSTATUS API::ZwOpenProcess(PHANDLE ProcessHandle, ACCESS_MASK DesiredAccess, 
	POBJECT_ATTRIBUTES ObjectAttributes, CLIENT_ID* ClientId) {
	return m_ZwOpenProcess(ProcessHandle, DesiredAccess,
		ObjectAttributes, ClientId);
}

HANDLE API::ZwOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
	HANDLE hProcess;
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, NULL, NULL, NULL, NULL);
	CLIENT_ID clientID = { 0 };
	clientID.UniqueProcess = ULongToHandle(dwProcessId);
	NTSTATUS status = m_ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &ObjectAttributes, &clientID);
	if (!NT_SUCCESS(status)) {
		std::cerr << "API::ZwOpenProcess \"Failed to open process\"";
		return 0;
	}

	return hProcess;
}

NTSTATUS API::ZwQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
	PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength) {
	return m_ZwQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation,
		ProcessInformationLength, ReturnLength);
}

NTSTATUS API::ZwQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
	ULONG SystemInformationLength, PULONG ReturnLength) {
	return m_ZwQuerySystemInformation(SystemInformationClass, SystemInformation,
		SystemInformationLength, ReturnLength);
}