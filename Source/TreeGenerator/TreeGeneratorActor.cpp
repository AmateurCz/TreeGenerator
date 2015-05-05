// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "TreeGeneratorActor.h"
#include "TreeRoot.h"
#include <fbxsdk.h>

#pragma region constants

static const char* gDiffuseElementName = "DiffuseUV";

#pragma endregion

#pragma region static functions

double calculateDiameter(double d1, double d2, double n){
	return FMath::Pow(FMath::Pow(d1, n) + FMath::Pow(d2, n), 1.0f / n);
}

double getBranchLenght(const Bud* bud){
	double lenght = 0;
	while (bud->allignedBud && !bud->isDead){
		lenght += FVector::Dist(bud->position, bud->allignedBud->position);
		bud = bud->allignedBud;
	}
	return lenght;
}

FVector rotateVecX(FVector vector, double angle){
	return FVector(vector.X, cos(angle) * vector.Y - sin(angle) * vector.Z, sin(angle) * vector.Y + cos(angle) * vector.Z);
}
FVector rotateVecY(FVector vector, double angle){
	return FVector(cos(angle) * vector.X + sin(angle) * vector.Z, vector.Y, -sin(angle) * vector.X + cos(angle) * vector.Z);
}
FVector rotateVecZ(FVector vector, double angle){
	return FVector(cos(angle) * vector.X - sin(angle) * vector.Y, sin(angle) * vector.X + cos(angle) * vector.Y, vector.Z);
}
FVector rotateVec(FVector vector, double xAngle, double yAngle, double zAngle){
	return rotateVecZ(rotateVecY(rotateVecX(vector, xAngle), yAngle), zAngle);
}

void buildVertRing(double radius, FVector center, FVector normal, uint32 count, TArray<FProceduralMeshVertex>& pointArray, uint32 pointArraySize, uint32 offset, float V){
	if (count == 0)
		return;
	if (count + offset > pointArraySize)
		return;

	float xyLen = sqrt(normal.X * normal.X + normal.Y * normal.Y);
	float cos_z = normal.X / xyLen;
	float sin_z = normal.Y / xyLen;
	//sin_z *= -1

	float x1 = (normal.X * cos_z) - (normal.Y * -sin_z);
	//float y1 = (normal.X * - sin_z) + (normal.Y * cos_z);
	float z1 = normal.Z;

	xyLen = sqrt(x1 * x1 + z1 * z1);
	float cos_y = z1 / xyLen;
	float sin_y = x1 / xyLen;
	sin_y = sin_y  * -1.0;

	FMatrix matrix =

		FMatrix(FVector(cos_z, -sin_z, 0),
		FVector(sin_z, cos_z, 0),
		FVector(0, 0, -1),
		FVector(0, 0, 0))*
		FMatrix(FVector(cos_y, 0, -sin_y),
		FVector(0, 1, 0),
		-FVector(sin_z, 0, cos_z),
		FVector(0, 0, 0))*FMatrix(FVector(cos_z, sin_z, 0),
		FVector(-sin_z, cos_z, 0),
		FVector(0, 0, -1),
		FVector(0, 0, 0));

	for (uint32 i = 0; i != count; ++i){
		FProceduralMeshVertex point;
		FVector position = FVector(radius * cos(i * (2 * PI / count)), radius * sin(i * (2 * PI / count)), 0);
		point.Position = matrix.TransformVector(position) + center;// *1.0f;
		point.Position = FVector(point.Position.X, point.Position.Z, point.Position.Y);
		point.U = (double)i / (double)count;
		point.V = V;
		point.Bitangent = point.Position - FVector(center.X, center.Z, center.Y);
		point.Bitangent.Normalize();
		point.Tangent = FVector(normal.X, normal.Z, normal.Y);
		point.Normal = point.Bitangent ^ point.Tangent;
		point.Color = FColor(255, 51, 51);
		pointArray[i + offset] = point;
	}
}

