// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "TreeGeneratorEnviromentWorker.h"

TreeGeneratorEnviromentWorker::TreeGeneratorEnviromentWorker(TArray<FVector> roots,
	TArray<FVector> rootDirections,
	TArray<FBox> boundingVolumes,
	TArray<FBox> obstacles,
	int randomSeed,
	int atrMin,
	int atrMax,
	int numberOfIterations,
	float ocupiedRadius,
	float envRadius,
	float envAngle,
	float envStr,
	FVector gStr,
	float lStr
	)
{
	isRunning = false;
	workerThread = NULL;
	enviroment = NULL;

	this->boundingVolumes = boundingVolumes;
	this->obstacles = obstacles;
	this->randomSeed = randomSeed;
	this->atrMin = atrMin;
	this->atrMax = atrMax;
	this->numberOfIterations = numberOfIterations;
	this->ocupiedRadius = ocupiedRadius;
	this->envRadius = envRadius;
	this->envAngle = envAngle;
	this->envStr = envStr;
	this->gStr = gStr;
	this->lStr = lStr;

	this->rootBuds.Init(0);
	for (int i = 0; i != roots.Num(); ++i){
		rootBuds.Add(new Bud(roots[i], rootDirections[i]));
	}
}

TreeGeneratorEnviromentWorker::~TreeGeneratorEnviromentWorker()
{
}


void TreeGeneratorEnviromentWorker::DestroyThread(){
	if (workerThread){
		this->workerThread->Kill(true);
		delete this->workerThread;
		this->workerThread = NULL;
	}
	if (enviroment){
		delete enviroment;
		enviroment = NULL;
	}
}

void TreeGeneratorEnviromentWorker::Start(){
	DestroyThread();

	workerThread = FRunnableThread::Create(this, TEXT("Tree"));
}


bool TreeGeneratorEnviromentWorker::Init(){
	isRunning = true;
	return true;
}
uint32 TreeGeneratorEnviromentWorker::Run(){
	

	TSharedPtr<FRandomStream> random = TSharedPtr<FRandomStream>::TSharedPtr(new FRandomStream());
	random->Initialize(this->randomSeed);
	enviroment = new Enviroment(random, this->atrMin, this->atrMax, this->boundingVolumes);

	for (int32 passNum = 0; passNum < this->numberOfIterations; ++passNum){
		TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = this->rootBuds.CreateIterator();
		for (iterator.Reset(); iterator; iterator++){
			ProcessBud(rootBuds[iterator.GetIndex()], &obstacles, &TreeGeneratorEnviromentWorker::ProcessAttractors);
			ProcessBud(rootBuds[iterator.GetIndex()], &obstacles, &TreeGeneratorEnviromentWorker::ProcessResourcesFirstPass);
			GatherResourcesToRootReturnValue retVal = GatherResourcesToRoot(rootBuds[iterator.GetIndex()]);
			rootBuds[iterator.GetIndex()]->resource = (retVal.resources * numberOfIterations) / (retVal.weight + numberOfIterations);
		}
		for (iterator.Reset(); iterator; iterator++){
			ProcessBud(rootBuds[iterator.GetIndex()], &obstacles, &TreeGeneratorEnviromentWorker::ProcessAttractorsConical);
			ProcessBud(rootBuds[iterator.GetIndex()], &obstacles, &TreeGeneratorEnviromentWorker::ProcessResourcesSecondPass);
			ProcessBud(rootBuds[iterator.GetIndex()], NULL, &TreeGeneratorEnviromentWorker::KillBud);
			GrowBuds(rootBuds[iterator.GetIndex()], obstacles);
		}
	}
	delete enviroment;
	enviroment = NULL;
	random.Reset();
	isRunning = false;
	return 0;
}

void TreeGeneratorEnviromentWorker::Stop(){
	delete enviroment;
	enviroment = NULL;
}

