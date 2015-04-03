// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bud.h"
#include "EnviromentTile.h"
#include "Array3DBase.h"

/**
 * 
 */
class TREEGENERATOR_API Enviroment : public Array3DBase
{
protected:
	EnviromentTile ** enviroment;
	FRandomStream* randomGenerator;

	uint32 minAttractors, maxAttractors;

	EnviromentTile* CreateTile(FVector position);
	EnviromentTile* CreateTile(int32 x, int32 y, int32 z);

	void ResizeTiles(int32 x1, int32 y1, int32 z1, int32 x2, int32 y2, int32 z2);
	void DeleteEnviroment();

public:
	Enviroment(FRandomStream* randomGenerator, uint32 minAttractors, uint32 maxAttractors);
	~Enviroment(void);

	EnviromentTile* GetTile(FVector position);
	EnviromentTile* GetTile(uint32 x, uint32 y, uint32 z);
	FVector GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, Bud** closeBuds, uint32 closeBudCount);
	void DeactivateAttractorsInSphere(FVector position, double radius);
	void DeactivateAttractorsInBox(FVector min, FVector max);

	//PolygonObject* GetAttractorsAsVertices();

	void ResetAll();
};
