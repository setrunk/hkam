#include "StdAfx.h"
#include "Player.h"
#include "SceneFile.h"

//////////////////////////////////////////////////////////////////////////

CSceneFile::CSceneFile(void)
{
}

CSceneFile::~CSceneFile(void)
{
}

XString CSceneFile::ToShortPath(const XString &path)
{
	XString str = path;

	int len = m_strRootPath.size();

	if (path.subString(0, len).equalsi(m_strRootPath))
	{
		str = path.subString(len, 256);
	}

	return str;
}

SimObject* CSceneFile::GetObjectByPath(LPCTSTR path)
{
	XTable<XString, SimObject*>::Iterator it = m_ObjectList.find(path);
	if (it != m_ObjectList.end())
	{
		return *it;
	}

	return NULL;
}

XString CSceneFile::GetPathByObject(SimObject *pObj)
{
	for (int i = 0; i < m_ObjectList.size(); ++i)
	{
		XTable<XString, SimObject*>::Iterator it = m_ObjectList.getByIndex(i);
		if (pObj == (*it))
		{
			return it.getKey();
		}
	}

	return XString(L"");
}

SimNode* CSceneFile::GetModelNode(LPCTSTR filename, int index)
{
	XTable<XString, SimObject*>::Iterator it = m_ObjectList.find(filename);
	if (it != m_ObjectList.end())
	{
		SModelFile *pModel = (SModelFile*)(*it);
		if (pModel)
		{
			if (index >= 0 && index < pModel->Objects.size())
			{
				SimNode *pNode = pModel->Objects[index];
				SIM_CLASS_ID cid = pNode->GetClassID();

				SimNode *pNew = (SimNode*)g_pContext->CreateObject(cid, pNode->GetName());
				pNew->SetWorldMatrix(pNode->GetWorldMatrix());
				g_pContext->GetSceneManager()->AddSceneNode(pNew);

				if (cid == SIM_CLASS_3DOBJECT)
				{
					pNew->SetMesh(pNode->GetMesh());
				}

				return pNew;
			}
		}
	}

	return NULL;
}

void CSceneFile::ClearAll()
{
	for (int i = 0; i < m_NodeList.size(); ++i)
	{
		SimNode *pNode = m_NodeList[i];
		g_pContext->GetSceneManager()->RemoveSceneNode(pNode);
		SAFE_RELEASE(pNode);
	}

	m_NodeList.clear();
}

