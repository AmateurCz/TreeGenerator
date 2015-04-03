// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "EnviromentTile.h"


int compareAttractor(const FVector a, const FVector b){
	return (int)((a.X + a.Y * ENVIROMENT_TILE_SIZE + a.Z * ENVIROMENT_TILE_SIZE * ENVIROMENT_TILE_SIZE)
		- (b.X + b.Y * ENVIROMENT_TILE_SIZE + b.Z * ENVIROMENT_TILE_SIZE * ENVIROMENT_TILE_SIZE));
}

int compareAttractor(const void* a, const void* b){
	FVector* aVec = (FVector*)a;
	FVector* bVec = (FVector*)b;
	return compareAttractor(*aVec, *bVec);
}


EnviromentTile::EnviromentTile(FVector begin, FRandomStream* randomGenerator) : Array3DBase(1, 1, 1)
{
	this->begin = begin;
	this->randomGenerator = randomGenerator;
	this->attractorActiveArray = NULL;
	this->attractorArray = NULL;
	this->attractorCount = 0;
	this->allAtractorsUsed = false;
	this->sizeX = this->sizeY = this->sizeZ = ENVIROMENT_TILE_SIZE;
}


EnviromentTile::~EnviromentTile(void)
{
	DestroyAttractors();
}

void EnviromentTile::DestroyAttractors(){
	if (this->attractorActiveArray){
		delete[] this->attractorActiveArray;
		this->attractorActiveArray = NULL;
	}

	if (this->attractorArray){
		delete[] this->attractorArray;
		this->attractorArray = NULL;
	}
}

void EnviromentTile::Initialize(uint32 minAttractors, uint32 maxAttractors){
	//uint32 arrSize = ENVIROMENT_TILE_SIZE * ENVIROMENT_TILE_SIZE * ENVIROMENT_TILE_SIZE;
	attractorCount = (uint32)randomGenerator->RandRange(minAttractors, maxAttractors);
	allAtractorsUsed = false;
	attractorsUsed = 0;
}

void EnviromentTile::GenerateAttractors(){
	for (uint32 i = 0; i != attractorCount; ++i)
		this->attractorArray[i] = begin + FVector(this->randomGenerator->FRandRange(0.0f, ENVIROMENT_TILE_SIZE),
		this->randomGenerator->FRandRange(0.0f, ENVIROMENT_TILE_SIZE),
		this->randomGenerator->FRandRange(0.0f, ENVIROMENT_TILE_SIZE));

	qsort(attractorArray, attractorCount, sizeof(FVector), compareAttractor);
}

void EnviromentTile::DeactivateAttractorsInSphere(FVector position, double radius){
	if (allAtractorsUsed)
		return;

	FVector min = position + FVector(-radius, -radius, -radius);
	FVector max = position + FVector(radius, radius, radius);

	if (min.X <= this->begin.X &&
		min.Y <= this->begin.X &&
		min.Z <= this->begin.X &&
		max.X >= this->begin.X + ENVIROMENT_TILE_SIZE &&
		max.Y >= this->begin.Y + ENVIROMENT_TILE_SIZE &&
		max.Z >= this->begin.Z + ENVIROMENT_TILE_SIZE){
		allAtractorsUsed = true;
		DestroyAttractors();
		return;
	}

	if (!this->attractorArray)
		ResetAttractors();

	for (uint32 i = 0; i != attractorCount; ++i){
		FVector direction;
		float lenght;
		(attractorArray[i] - position).ToDirectionAndLength(direction, lenght);
		if (lenght <= radius){
			if (++attractorsUsed == attractorCount){
				allAtractorsUsed = true;
				DestroyAttractors();
				return;
			}
			attractorActiveArray[i] = false;
		}
	}
}
void EnviromentTile::DeactivateAttractorsInBox(FVector min, FVector max){
	if (min.X <= this->begin.X &&
		min.Y <= this->begin.X &&
		min.Z <= this->begin.X &&
		max.X >= this->begin.X + ENVIROMENT_TILE_SIZE &&
		max.Y >= this->begin.Y + ENVIROMENT_TILE_SIZE &&
		max.Z >= this->begin.Z + ENVIROMENT_TILE_SIZE){
		allAtractorsUsed = true;
		DestroyAttractors();
		return;
	}
	if (!this->attractorArray)
		ResetAttractors();

}

FVector EnviromentTile::GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, Bud** closeBuds, uint32 closeBudCount){
	FVector v = FVector(0, 0, 0);
	if (allAtractorsUsed)
		return v;

	if (!this->attractorArray)
		ResetAttractors();

	axis.Normalize();
	for (uint32 i = 0; i != attractorCount; ++i){
		if (attractorArray[i].Y <= 0)
			continue;
		if (attractorActiveArray[i] && FVector::Dist(attractorArray[i] , position) <= radius){
			bool add = true;
			for (uint32 z = 0; z != closeBudCount; ++z){
				if (FVector::Dist(attractorArray[i], position) > FVector::Dist(attractorArray[i], closeBuds[z]->position)){

					closeBuds[z]->axis.Normalize();
					FVector secondDir = attractorArray[i] - closeBuds[z]->position;
					secondDir.Normalize();
					add = !(abs(acos( FVector::DotProduct(closeBuds[z]->axis, secondDir))) < angle);
				}
			}
			FVector direction = attractorArray[i] - position;
			direction.Normalize();
			add = add && abs(acos(FVector::DotProduct(axis, direction))) < angle;
			if (add){
				v += direction;
			}
		}
	}

	return v;
}

void EnviromentTile::ResetAttractors(){
	allAtractorsUsed = false;
	attractorsUsed = 0;
	if (!this->attractorActiveArray){
		this->attractorActiveArray = new bool[this->attractorCount];
		this->attractorArray = new FVector[this->attractorCount];
	}
	for (uint32 i = 0; i != attractorCount; ++i)
		this->attractorActiveArray[i] = true;
	GenerateAttractors();
}
uint32 EnviromentTile::GetAttractorCount(){
	return this->attractorCount;
}

void EnviromentTile::GetAttractorsAsVertices(FVector* vertArray, uint32 offset){
	for (uint32 i = 0; i != attractorCount; ++i){
		vertArray[i + offset] = this->attractorArray[i];
	}
}

void EnviromentTile::ResetAll(){
	ResetAttractors();
}