void buildPolyCap(TArray<PolygonIndices>& polygons, uint32 polygonsSize, uint32 bottomOffset, uint32 topOffset, uint32 arrayOffset, uint32 count){
	if (polygonsSize < arrayOffset + count)
		return;
	for (uint32 num = 0; num < count; ++num){
		uint32 secIndex = (num + 1) % count;
		PolygonIndices indice;
		indice.A = topOffset;
		indice.B = bottomOffset + secIndex;
		indice.C = bottomOffset + num;
		indice.D = -1;
		polygons[num + arrayOffset] = indice;
	}
}

void buildPolyRing(TArray<PolygonIndices>& polygons, uint32 polygonsSize, uint32 bottomOffset, uint32 topOffset, uint32 arrayOffset, uint32 count){
	if (polygonsSize < arrayOffset + count)
		return;
	for (uint32 num = 0; num != count; ++num){
		uint32 secIndex = (num + 1) % count;
		PolygonIndices indice;
		indice.B = bottomOffset + secIndex;
		indice.D = bottomOffset + num;
		indice.C = topOffset + num;
		indice.A = topOffset + secIndex;
		polygons[num + arrayOffset] = indice;
	}
}

#pragma endregion

#pragma region FBXfunctions

inline FbxVector4 ToFbxVector(FVector vector){
	return FbxVector4(vector.X, vector.Y, vector.Z, 1);
}

void SetVertNormTangBitang(FbxGeometryElementNormal* lGeometryElementNormal,
	FbxGeometryElementTangent* lGeometryElementTangent,
	FbxGeometryElementBinormal* lGeometryElementBinormal,
	FProceduralMeshVertex point){

	lGeometryElementNormal->GetDirectArray().Add(ToFbxVector(FVector(point.Bitangent.X, point.Bitangent.Y, point.Bitangent.Z)));
	lGeometryElementTangent->GetDirectArray().Add(ToFbxVector(FVector(point.Tangent.X, point.Tangent.Y, point.Tangent.Z)));
	lGeometryElementBinormal->GetDirectArray().Add(ToFbxVector(FVector(point.Normal.X, point.Normal.Y, point.Normal.Z)));
}

void DestroySdkObjects(FbxManager* pManager, bool pExitStatus){
	if (pManager) pManager->Destroy();
	if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	//FbxString lPath = FbxGetApplicationDirectory();
	//pManager->LoadPluginsDirectory(lPath.Buffer());
	//pManager->LoadPluginsDirectory("D:\\Dokumenty\\Unreal Projects\\TreeGenerator\\ThirdParty\\FBX\\lib\\vs2013\\x64\\release");

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "Tree Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		exit(1);
	}
}

bool SaveScene(FbxManager* pManager, FbxDocument* pScene, FString pFilename, int pFileFormat = -1, bool pEmbedMedia = false)
{
	int lMajor, lMinor, lRevision;
	bool lStatus = true;

	// Create an exporter.
	FbxExporter* lExporter = FbxExporter::Create(pManager, "");

	if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// Write in fall back format in less no ASCII format found
		pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		//Try to export in ASCII if possible
		int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

		for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
		{
			if (pManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
			{
				FbxString lDesc = pManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
				const char *lASCII = "ascii";
				if (lDesc.Find(lASCII) >= 0)
				{
					pFileFormat = lFormatIndex;
					break;
				}
			}
		}
	}

	// Set the export states. By default, the export states are always set to 
	// true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
	// shows how to change these states.
	//IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
	//IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
	//IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
	//IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
	//IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
	//IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
	//IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	// Initialize the exporter by providing a filename.

	char *path = new char[MAX_PATH];
	ZeroMemory(path, MAX_PATH);
	wchar_t *wPath = pFilename.GetCharArray().GetData();
	WideCharToMultiByte(CP_UTF8, 0, wPath, pFilename.GetCharArray().Num(), path, MAX_PATH, NULL, NULL);
	if (lExporter->Initialize( path, pFileFormat, pManager->GetIOSettings()) == false)
	{
		FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
		return false;
	}
	delete[] path;
	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
	FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

	// Export the scene.
	lStatus = lExporter->Export(pScene);

	// Destroy the exporter.
	lExporter->Destroy();
	return lStatus;
}
#pragma endregion

