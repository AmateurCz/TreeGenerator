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
	FVector GetPosition() const { return m_begin; }
};

inline bool operator< (const EnviromentTile& lhs, const EnviromentTile& rhs){ 
	if (lhs.GetPosition().X == rhs.GetPosition().X){
		if (lhs.GetPosition().Y == rhs.GetPosition().Y){
			return lhs.GetPosition().Z < rhs.GetPosition().Z;
		}
		return lhs.GetPosition().Y < rhs.GetPosition().Y;
	}
	return lhs.GetPosition().X < rhs.GetPosition().X;
}