void CSceneFile::OnInit()
{
	m_MainCamera = (SimCamera*)g_pContext->CreateObject(SIM_CLASS_CAMERA);
	g_pContext->GetSceneManager()->SetActiveCamera(m_MainCamera);

	//////////////////////////////////////////////////////////////////////////

	m_strRootPath = getStartPath() + L"Library";

	ResolveFolder(m_strRootPath);

	InitData();

	//////////////////////////////////////////////////////////////////////////

	XString path = L"PreCreate.fbx";
	g_pContext->GetPathManager()->resolvePath(SIM_PATH_DATA, path);

	XArray<SimObject*> loaded;
	g_pContext->Load(path, loaded, false);

	for (int i = 0; i < loaded.size(); ++i)
	{
		if (SimIsKindof(loaded[i], SIM_CLASS_SCENE_NODE))
		{
			m_PreCreate.push_back((SimNode*)loaded[i]);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	m_DefaultMat = (SimMaterial*)g_pContext->CreateObject(SIM_CLASS_MATERIAL, L"DefaultMaterial");
}

void CSceneFile::ResolveFolder(const XString& folder)
{
	WIN32_FIND_DATA FindFileData; 
	HANDLE FileHandle = FindFirstFile(folder+L"\\*.*", &FindFileData);
	BOOL res = FileHandle != NULL;

	while(res)
	{
		XString strPath = FindFileData.cFileName;

		if (strPath == L"." || strPath == L"..")
		{
			res = FindNextFile(FileHandle, &FindFileData);
			continue;
		}

		strPath = folder + L"\\" + strPath;

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ResolveFolder(strPath);
		}
		else
		{
			XString name;
			cutFilenamePath(name, strPath);
			cutFilenameExtension(name, name);

			SimObject *pObject = NULL;

			if (IsImageFile(strPath))
			{
				pObject	= g_pContext->CreateObject(SIM_CLASS_TEXTURE2D, name);
				((SimTexture2D*)pObject)->SetSlotCount(1);
				((SimTexture2D*)pObject)->SetSlotFilename(strPath);
			}
			else if (IsRenderTextureFile(strPath))
			{
				pObject	= g_pContext->CreateObject(SIM_CLASS_RENDERTEXTURE, name);
			}
			else if (IsMaterialFile(strPath))
			{
				pObject	= g_pContext->CreateObject(SIM_CLASS_MATERIAL, name);
			}
			else if (IsTransformAnimFile(strPath))
			{
				pObject	= g_pContext->CreateObject(SIM_CLASS_OBJECT_ANIM, name);
			}
			else if (IsTextureAnimFile(strPath))
			{
				pObject	= g_pContext->CreateObject(SIM_CLASS_TEXTURE_ANIM, name);
			}
			else if (IsSoundFile(strPath))
			{
			}
			else if (IsModelFile(strPath))
			{
				pObject	= (SimObject*)new SModelFile;
			}

			if (pObject)
			{
				m_ObjectList.insert(ToShortPath(strPath), pObject);
			}
		}

		res = FindNextFile(FileHandle, &FindFileData);
	}

	FindClose(FileHandle);
}

void CSceneFile::InitData()
{
	for (int i = 0; i < m_ObjectList.size(); ++i)
	{
		XTable<XString, SimObject*>::Iterator it = m_ObjectList.getByIndex(i);

		const XString & path = it.getKey();
		SimObject *pObject = *it;

		if		(IsImageFile(path))				ReadTexture(ToLongPath(path), (SimTexture2D*)pObject);
		else if (IsRenderTextureFile(path))		ReadRenderTexture(ToLongPath(path), (SimRenderTexture*)pObject);
		else if (IsMaterialFile(path))			ReadMaterial(ToLongPath(path), (SimMaterial*)pObject);
		else if (IsTransformAnimFile(path))		ReadTransformAnim(ToLongPath(path), (SimObjectAnim*)pObject);
		else if (IsTextureAnimFile(path))		ReadTextureAnim(ToLongPath(path), (SimTextureAnim*)pObject);
		//else if (IsSoundFile(pData->Path))	ReadSoundFile(ToLongPath(path), (SimSound*)pObject);
		else if (IsModelFile(path))				ReadModelFile(ToLongPath(path), (SModelFile*)pObject);
	}
}

BOOL CSceneFile::IsImageFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".dds")) return TRUE;
	if (ext.equalsi(L".jpg")) return TRUE;
	if (ext.equalsi(L".png")) return TRUE;
	if (ext.equalsi(L".bmp")) return TRUE;
	if (ext.equalsi(L".tif")) return TRUE;

	return FALSE;
}

BOOL CSceneFile::IsRenderTextureFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".rentex"))
		return TRUE;

	return FALSE;
}

BOOL CSceneFile::IsMaterialFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".mtl"))
		return TRUE;

	return FALSE;
}

BOOL CSceneFile::IsTransformAnimFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".trananim"))
		return TRUE;

	return FALSE;
}

BOOL CSceneFile::IsTextureAnimFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".texanim"))
		return TRUE;

	return FALSE;
}

BOOL CSceneFile::IsSoundFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".wav"))
		return TRUE;

	return FALSE;
}

BOOL CSceneFile::IsModelFile(LPCTSTR filename)
{
	XString ext;
	getFileNameExtension(ext, filename);

	if (ext.equalsi(L".fbx") || ext.equalsi(L".u3d"))
		return TRUE;

	return FALSE;
}

SimNode* CSceneFile::CreateNode(CREATE_NODE_TYPE type)
{
	SimNode *pRet = NULL;

	switch (type)
	{
	case CREATE_SCENE_NODE:
		{
			pRet = (SimNode*)g_pContext->CreateObject(SIM_CLASS_SCENE_NODE);
		}
		break;
	case CREATE_CAMERA:
		{
			pRet = (SimCamera*)g_pContext->CreateObject(SIM_CLASS_CAMERA);
		}
		break;
	case CREATE_LIGHT:
		{
			pRet = (SimLight*)g_pContext->CreateObject(SIM_CLASS_LIGHT);
		}
		break;
	case CREATE_SOUND:
		{
			pRet = (SimSound*)g_pContext->CreateObject(SIM_CLASS_SOUND);
		}
		break;
	case CREATE_WATER:
		{
			pRet = (SimWaterSurface*)g_pContext->CreateObject(SIM_CLASS_WATER);
		}
		break;
	case CREATE_PLANE:
		{
			pRet = ClonePreCreate(L"Plane");
		}
		break;
	case CREATE_CUBE:
		{
			pRet = ClonePreCreate(L"Cube");
		}
		break;
	case CREATE_CYLINDER:
		{
			pRet = ClonePreCreate(L"Cylinder");
		}
		break;
	case CREATE_SPHERE:
		{
			pRet = ClonePreCreate(L"Sphere");
		}
		break;
	}

	if (pRet)
	{
		m_NodeList.push_back(pRet);
		g_pContext->GetSceneManager()->AddSceneNode(pRet);
	}

	return pRet;
}