ATreeGeneratorActor::ATreeGeneratorActor(const FObjectInitializer& ObjectInitializer){


	// Apply a material
	//static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/BaseColor.BaseColor"));
	this->TreeMeshComponent = ObjectInitializer.CreateDefaultSubobject<UProceduralMeshComponent>(this, TEXT("TreeMeshComponent"));

	this->RootComponent = TreeMeshComponent;

	this->LightStrength = 30;
	//this->GravityStrength = 0.4;
	this->EnviromentStrength = 0.7;

	this->OcupiedRadius = 130.0;
	this->EnviromentConeRadius = 150;
	this->EnviromentConeAngle = PI / 3;

	this->AttractorsMaximum = 128;
	this->AttractorsMinimum = 16;

	this->NumberOfIterations = 10;
	this->NumberOfSegments = 5;
	this->GenerateLeafsUpToDiameter = 2;
	this->BranchDiameterExponent = 2.5;
	this->RandomSeed = 1234;
	generator = NULL;
	modelData = NULL;
	this->SetActorTickEnabled(true);
}

// Called when the game starts or when spawned
void ATreeGeneratorActor::BeginPlay()
{
	this->TreeMeshComponent->SetMaterial(0, Material);
	Super::BeginPlay();
}

// Called every frame
void ATreeGeneratorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (generator && !generator->IsRunning()){
		GenerateModel();
		delete generator; 
		generator = NULL;
	}
}


void ATreeGeneratorActor::GenerateTree(){

	boundingVol.Reset();
	if (GetWorld()){
		for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr){

			if (ActorItr->ActorHasTag(TEXT("Bounding volume")))
				boundingVol.Add(ActorItr->GetComponentsBoundingBox());
		}
	}
	obstacles.Reset();
	if (GetWorld()){
		for (TActorIterator<AStaticMeshActor> ActorItr(GetWorld()); ActorItr; ++ActorItr){
			if (ActorItr->ActorHasTag(TEXT("Obstacle")))
			obstacles.Add(ActorItr->GetComponentsBoundingBox());
		}
	}
	if (generator){
		delete generator;
	}

	TArray<FVector> roots;
	TArray<FVector> rootDirections;

	if (GetWorld()){
		for (TActorIterator<ATreeRoot> ActorItr(GetWorld()); ActorItr; ++ActorItr){
			FVector position = ActorItr->GetTransform().GetTranslation();
			FVector normal = ActorItr->GetTransform().GetUnitAxis(EAxis::Z);
			roots.Add(FVector(position.X, position.Z, position.Y));
			rootDirections.Add(FVector(normal.X, normal.Z, normal.Y));;
		}
	}

	generator = new TreeGeneratorEnviromentWorker(roots, rootDirections, boundingVol, obstacles, RandomSeed, AttractorsMinimum,
		AttractorsMaximum, NumberOfIterations, OcupiedRadius, EnviromentConeRadius, EnviromentConeAngle, EnviromentStrength, GravityVector, LightStrength);
	generator->Start();
	//GenerateModel();
}

void ATreeGeneratorActor::BeginDestroy(){
	if (modelData){
		delete modelData;
		modelData = NULL;
	}
	AActor::BeginDestroy();
}

void ATreeGeneratorActor::UpdateDiameter(Bud* bud, double branchDiameter){
	if (!bud->secondaryAllignedBud)
	{
		if (!bud->allignedBud){
			bud->diameter = branchDiameter;
			return;
		}
		UpdateDiameter(bud->allignedBud, branchDiameter);
		bud->diameter = bud->allignedBud->diameter;
		return;
	}
	UpdateDiameter(bud->allignedBud, branchDiameter);
	UpdateDiameter(bud->secondaryAllignedBud, branchDiameter);
	bud->diameter = calculateDiameter(bud->allignedBud->diameter, bud->secondaryAllignedBud->diameter, this->BranchDiameterExponent);
	return;
}

