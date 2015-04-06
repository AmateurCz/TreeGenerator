// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Bud.h"

#define ENVIROMENT_TILE_SIZE 64

/**
 * 
 */
class TREEGENERATOR_API EnviromentTile
{
protected:
	TArray<FVector> m_attractors;
	FVector m_begin;
	bool isInitialized;
	int32 value;
	void DestroyAttractors();

public:
	EnviromentTile(FVector begin);
	~EnviromentTile(void);

	bool IsInitialized() { return isInitialized; }
	void Initialize(TSharedPtr<FRandomStream> randomGenerator, uint32 minAttractors, uint32 maxAttractors, TArray<FBox> boundingVolumes);
	void Reset();

	void DeactivateAttractorsInSphere(FVector position, double radius);
	void DeactivateAttractorsInBox(FBox box);
	FVector GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, TArray<Bud*> closeBuds);
	int32 GetValue() const { return value; }
};

inline bool operator< (const EnviromentTile& lhs, const EnviromentTile& rhs){ return lhs.GetValue() < rhs.GetValue(); }
