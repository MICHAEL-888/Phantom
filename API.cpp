#include "API.h"

API::API()
{
	hNtDll = nullptr;
	ZwQuerySystemInformation = nullptr;
	ZwQueryInformationProcess = nullptr;
	InitModuleHandle();
	InitProcAdress();
}

API::~API()
{
}

void API::InitModuleHandle()
{
	hNtDll = GetModuleHandleW(L"ntdll.dll");

	if (!hNtDll) {
		std::cerr << "Failed to get ntdll.dll handle" << std::endl;
		return;
	}

	return;

}

void API::InitProcAdress()
{
	ZwQuerySystemInformation = 
		(hZwQuerySystemInformation)GetProcAddress(hNtDll, "ZwQuerySystemInformation");

	if (!ZwQuerySystemInformation) {
		std::cerr << "Failed to get ZwQuerySystemInformation address" << std::endl;
		return;
	}

	ZwQueryInformationProcess =
		(hZwQueryInformationProcess)GetProcAddress(hNtDll, "ZwQueryInformationProcess");

	if (!ZwQueryInformationProcess) {
		std::cerr << "Failed to get ZwQueryInformationProcess address" << std::endl;
		return;
	}

	ZwOpenProcess = 
		(hZwOpenProcess)GetProcAddress(hNtDll, "ZwOpenProcess");

	if (!ZwOpenProcess) {
		std::cerr << "Failed to get ZwQueryInformationProcess address" << std::endl << std::flush;
		return;
	}

	return;
}

