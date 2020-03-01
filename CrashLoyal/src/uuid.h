#pragma once
#include "Mob.h"
#include <windows.h>
#pragma comment(lib, "rpcrt4.lib") 
class Uuid {

public:
	UUID uuid;
	char* str;
	Uuid() {
	
		UuidCreate(&uuid);
		UuidToStringA(&uuid, (RPC_CSTR*)&str);
		//std::cout << str<< std::endl;
		
	}
	~Uuid() {
		
	}
	char* getUUid() {
		return str;
	}

};
