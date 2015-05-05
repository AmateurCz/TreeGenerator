// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bud.h"
#include "Enviroment.h"


struct GatherResourcesToRootReturnValue{
	double resources;
	uint32 weight;
};
struct CalcShadowParameters{
	short shadow;
	Bud* bud;
};

/**
 *
 */
class TREEGENERATOR_API TreeGeneratorEnviromentWorker : public FRunnable
{
private:
	FRunnableThread* workerThread;
	Enviroment* enviroment;
	bool isRunning;


	TArray<Bud*> rootBuds;

	TArray<FBox> boundingVolumes;
	TArray<FBox> obstacles;
	int randomSeed;
	int atrMin;
	int atrMax;
	int numberOfIterations;
	float ocupiedRadius;
	float envRadius;
	float envAngle;
	float envStr;
	FVector gStr;
	float lStr;

	void DestroyThread();

	typedef void (TreeGeneratorEnviromentWorker::*BudFunction)(Bud*, void*);
	void ProcessBud(Bud* bud, void* parameters, BudFunction);

	void ProcessAttractors(Bud* bud, void* parameters);
	void KillBud(Bud* bud, void* parameters);
	void ProcessAttractorsConical(Bud* bud, void* parameters);
	void ProcessShadowMap(Bud* bud, void* parameters);
	void ProcessResourcesFirstPass(Bud* bud, void* parameters);
	void CalcShadow(Bud* bud, void* parameters);
	void GetBudsInRadius(Bud* bud, void* parameters);
	void GrowBuds(Bud* bud, TArray<FBox> obstacles);
	void ProcessResourcesSecondPass(Bud* bud, void* parameters);

	GatherResourcesToRootReturnValue GatherResourcesToRoot(Bud* bud);

public:
	TreeGeneratorEnviromentWorker(TArray<FVector> roots, 
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
		float lStr);
	~TreeGeneratorEnviromentWorker();

	void Start();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface

	TArray<Bud*> GetResult();
	bool IsRunning() { return isRunning; };
};
