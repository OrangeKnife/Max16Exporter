// Max16Exporter.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <vector>
#include "Max16Exporter.h"
#include <algorithm>

namespace NBE
{
	
Max16Exporter::Max16Exporter() :
	m_igame( 0 ),
	m_fh( 0 )
{
}

Max16Exporter::~Max16Exporter()
{
	deinit();
}

void Max16Exporter::deinit()
{
	//if ( m_fh != 0 )
	//{
	//	fclose( m_fh );
	//	m_fh = 0;
	//}
	m_ofs.close();

	if ( m_igame != 0 )
	{
		m_igame->ReleaseIGame();
		m_igame = 0;
	}
}

int Max16Exporter::DoExport( const TCHAR* name, ExpInterface* ei, Interface* i, BOOL suppressprompts, DWORD options )
{
	try
	{
		const bool exportselected = 0 != (options & SCENE_EXPORT_SELECTED); // export only selected objects

		// open file for writing
		m_ofs.open(name,std::ios_base::binary);
 
		m_igame = GetIGameInterface();
		if ( !m_igame )
			throw std::exception( "Failed to initialize IGame interface" );

		IGameConversionManager* cm = GetConversionManager();
		cm->SetCoordSystem(IGameConversionManager::IGAME_OGL);
	 

		m_igame->InitialiseIGame( exportselected );

		size_t len = wcslen(m_igame->GetSceneFileName());

		char* nameChars = new char[len + 1];
		wcstombs(nameChars, m_igame->GetSceneFileName(), len + 1);
		

		std::string line = "[" + std::string(nameChars) + "]\n";
		m_ofs << line.c_str();
		delete[] nameChars;

		startTime = m_igame->GetSceneStartTime();
		endTime = m_igame->GetSceneEndTime();

		int count_topLevelNode = m_igame->GetTopLevelNodeCount(); 

		for(int i = 0; i < count_topLevelNode; ++i)
		{
			IGameNode* node = m_igame->GetTopLevelNode(i);
			if(node)
			{
				ExportMat(node);//get mat lib
			}
		}
 
		for(auto it = matVec.begin(); it!= matVec.end(); ++it)
		{
			outputMaterialInfo(*it);
		}
 
		for(int i = 0; i < count_topLevelNode; ++i)
		{
			IGameNode* node = m_igame->GetTopLevelNode(i);
			if(node)
			{
				ExportBones(node);
			}
		}

		/// now export the objects, having their own vertices and indices, so the num will not be too large

		for(int i = 0; i < count_topLevelNode; ++i)
		{
			IGameNode* node = m_igame->GetTopLevelNode(i);
			if(node)
			{
				ExportObjectsFromNode(node);
			}
		}


		//////////////////////////////////////////////////////////////////////////
		

		// release initialized stuff
		deinit();
	}
	catch ( std::exception& e )
	{
		wchar_t* estr = charToWchar(e.what());
		TSTR tstr(estr);
		MessageBox( i->GetMAXHWnd(), tstr, _T("Export Error"), MB_OK|MB_ICONERROR );
		deinit();
		delete[] estr;
	}
	return TRUE;
}

void Max16Exporter::outputFacesByMesh(IGameNode* node)
{
	//std::string objName = std::string(node->GetName());
	//int belongToObjMark[3] = {-2,(int)objName.size(),0};
	////this means the name of the obj it belongs to
	//m_ofs.write((const char*)belongToObjMark,sizeof(int) * 3);
	//m_ofs.write((const char*)objName.c_str(),belongToObjMark[1]);

	IGameMesh* mesh = (IGameMesh*)node->GetIGameObject();
	if(mesh->InitializeData())
	{
		//face from mat
		Tab<int> tabMtlIDs = mesh->GetActiveMatIDs();
		int tempIdx = 0;
		if (tabMtlIDs.Count() > 0)
		{
			for(int m = 0; m< tabMtlIDs.Count(); ++m)
			{
				Tab<FaceEx*> tabFacesByMatID = mesh->GetFacesFromMatID(tabMtlIDs[m]);
				//output the usemtl
				if(tabFacesByMatID.Count() > 0)
				{
					IGameMaterial* pMaterial = mesh->GetMaterialFromFace(tabFacesByMatID[0]);
					if(pMaterial)
					{
						int materialIdx = getIdxOfMaterial(pMaterial);
						//std::string line = std::string(pMaterial->GetMaterialName());
						int usemtlMark[3] = {-1,materialIdx,0};//indicate the engien , hey I will use this material 
						m_ofs.write((const char*)usemtlMark,sizeof(int) * 3); 
					}
					else
					{
						//no material !!!
						int usemtlMark[3] = {-1,0,0};//indicate the engine , no mat 
						m_ofs.write((const char*)usemtlMark,sizeof(int) * 3);
					}
				}

				

				for(int j = 0; j< tabFacesByMatID.Count(); ++j)
				{
					//face
					unsigned int p[3]={0};
					for(int k = 0; k<3; ++k)
					{
						FaceEx* face = tabFacesByMatID[j];
						if(face)
						{
 
							p[k]  = (unsigned int)indexVec[tempIdx++];
			 
						}
		
						//;
					}
					
					 
					m_ofs.write((const char*)p,sizeof(unsigned int) * 3);
					
				}
			}
		}
	}	
}

void Max16Exporter::outputSkinIndicesByMesh(IGameNode* node)
{
	IGameMesh* mesh = (IGameMesh*)node->GetIGameObject();
	if(mesh->InitializeData())
	{
		//face from mat
		Tab<int> tabMtlIDs = mesh->GetActiveMatIDs();

		if (tabMtlIDs.Count() > 0)
		{
			for(int m = 0; m< tabMtlIDs.Count(); ++m)
			{
				Tab<FaceEx*> tabFacesByMatID = mesh->GetFacesFromMatID(tabMtlIDs[m]);
				//output the usemtl
				if(tabFacesByMatID.Count() > 0)
				{
					IGameMaterial* pMaterial = mesh->GetMaterialFromFace(tabFacesByMatID[0]);
					if(pMaterial)
					{
						int materialIdx = getIdxOfMaterial(pMaterial);
						//std::string line = std::string(pMaterial->GetMaterialName());
						int usemtlMark[3] = {-1,materialIdx,0};//indicate the engine , hey I will use this material 
						m_ofs.write((const char*)usemtlMark,sizeof(int) * 3);
					}
					else
					{
						//no material !!!
						int usemtlMark[3] = {-1,0,0};//indicate the engine , no mat 
						m_ofs.write((const char*)usemtlMark,sizeof(int) * 3);
					}
				}



				for(int j = 0; j< tabFacesByMatID.Count(); ++j)
				{
					//face
					unsigned int p[3]={0};
					for(int k = 0; k<3; ++k)
					{
						FaceEx* face = tabFacesByMatID[j];
						if(face)
						{
							Point3 WorldVertex = mesh->GetVertex(face->vert[k]);

							SkinVertex sv;
							sv.pos = WorldVertex;
							sv.uv = mesh->GetTexVertex( face->texCoord[k] );
							sv.normal = mesh->GetNormal( face->norm[k] );
							sv.color = Point4(0,0,0,1);
							//indexVec.push_back((unsigned int)skinVertexExist(sv));


							//sv.uv.y = 1 - sv.uv.y;

							p[k]  = (unsigned int)skinVertexExist(sv);

						}

					}


					m_ofs.write((const char*)p,sizeof(unsigned int) * 3);

				}
			}
		}
	}	
}

void Max16Exporter::ExportObjectsFromNode(IGameNode* node)
{	
	int tp = node->GetIGameObject()->GetIGameType();
	switch(tp)
	{
 	case IGameObject::IGAME_UNKNOWN: break;
 	case IGameObject::IGAME_LIGHT:	 break;
    case IGameObject::IGAME_MESH: ExportMeshFromNode(node);	 break;
 	case IGameObject::IGAME_SPLINE:	 break;
 	case IGameObject::IGAME_CAMERA:	 break;
    case IGameObject::IGAME_HELPER:	ExportMeshFromNode(node);	 break; // like a dummy transparent obj
  	case IGameObject::IGAME_BONE:/*	ExportBones(node); */break;//I export bones before export any skin meshes , so no need to export again
  	case IGameObject::IGAME_IKCHAIN: break;
  	case IGameObject::IGAME_XREF:	 break;
	default:
		throw std::exception( "Cant recognize this node type to export material" );
	}
}

int Max16Exporter::ExportMeshFromNode(IGameNode* node) // includes materials
{
	// a IGame helper can also call this function
	IGameMesh* mesh = (IGameMesh*)node->GetIGameObject();

	size_t len = wcslen(node->GetName());

	char* nameChars = new char[len + 1];
	wcstombs(nameChars, node->GetName(), len + 1);


	m_ofs << "[object]\n" << std::string(nameChars) << "\n";

	delete[] nameChars;

	if (node->GetNodeParent())
	{
		len = wcslen(node->GetNodeParent()->GetName());

		nameChars = new char[len + 1];
		wcstombs(nameChars, node->GetNodeParent()->GetName(), len + 1);

		m_ofs << "[parent]\n" << std::string(nameChars) << "\n";
		delete[] nameChars;
	}

	if(mesh->InitializeData())
	{
	//check whether it has modifiers? if yes, use skin mesh , if no , use static mesh
	int numOfModifiers = mesh->GetNumModifiers();
	if (numOfModifiers > 0)
	{
		for (int i = 0;i<numOfModifiers;++i)
		{
			IGameModifier* mdf = mesh->GetIGameModifier(i);
			if( mdf->IsSkin() )
			{
				m_ofs<<"[skeleton]\n";
				IGameSkin* sk = (IGameSkin*)mdf;

				//face from mat
				Tab<int> tabMtlIDs = mesh->GetActiveMatIDs();
				if (tabMtlIDs.Count() > 0)
				{
					for(int m = 0; m< tabMtlIDs.Count(); ++m)
					{
						Tab<FaceEx*> tabFacesByMatID = mesh->GetFacesFromMatID(tabMtlIDs[m]);
						for(int j = 0; j< tabFacesByMatID.Count(); ++j)
						{
							//face  3 vertices
							for(int k = 0; k<3; ++k)
							{
								FaceEx* face = tabFacesByMatID[j];
								int numOfBonesInAVertex = sk->GetNumberOfBones(face->vert[k]);
								std::vector<WeightIndex> boneWeightVec;
								for (int bi = 0;bi < numOfBonesInAVertex;++bi)
								{
									float wt = sk->GetWeight(face->vert[k],bi);
									IGameNode* boneNode = sk->GetIGameBone(face->vert[k],bi);
									int boneId  = getBoneIdx(boneNode);
									boneWeightVec.push_back(WeightIndex(wt,boneId));
								}

								if (numOfBonesInAVertex > 4)
								{
									//re- weight
									std::sort(boneWeightVec.begin(),boneWeightVec.end(),Max16Exporter::descending);
									float sumOf4 = boneWeightVec[0].wt + boneWeightVec[1].wt +
													boneWeightVec[2].wt + boneWeightVec[3].wt;
									boneWeightVec[0].wt /= sumOf4;
									boneWeightVec[1].wt /= sumOf4;
									boneWeightVec[2].wt /= sumOf4;
									boneWeightVec[3].wt /= sumOf4;
								}
								else
								{
									for(int restOf4 = 0;restOf4 < 4 - numOfBonesInAVertex;++restOf4)
									{
										boneWeightVec.push_back(WeightIndex(0,-1));
									}
									
								}

								SkinVertex sv;
								for (int copyIdx = 0;copyIdx < 4;++copyIdx)
								{
									sv.idx4[copyIdx] = boneWeightVec[copyIdx].idx;
									sv.wt4[copyIdx] = boneWeightVec[copyIdx].wt;
								}
						 
								Point3 WorldVertex = mesh->GetVertex(face->vert[k]);

								sv.pos = WorldVertex;
								sv.uv = mesh->GetTexVertex( face->texCoord[k] );
								sv.normal = mesh->GetNormal( face->norm[k] );
								sv.color = Point4(0,0,0,1);
								indexVec.push_back((unsigned int)skinVertexExist(sv));

								//for (int bi = 0; bi < numOfBonesInAVertex; ++bi)
								//{
								//	m_ofs.write((char*)&boneWeightVec[bi].idx,sizeof(int)); //idx in the whole bone vec
								//}
								
								 

							}
						}
					}		
				}

				//here it is , could be 00 so there is no vertices and indices
				unsigned int outputUINT = (unsigned int)skinVertexVec.size();
				m_ofs.write((const char*)&outputUINT,sizeof(unsigned int));
				outputUINT = (unsigned int)indexVec.size();
				m_ofs.write((const char*)&outputUINT,sizeof(unsigned int));
				//skin vertices:
				for (unsigned int i = 0; i< skinVertexVec.size(); ++i)
				{
					m_ofs.write((const char*)&(skinVertexVec[i]),sizeof(SkinVertex));
				}

				//indices
				outputSkinIndicesByMesh(node);

				skinVertexVec.clear();
				indexVec.clear();

				//all children meshes
				int childCount = node->GetChildCount();
				for(int j = 0; j < childCount; ++j)
				{
					ExportObjectsFromNode(node->GetNodeChild(j));
				}

				return TRUE;
			}
		}
		
	}
			
	 }
	



	//////////////////////////////////////////////////////////////////////////
	//object animation
	m_ofs << "[animation]\n";
	std::vector<KeyFrame> realFramesData;
	KeyFrame lastFrameData;
	int animationFrameCount = (endTime - startTime ) / 160 + 1; //frames ,frame rate 30
	m_ofs.write((const char*)&animationFrameCount,sizeof(int));
	for (TimeValue i = 0; i<animationFrameCount; ++i)
	{
		KeyFrame frame;
		GMatrix	ParentWorldTM;
		IGameNode* parent = node->GetNodeParent();
		if( parent!= 0)
		{
			ParentWorldTM = parent->GetWorldTM(i * 160 + startTime);
		}
		else 
		{
			ParentWorldTM.SetIdentity();
		}

		GMatrix WorldToParent = ParentWorldTM.Inverse();
		GMatrix	LocalTM = node->GetWorldTM(i * 160 + startTime) * WorldToParent;

		frame.pos = LocalTM.Translation();
		frame.rot = LocalTM.Rotation();
		frame.scale = LocalTM.Scaling();
		frame.frameIdx = i;

		if(frame != lastFrameData || i == 0)
		{
			lastFrameData = frame;
			realFramesData.push_back(frame);
		}
	}

	//output realFrameData
	if(animationFrameCount > 0)
	{
		int realFrameCount =  ( int)realFramesData.size();
		m_ofs.write((const char*)&realFrameCount,sizeof(unsigned int));
		m_ofs.write((const char*)&realFramesData[0],sizeof(KeyFrame) * realFrameCount);
	}


	//////////////////////////////////////////////////////////////////////////
	if(mesh->InitializeData())
	{
		//face from mat
		Tab<int> tabMtlIDs = mesh->GetActiveMatIDs();
		if (tabMtlIDs.Count() > 0)
		{
			for(int m = 0; m< tabMtlIDs.Count(); ++m)
			{
				Tab<FaceEx*> tabFacesByMatID = mesh->GetFacesFromMatID(tabMtlIDs[m]);
				for(int j = 0; j< tabFacesByMatID.Count(); ++j)
				{
					//face  3 vertices
					for(int k = 0; k<3; ++k)
					{
						FaceEx* face = tabFacesByMatID[j];
						if(face)
						{
						Point3 WorldVertex = mesh->GetVertex(face->vert[k]);
						GMatrix WorldToLocal =  node->GetWorldTM().Inverse();
						Point3 LocalVertex = WorldToLocal.ExtractMatrix3() * WorldVertex;
						


						Vertex v;
						v.pos = LocalVertex;//mesh->GetVertex( face->vert[k] );
						v.uv = mesh->GetTexVertex( face->texCoord[k] );
						v.normal = mesh->GetNormal( face->norm[k] );
						v.color = Point4(0,0,0,1);
						indexVec.push_back((unsigned int)vertexExist(v));
						}
					}
				}
			}		
		}
	}
	//here it is , could be 00 so there is no vertices and indices
	unsigned int outputUINT = (unsigned int)vertexVec.size();
	m_ofs.write((const char*)&outputUINT,sizeof(unsigned int));
	outputUINT = (unsigned int)indexVec.size();
	m_ofs.write((const char*)&outputUINT,sizeof(unsigned int));
	//vertices:
	for (unsigned int i = 0; i< vertexVec.size(); ++i)
	{
		m_ofs.write((const char*)&(vertexVec[i]),sizeof(Vertex));
	}

	//indices
	outputFacesByMesh(node);

	vertexVec.clear();
	indexVec.clear();

	//all children meshes
	int childCount = node->GetChildCount();
	for(int j = 0; j < childCount; ++j)
	{
		ExportObjectsFromNode(node->GetNodeChild(j));
	}

	return TRUE;
}

int Max16Exporter::ExportMat(IGameNode* node) 
{
	int tp = node->GetIGameObject()->GetIGameType();
	if(tp!= IGameObject::IGAME_MESH && tp != IGameObject::IGAME_HELPER)
		return -1;

	IGameMesh* mesh = (IGameMesh*)node->GetIGameObject();

	if(mesh->InitializeData())
	{
		//face from mat
		Tab<int> tabMtlIDs = mesh->GetActiveMatIDs();
		if (tabMtlIDs.Count() > 0)
		{
			//1st pass
			for(int m = 0; m< tabMtlIDs.Count(); ++m)
			{
				Tab<FaceEx*> tabFacesByMatID = mesh->GetFacesFromMatID(tabMtlIDs[m]);
				for(int j = 0; j< tabFacesByMatID.Count(); ++j)
				{
					//material lib
					IGameMaterial* pMaterial = mesh->GetMaterialFromFace(tabFacesByMatID[j]);

					if(pMaterial >0 && !matExist(pMaterial))
					{

						matVec.push_back(pMaterial);
						//outputMaterialInfo(pMaterial);

						int subMaterialCount = pMaterial->GetSubMaterialCount();
						for (int k = 0; k < subMaterialCount; ++k)
						{
							IGameMaterial* subMat = pMaterial->GetSubMaterial(k);
							if (subMat > 0)
							{
								matVec.push_back(subMat);
								//outputMaterialInfo(subMat);
							}
						}
						
					}


				}
			}		
		}


	}		


	//all children meshes mat should be exported
	int childCount = node->GetChildCount();
	for(int j = 0; j < childCount; ++j)
	{
		ExportMat(node->GetNodeChild(j));
	}

	return TRUE;
}

void Max16Exporter::outputMaterialInfo(IGameMaterial* mat)
{
	std::string line;
	char c[256];

	size_t len = wcslen(mat->GetMaterialName());

	char* nameChars = new char[len + 1];
	wcstombs(nameChars, mat->GetMaterialName(), len + 1);
	
	line = std::string(nameChars);

	sprintf_s(c,"[material]\n%s\n", line.c_str());
	m_ofs << c;
	
	delete[] nameChars;

	int numoftex = mat->GetNumberOfTextureMaps();
	int real_numOfTex = numoftex;

	for (int i = 0; i < numoftex; ++i)
	{
		IGameTextureMap* mp = mat->GetIGameTextureMap(i);
		if (mp->IsEntitySupported() && mp->GetBitmapFileName())
		{
			size_t len = wcslen(mp->GetBitmapFileName());

			char* nameChars = new char[len + 1];
			wcstombs(nameChars, mp->GetBitmapFileName(), len + 1);
			line = std::string(nameChars);
			delete[] nameChars;
		}
		else
			line = "";

		int mapslot = mp->GetStdMapSlot();
		if(mapslot == -1)
			throw std::exception("un supported material");
			//real_numOfTex -= 1;
	}
	m_ofs.write((const char*)&real_numOfTex,sizeof(int));

	for (int i = 0; i < numoftex; ++i)
	{
		IGameTextureMap* mp = mat->GetIGameTextureMap(i);
		if (mp->IsEntitySupported() && mp->GetBitmapFileName())
		{
			size_t len = wcslen(mp->GetBitmapFileName());
			char* nameChars = new char[len + 1];
			wcstombs(nameChars, mp->GetBitmapFileName(), len + 1);
			line = std::string(nameChars);
			delete[] nameChars;
		}
		else
			line = "default-alpha.png";
		int mapslot = mat->GetIGameTextureMap(i)->GetStdMapSlot();
		char slopStr[16];
		switch(mapslot)
		{
			/*
			#define ID_AM 0   //!< Ambient 
			#define ID_DI 1   //!< Diffuse
			#define ID_SP 2   //!< Specular
			#define ID_SH 3   //!< Glossiness (Shininess in 3ds Max release 2.0 and earlier)
			#define ID_SS 4   //!< Specular Level (Shininess strength in 3ds Max release 2.0 and earlier)
			#define ID_SI 5   //!< Self-illumination
			#define ID_OP 6   //!< Opacity
			#define ID_FI 7   //!< Filter color
			#define ID_BU 8   //!< Bump 
			#define ID_RL 9   //!< Reflection
			#define ID_RR 10  //!< Refraction 
			#define ID_DP 11  //!< Displacement 
			#define NTEXMAPS 12
			*/
			case ID_AM: sprintf_s(slopStr,"ID_AM_%d",mapslot);break;
			case ID_DI: sprintf_s(slopStr,"ID_DI_%d",mapslot);break;
			case ID_SP: sprintf_s(slopStr,"ID_SP_%d",mapslot);break;
			case ID_SH: continue;//sprintf_s(slopStr,"ID_SH_%d",mapslot);break;
			case ID_SS: sprintf_s(slopStr,"ID_SS_%d",mapslot);break;//continue;//
			case ID_SI: sprintf_s(slopStr,"ID_SI_%d",mapslot);break;
			case ID_OP: sprintf_s(slopStr,"ID_OP_%d",mapslot);break;
			case ID_FI: sprintf_s(slopStr,"ID_FI_%d",mapslot);break;
			case ID_BU: sprintf_s(slopStr,"ID_BU_%d",mapslot);break;
			case ID_RL: sprintf_s(slopStr,"ID_RL_%d",mapslot);break;
			case ID_RR: sprintf_s(slopStr,"ID_RR_%d",mapslot);break;
			case ID_DP: sprintf_s(slopStr,"ID_DP_%d",mapslot);break;
			default :
				sprintf_s(slopStr,"ID_DI_1",mapslot);
				break;
		}
		sprintf_s(c,"[%s]\n%s\n",slopStr,line.substr(line.find_last_of('\\')+1).c_str());
		m_ofs<<c;
		
	}
	
}


int Max16Exporter::matExist(IGameMaterial* pMat)
{
	bool rt = false;
	std::for_each(std::begin(matVec),std::end(matVec),[&](IGameMaterial* p){
		if (p == pMat)rt = true;
	});
	
	return rt;
}

size_t Max16Exporter::vertexExist(Vertex& v)
{
	//v.uv.y = 1 - v.uv.y;

	for (size_t i = 0;i < vertexVec.size();++i)
	{
		if(vertexVec[i] ==  v)
			return i;
	}

	 //add the vertex
	 
	
	vertexVec.push_back(v);
	return vertexVec.size()-1;	
}

size_t Max16Exporter::skinVertexExist(SkinVertex& sv)
{
	//sv.uv.y = 1 - sv.uv.y;

	for (size_t i = 0;i < skinVertexVec.size();++i)
	{
		if(skinVertexVec[i] == sv )
			return i;
	}

	//add the skin vertex
	
	skinVertexVec.push_back(sv);
	return skinVertexVec.size()-1;
}

void Max16Exporter::ExportBones( IGameNode* node )
{
	//export the bones tree
	int tp = node->GetIGameObject()->GetIGameType() ;
	if(tp!= IGameObject::IGAME_MESH && tp != IGameObject::IGAME_HELPER)
		return;//cuz I check every vertices to get the list of bones , so need to get the meshes first

	IGameMesh* mesh = (IGameMesh*)node->GetIGameObject();
	//check whether it has modifiers? if yes, use skin mesh , if no , use static mesh
	int numOfModifiers = mesh->GetNumModifiers();
	if (numOfModifiers > 0)
	{
		for (int i = 0;i<numOfModifiers;++i)
		{
			IGameModifier* mdf = mesh->GetIGameModifier(i);
			if( mdf->IsSkin() )
			{
				IGameSkin* sk = (IGameSkin*)mdf;

				//face from mat
				Tab<int> tabMtlIDs = mesh->GetActiveMatIDs();
				if (tabMtlIDs.Count() > 0)
				{
					for(int m = 0; m< tabMtlIDs.Count(); ++m)
					{
						Tab<FaceEx*> tabFacesByMatID = mesh->GetFacesFromMatID(tabMtlIDs[m]);
						for(int j = 0; j< tabFacesByMatID.Count(); ++j)
						{
							//face  3 vertices
							for(int k = 0; k<3; ++k)
							{
								FaceEx* face = tabFacesByMatID[j];
								int numOfBonesInAVertex = sk->GetNumberOfBones(face->vert[k]);
								for (int bi = 0;bi < numOfBonesInAVertex;++bi)
								{
									float wt = sk->GetWeight(face->vert[k],bi);
									IGameNode* boneNode = sk->GetIGameBone(face->vert[k],bi);
									buildBoneTree(boneNode); 
								}

							}
						}
					}		
				}
			}
		}

	}

	int childCount = node->GetChildCount();
	for(int j = 0; j < childCount; ++j)
	{
		ExportBones(node->GetNodeChild(j));
	}

}

int Max16Exporter::getBoneIdx(IGameNode* node)
{
	 for (unsigned int i = 0;i<boneVec.size();++i)
	 {
		 if (boneVec[i] == node)
		 {
			 return i;
		 }
	 }
	 //that means it is not igamebone but, it was used by the skin mesh
	 throw std::exception( "get bone index error" );
	 return -1;
}

int Max16Exporter::buildBoneTree(IGameNode* node)
{
	for (unsigned int i = 0;i<boneVec.size();++i)
	{
		if (boneVec[i] == node)
		{
			return i;
		}
	}
	
	

	//build the hierarchy
	int parentboneIdx = -1;
	if(node->GetNodeParent())
		parentboneIdx = buildBoneTree(node->GetNodeParent());

	boneVec.push_back(node);
	m_ofs << "[bone]\n"; //bone animation

	m_ofs.write((const char*)&parentboneIdx,sizeof(int));

	int animationFrameCount = (endTime - startTime ) / 160 + 1; //frames ,frame rate 30
	m_ofs.write((const char*)&animationFrameCount,sizeof(unsigned int));

	//world - to - bone TM
	GMatrix W2B = node->GetWorldTM( startTime).Inverse(); // frame = 0; world basis
	KeyFrame f0_w2b;
	f0_w2b.pos = W2B.Translation();
	f0_w2b.rot = W2B.Rotation();
	f0_w2b.scale = W2B.Scaling();
	m_ofs.write((const char*)&f0_w2b,sizeof(KeyFrame));

	for (TimeValue i = 0; i<animationFrameCount; ++i)
	{
		KeyFrame frame;
		GMatrix	ParentWorldTM;
		IGameNode* parent = node->GetNodeParent();
		if( parent!= 0)
		{
			ParentWorldTM = parent->GetWorldTM(i * 160 + startTime);
		}
		else 
		{
			ParentWorldTM.SetIdentity();
		}

		GMatrix WorldToParent = ParentWorldTM.Inverse();
		GMatrix	LocalTM = node->GetWorldTM(i * 160 + startTime)  * WorldToParent;
		// all the bones are local TM , so when rendering, make sure get real world transformation 
		//to shader
		frame.pos = LocalTM.Translation();
		frame.rot = LocalTM.Rotation();
		frame.scale = LocalTM.Scaling();

		m_ofs.write((const char*)&frame,sizeof(KeyFrame));
	}

	return (int)boneVec.size() - 1;
}


int Max16Exporter::getIdxOfMaterial(IGameMaterial* mat)
{

	for(int i = 0; i < matVec.size(); ++i)
	{
		if(mat == matVec[i])
			return i;
	}

	throw std::exception( "fail to get the material idx" );

}





void Max16Exporter::ShowAbout( HWND hwnd )
{
}

int Max16Exporter::ExtCount()
{
	return 1;
}

const TCHAR* Max16Exporter::Ext( int /*n*/ )
{
	return _T("N3D");
}

const TCHAR* Max16Exporter::LongDesc()
{
	return _T("My 3dsmax 12 Exporter");
}

const TCHAR* Max16Exporter::ShortDesc()
{
	return _T("Max16Exporter");
}

const TCHAR* Max16Exporter::AuthorName()
{
	return _T("");
}

const TCHAR* Max16Exporter::CopyrightMessage()
{
	return _T("Copyright (C)");
}

const TCHAR* Max16Exporter::OtherMessage1()
{
	return _T("");
}

const TCHAR* Max16Exporter::OtherMessage2()
{
	return _T("");
}

unsigned int Max16Exporter::Version()
{
	return 1;
}

BOOL Max16Exporter::SupportsOptions( int ext, DWORD options )
{
	return TRUE;
}

wchar_t* Max16Exporter::charToWchar(const char* obj)
{
	int len = (int)strlen(obj) + 1;//'\0'
	if (len > 1024)
		return nullptr;
	wchar_t* strWchar(new wchar_t[1024]);
	int w_Len = MultiByteToWideChar(CP_ACP, 0, obj, len, strWchar, 1024);
	return strWchar;
}

}

 