// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "TreeGeneratorActor.h"


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

struct CalcShadowParameters{
	short shadow;
	Bud* bud;
};
#pragma endregion

ATreeGeneratorActor::ATreeGeneratorActor(){

	this->TreeMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Tree"));

	// Apply a material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/Materials/BaseColor.BaseColor"));
	this->TreeMesh->SetMaterial(0, Material.Object);

	this->RootComponent = TreeMesh;

	this->LightStrength = 30;
	this->GravityStrength = 0.4;
	this->EnviromentStrength = 0.7;

	this->OcupiedRadius = 130.0;
	this->EnviromentConeRadius = 150;
	this->EnviromentConeAngle = PI / 3;

	this->AttractorsMaximum = 128;
	this->AttractorsMinimum = 16;

	this->NumberOfIterations = 10;
	this->NumberOfSegments = 5;
	this->LeafActor = NULL;
	this->leafs.Init(0);
	rand = NULL;
	enviroment = NULL;
	m_rootBuds.Init(0);
}

// Called when the game starts or when spawned
void ATreeGeneratorActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateTree();
}

// Called every frame
void ATreeGeneratorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATreeGeneratorActor::GenerateTree(){

	TArray<FBox> boundingVol;
	boundingVol.Init(0);
	if (GetWorld()){
		for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr){

			if (BoundingVolumes.Contains(ActorItr->GetName()))
				boundingVol.Add(ActorItr->GetComponentsBoundingBox());
		}
	}

	TIndexedContainerIterator<TArray<AActor*>, AActor*, int32> leafIterator = this->leafs.CreateIterator();
	for (leafIterator.Reset(); leafIterator; leafIterator++){
		if (leafs[leafIterator.GetIndex()] != NULL){
			leafs[leafIterator.GetIndex()]->Destroy();
		}
	}
	this->leafs.Reset();

	if (this->rand.Get() == NULL){
		this->rand = TSharedPtr<FRandomStream>::TSharedPtr(new FRandomStream());
	}
	this->rand->Initialize(1234);
	if (this->enviroment.Get() == NULL){
		this->enviroment = TSharedPtr<Enviroment>::TSharedPtr(new Enviroment(this->rand, this->AttractorsMinimum, this->AttractorsMaximum, boundingVol));
	}
	if (GetWorld()){
		for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr){
			
			if (Obstacles.Contains(ActorItr->GetName()))
				this->enviroment->DeactivateAttractorsInBox(ActorItr->GetComponentsBoundingBox());
		}
	}

	TIndexedContainerIterator<TArray<FVector>, FVector, int32> rootIterator = this->Roots.CreateIterator();
	for (rootIterator.Reset(); rootIterator; rootIterator++){
		m_rootBuds.Add(new Bud(this->Roots[rootIterator.GetIndex()], FVector(0, 1, 0)));
	}

	for (int32 passNum = 0; passNum < this->NumberOfIterations; ++passNum){
		TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = this->m_rootBuds.CreateIterator();
		for (iterator.Reset(); iterator; iterator++){
			ProcessBud(m_rootBuds[iterator.GetIndex()], NULL, &ATreeGeneratorActor::ProcessAttractors);
			ProcessBud(m_rootBuds[iterator.GetIndex()], NULL, &ATreeGeneratorActor::ProcessResourcesFirstPass);
			GatherResourcesToRootReturnValue retVal = GatherResourcesToRoot(m_rootBuds[iterator.GetIndex()]);
			m_rootBuds[iterator.GetIndex()]->resource = (retVal.resources * NumberOfIterations) / (retVal.weight + NumberOfIterations);
		}
		for (iterator.Reset(); iterator; iterator++){
			ProcessBud(m_rootBuds[iterator.GetIndex()], NULL, &ATreeGeneratorActor::ProcessAttractorsConical);
			ProcessBud(m_rootBuds[iterator.GetIndex()], NULL, &ATreeGeneratorActor::ProcessResourcesSecondPass);
			ProcessBud(m_rootBuds[iterator.GetIndex()], NULL, &ATreeGeneratorActor::KillBud);
			GrowBuds(m_rootBuds[iterator.GetIndex()]);
		}
	}
	GenerateModel();
	this->enviroment.Reset();
	this->m_rootBuds.Reset();
}

void ATreeGeneratorActor::BeginDestroy(){
	this->m_rootBuds.Reset();
	TIndexedContainerIterator<TArray<AActor*>, AActor*, int32> leafIterator = this->leafs.CreateIterator();
	for (leafIterator.Reset(); leafIterator; leafIterator++){
		if (leafs[leafIterator.GetIndex()] != NULL){
			leafs[leafIterator.GetIndex()]->Destroy();
		}
	}
	AActor::BeginDestroy();
}