SimNode* CSceneFile::ClonePreCreate(LPCTSTR name)
{
	for (int i = 0; i < m_PreCreate.size(); ++i)
	{
		SimObject *pObj = m_PreCreate[i];

		if (pObj && SIM_CLASS_3DOBJECT==pObj->GetClassID())
		{
			if (lstrcmp(name, pObj->GetName()) == 0)
			{
				Sim3DObject *pNode = (Sim3DObject*)pObj;
				SimMesh *pMesh = pNode->GetMesh();

				if (pMesh)
				{
					SimMesh *pMesh2 = (SimMesh*)g_pContext->CreateObject(SIM_CLASS_MESH, XString(name)+L"_Mesh");
					pMesh2->setVertexLayout(pMesh->getVertexLayout());
					pMesh2->setVertexCount(pMesh->getVertexCount());

					if (pMesh->getVertexLayout() & SIM_VERTEX_POSITION)
					{
						memcpy(pMesh2->getPositionPtr(), pMesh->getPositionPtr(), sizeof(float4)*pMesh->getVertexCount());
					}
					if (pMesh->getVertexLayout() & SIM_VERTEX_NORMAL)
					{
						memcpy(pMesh2->getNormalPtr(), pMesh->getNormalPtr(), sizeof(float3)*pMesh->getVertexCount());
					}
					if (pMesh->getVertexLayout() & SIM_VERTEX_TEXCOORD0)
					{
						memcpy(pMesh2->getTextureCoordPtr(0), pMesh->getTextureCoordPtr(0), sizeof(float2)*pMesh->getVertexCount());
					}

					pMesh2->recalculateBoundingBox();

					if (pMesh->getFaceCount())
					{
						pMesh2->setFaceCount(pMesh->getFaceCount());
						memcpy(pMesh2->getFaceIndices(), pMesh->getFaceIndices(), sizeof(UINT)*pMesh->getFaceCount()*3);
					}

					pMesh2->setFaceMaterial(m_DefaultMat, 0, pMesh2->getFaceCount()*3);

					Sim3DObject *pNode2 = (Sim3DObject*)g_pContext->CreateObject(SIM_CLASS_3DOBJECT, name);
					pNode2->SetMesh(pMesh2);
					SAFE_RELEASE(pMesh2);

					return pNode2;
				}
			}
		}
	}

	return NULL;
}

BOOL CSceneFile::OnOpen(LPCTSTR filename)
{
	g_pContext->GetAttributeManager()->SetString(L"Video", filename);
	g_pContext->GetAttributeManager()->SetBool(L"NewVideo", true);

	//SimVideo *video = (SimVideo*)g_pContext->CreateObject(SIM_CLASS_VIDEO, L"");
	//video->SetLoop(FALSE);
	//video->EnableCompress(TRUE);
	//video->EnableSound(TRUE);
	//if (video->OpenFile(filename))
	//{
	//	//video->Play();
	//	g_pContext->GetAttributeManager()->SetInt(L"Video", (int)video);
	//	g_pContext->GetAttributeManager()->SetBool(L"NewVideo", true);
	//}

	//XString ext;
	//getFileNameExtension(ext, filename);

	//if (!ext.equalsi(L".scn"))
	//	return FALSE;

	//SimXMLReader *reader = CreateSimXMLReader(filename);
	//if (!reader)
	//	return FALSE;

	//ClearAll();

	//while (reader->read())
	//{
	//	if (EXN_ELEMENT == reader->getNodeType())
	//	{
	//		if (XString(L"Scene") == reader->getNodeName())
	//		{
	//			ReadScene(reader);
	//		}
	//		else if (XString(L"AnimClip") == reader->getNodeName())
	//		{
	//			//ReadAnimClip(reader);
	//		}
	//	}
	//}

	//SAFE_RELEASE(reader);

	return TRUE;
}

