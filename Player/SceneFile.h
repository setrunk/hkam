#pragma once

enum CREATE_NODE_TYPE
{
	CREATE_SCENE_NODE,
	CREATE_CAMERA,
	CREATE_LIGHT,
	CREATE_SOUND,
	CREATE_WATER,
	CREATE_CUBE,
	CREATE_PLANE,
	CREATE_CYLINDER,
	CREATE_SPHERE,
};

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

//////////////////////////////////////////////////////////////////////////

struct SModelFile
{
	XArray<SimNode*> Objects;
};

class CSceneFile
{
public:
	CSceneFile(void);
	~CSceneFile(void);

	void			OnInit();
	void			InitData();
	void			ResolveFolder(const XString& path);

	void			ClearAll();

	BOOL			OnOpen(LPCTSTR filename);
	void			ReadScene(SimXMLReader *reader);
	SimNode*		ReadNode(SimXMLReader *reader, SimNode* pParent);
	void			Read3DObject(SimXMLReader* reader, Sim3DObject *p3DObj);
	void			ReadCamera(SimXMLReader* reader, SimCamera *camera);
	void			ReadLight(SimXMLReader* reader, SimLight *light);
	void			ReadSound(SimXMLReader* reader, SimSound *pSound);
	void			ReadWater(SimXMLReader* reader, SimWaterSurface *pWater);

	void			ReadMaterial(LPCTSTR path, SimMaterial *pMat);
	void			ReadTexture(LPCTSTR path, SimTexture2D *pTex);
	void			ReadRenderTexture(LPCTSTR path, SimRenderTexture *pTex);
	void			ReadTransformAnim(LPCTSTR path, SimObjectAnim *pAnim);
	void			ReadTextureAnim(LPCTSTR path, SimTextureAnim *pAnim);
	void			ReadModelFile(LPCTSTR path, SModelFile *pModel);

	SimNode*		GetModelNode(LPCTSTR filename, int index);
	SimNode*		CreateNode(CREATE_NODE_TYPE type);
	SimNode*		ClonePreCreate(LPCTSTR name);

	SimObject*		GetObjectByPath(LPCTSTR path);
	XString			GetPathByObject(SimObject *pObj);

	void			SetAnimTextureSequence(SimTextureAnim *pAnim, SimTexture2D *pFirstTex);

	BOOL			IsImageFile(LPCTSTR filename);
	BOOL			IsRenderTextureFile(LPCTSTR filename);
	BOOL			IsMaterialFile(LPCTSTR filename);
	BOOL			IsTransformAnimFile(LPCTSTR filename);
	BOOL			IsTextureAnimFile(LPCTSTR filename);
	BOOL			IsSoundFile(LPCTSTR filename);
	BOOL			IsModelFile(LPCTSTR filename);

	XString			ToShortPath(const XString &path);
	XString			ToLongPath(LPCTSTR path) { return m_strRootPath + path; }

	XArray<SimNode*>				m_NodeList;
	XArray<SimNode*>				m_PreCreate;
	SimMaterial*					m_DefaultMat;
	SimCamera*						m_MainCamera;
	XString							m_strRootPath;

	XTable<XString, SimObject*>		m_ObjectList;
};
