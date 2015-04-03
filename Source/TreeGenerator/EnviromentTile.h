// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bud.h"
#include "Array3DBase.h"
/**
 * 
 */
class TREEGENERATOR_API EnviromentTile : public Array3DBase
{
protected:
	uint32 attractorCount, attractorsUsed;
	bool* attractorActiveArray;
	bool allAtractorsUsed;
	FVector* attractorArray;
	FRandomStream* randomGenerator;

	void DestroyAttractors();

public:
	EnviromentTile(FVector begin, FRandomStream* randomGenerator);
	~EnviromentTile(void);

	void Initialize(uint32 minAttractors, uint32 maxAttractors);

	void GenerateAttractors();
	void ResetAttractors();
	void ResetAll();

	void DeactivateAttractorsInSphere(FVector position, double radius);
	void DeactivateAttractorsInBox(FVector min, FVector max);
	FVector GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, Bud** closeBuds, uint32 closeBudCount);
	uint32 GetAttractorCount();
	void GetAttractorsAsVertices(FVector* vertArray, uint32 offset);
};
