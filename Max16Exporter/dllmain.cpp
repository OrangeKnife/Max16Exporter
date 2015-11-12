// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Max16Exporter.h"
#include <tchar.h>
#include <windows.h>

#define MAXEXPORT_API extern "C" __declspec(dllexport)

using namespace NBE;
using namespace std;

HINSTANCE g_instance;

class ExporterDesc : public ClassDesc2
{
public:
	int IsPublic() {return TRUE;}
	void* Create(BOOL loading = FALSE) {return new Max16Exporter();}
	const MCHAR* ClassName() { return _T("3ds max 12 exporter"); }
	SClass_ID SuperClassID() {return SCENE_EXPORT_CLASS_ID;}
	Class_ID ClassID() {return Class_ID(0x9d92b10, 0x5aaa21cd);}
	const MCHAR* Category() { return _T("Exporters"); }
	const MCHAR* InternalName() { return _T("Max16Exporter"); }
	HINSTANCE HInstance() {return g_instance;}
	
};


BOOL WINAPI DllMain( HINSTANCE inst, ULONG reason, LPVOID )
{
	if ( DLL_PROCESS_ATTACH == reason )
	{
		g_instance = inst;
		DisableThreadLibraryCalls( g_instance );
		return TRUE;
	}

	return TRUE;
}

MAXEXPORT_API const TCHAR* LibDescription()
{
	return _T("3dsmax 16 Exporter");
}

MAXEXPORT_API int LibNumberClasses()
{
	return 1;
}

MAXEXPORT_API ClassDesc* LibClassDesc( int i )
{
	switch( i ) 
	{
		case 0:{
			static ExporterDesc s_classDesc;
			return &s_classDesc;}
		default:
			return 0;
	}
}

MAXEXPORT_API ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

MAXEXPORT_API ULONG CanAutoDefer()
{
	return 1;
}