void CSceneFile::ReadScene(SimXMLReader *reader)
{
	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"Node") == reader->getNodeName())
			{
				ReadNode(reader, NULL);
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"Scene") == reader->getNodeName())
				break;
		}
	}

	// set node target
	for (int i = 0; i < m_NodeList.size(); ++i)
	{
		SimNode *node = m_NodeList[i];
		int target = node->GetAttributes()->getAttributeAsInt(L"Target");

		if (target >= 0 && target < m_NodeList.size())
		{
			node->SetTarget(m_NodeList[target]);
		}
	}
}

SimNode* CSceneFile::ReadNode(SimXMLReader* reader, SimNode* pParent)
{
	int target = -1;
	SimNode *pNode = NULL;

	SimAttributes *attrs = CreateSimAttributes();
	attrs->read(reader);
	{
		XString			sourceType	= attrs->getAttributeAsString(L"Source");
		XString			nodeType	= attrs->getAttributeAsString(L"NodeType");
		XString			name		= attrs->getAttributeAsString(L"Name");
		bool			visible		= attrs->getAttributeAsBool(L"Visible");
		XMatrix			matrix		= attrs->getAttributeAsMatrix(L"Matrix");
		bool			bSync		= attrs->getAttributeAsBool(L"Sync");
						target		= attrs->getAttributeAsInt(L"Target");

		SimAttributes *initStateAttrs = NULL;
		int initState = attrs->findAttribute(L"HasInitialState");
		if (initState >= 0)
		{
			if (attrs->getAttributeAsBool(initState))
			{
				initStateAttrs = CreateSimAttributes();
				initStateAttrs->read(reader, false, L"InitialState");
			}
		}

		switch (GetSourceType(sourceType))
		{
		case SOURCE_FROM_FILE:
			{
				XString SourceFile = attrs->getAttributeAsString(L"SourceFile");
				int SourceIndex = attrs->getAttributeAsInt(L"SourceIndex");
				pNode = GetModelNode(SourceFile, SourceIndex);

				if (pNode)
				{
					m_NodeList.push_back(pNode);

					if (nodeType == L"3DObject")
					{
						Read3DObject(reader, (Sim3DObject*)pNode);
					}
				}
			}
			break;
		case SOURCE_INTERNAL:
			{
				if (nodeType == L"SceneNode")
				{
					pNode = CreateNode(CREATE_SCENE_NODE);
				}
				else if (nodeType == L"Camera")
				{
					pNode = CreateNode(CREATE_CAMERA);
					ReadCamera(reader, (SimCamera*)pNode);
				}
				else if (nodeType == L"Light")
				{
					pNode = CreateNode(CREATE_LIGHT);
					ReadLight(reader, (SimLight*)pNode);
				}
				else if (nodeType == L"Sound")
				{
					pNode = CreateNode(CREATE_SOUND);
					ReadSound(reader, (SimSound*)pNode);
				}
				else if (nodeType == L"Water")
				{
					pNode = CreateNode(CREATE_WATER);
					ReadWater(reader, (SimWaterSurface*)pNode);
				}
			}
			break;
		case SOURCE_BUILDIN_CUBE:
			{
				pNode = CreateNode(CREATE_CUBE);
				Read3DObject(reader, (Sim3DObject*)pNode);
			}
			break;
		case SOURCE_BUILDIN_PLANE:
			{
				pNode = CreateNode(CREATE_PLANE);
				Read3DObject(reader, (Sim3DObject*)pNode);
			}
			break;
		case SOURCE_BUILDIN_CYLINDER:
			{
				pNode = CreateNode(CREATE_CYLINDER);
				Read3DObject(reader, (Sim3DObject*)pNode);
			}
			break;
		case SOURCE_BUILDIN_SPHERE:
			{
				pNode = CreateNode(CREATE_SPHERE);
				Read3DObject(reader, (Sim3DObject*)pNode);
			}
			break;
		}

		if (pNode)
		{
			if (initStateAttrs)
			{
				pNode->SetFlags(initStateAttrs->getAttributeAsInt(L"Flags"));
				pNode->SetWorldMatrix(initStateAttrs->getAttributeAsMatrix(L"Matrix"));
				pNode->SetInitState();
			}

			pNode->SetName(name);
			pNode->SetParent(pParent);
			pNode->Show(visible);
			pNode->SetSync(bSync);
			pNode->SetWorldMatrix(matrix);
			pNode->GetAttributes()->setAttribute(L"Target", target);
		}

		SAFE_RELEASE(initStateAttrs);
	}
	SAFE_RELEASE(attrs);

	// Read children
	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"Node") == reader->getNodeName())
			{
				ReadNode(reader, pNode);
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"Node") == reader->getNodeName())
				break;
		}
	}

	return pNode;
}