void ATreeGeneratorActor::CalculateBranchModel(Bud* bud, GeometryParameters* parameters){
	Bud* tmp = bud;
	if (bud->isDead)
		return;
	while (bud && !bud->isDead){
		if (bud->secondaryAllignedBud && !bud->secondaryAllignedBud->isDead){
			CalculateBranchModel(bud->secondaryAllignedBud, parameters);
		}
		bud = bud->allignedBud;
	}
	bud = tmp;
	if (bud->parentBud){
		parameters->pointSize += this->NumberOfSegments;
		parameters->polySize += this->NumberOfSegments;
	}

	while (bud && !bud->isDead){
		if (bud->allignedBud && !bud->allignedBud->isDead){
			parameters->polySize += this->NumberOfSegments;
		}
		parameters->pointSize += this->NumberOfSegments;
		bud = bud->allignedBud;
	}
	parameters->pointSize += 1;
	parameters->capsCount += 1;
	parameters->polySize += this->NumberOfSegments;
}

void ATreeGeneratorActor::BuildBranchModel(Bud* bud, GeometryParameters* parameters){
	if (bud->isDead)
		return;
	Bud* tmp = bud;
	FProceduralMeshVertex point;
	while (bud && !bud->isDead){
		if (bud->secondaryAllignedBud && !bud->secondaryAllignedBud->isDead){
			BuildBranchModel(bud->secondaryAllignedBud, parameters);
		}
		bud = bud->allignedBud;
	}
	bud = tmp;
	double branchLenght = 0;
	double elapsedLenght = 0;
	double currentSegmentLenght = 0;
	if (bud->parentBud){
		branchLenght = getBranchLenght(bud->parentBud);
		buildVertRing(bud->diameter,
			bud->parentBud->position,
			bud->originalAxis,
			this->NumberOfSegments,
			parameters->points,
			parameters->pointSize,
			parameters->pointOffset, elapsedLenght / 22.0);
		buildPolyRing(parameters->polys,
			parameters->polySize,
			parameters->pointOffset,
			parameters->pointOffset + this->NumberOfSegments,
			parameters->polyOffset, this->NumberOfSegments);
		currentSegmentLenght = FVector::Dist(bud->position, bud->parentBud->position);
		//buildPolyUVRing(parameters->uvtag,
		//	parameters->polySize, elapsedLenght / branchLenght, (elapsedLenght + currentSegmentLenght) / branchLenght, parameters->polyOffset, this->NumberOfSegments);
		elapsedLenght += currentSegmentLenght;
		parameters->pointOffset += this->NumberOfSegments;
		parameters->polyOffset += this->NumberOfSegments;
	}
	else{
		branchLenght = getBranchLenght(bud);
	}
	Bud* lastBud = bud;
	while (bud && !bud->isDead){
		buildVertRing(
			bud->diameter,
			bud->position,
			bud->originalAxis,
			this->NumberOfSegments,
			parameters->points,
			parameters->pointSize,
			parameters->pointOffset, elapsedLenght / 22.0);

		if (bud->allignedBud && !bud->allignedBud->isDead){
			buildPolyRing(
				parameters->polys,
				parameters->polySize,
				parameters->pointOffset,
				parameters->pointOffset + this->NumberOfSegments,
				parameters->polyOffset, this->NumberOfSegments);
			currentSegmentLenght = FVector::Dist(bud->position, bud->allignedBud->position);
			if (bud->diameter < this->GenerateLeafsUpToDiameter){
				point = parameters->points[parameters->pointOffset + random.RandRange(0, this->NumberOfSegments)];
				SpawnLeaf(point.Position, point.Normal);
			}
			elapsedLenght += currentSegmentLenght;
			parameters->polyOffset += this->NumberOfSegments;
		}
		parameters->pointOffset += this->NumberOfSegments;
		lastBud = bud;
		bud = bud->allignedBud;
	}
	bud = lastBud;
	bud->originalAxis.Normalize();

	point.Position = bud->position + bud->originalAxis;
	point.Position = FVector(point.Position.X, point.Position.Z, point.Position.Y);
	point.U = 0.5;
	point.V = 1.0;
	point.Bitangent = FVector(bud->originalAxis.X, bud->originalAxis.Z, bud->originalAxis.Y);
	point.Tangent = point.Bitangent ^ FVector(0, 0, 1);
	point.Normal = point.Bitangent ^ point.Tangent;
	parameters->points[parameters->pointOffset] = point;

	SpawnLeaf(point.Position, point.Normal);

	buildPolyCap(
		parameters->polys,
		parameters->polySize,
		parameters->pointOffset - this->NumberOfSegments,
		parameters->pointOffset,
		parameters->polyOffset, this->NumberOfSegments);
	currentSegmentLenght = 1;
	/*buildPolyUVCap(parameters->uvtag,
		parameters->polySize, elapsedLenght / branchLenght, (elapsedLenght + currentSegmentLenght) / branchLenght, parameters->polyOffset, this->NumberOfSegments);*/
	elapsedLenght += currentSegmentLenght;
	parameters->polyOffset += this->NumberOfSegments;
	parameters->pointOffset += 1;
	bud = bud->allignedBud;
}


