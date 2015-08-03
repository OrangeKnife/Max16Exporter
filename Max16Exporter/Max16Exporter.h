#pragma once
#include <fstream>
namespace NBE
{
	class Max16Exporter : public SceneExport
{
public:
	Max16Exporter();
	~Max16Exporter();

	/** Exports the scene. */
	int				DoExport( const TCHAR* name, ExpInterface *ei, Interface* i, BOOL suppressprompts=FALSE, DWORD options=0 );

	/** Show DLL's "About..." box */
	void			ShowAbout( HWND hwnd );

	/** Number of extensions supported */
	int				ExtCount();
	
	/** Extension #n (i.e. "3DS") */
	const MCHAR*	Ext( int n );					
	
	/** Long ASCII description (i.e. "Autodesk 3D Studio File") */
	const MCHAR*	LongDesc();
	
	/** Short ASCII description (i.e. "3D Studio") */
	const MCHAR*	ShortDesc();
	
	/** ASCII Author name */
	const MCHAR*	AuthorName();
	
	/** ASCII Copyright message */
	const MCHAR*	CopyrightMessage();
	
	/** Other message #1 */
	const MCHAR*	OtherMessage1();
	
	/** Other message #2 */
	const MCHAR*	OtherMessage2();
	
	/** Version number * 100 (i.e. v3.01 = 301) */
	unsigned int	Version();
	
	/** Returns TRUE if option supported. */
	BOOL			SupportsOptions( int ext, DWORD options );



	enum TAG
	{
		OBJECT = 0,
		PARENT,
	};

	struct WeightIndex
	{

		WeightIndex(float w = 0,int i = 0):wt(w),idx(i)
		{}

		bool operator > (const WeightIndex &m)const {
			return wt > m.wt;
		}
		float wt;
		int idx;
	};


	struct KeyFrame
	{
		Point3 pos; // if you're sampling 30fps, you end up with 60 frames for 2s anim 
		Point3 scale;
		Quat rot; // per single object
		unsigned int frameIdx;
		bool operator!= (const KeyFrame& other)const {
			return !pos.Equals(other.pos) || !scale.Equals(other.scale) || !rot.Equals(other.rot);
		}
	};

	struct MyFace
	{
		MyFace(int v0,int v1,int v2):x(v0),y(v1),z(v2)
		{}
		int x,y,z;
	};

	struct Vertex
	{
		Point3 pos;
		Point3 normal;
		Point4 color;
		Point2 uv;
		bool operator == (Vertex& v)
		{
			return memcmp(this,&v,sizeof(Vertex))==0;
		}
	};

	struct SkinVertex
	{
		Point3 pos;
		Point3 normal;
		Point4 color;
		Point2 uv;
		int idx4[4];//bone idx
		float wt4[4];//weight 

		bool operator == (SkinVertex& v)
		{
			return memcmp(this,&v,sizeof(SkinVertex))==0;
		}
	};

	size_t vertexExist(Vertex& v);//return index of vertex

	size_t skinVertexExist(SkinVertex& sv);// support skin vertex

	int ExportMeshFromNode(IGameNode* node);

	int ExportMat(IGameNode* node);//export all the material in the obj, then later , only export the index of material

	void ExportObjectsFromNode(IGameNode* node);//switch case 
 
	int matExist(IGameMaterial* pMat);

	void outputFacesByMesh(IGameNode* node);

	void outputSkinIndicesByMesh(IGameNode* node);

	void outputMaterialInfo(IGameMaterial* mat);

	void ExportBones(IGameNode* node);

	int getBoneIdx(IGameNode* node);

	int buildBoneTree(IGameNode* node);

	static bool descending (const WeightIndex& i,const WeightIndex& j) { return i > j; }

	int getIdxOfMaterial(IGameMaterial* mat);

	static wchar_t* charToWchar(const char* obj);
private:
	IGameScene*		m_igame;
	FILE*			m_fh;
	std::ofstream		m_ofs;
	void			deinit();

	Max16Exporter(const Max16Exporter&);
	Max16Exporter& operator=(const Max16Exporter&);

	std::vector<IGameNode*> boneVec;//recorded all the bones in the scene
	std::vector<MyFace> faceVec;
	std::vector<Vertex> vertexVec;
	std::vector<IGameMaterial*> matVec;
	std::vector<size_t> indexVec;

	std::vector<SkinVertex> skinVertexVec;

	TimeValue startTime,endTime;
};
	




}