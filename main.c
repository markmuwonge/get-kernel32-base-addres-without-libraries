void* GetKernel32();
void* GetThreadInformationBlock();
void* GetProcessEnvironmentBlock(void *threadInformationBlock);
void* GetPEBLdrData(void* processEnvironmentBlock);
void* GetInMemoryOrderModuleList(void* pebLdrData);
void* GetLdrDataTableEntryFromFlink(void* flink);
void* GetLdrDataTableEntryBase(void* ldrDataTableEntry);
void* GetLdrDataTableEntryFullDllName(void* ldrDataTableEntry);

#define HIDE_LETTER(str)  (str) + 300 //hide artefacts from compiled binary
#define UNHIDE_LETTER(str)  (str) - 300

int _start(){
	void *kernel32 = GetKernel32();

	if (kernel32 == 0x0) return 0;

	return 1;
}

void* GetKernel32(){

	register void *threadInformationBlock = GetThreadInformationBlock(); //holds address to thread information block

	void *processEnvironmentBlock = GetProcessEnvironmentBlock(threadInformationBlock);

	void *pebLdrData = GetPEBLdrData(processEnvironmentBlock);

	void *inMemoryOrderModuleList = GetInMemoryOrderModuleList(pebLdrData);

	void *kernel32DllBase = 0x0;
	char kernel32EncrypedFileName[] = { 
		HIDE_LETTER('k'),
		HIDE_LETTER('e'),
		HIDE_LETTER('r'),
		HIDE_LETTER('n'),
		HIDE_LETTER('e'),
		HIDE_LETTER('l'),
		HIDE_LETTER('3'),
		HIDE_LETTER('2'),
		HIDE_LETTER('.'),
		HIDE_LETTER('d'),
		HIDE_LETTER('l'),
		HIDE_LETTER('l'),
		0
	};

	for (void *flink = *(void **)inMemoryOrderModuleList; ; flink = *(void **) flink){
		void *ldrDataTableEntry = GetLdrDataTableEntryFromFlink(flink);

		void *ldrDataTableEntryDllBase = GetLdrDataTableEntryBase(ldrDataTableEntry);

		if (ldrDataTableEntryDllBase == 0){
			break;
		}

		void *ldrDataTableEntryFullDllName = GetLdrDataTableEntryFullDllName(ldrDataTableEntry);

		unsigned short fullDllNameLength = (unsigned short) (*(void *) ldrDataTableEntryFullDllName);
		
		void *fullDllNameBuffer = ldrDataTableEntryFullDllName + 8;

		void *modulePath = *(void **) fullDllNameBuffer;

		void *modulePathEnd = modulePath + (fullDllNameLength - 1);

		void *moduleNameStart = 0x0;

		for (void* optionalModuleNameStart = modulePathEnd; ; optionalModuleNameStart--){
			if ((char) (*(void*) (optionalModuleNameStart - 2)) == 0x5C){
				moduleNameStart = optionalModuleNameStart;
				break;
			}
		}

		char moduleFound = 0;
		for (int i = 0; ; i++){
			char kernel32FileNameLetter = UNHIDE_LETTER((*(kernel32EncrypedFileName + i)));
			char moduleNameCharacter = (char) *(void *)(moduleNameStart + (i * 2));

			//uppercase letter
			if (((moduleNameCharacter >= 0x41) && (moduleNameCharacter <= 0x5A)) && (moduleNameCharacter + 0x20) != kernel32FileNameLetter){
				break;		
			}
			//lowercase letter
			else if (((moduleNameCharacter >= 0x61) && (moduleNameCharacter <= 0x7A)) && moduleNameCharacter != kernel32FileNameLetter){
				break;
			}
			
			if ((*(kernel32EncrypedFileName + i + 1)) == 0){
				moduleFound = 1;
				break;
			}
		}
		if (moduleFound){
			kernel32DllBase = ldrDataTableEntryDllBase;
			break;
		}
	}
	return kernel32DllBase;
}

void* GetThreadInformationBlock(){
	#ifdef _WIN64
		#define SET_THREAD_INFORMATION_BLOCK_INSTRUCTION "mov %%gs:0x30, %0;" //double percentage to signify literal register name, single percentage for operand (either output register operand or input operand)
	#else
	#endif

	register void *threadInformationBlock; //holds address to thread information block
	asm volatile(
		SET_THREAD_INFORMATION_BLOCK_INSTRUCTION 
		: "=r" (threadInformationBlock)
	);
	return threadInformationBlock;
}

void* GetProcessEnvironmentBlock(void* threadInformationBlock){
	#ifdef _WIN64
		#define THREAD_INFORMATION_BLOCK_PROCESS_ENVIRONMENT_BLOCK_OFFSET 0x60
	#else
	#endif
	
	return *(void **)
	(threadInformationBlock +
	THREAD_INFORMATION_BLOCK_PROCESS_ENVIRONMENT_BLOCK_OFFSET);
}

void* GetPEBLdrData(void* processEnvironmentBlock){
	#ifdef _WIN64
		#define PROCESS_ENVIRONMENT_BLOCK_LDR_DATA_OFFSET 0x18
	#else
	#endif

	return *(void **)
	(processEnvironmentBlock +
	PROCESS_ENVIRONMENT_BLOCK_LDR_DATA_OFFSET);
}
void* GetInMemoryOrderModuleList(void* pebLdrData){
	#ifdef _WIN64
		#define IN_MEMORY_ORDER_MODULE_LIST_OFFSET 0x20 //aka the pointer to the LIST_ENTRY "Flink"
	#else
	#endif
	return pebLdrData +
	IN_MEMORY_ORDER_MODULE_LIST_OFFSET;

}

void* GetLdrDataTableEntryFromFlink(void* flink){
	#ifdef _WIN64
		#define PEB_LDR_DATA_TABLE_ENTRY_FROM_FLINK_OFFSET -0x10
	#else
	#endif
	return flink + PEB_LDR_DATA_TABLE_ENTRY_FROM_FLINK_OFFSET; 
}

void* GetLdrDataTableEntryBase(void* ldrDataTableEntry){
	#ifdef _WIN64
		#define PEB_LDR_DATA_TABLE_ENTRY_DLL_BASE_OFFSET 0x30
	#else
	#endif
	return *(void **) (ldrDataTableEntry + PEB_LDR_DATA_TABLE_ENTRY_DLL_BASE_OFFSET);

}

void* GetLdrDataTableEntryFullDllName(void* ldrDataTableEntry){
	#ifdef _WIN64
		#define PEB_LDR_DATA_TABLE_ENTRY_FULL_DLL_NAME 0x48
	#else
	#endif
	return ldrDataTableEntry + PEB_LDR_DATA_TABLE_ENTRY_FULL_DLL_NAME;
}