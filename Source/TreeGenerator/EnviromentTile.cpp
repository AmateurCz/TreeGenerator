// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "EnviromentTile.h"

bool RayIntersectsBox(FVector origin, FVector axis, FBox box, float distance){

	FVector dirfrac;

	float t1 = (box.Min.X - origin.X)*axis.X;
	float t2 = (box.Max.X - origin.X)*axis.X;
	float t3 = (box.Min.Y - origin.Y)*axis.Y;
	float t4 = (box.Max.Y - origin.Y)*axis.Y;
	float t5 = (box.Min.Z - origin.Z)*axis.Z;
	float t6 = (box.Max.Z - origin.Z)*axis.Z;

	float tmin = FMath::Max(FMath::Max(FMath::Min(t1, t2), FMath::Min(t3, t4)), FMath::Min(t5, t6));
	float tmax = FMath::Min(FMath::Min(FMath::Max(t1, t2), FMath::Max(t3, t4)), FMath::Max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
	if (tmax < 0)
	{
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		return false;
	}
	return tmin <= distance;
}

EnviromentTile::EnviromentTile(FVector begin)
{
	this->m_begin = begin;
	isInitialized = false;
}


EnviromentTile::~EnviromentTile(void)
{
	this->m_attractors.Reset();
	m_attractors.Shrink();
}

void EnviromentTile::Initialize(TSharedPtr<FRandomStream> randomGenerator, uint32 minAttractors, uint32 maxAttractors, TArray<FBox> boundingVolumes){
	int32 attractorCount = randomGenerator->RandRange(minAttractors, maxAttractors);
	m_attractors.Init(0);
	for (int i = 0; i != attractorCount; ++i){
		FVector pos = m_begin + FVector(randomGenerator->FRandRange(0.0f, ENVIROMENT_TILE_SIZE),
			randomGenerator->FRandRange(0.0f, ENVIROMENT_TILE_SIZE),
			randomGenerator->FRandRange(0.0f, ENVIROMENT_TILE_SIZE));
		if (boundingVolumes.Num() == 0)
			this->m_attractors.Add(pos);
		else{
			for (TIndexedContainerIterator<TArray<FBox>, FBox, int32>iterator = boundingVolumes.CreateIterator(); iterator; iterator++){
				if (iterator->IsInside(FVector(pos.X, pos.Z, pos.Y))){
					this->m_attractors.Add(pos);
					return;
				}
			}
		}
	}
	isInitialized = true;
}

void EnviromentTile::DeactivateAttractorsInSphere(FVector position, double radius){

	FVector min = position + FVector(-radius, -radius, -radius);
	FVector max = position + FVector(radius, radius, radius);

	if (min.X <= this->m_begin.X &&
		min.Y <= this->m_begin.X &&
		min.Z <= this->m_begin.X &&
		max.X >= this->m_begin.X + ENVIROMENT_TILE_SIZE &&
		max.Y >= this->m_begin.Y + ENVIROMENT_TILE_SIZE &&
		max.Z >= this->m_begin.Z + ENVIROMENT_TILE_SIZE){
		m_attractors.Reset();
		m_attractors.Shrink();
		return;
	}
	TIndexedContainerIterator<TArray<FVector>, FVector, int32> iterator = this->m_attractors.CreateIterator();

	for (iterator.Reset(); iterator; iterator++){
		FVector direction;
		float lenght;
		(m_attractors[iterator.GetIndex()] - position).ToDirectionAndLength(direction, lenght);
		if (lenght <= radius){
			m_attractors.RemoveAt(iterator.GetIndex(), false);
		}
	}
	m_attractors.Shrink();
}
void EnviromentTile::DeactivateAttractorsInBox(FBox box){
	if (box.Min.X <= this->m_begin.X &&
		box.Min.Y <= this->m_begin.X &&
		box.Min.Z <= this->m_begin.X &&
		box.Max.X >= this->m_begin.X + ENVIROMENT_TILE_SIZE &&
		box.Max.Y >= this->m_begin.Y + ENVIROMENT_TILE_SIZE &&
		box.Max.Z >= this->m_begin.Z + ENVIROMENT_TILE_SIZE){
		m_attractors.Reset();
		m_attractors.Shrink();
		return;
	}

	TIndexedContainerIterator<TArray<FVector>, FVector, int32> iterator = this->m_attractors.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		FVector pos = m_attractors[iterator.GetIndex()];
		if (box.IsInside(FVector(pos.X, pos.Z, pos.Y))){
			m_attractors.RemoveAt(iterator.GetIndex(), false);
		}
	}
	m_attractors.Shrink();
}

FVector EnviromentTile::GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, TArray<Bud*>  closeBuds, TArray<FBox> obstacles){
	FVector v = FVector(0, 0, 0);
	axis.Normalize();	
	TIndexedContainerIterator<TArray<FVector>, FVector, int32> iterator = this->m_attractors.CreateIterator();
	for (iterator.Reset(); iterator; iterator++){
		FVector attractorPosition = m_attractors[iterator.GetIndex()];
		if (attractorPosition.Y <= 0)
			continue;
		if (FVector::Dist(attractorPosition, position) <= radius){
			bool add = true;
			for (uint32 z = 0; z != closeBuds.Num(); ++z){
				if (FVector::Dist(attractorPosition, position) > FVector::Dist(attractorPosition, closeBuds[z]->position)){

					closeBuds[z]->axis.Normalize();
					FVector secondDir = attractorPosition - closeBuds[z]->position;
					secondDir.Normalize();
					add = !(abs(acos(FVector::DotProduct(closeBuds[z]->axis, secondDir))) < angle);
				}
			}
			FVector direction = attractorPosition - position;
			direction.Normalize();
			add = add && abs(acos(FVector::DotProduct(axis, direction))) < angle;
			for (TIndexedContainerIterator<TArray<FBox>, FBox, int32>iterator = obstacles; iterator; iterator++){
				add = add && !RayIntersectsBox(FVector(position.X, position.Z, position.Y), FVector(direction.X, direction.Z, direction.Y), obstacles[iterator.GetIndex()], FVector::Dist(attractorPosition ,position));
			}
			if (add){
				v += direction;
			}
		}
	}
	return v;
}