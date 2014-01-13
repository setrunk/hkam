#pragma once

enum NODE_SOURCE
{
	SOURCE_FROM_FILE,
	SOURCE_INTERNAL,
	SOURCE_SOUND,
	SOURCE_BUILDIN_CUBE,
	SOURCE_BUILDIN_PLANE,
	SOURCE_BUILDIN_CYLINDER,
	SOURCE_BUILDIN_SPHERE,
	SOURCE_COUNT,
};

static LPCTSTR SOURCE_NAME[] = 
{
	L"FromFile",
	L"Internal",
	L"Sound",
	L"Build_in_Cube",
	L"Build_in_Plane",
	L"Build_in_Cylinder",
	L"Build_in_Sphere",
};

struct SNodeSource
{
	SimNode*	Node;
	int			Target;
	NODE_SOURCE	SourceType;
	XString		SourceFile;
	int			SourceIndex;
};

static LPCTSTR GetSourceName(NODE_SOURCE source)
{
	return SOURCE_NAME[source];
}

static NODE_SOURCE GetSourceType(LPCTSTR name)
{
	for (int i = 0; i < SOURCE_COUNT; ++i)
	{
		if (lstrcmp(name, SOURCE_NAME[i]) == 0)
			return (NODE_SOURCE)i;
	}

	return (NODE_SOURCE)-1;
}
