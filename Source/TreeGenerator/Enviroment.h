// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bud.h"
#include "EnviromentTile.h"
/**
 * 
 */
class TREEGENERATOR_API Enviroment 
{
protected:
	TArray<EnviromentTile*> m_tiles;
	TSharedPtr<FRandomStream> m_randomGenerator;
	TArray<FBox> m_boundingVolumes;

	uint32 m_minAttractors, m_maxAttractors;

public:
	Enviroment(TSharedPtr<FRandomStream> randomGenerator, uint32 minAttractors, uint32 maxAttractors, TArray<FBox> boundingVolumes);
	~Enviroment(void);

	EnviromentTile* GetTile(FVector position);
	EnviromentTile* GetTile(float x, float y, float z);
	FVector GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, TArray<Bud*>  closeBuds, TArray<FBox> obstacles);
	void DeactivateAttractorsInSphere(FVector position, double radius);
	void DeactivateAttractorsInBox(FBox box);
};
