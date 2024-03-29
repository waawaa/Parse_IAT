#include <stdio.h>

#define ULONG unsigned long
#define USHORT unsigned short
#define UCHAR unsigned char



void main(void)
{


	typedef struct _IMAGE_IMPORT_DESCRIPTOR {
		union {
			ULONG Characteristics;
			ULONG OriginalFirstThunk;
		} DUMMYUNIONNAME;
		ULONG TimeDateStamp;
		ULONG ForwarderChain;
		ULONG Name;
		ULONG FirstThunk;
	} IMAGE_IMPORT_DESCRIPTOR, * PIMAGE_IMPORT_DESCRIPTOR;


	typedef struct _IMAGE_IMPORT_BY_NAME {
		USHORT Hint;
		UCHAR Name[1];
	} IMAGE_IMPORT_BY_NAME, * PIMAGE_IMPORT_BY_NAME;



	typedef struct _IMAGE_THUNK_DATA {
		union {
			ULONG ForwarderString;
			ULONG Function;
			ULONG Ordinal;
			ULONG AddressOfData;
		} u1;
	} IMAGE_THUNK_DATA, * PIMAGE_THUNK_DATA;


	_asm
	{
		//The first adjust the stack to save data of IMAGE_EXPORT_DIRECTORY, pToAddrOfOrdinal, pToAddrOfNames, pToAddrOfAddr
		sub esp, 0x400
		xor ecx, ecx //Zero ECX
		mov esi, fs: [ecx + 0x18] //Go to _TEB
		mov esi, dword ptr ds : [esi + 0x30] //Go to _PEB
		mov ecx, dword ptr ds : [esi + 0x08] //Extract ImageBase from _PEB
		mov[ebp - 4], ecx //Save ImageBase in Stack
		add ecx, dword ptr ds : [ecx + 0x3C] //Look for _IMAGE_NT_HEADERS from _IMAGE_DOS_HEADERS
		lea ecx, dword ptr ds : [ecx + 0x78] //Look from _IMAGE_NT_HEADERS to _IMAGE_OPTIONAL_HEADER 
		//(_IMAGE_NT_HEADERS->_IMAGE_OPTIONAL_HEADER (ImageNtHeaders+0x18)
		//And later _IMAGE_OPTIONAL_HEADER->DataDirectory (ImageOptionalHeader+0x60) 
		//So _IMAGE_NT_HEADERS+0x78 points to DataDirectory
		mov esi, dword ptr ds : [ecx + 8] //Move to IMAGE_DIRECTORY_ENTRY_IMPORT      
		mov eax, [ebp - 4]
		add eax, esi  //Add ImageBase to RVA of _IMAGE_IMPORT_DESCRIPTORS
		mov[ebp - 0x8], eax //Save addr of _IMAGE_IMPORT_DESCRIPTORS in the stack
		xor ecx, ecx
		xor esi, esi
		IterateNames :
		mov eax, [ebp - 0x8] //For the loop, we restore _IMAGE_IMPORT_DESCRIPTORS addr from stack
			mov eax, dword ptr ds : [eax] //Save in eax RVA of addr of _IMAGE_IMPORT_BY_NAME
			add eax, [ebp - 4] //Add ImageBase
			//We have EAX pointing to _IMAGE_IMPORT_BY_NAME
			mov ebx, dword ptr ds : [eax + esi] //Add esi plus 4
			add esi, 0x4 //Increment esi
			inc ecx //Increment ecx (counter)
			add ebx, [ebp - 4] //Add to ebx the ImageBase + offset of addr
			add ebx, 0x2 //We jump to the CHAR
			lea edi, [ebx] //Move name to EDI
			mov eax, 0x50746547 //Move to EAX the string
			scasd //Scan GetP
			mov edx, ebx //Save in EDX the addr of the string
			jnz IterateNames //Loop if not equal
			lea edi, [edx + 0xD] //Save in EDI the GetProcAddre(s) and '\0'
			xor eax, eax //Zero eax
			mov al, 0x73 //Move to EAX the final S
			scasw //Scan GetProcAddress
			jnz IterateNames //Loop if not equal
			dec ecx //We have a ECX extra, adjuts
			GetAddrOfFirstThunk :
		mov eax, [ebp - 0x8] //Save addr of _IMAGE_IMPORT_DESCRIPTORS in EAX
			add eax, 0x10 //Go to FirstThunk
			mov eax, dword ptr ds : [eax] //Move to EAX the RVA of FirstThunk
			add eax, [ebp - 0x4] //Add to EAX ImageBase
			imul ecx, 4 //Adjust ecx by multiplying with 4 because each addr is a DWORD
			add eax, ecx //Add the counter*4 to the addr of FirstThunk
			//This is to jump to the addr of the function GetProcAddr
			mov edi, dword ptr ds : [eax] //Move the addr to Stack
			mov[ebp - 0xc], edi
			GetAddrOfKernel : //This function get addr of Kernel32
		xor ecx, ecx
			mov esi, fs : [ecx + 0x18]
			mov edi, dword ptr ds : [esi + 0x30]
			mov esi, dword ptr ds : [edi + 0xC]
			mov ecx, dword ptr ds : [esi + 0xc]
			GetNamesLoop :
			mov[ebp - 0x10], ecx
			mov esi, dword ptr ds : [ecx + 0x4 + 0x2c]
			cmp dword ptr ds : [esi + 0xc] , 0x320033
			mov ecx, [ebp - 0x10]
			mov esi, ecx
			mov ecx, dword ptr ds : [ecx]
			jne GetAddrOfKernel //Once out of the loop save the DllBase to [ebp-0x10]
			add esi, 0x18
			mov ecx, dword ptr ds : [esi]
			mov[ebp - 0x10], ecx //Guarda DLLBase


			CallWinExec : //Call calc.exe with the GetProcAddress of IAT and resolving address of WinExec (not imported by the IAT)
		xor ecx, ecx
			mov ebx, [ebp - 0x10]
			mov edx, [ebp - 0xc]
			push ebx
			push edx
			push ecx
			push 0x636578 //push WinExec
			push 0x456E6957
			push esp
			push ebx
			call edx
			test al, al
			je EndFunction
			xor ecx, ecx
			push ecx
			push 0x6578652E
			push 0x636c6163
			push esp
			inc ecx
			call eax
			EndFunction :
		add esp, 0x400
	};
}