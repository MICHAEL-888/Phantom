#pragma once
#include <windows.h>

class PrivilegeElevate
{
public:
	PrivilegeElevate() = default;
	~PrivilegeElevate() = default;

	bool UsertoAdmin();
	bool AdmintoSystem();


};
