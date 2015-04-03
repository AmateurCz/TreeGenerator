// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "Array3DBase.h"

Array3DBase::Array3DBase(uint32 stepX, uint32 stepY, uint32 stepZ)
{
	this->stepX = stepX;
	this->stepY = stepY;
	this->stepZ = stepZ;
}


Array3DBase::~Array3DBase()
{
}

uint32 Array3DBase::To3DCoord(uint32 x, uint32 y, uint32 z, uint32 sizeX, uint32 sizeY){
	return x + y * sizeX + z * sizeX*sizeY;
}

void Array3DBase::WorldCoordToArrayCoord(FVector position, int32& arrayX, int32& arrayY, int32& arrayZ){
	WorldCoordToArrayCoord(position.X, position.Y, position.Z, arrayX, arrayY, arrayZ);
}
void Array3DBase::WorldCoordToArrayCoord(double x, double y, double z, int32& arrayX, int32& arrayY, int32& arrayZ){
	arrayX = (int32)floor((x - this->begin.X) / stepX);
	arrayY = (int32)floor((y - this->begin.Y) / stepY);
	arrayZ = (int32)floor((z - this->begin.Z) / stepZ);
}
