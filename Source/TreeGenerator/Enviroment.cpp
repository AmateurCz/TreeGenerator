// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "Enviroment.h"


Enviroment::Enviroment(FRandomStream * randomGenerator, uint32 minAttractors, uint32 maxAttractors) : Array3DBase(ENVIROMENT_TILE_SIZE, ENVIROMENT_TILE_SIZE, ENVIROMENT_TILE_SIZE)
{
	enviroment = NULL;
	sizeX = sizeY = sizeZ = 0;
	this->randomGenerator = randomGenerator;
	this->minAttractors = minAttractors;
	this->maxAttractors = maxAttractors;
	this->begin = FVector(0, 0, 0);
}


Enviroment::~Enviroment(void)
{
	DeleteEnviroment();
}

void Enviroment::DeleteEnviroment(){
	if (enviroment)
	{
		for (unsigned int i = 0; i != sizeX * sizeY * sizeZ; ++i){
			if (enviroment[i])
				delete enviroment[i];
		}
		delete[] enviroment;
	}
	this->begin = FVector(0, 0, 0);
	sizeX = sizeY = sizeZ = 0;
}

void Enviroment::DeactivateAttractorsInSphere(FVector position, double radius){
	int32 minX, maxX,
		minY, maxY,
		minZ, maxZ;

	WorldCoordToArrayCoord(position - FVector(radius, radius, radius), minX, minY, minZ);
	WorldCoordToArrayCoord(position + FVector(radius, radius, radius), maxX, maxY, maxZ);

	if (0 > minX ||
		0 > minY ||
		0 > minZ ||
		(int)this->sizeX <= maxX ||
		(int)this->sizeY <= maxY ||
		(int)this->sizeZ <= maxZ){
		ResizeTiles(minX, minY, minZ, maxX, maxY, maxZ);
		WorldCoordToArrayCoord(position - FVector(radius, radius, radius), minX, minY, minZ);
		WorldCoordToArrayCoord(position + FVector(radius, radius, radius), maxX, maxY, maxZ);
	}


	for (int x = minX; x <= maxX; ++x){
		for (int y = minY; y <= maxY; ++y){
			for (int z = minZ; z <= maxZ; ++z){
				EnviromentTile* tile = GetTile(x, y, z);
				if (!tile)
					CreateTile(x, y, z);
				tile = GetTile(x, y, z);
				tile->DeactivateAttractorsInSphere(position, radius);
			}
		}
	}
}

FVector Enviroment::GetDirectionFromCone(FVector position, FVector axis, double radius, double angle, Bud** closeBuds, uint32 closeBudCount){

	int32 minX, maxX,
		minY, maxY,
		minZ, maxZ;

	WorldCoordToArrayCoord(position - FVector(radius, radius, radius), minX, minY, minZ);
	WorldCoordToArrayCoord(position + FVector(radius, radius, radius), maxX, maxY, maxZ);

	if (0 > minX ||
		0 > minY ||
		0 > minZ ||
		(int)this->sizeX <= maxX ||
		(int)this->sizeY <= maxY ||
		(int)this->sizeZ <= maxZ){
		ResizeTiles(minX, minY, minZ, maxX, maxY, maxZ);
		WorldCoordToArrayCoord(position - FVector(radius, radius, radius), minX, minY, minZ);
		WorldCoordToArrayCoord(position + FVector(radius, radius, radius), maxX, maxY, maxZ);
	}

	FVector v = FVector(0, 0, 0);

	for (int x = minX; x <= maxX; ++x){
		for (int y = minY; y <= maxY; ++y){
			for (int z = minZ; z <= maxZ; ++z){
				EnviromentTile* tile = GetTile(x, y, z);
				if (!tile){
					CreateTile(x, y, z);
				}
				tile = GetTile(x, y, z);
				v += tile->GetDirectionFromCone(position, axis, radius, angle, closeBuds, closeBudCount);
			}
		}
	}
	v.Normalize();
	return v;
}

void Enviroment::DeactivateAttractorsInBox(FVector min, FVector max){
	int32 minX, maxX,
		minY, maxY,
		minZ, maxZ;

	WorldCoordToArrayCoord(min, minX, minY, minZ);
	WorldCoordToArrayCoord(max, maxX, maxY, maxZ);

	if (0 > minX ||
		0 > minY ||
		0 > minZ ||
		(int)this->sizeX <= maxX ||
		(int)this->sizeY <= maxY ||
		(int)this->sizeZ <= maxZ){
		ResizeTiles(minX, minY, minZ, maxX, maxY, maxZ);
		WorldCoordToArrayCoord(min, minX, minY, minZ);
		WorldCoordToArrayCoord(max, maxX, maxY, maxZ);
	}


	for (int x = minX; x <= maxX; ++x){
		for (int y = minY; y <= maxY; ++y){
			for (int z = minZ; z <= maxZ; ++z){
				EnviromentTile* tile = GetTile(x, y, z);
				if (!tile)
					CreateTile(x, y, z);
				tile = GetTile(x, y, z);
				tile->DeactivateAttractorsInBox(min, max);
			}
		}
	}
}