void CSceneFile::ReadSound(SimXMLReader* reader, SimSound *pSound)
{
	SimAttributes *attrs = CreateSimAttributes();
	attrs->read(reader, false, L"Sound");

	XString path = attrs->getAttributeAsString(L"Path");

	if (pSound->Open(ToLongPath(path)))
	{
		pSound->SetLoop(attrs->getAttributeAsBool(L"Loop"));
		pSound->SetAutoPlay(attrs->getAttributeAsBool(L"AutoPlay"));
		pSound->SetPositionalSound(attrs->getAttributeAsBool(L"3DSound"));
		pSound->SetChannelCount(attrs->getAttributeAsInt(L"ChannelCount"));
		pSound->SetSoundRadius(attrs->getAttributeAsFloat(L"Radius"));
	}

	SAFE_RELEASE(attrs);
}

void CSceneFile::ReadWater(SimXMLReader* reader, SimWaterSurface *pWater)
{
	SimAttributes *attrs = CreateSimAttributes();
	attrs->read(reader, false, L"Water");
	SAFE_RELEASE(attrs);
}

void CSceneFile::Read3DObject(SimXMLReader* reader, Sim3DObject *p3DObj)
{
	int iMat = 0;
	SimMesh *pMesh = p3DObj->GetMesh();

	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"Material") == reader->getNodeName())
			{
				SimAttributes *attrs = CreateSimAttributes();
				{
					attrs->read(reader);
					XString path = attrs->getAttributeAsString(L"Path");
					SimMaterial *pMat = (SimMaterial*)GetObjectByPath(path);
					if (pMat)
						pMesh->replaceMaterial(iMat, pMat);
				}
				SAFE_RELEASE(attrs);

				++iMat;
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"3DObject") == reader->getNodeName())
				break;
		}
	}
}

void CSceneFile::ReadCamera(SimXMLReader* reader, SimCamera *camera)
{
	SimAttributes *attrs = CreateSimAttributes();
	attrs->read(reader, false, L"Camera");
	{
		bool isMainCam = attrs->getAttributeAsBool(L"IsMainCamera");
		if (isMainCam)
		{
			//SetMainCamera(camera);
			g_pContext->GetSceneManager()->SetActiveCamera(camera);
		}

		bool isOrtho = attrs->getAttributeAsString(L"Projection") == L"Ortho" ? true : false;
		camera->SetOrthogonal(isOrtho);
		if (isOrtho)
			camera->SetOrthoWidth(attrs->getAttributeAsFloat(L"OrthoWidth"));
		else
			camera->SetFov(attrs->getAttributeAsFloat(L"FOV"));

		camera->SetNearValue(attrs->getAttributeAsFloat(L"Near"));
		camera->SetFarValue(attrs->getAttributeAsFloat(L"Far"));
		camera->SetAspectRatio(attrs->getAttributeAsFloat(L"Aspect"));
		camera->SetRenderOrder(attrs->getAttributeAsInt(L"RenderOrder"));

		XString strRT = attrs->getAttributeAsString(L"RenderTarget");
		if (strRT.size())
		{
			SimRenderTexture *pRT = (SimRenderTexture*)GetObjectByPath(strRT);
			camera->SetRenderTarget(pRT);
		}

		float4 renderRect = attrs->getAttributeAsVector4d(L"RenderRect");
		camera->SetRenderRect(&renderRect);

		camera->SetBackgroundColor(attrs->getAttributeAsVector4d(L"Background"));
	}
	SAFE_RELEASE(attrs);
}

void CSceneFile::ReadLight(SimXMLReader* reader, SimLight *light)
{
	SimAttributes *attrs = CreateSimAttributes();
	attrs->read(reader, false, L"Light");

	light->SetType((SIM_LIGHT_TYPE)attrs->getAttributeAsInt(L"Type"));
	light->SetColor(attrs->getAttributeAsVector3d(L"Color"));
	light->SetAmbient(attrs->getAttributeAsVector3d(L"Ambient"));
	light->SetIntensity(attrs->getAttributeAsFloat(L"Intensity"));
	light->SetRange(attrs->getAttributeAsFloat(L"Range"));
	light->SetSpotAngle(attrs->getAttributeAsFloat(L"SpotAngle"));

	SAFE_RELEASE(attrs);
}


