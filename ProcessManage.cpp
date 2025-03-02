#include "ProcessManage.h"
#include <Windows.h>
#include <iostream>
#include <winternl.h>

// [ZwQuerySystemInformation ��Windows 8���ٿ��á� ����ñ��������г��ı��ú�����]
// ���붯̬���ã�������������ָ��
typedef NTSTATUS(WINAPI* hZwQuerySystemInformation)(
	SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation,
	ULONG SystemInformationLength, PULONG ReturnLength);

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

ProcessManage::ProcessManage() {}

ProcessManage::~ProcessManage() {}

void ProcessManage::InitProcessList() {
	// ��ȡ ntdll.dll ģ����
	HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtDll) {
		std::cerr << "Failed to get ntdll.dll handle" << std::endl;
		return;
	}

	// ��ȡ ZwQuerySystemInformation ������ַ
	hZwQuerySystemInformation ZwQuerySystemInformation =
		(hZwQuerySystemInformation)GetProcAddress(hNtDll,
			"ZwQuerySystemInformation");
	if (!ZwQuerySystemInformation) {
		std::cerr << "Failed to get ZwQuerySystemInformation address" << std::endl;
		return;
	}

	// ָ����д����������Ϣ��ʵ�ʴ�С��λ�õĿ�ѡָ�롣 ����ô�СС�ڻ���� SystemInformationLength ������
	// �ú����Ὣ��Ϣ���Ƶ� SystemInformation ��������;������������ NTSTATUS ������룬���� ReturnLength ��
	// ���ؽ����������Ϣ����Ļ�������С��

	ULONG m_BufferSize = 0;
	PVOID m_Buffer = nullptr;

	// �����ָ���ȡ��������С
	NTSTATUS status = ZwQuerySystemInformation(SystemProcessInformation, nullptr,
		0, &m_BufferSize);

	if (m_BufferSize == 0) {
		std::cerr << "Failed to get buffer size" << std::endl;
		return;
	}

	// �����ڴ�
	m_Buffer = malloc(m_BufferSize);
	if (!m_Buffer) {
		std::cerr << "Failed to allocate memory" << std::endl;
		return;
	}

	// ��ȡ������Ϣ
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


	// ����������Ϣ
	while (true) {
		if (processInfo->ImageName.Buffer) {
			processList.pid = HandleToULong(processInfo->UniqueProcessId);
			processList.processName = processInfo->ImageName.Buffer;
		}
		else {
			processList.pid = HandleToULong(processInfo->UniqueProcessId);
			processList.processName = L"ϵͳ���н���";
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