void ATreeGeneratorActor::GenerateModel(){

	if (IsGenerating())
		return;

	random.Initialize(this->RandomSeed);
	for (int i = 0; i != leafs.Num(); ++i){
		leafs[i]->SetStaticMesh(NULL);
	}

	if (!modelData){
		modelData = new GeometryParameters();
	}
	modelData->pointOffset = 0;
	modelData->pointSize = 0;
	modelData->polyOffset = 0;
	modelData->polySize = 0;
	modelData->capsCount = 0;

	TArray<Bud*> m_rootBuds = generator->GetResult();

	TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = m_rootBuds.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		UpdateDiameter(m_rootBuds[iterator.GetIndex()], this->BranchDiameter);
	}

	for (iterator.Reset(); iterator; iterator++){
		CalculateBranchModel(m_rootBuds[iterator.GetIndex()], modelData);
	}
	modelData->points.Init(modelData->pointSize);
	modelData->polys.Init(modelData->polySize);
	for (iterator.Reset(); iterator; iterator++){
		BuildBranchModel(m_rootBuds[iterator.GetIndex()], modelData);
	}

	TArray<FProceduralMeshTriangle> polys;
	int32 polyCount = modelData->polySize * 2 - modelData->capsCount * this->NumberOfSegments;
	polys.Init(polyCount);
	modelData->polyOffset = 0;
	for (int i = 0; polyCount > i; i++){
		PolygonIndices indices = modelData->polys[modelData->polyOffset++];
		FProceduralMeshTriangle polygon;
		polygon.Vertex0 = modelData->points[indices.B];
		polygon.Vertex1 = modelData->points[indices.A];
		polygon.Vertex2 = modelData->points[indices.C];
		polys[i] = polygon;
		if (indices.D != -1){
			polygon.Vertex0 = modelData->points[indices.B];
			polygon.Vertex1 = modelData->points[indices.C];
			polygon.Vertex2 = modelData->points[indices.D];
			polys[++i] = polygon;
		}
	}

	this->TreeMeshComponent->SetProceduralMeshTriangles(polys);
	this->TreeMeshComponent->UpdateBodySetup();
}


void ATreeGeneratorActor::SpawnLeaf(FVector position, FVector normal){
	UWorld* world = GetWorld();
	if (!(world && LeafMeshes.Num()))
		return;


	float angle = 0;

	if (normal.X == 0.0f){
		angle = normal.Y < 0 ? -90 : 90;
	}
	else{
		angle = FMath::RadiansToDegrees(FMath::Atan(normal.Y / normal.X));
		if (normal.X < 0){
			angle = 180 + angle;
		}
	}
	if (angle > 180)
		angle -= 360;

	FRotator rotator = FRotator::MakeFromEuler(FVector(0, 0, angle) + FVector(random.FRandRange(AngleX.X, AngleX.Y), random.FRandRange(AngleY.X, AngleY.Y), random.FRandRange(AngleZ.X, AngleZ.Y)));
	FTransform transform;
	transform.SetComponents(rotator.Quaternion(), position, FVector(random.FRandRange(1, 2), random.FRandRange(1, 2), random.FRandRange(1, 2)));
	UStaticMeshComponent* leaf = NULL;

	for (int i = 0; i != leafs.Num(); ++i){
		if (!leafs[i]->StaticMesh){
			leaf = leafs[i];
			break;
		}
	}

	if (leaf){
		leaf->SetStaticMesh(LeafMeshes[random.RandRange(0, LeafMeshes.Num() - 1)]);
		leaf->SetRelativeTransform(transform);
		leaf->UpdateComponentToWorld();
	}
	else{
		leaf = NewObject<UStaticMeshComponent>(this);
		leaf->SetStaticMesh(LeafMeshes[random.RandRange(0, LeafMeshes.Num() - 1)]);
		leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		leaf->AttachTo(this->RootComponent);
		leaf->SetRelativeTransform(transform);
		leaf->RegisterComponent();
		leaf->UpdateComponentToWorld();
		leafs.Add(leaf);
	}
	AddOwnedComponent(leaf);
}