GatherResourcesToRootReturnValue TreeGeneratorEnviromentWorker::GatherResourcesToRoot(Bud* bud){
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

void TreeGeneratorEnviromentWorker::ProcessBud(Bud* bud, void* parameters, BudFunction function){
	(this->*function)(bud, parameters);
	if (bud->allignedBud != NULL){
		ProcessBud(bud->allignedBud, parameters, function);
	}
	if (bud->secondaryAllignedBud != NULL){
		ProcessBud(bud->secondaryAllignedBud, parameters, function);
	}
}

void TreeGeneratorEnviromentWorker::ProcessAttractors(Bud* bud, void* parameters){
	this->enviroment->DeactivateAttractorsInSphere(bud->position, this->ocupiedRadius);
}

struct CloseBudsParameters
{
	TArray<Bud*> buds;
	Bud* targetBud;
	float radius;
};

void TreeGeneratorEnviromentWorker::GetBudsInRadius(Bud* bud, void* parameters){
	CloseBudsParameters* params = (CloseBudsParameters*)parameters;
	const int arrayStep = 16;
	if (FMath::Abs(FVector::Dist(bud->position, params->targetBud->position)) <= params->radius){
		params->buds.Add(bud);
	}
}

void TreeGeneratorEnviromentWorker::ProcessAttractorsConical(Bud* bud, void* parameters){
	if (bud->allignedBud && bud->secondaryAllignedBud)
		return;

	CloseBudsParameters* params = new CloseBudsParameters();
	params->buds.Init(0);
	params->targetBud = bud;
	params->radius = 2 * this->envRadius;

	TIndexedContainerIterator<TArray<Bud*>, Bud*, int32> iterator = this->rootBuds.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		ProcessBud(rootBuds[iterator.GetIndex()], params, &TreeGeneratorEnviromentWorker::GetBudsInRadius);
	}

	FVector direction = this->enviroment->GetDirectionFromCone(bud->position, bud->axis, envRadius, envAngle, params->buds, *(TArray<FBox>*)parameters);
	delete params;

	float lenght;
	FVector directionVect;
	direction.ToDirectionAndLength(directionVect, lenght);

	if (lenght > 0.01){
		direction.Normalize();
	}
	bud->axis = direction;
}
void TreeGeneratorEnviromentWorker::ProcessResourcesFirstPass(Bud* bud, void* parameters){
	if (bud->isDead)
	{
		bud->resource = 0;
		return;
	}
	CalcShadowParameters params;
	params.bud = bud;
	params.shadow = 0;
	for (int i = 0; rootBuds.Num() != i; ++i){
		ProcessBud(rootBuds[i], &params, &TreeGeneratorEnviromentWorker::CalcShadow);
	}

	bud->resource = FMath::Max(0.0f, this->lStr - params.shadow);
}

void TreeGeneratorEnviromentWorker::CalcShadow(Bud* bud, void* parameters){
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

void TreeGeneratorEnviromentWorker::KillBud(Bud* bud, void* parameters){
	if (!bud->isDead && bud->resource == 0){
		bud->resource = 0;
		bud->isDead = true;
	}
}

void TreeGeneratorEnviromentWorker::ProcessResourcesSecondPass(Bud* bud, void* parameters){
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

void TreeGeneratorEnviromentWorker::GrowBuds(Bud* bud, TArray<FBox> obstacles){
	Bud* tmp;
	if (bud->isDead)
		return;

	float lenght;
	FVector directionVect;
	bud->axis.ToDirectionAndLength(directionVect, lenght);

	if (lenght < 0.01){
		if (bud->allignedBud)
			GrowBuds(bud->allignedBud, obstacles);
		if (bud->secondaryAllignedBud)
			GrowBuds(bud->secondaryAllignedBud, obstacles);
		return;
	}

	bud->axis.Normalize();
	FVector axis = bud->axis * bud->resource * this->envStr + this->gStr;
	FVector position = bud->position + axis;
	position.Y = FMath::Max(position.Y, 0.0f);
	if (bud->secondaryAllignedBud){
		GrowBuds(bud->allignedBud, obstacles);
		GrowBuds(bud->secondaryAllignedBud, obstacles);
		return;
	}
	else{
		if (bud->allignedBud){
			GrowBuds(bud->allignedBud, obstacles);
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

TArray<Bud*> TreeGeneratorEnviromentWorker::GetResult(){
	this->workerThread->WaitForCompletion();
	return rootBuds;
}