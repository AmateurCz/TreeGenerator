// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define ENVIROMENT_TILE_SIZE 32
/**
 * 
 */
class TREEGENERATOR_API Array3DBase
{
protected:
	FVector begin;
	uint32 sizeX, sizeY, sizeZ, stepX, stepY, stepZ;
	uint32 To3DCoord(uint32 x, uint32 y, uint32 z, uint32 sizeX, uint32 sizeY);
	void WorldCoordToArrayCoord(FVector position, int32& arrayX, int32& arrayY, int32& arrayZ);
	void WorldCoordToArrayCoord(double x, double y, double z, int32& arrayX, int32& arrayY, int32& arrayZ);

public:
	Array3DBase(uint32 stepX, uint32 stepY, uint32 stepZ);
	virtual ~Array3DBase();
};