void CSceneFile::ReadTexture(LPCTSTR path, SimTexture2D *pTex)
{
	if (pTex->CreateFromFile(path))
	{
	}
}

void CSceneFile::ReadMaterial(LPCTSTR path, SimMaterial *pMat)
{
	SimXMLReader *reader = CreateSimXMLReader(path);
	if (!reader)
		return;

	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"Material") == reader->getNodeName())
			{
				SimAttributes *attrs = CreateSimAttributes();
				attrs->read(reader);
				{
					pMat->SetCastShadow(attrs->getAttributeAsBool(L"CastShadow"));
					pMat->SetReceiveShadow(attrs->getAttributeAsBool(L"ReceiveShadow"));
					pMat->SetLighting(attrs->getAttributeAsBool(L"Lighting"));
					pMat->SetBothSided(attrs->getAttributeAsBool(L"BothSided"));
					pMat->SetOpacity(attrs->getAttributeAsFloat(L"Opacity"));
					pMat->SetAlphaRefValue(attrs->getAttributeAsFloat(L"AlphaRef"));

					pMat->SetDiffuseColor(attrs->getAttributeAsVector3d(L"DiffuseColor"));
					XString strDiffMap = attrs->getAttributeAsString(L"DiffuseMap");
					if (strDiffMap.size())
					{
						SimTexture* diffuseMap = (SimTexture*)GetObjectByPath(strDiffMap);
						pMat->SetDiffuseMap(diffuseMap);
					}
					pMat->SetDiffuseMapAmount(attrs->getAttributeAsFloat(L"DiffuseMapAmount"));

					// specular
					pMat->SetSpecularColor(attrs->getAttributeAsVector3d(L"SpecularColor"));
					XString strSpecMap = attrs->getAttributeAsString(L"SpecularMap");
					if (strSpecMap.size())
					{
						SimTexture* specularMap = (SimTexture*)GetObjectByPath(strSpecMap);
						pMat->SetSpecularMap(specularMap);
					}
					pMat->SetSpecularMapAmount(attrs->getAttributeAsFloat(L"SpecularMapAmount"));

					pMat->SetGlossiness(attrs->getAttributeAsFloat(L"Glossiness"));
					pMat->SetSpecularLevel(attrs->getAttributeAsFloat(L"SpecularLevel"));

					// emissive
					pMat->SetEmissiveColor(attrs->getAttributeAsVector3d(L"EmissiveColor"));
					pMat->SetEmissiveAmount(attrs->getAttributeAsFloat(L"EmissiveAmount"));

					// bump map
					XString strBumpMap = attrs->getAttributeAsString(L"BumpMap");
					if (strBumpMap.size())
					{
						SimTexture* bumpMap = (SimTexture*)GetObjectByPath(strBumpMap);
						pMat->SetBumpMap(bumpMap);
					}
					pMat->SetBumpMapAmount(attrs->getAttributeAsFloat(L"BumpMapAmount"));

					// displacement map
					XString strDisplacementMap = attrs->getAttributeAsString(L"DisplacementMap");
					if (strDisplacementMap.size())
					{
						SimTexture* displacementMap = (SimTexture*)GetObjectByPath(strDisplacementMap);
						pMat->SetDisplacementMap(displacementMap);
					}
					pMat->SetDisplacementMapAmount(attrs->getAttributeAsFloat(L"DisplacementMapAmount"));

					// Reflection map
					XString strReflectionMap = attrs->getAttributeAsString(L"ReflectionMap");
					if (strReflectionMap.size())
					{
						SimTexture* ReflectionMap = (SimTexture*)GetObjectByPath(strReflectionMap);
						pMat->SetReflectionMap(ReflectionMap);
					}
					pMat->SetReflectionMapAmount(attrs->getAttributeAsFloat(L"ReflectionMapAmount"));

					// Refraction map
					XString strRefractionMap = attrs->getAttributeAsString(L"RefractionMap");
					if (strRefractionMap.size())
					{
						SimTexture* RefractionMap = (SimTexture*)GetObjectByPath(strRefractionMap);
						pMat->SetRefractionMap(RefractionMap);
					}
					pMat->SetRefractionMapAmount(attrs->getAttributeAsFloat(L"RefractionMapAmount"));

					pMat->SetFresnelBias(attrs->getAttributeAsFloat(L"FresnelBias"));
					pMat->SetFresnelPow(attrs->getAttributeAsFloat(L"FresnelPow"));
					pMat->SetFresnelScale(attrs->getAttributeAsFloat(L"FresnelScale"));
				}
				SAFE_RELEASE(attrs);
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"Material") == reader->getNodeName())
				break;
		}
	}

	SAFE_RELEASE(reader);
}

