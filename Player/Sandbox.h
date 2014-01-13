#pragma once

class CLibrary;

class CSandbox
{
public:
	CSandbox(void);
	~CSandbox(void);

	CLibrary* m_pLibrary;
};

extern CSandbox theApp;