GatherResourcesToRootReturnValue ATreeGeneratorActor::GatherResourcesToRoot(Bud* bud){
	GatherResourcesToRootReturnValue retVal, tmp;
	retVal.resources = bud->resource;
	retVal.weight = 1;
	if (bud->allignedBud && !bud->allignedBud->isDead){
		tmp = GatherResourcesToRoot(bud->allignedBud);
		retVal.resources += tmp.resources;
		retVal.weight = tmp.weight;
	}
	if (bud->secondaryAllignedBud && !bud->secondaryAllignedBud->isDead){
		tmp = GatherResourcesToRoot(bud->secondaryAllignedBud);
		retVal.resources += tmp.resources;
		retVal.weight += tmp.weight;
	}
	bud->resource = retVal.resources;
	bud->weight = retVal.weight;
	return retVal;
}

void ATreeGeneratorActor::ProcessBud(Bud* bud, void* parameters, BudFunction function){
	(this->*function)(bud, parameters);
	if (bud->allignedBud != NULL){
		ProcessBud(bud->allignedBud, parameters, function);
	}
	if (bud->secondaryAllignedBud != NULL){
		ProcessBud(bud->secondaryAllignedBud, parameters, function);
	}
}

void ATreeGeneratorActor::ProcessAttractors(Bud* bud, void* parameters){
	this->enviroment->DeactivateAttractorsInSphere(bud->position, this->OcupiedRadius);
}

struct CloseBudsParameters
{
	TArray<Bud*> buds;
	Bud* targetBud;
	float radius;
};

void ATreeGeneratorActor::GetBudsInRadius(Bud* bud, void* parameters){
	CloseBudsParameters* params = (CloseBudsParameters*)parameters;
	const int arrayStep = 16;
	if (FMath::Abs(FVector::Dist(bud->position, params->targetBud->position)) <= params->radius){
		params->buds.Add(bud);
	}
}

void ATreeGeneratorActor::ProcessAttractorsConical(Bud* bud, void* parameters){
	if (bud->allignedBud && bud->secondaryAllignedBud)
		return;

	CloseBudsParameters* params = new CloseBudsParameters();
	params->buds.Init(0);
	params->targetBud = bud;
	params->radius = 2 * this->EnviromentConeRadius;

	TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = this->m_rootBuds.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		ProcessBud(m_rootBuds[iterator.GetIndex()], params, &ATreeGeneratorActor::GetBudsInRadius);
	}

	FVector direction = this->enviroment->GetDirectionFromCone(bud->position, bud->axis, EnviromentConeRadius, EnviromentConeAngle, params->buds);
	delete params;

	float lenght;
	FVector directionVect;
	direction.ToDirectionAndLength(directionVect, lenght);

	if (lenght > 0.01){
		direction.Normalize();
	}
	bud->axis = direction;
}
void ATreeGeneratorActor::ProcessResourcesFirstPass(Bud* bud, void* parameters){
	if (bud->isDead)
	{
		bud->resource = 0;
		return;
	}
	CalcShadowParameters params;
	params.bud = bud;
	params.shadow = 0;
	TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = this->m_rootBuds.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		ProcessBud(m_rootBuds[iterator.GetIndex()], &params, &ATreeGeneratorActor::CalcShadow);
	}

	bud->resource = FMath::Max(0.0f, this->LightStrength - params.shadow);
}

void ATreeGeneratorActor::CalcShadow(Bud* bud, void* parameters){
	CalcShadowParameters* params = (CalcShadowParameters*)parameters;

	if (bud == params->bud || bud->isDead || bud->position.Y < params->bud->position.Y){
		return;
	}
	FVector direction = bud->position - params->bud->position;
	direction.Normalize();
	if (FMath::Acos(FVector::DotProduct(direction, FVector(0, -1, 0))) < (PI / 2)){
		++(params->shadow);
	}
}

void ATreeGeneratorActor::KillBud(Bud* bud, void* parameters){
	if (!bud->isDead && bud->resource == 0){
		bud->resource = 0;
		bud->isDead = true;
	}
}

void ATreeGeneratorActor::ProcessResourcesSecondPass(Bud* bud, void* parameters){
	if (!bud->allignedBud || bud->isDead)
		return;

	double primaryResources = 0;
	double secondaryResources = 0;
	double lambda = 0.5;

	if (bud->allignedBud && !bud->allignedBud->isDead)
		primaryResources = bud->allignedBud->resource;

	if (bud->secondaryAllignedBud && !bud->secondaryAllignedBud->isDead){
		secondaryResources = bud->secondaryAllignedBud->resource;
	}
	if (primaryResources + secondaryResources){
		bud->allignedBud->resource = (bud->resource * lambda * primaryResources) / (lambda * primaryResources + (1 - lambda)*secondaryResources);
		if (bud->secondaryAllignedBud){
			bud->secondaryAllignedBud->resource = (bud->resource * (1 - lambda) * secondaryResources) / (lambda * primaryResources + (1 - lambda)*secondaryResources);
		}
	}
}