void CSceneFile::ReadRenderTexture(LPCTSTR path, SimRenderTexture *pTex)
{
	SimXMLReader *reader = CreateSimXMLReader(path);
	if (!reader)
		return;

	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"RenderTexture") == reader->getNodeName())
			{
				SimAttributes *attrs = CreateSimAttributes();
				{
					attrs->read(reader);

					int width = attrs->getAttributeAsInt(L"Width");
					int height = attrs->getAttributeAsInt(L"Height");

					SimTextureDesc desc;
					ZeroMemory(&desc, sizeof(desc));
					desc.Width			= width;
					desc.Height			= height;
					desc.ArraySize		= 1;
					desc.MipLevels		= 1;
					desc.SampleCount	= 1;
					desc.Format			= SIM_FORMAT_RGBA8;

					pTex->CreateRTV(g_pContext->GetRenderContext()->GetDevice(0), desc);

					//SimRenderTexture *pDSV = (SimRenderTexture*)g_pContext->CreateObject(SIM_CLASS_RENDERTEXTURE);

					//desc.Format = SIM_FORMAT_DEPTH24;
					//pDSV->CreateDSV(g_pContext->GetRenderContext()->GetDevice(0), desc);

					//pTex->SetAttachedDepthBuffer(pDSV);
				}
				SAFE_RELEASE(attrs);
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"RenderTexture") == reader->getNodeName())
				break;
		}
	}

	SAFE_RELEASE(reader);
}

void CSceneFile::ReadTransformAnim(LPCTSTR path, SimObjectAnim *pAnim)
{
	SimXMLReader *reader = CreateSimXMLReader(path);
	if (!reader)
		return;

	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"TransformAnim") == reader->getNodeName())
			{
				SimAttributes *attrs = CreateSimAttributes();
				attrs->read(reader);
				{
					bool bAutoPlay = attrs->getAttributeAsBool(L"AutoPlay");
					pAnim->SetAutoPlay(bAutoPlay);
					//pAnim->Play(bAutoPlay);

					pAnim->SetDuration(attrs->getAttributeAsFloat(L"Duration"));

					int keyCount = attrs->getAttributeAsInt(L"KeyCount");
					if (keyCount)
					{
						attrs->read(reader, false, L"Keys");
						for (int i = 0; i < keyCount; ++i)
						{
							float fTime = attrs->getAttributeAsFloat(i*2);
							XMatrix matrix = attrs->getAttributeAsMatrix(i*2+1);
							pAnim->InsertKey(fTime)->SetMatrix(L"WorldMatrix", matrix);
						}
					}
				}
				SAFE_RELEASE(attrs);
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"TransformAnim") == reader->getNodeName())
				break;
		}
	}

	SAFE_RELEASE(reader);
}

void CSceneFile::ReadTextureAnim(LPCTSTR path, SimTextureAnim *pAnim)
{
	SimXMLReader *reader = CreateSimXMLReader(path);
	if (!reader)
		return;

	while (reader->read())
	{
		if (EXN_ELEMENT == reader->getNodeType())
		{
			if (XString(L"TextureAnim") == reader->getNodeName())
			{
				SimAttributes *attrs = CreateSimAttributes();
				attrs->read(reader);
				{
					for (int i = 0; i < attrs->getAttributeCount(); ++i)
					{
						XString attrName = attrs->getAttributeName(i);

						if (attrName == L"Material")
						{
							XString strMat = attrs->getAttributeAsString(i);
							if (strMat.size())
							{
								SimMaterial *pMat = (SimMaterial*)GetObjectByPath(strMat);
								pAnim->SetOwner(pMat);
							}
						}
						else if (attrName == L"AutoPlay")
						{
							bool bAutoPlay = attrs->getAttributeAsBool(i);
							pAnim->SetAutoPlay(bAutoPlay);
							pAnim->Play(bAutoPlay);
						}
						else if (attrName == L"Loop")
						{
							bool bLoop = attrs->getAttributeAsBool(i);
							pAnim->SetLoop(bLoop);
						}
						else if (attrName == L"DelayPlay")
						{
							float fStartTime = attrs->getAttributeAsFloat(i);
							pAnim->GetAttributes()->setAttribute(L"DelayPlay", fStartTime);
						}
						else if (attrName == L"Duration")
						{
							float duration = attrs->getAttributeAsFloat(i);
							pAnim->SetDuration(duration);
						}
						else if (attrName == L"TextureSequence")
						{
							XString texSequ = attrs->getAttributeAsString(i);
							if (texSequ.size())
							{
								SimTexture2D *pTexSequ = (SimTexture2D *)GetObjectByPath(texSequ);
								SetAnimTextureSequence(pAnim, pTexSequ);
							}
						}
						else if (attrName == L"TextureCount")
						{
							int texCount = attrs->getAttributeAsInt(i);
							pAnim->SetTextureCount(texCount);
						}
					}
				}
				SAFE_RELEASE(attrs);
			}
		}
		else if (EXN_ELEMENT_END == reader->getNodeType())
		{
			if (XString(L"TextureAnim") == reader->getNodeName())
				break;
		}
	}

	SAFE_RELEASE(reader);
}