void Enviroment::ResizeTiles(int x1, int y1, int z1, int x2, int y2, int z2){

	x1 = FMath::Min(x1, 0);
	y1 = FMath::Min(y1, 0);
	z1 = FMath::Min(z1, 0);
	x2 = FMath::Max(x2, (int)this->sizeX);
	y2 = FMath::Max(y2, (int)this->sizeY);
	z2 = FMath::Max(z2, (int)this->sizeZ);
	x2 += 1;
	y2 += 1;
	z2 += 1;
	int newSizeX = x2 - x1,
		newSizeY = y2 - y1,
		newSizeZ = z2 - z1;

	EnviromentTile ** newTiles = new EnviromentTile*[newSizeX*newSizeY*newSizeZ];
	for (int i = 0; i != newSizeX * newSizeY * newSizeZ; ++i)
		newTiles[i] = NULL;

	if (enviroment == NULL){
		this->sizeX = newSizeX;
		this->sizeY = newSizeY;
		this->sizeZ = newSizeZ;
		this->begin.X = x1 * ENVIROMENT_TILE_SIZE;
		this->begin.Y = y1 * ENVIROMENT_TILE_SIZE;
		this->begin.Z = z1 * ENVIROMENT_TILE_SIZE;
		this->enviroment = newTiles;
		return;
	}

	for (int x = 0; x < ((int32)this->sizeX); ++x){
		for (int y = 0; y < ((int32)this->sizeY); ++y){
			for (int z = 0; z < ((int32)this->sizeZ); ++z){
				newTiles[To3DCoord((x - x1), (y - y1), (z - z1), newSizeX, newSizeY)] = this->enviroment[To3DCoord(x, y, z, this->sizeX, this->sizeY)];
			}
		}
	}
	this->sizeX = newSizeX;
	this->sizeY = newSizeY;
	this->sizeZ = newSizeZ;
	this->begin.X += x1 * ENVIROMENT_TILE_SIZE;
	this->begin.Y += y1 * ENVIROMENT_TILE_SIZE;
	this->begin.Z += z1 * ENVIROMENT_TILE_SIZE;

	delete[] this->enviroment;
	this->enviroment = newTiles;
}


EnviromentTile* Enviroment::GetTile(FVector position){
	int32 x, y, z;
	WorldCoordToArrayCoord(position.X, position.Y, position.Z, x, y, z);
	return GetTile(x, y, z);
}
EnviromentTile* Enviroment::GetTile(uint32 x, uint32 y, uint32 z){
	if (x < 0 ||
		y < 0 ||
		z < 0 ||
		x >= (int)this->sizeX ||
		y >= (int)this->sizeY ||
		z >= (int)this->sizeZ){
		return NULL;
	}
	return this->enviroment[To3DCoord(x, y, z, this->sizeX, this->sizeY)];
}

EnviromentTile* Enviroment::CreateTile(int x, int y, int z){
	if (x < 0 ||
		y < 0 ||
		z < 0 ||
		x >= (int)this->sizeX ||
		y >= (int)this->sizeY ||
		z >= (int)this->sizeZ){
		return NULL;
	}

	EnviromentTile* tile = new EnviromentTile(FVector(x*ENVIROMENT_TILE_SIZE, y*ENVIROMENT_TILE_SIZE, z*ENVIROMENT_TILE_SIZE) + this->begin,
		this->randomGenerator);
	tile->Initialize(minAttractors, maxAttractors);

	this->enviroment[To3DCoord(x, y, z, this->sizeX, this->sizeY)] = tile;
	return tile;
}

EnviromentTile* Enviroment::CreateTile(FVector position){
	int32 arrX, arrY, arrZ;
	WorldCoordToArrayCoord(position, arrX, arrY, arrZ);
	return CreateTile(arrX, arrY, arrZ);
}


//PolygonObject* Enviroment::GetAttractorsAsVertices(){
//	uint32 attractorCount = 0;
//	for (uint32 i = 0; i != sizeX*sizeY*sizeZ; ++i){
//		if (this->enviroment[i])
//			attractorCount = attractorCount + this->enviroment[i]->GetAttractorCount();
//	}
//	PolygonObject* polyObj = PolygonObject::Alloc(attractorCount, 0);
//
//	attractorCount = 0;
//	for (uint32 i = 0; i != sizeX*sizeY*sizeZ; ++i){
//		if (this->enviroment[i]){
//			this->enviroment[i]->GetAttractorsAsVertices(polyObj->GetPointW(), attractorCount);
//			attractorCount += this->enviroment[i]->GetAttractorCount();
//		}
//	}
//	return polyObj;
//}

void Enviroment::ResetAll(){
	//DeleteEnviroment();
}