void ATreeGeneratorActor::GrowBuds(Bud* bud){
	Bud* tmp;
	if (bud->isDead)
		return;

	float lenght;
	FVector directionVect;
	bud->axis.ToDirectionAndLength(directionVect, lenght);

	if (lenght < 0.01){
		if (bud->allignedBud)
			GrowBuds(bud->allignedBud);
		if (bud->secondaryAllignedBud)
			GrowBuds(bud->secondaryAllignedBud);
		return;
	}

	bud->axis.Normalize();
	FVector axis = bud->axis * bud->resource * this->EnviromentStrength - FVector(0, this->GravityStrength, 0);
	FVector position = bud->position + axis;
	position.Y = FMath::Max(position.Y, 0.0f);
	if (bud->secondaryAllignedBud){
		GrowBuds(bud->allignedBud);
		GrowBuds(bud->secondaryAllignedBud);
		return;
	}
	else{
		if (bud->allignedBud){
			GrowBuds(bud->allignedBud);
			if (!bud->parentBud)
				return;
			tmp = new Bud(position, axis, bud);
			ProcessResourcesFirstPass(tmp, NULL);
			return;
		}
		tmp = new Bud(position, axis, bud);
		ProcessResourcesFirstPass(tmp, NULL);
	}
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
	bud->diameter = calculateDiameter(bud->allignedBud->diameter, bud->secondaryAllignedBud->diameter, 2.5);
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
			parameters->pointOffset, elapsedLenght/22.0);
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
			parameters->pointOffset, elapsedLenght /22.0);

		if (bud->allignedBud && !bud->allignedBud->isDead){
			buildPolyRing(
				parameters->polys,
				parameters->polySize,
				parameters->pointOffset,
				parameters->pointOffset + this->NumberOfSegments,
				parameters->polyOffset, this->NumberOfSegments);
			currentSegmentLenght = FVector::Dist(bud->position, bud->allignedBud->position);
			/*buildPolyUVRing(parameters->uvtag,
				parameters->polySize, elapsedLenght / brachLenght, (elapsedLenght + currentSegmentLenght) / branchLenght, parameters->polyOffset, this->NumberOfSegments);*/
			elapsedLenght += currentSegmentLenght;
			parameters->polyOffset += this->NumberOfSegments;
		}
		parameters->pointOffset += this->NumberOfSegments;
		lastBud = bud;
		bud = bud->allignedBud;
	}
	bud = lastBud;
	bud->originalAxis.Normalize();
	FProceduralMeshVertex point;
	point.Position = bud->position + bud->originalAxis;
	point.Position = FVector(point.Position.X, point.Position.Z, point.Position.Y);
	point.U = 0.5;
	point.V = 1.0;
	point.Bitangent = FVector(bud->originalAxis.X, bud->originalAxis.Z, bud->originalAxis.Y);
	point.Tangent = point.Bitangent ^ FVector(0, 0, 1);
	point.Normal = point.Bitangent ^ point.Tangent;
	parameters->points[parameters->pointOffset] = point;

	parameters->caps.Add(point);
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
	GeometryParameters* parameters = new GeometryParameters();
	parameters->pointOffset = 0;
	parameters->pointSize = 0;
	parameters->polyOffset = 0;
	parameters->polySize = 0;
	parameters->capsCount = 0;
	parameters->capsOffset = 0;

	TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = this->m_rootBuds.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		UpdateDiameter(m_rootBuds[iterator.GetIndex()], this->BranchDiameter);
	}

	for (iterator.Reset(); iterator; iterator++){
		CalculateBranchModel(m_rootBuds[iterator.GetIndex()], parameters);
	}

	parameters->caps.Init(0);
	parameters->points.Init(parameters->pointSize);
	parameters->polys.Init(parameters->polySize);
	for (iterator.Reset(); iterator; iterator++){
		BuildBranchModel(m_rootBuds[iterator.GetIndex()], parameters);
	}

	TArray<FProceduralMeshTriangle> polys;
	int32 polyCount = parameters->polySize * 2 - parameters->capsCount * this->NumberOfSegments;
	polys.Init(polyCount);
	parameters->polyOffset = 0;
	for (int i = 0; polyCount > i; i++){
		PolygonIndices indices = parameters->polys[parameters->polyOffset++];
		FProceduralMeshTriangle polygon;
		polygon.Vertex0 = parameters->points[indices.B];
		polygon.Vertex1 = parameters->points[indices.A];
		polygon.Vertex2 = parameters->points[indices.C];
		polys[i] = polygon;
		if (indices.D != -1){
			polygon.Vertex0 = parameters->points[indices.B];
			polygon.Vertex1 = parameters->points[indices.C];
			polygon.Vertex2 = parameters->points[indices.D];
			polys[++i] = polygon;
		}
	}

	this->TreeMesh->SetProceduralMeshTriangles(polys);

	UWorld* const World = GetWorld();

	if (World && this->LeafActor){
		TIndexedContainerIterator<TArray<FProceduralMeshVertex>, FProceduralMeshVertex, int32> rootIterator = parameters->caps;
		for (rootIterator.Reset(); rootIterator; rootIterator++){
			FVector position = parameters->caps[rootIterator.GetIndex()].Position;
			FVector normal = parameters->caps[rootIterator.GetIndex()].Normal;

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
			FRotator rotator = FRotator::MakeFromEuler(FVector(0, 0, angle));

			FActorSpawnParameters spawnParams = FActorSpawnParameters();
			spawnParams.Owner = this;
			AActor* leaf = World->SpawnActor(this->LeafActor, &position, &rotator, spawnParams);
			leafs.Add(leaf);
		}
	}
	delete parameters;
}