void CSceneFile::ReadModelFile(LPCTSTR filename, SModelFile *pModel)
{
	XArray<SimObject*> loaded;
	if (!g_pContext->Load(filename, loaded, false))
		return;

	XString path;
	getFilenamePath(path, filename);
	path = path.subString(0, path.size()-1);
	path = ToShortPath(path.c_str());

	for (int i = 0; i < loaded.size(); ++i)
	{
		SimObject* obj = loaded[i];

		if (SimIsKindof(obj, SIM_CLASS_SCENE_NODE))
		{
			pModel->Objects.push_back((SimNode*)obj);
		}
	}

	if (pModel->Objects.size())
	{
		int first = 1;
		XBox box;

		for (int i = 0; i < pModel->Objects.size(); ++i)
		{
			if (pModel->Objects[i]->GetClassID() == SIM_CLASS_3DOBJECT)
			{
				if (first)
				{
					box = pModel->Objects[i]->GetBoundingBox();
					first = 0;
				}
				else
				{
					box.addInternalBox(pModel->Objects[i]->GetBoundingBox());
				}
			}
		}

		if (first != 1)
		{
			XString title;
			cutFilenamePath(title, filename);
			cutFilenameExtension(title, title);

			SimNode *node = (SimNode*)g_pContext->CreateObject(SIM_CLASS_SCENE_NODE, title);
			node->SetPosition(box.getCenter());

			for (int i = 0; i < pModel->Objects.size(); ++i)
			{
				if (!pModel->Objects[i]->GetParent())
				{
					pModel->Objects[i]->SetParent(node);
				}
			}

			pModel->Objects.insert(0, node);
		}
	}
}

static void StringAddOne(XString &str)
{
	XString ext;
	getFileNameExtension(ext, str);

	XString name;
	cutFilenameExtension(name, str);

	int n = 0;
	for (int i = name.size()-1; i >= 0; --i)
	{
		if (!isDigit(name.c_str()[i]))
		{
			break;
		}
		++n;
	}

	if (n > 0)
	{
		str.cut(str.size() - ext.size(), ext.size());

		//int num = ::_ttoi(str.Right(n).GetBuffer());
		int num = ::_ttoi(str.subString(str.size()-n, n).c_str());
		num += 1;

		XString tmp;

		if (10 == num)				tmp = L"10";
		else if (100 == num)		tmp = L"100";
		else if (1000 == num)		tmp = L"1000";
		else if (10000 == num)		tmp = L"10000";
		else if (100000 == num)		tmp = L"100000";
		else if (1000000 == num)	tmp = L"1000000";
		else						tmp.format(L"%d", num);

		int len = tmp.size();
		str.cut(str.size() - len, len);
		str += tmp;
		str += ext.c_str();
	}
}

void CSceneFile::SetAnimTextureSequence(SimTextureAnim *pAnim, SimTexture2D *pFirstTex)
{
	if (pFirstTex)
	{
		BOOL bRet = pAnim->GetTextureCount() && pFirstTex == pAnim->GetTexture(0);
		if (!bRet)
		{
			pAnim->RemoveAllTextures();

			XString filename = GetPathByObject(pFirstTex);

			while (pFirstTex)
			{
				pAnim->AddTexture(pFirstTex);
				StringAddOne(filename);
				pFirstTex = (SimTexture2D*)GetObjectByPath(filename);
			}
		}
	}
	else
	{
		pAnim->RemoveAllTextures();
	}
}
