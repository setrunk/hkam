#include "StdAfx.h"
#include "Sandbox.h"

CSandbox theApp;

CSandbox::CSandbox(void)
{
	m_pLibrary = new CLibrary();
}

CSandbox::~CSandbox(void)
{
	SAFE_DELETE(m_pLibrary);
}