bool ATreeGeneratorActor::IsGenerating(){
	return generator && generator->IsRunning();
}

bool ATreeGeneratorActor::Export(){

	if (!modelData)
		return true;

	FbxManager* lSdkManager = NULL;
	FbxScene* lScene = NULL;
	bool lResult = true;

	// Prepare the FBX SDK.
	InitializeSdkObjects(lSdkManager, lScene);

	// Create the scene.
	//lResult = CreateScene(lSdkManager, lScene);
	FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(lSdkManager, "SceneInfo");
	sceneInfo->mTitle = "Tree";
	sceneInfo->mSubject = "Tree exported from UnrealTool.";
	sceneInfo->mAuthor = "David Kubát.";
	sceneInfo->mRevision = "rev. 1.0";
	sceneInfo->mKeywords = "tree trunk";
	sceneInfo->mComment = "";
	lScene->SetSceneInfo(sceneInfo);
	lScene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::eMayaZUp);

	FbxMesh* lMesh = FbxMesh::Create(lScene, "Tree");
	//lMesh->

	lMesh->InitControlPoints(modelData->pointSize);
	//lMesh->InitNormals(modelData->pointSize);
	FbxVector4 *cp = lMesh->GetControlPoints();
	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();
	FbxGeometryElementTangent* lGeometryElementTangent = lMesh->CreateElementTangent();
	FbxGeometryElementBinormal* lGeometryElementBinormal = lMesh->CreateElementBinormal();
	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);
	lGeometryElementTangent->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lGeometryElementTangent->SetReferenceMode(FbxGeometryElement::eDirect);
	lGeometryElementBinormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lGeometryElementBinormal->SetReferenceMode(FbxGeometryElement::eDirect);

	FbxGeometryElementUV* lUVDiffuseElement = lMesh->CreateElementUV(gDiffuseElementName);
	lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eDirect);

	for (int i = 0; i != modelData->pointSize; ++i){
		cp[i] = ToFbxVector(modelData->points[i].Position);
		SetVertNormTangBitang(lGeometryElementNormal, lGeometryElementTangent, lGeometryElementBinormal, modelData->points[i]);
		lUVDiffuseElement->GetDirectArray().Add(FbxVector2(modelData->points[i].U, modelData->points[i].V));
	}

	modelData->polyOffset = 0;
	for (uint32 i = 0; modelData->polySize > i; i++){
		PolygonIndices indices = modelData->polys[modelData->polyOffset++];

		lMesh->BeginPolygon(-1, -1, -1, false);

		lMesh->AddPolygon(indices.B);
		lMesh->AddPolygon(indices.A);
		lMesh->AddPolygon(indices.C);
		
		if (indices.D != -1){
			lMesh->AddPolygon(indices.D);
		}
		lMesh->EndPolygon();
	}

	FbxNode* lNode = FbxNode::Create(lScene, "Tree");
	lNode->SetNodeAttribute(lMesh);
	FbxNode* root = lScene->GetRootNode();
	root->AddChild(lNode);

	FString filePath = FPaths::Combine( FPaths::GameDir().GetCharArray().GetData(), TEXT("Output\\Tree.fbx"));
	lResult = SaveScene(lSdkManager, lScene, filePath);
	if (lResult == false)
	{
		FBXSDK_printf("\n\nAn error occurred while creating the scene...\n");
		DestroySdkObjects(lSdkManager, lResult);
		return false;
	}

	DestroySdkObjects(lSdkManager, lResult);
	return true;
}