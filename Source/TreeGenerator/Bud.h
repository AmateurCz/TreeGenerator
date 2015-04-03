// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * 
 */
class TREEGENERATOR_API Bud
{
public:
	Bud(FVector position, FVector axis);
	Bud(FVector position, FVector axis, Bud* parrent);
	~Bud();

	bool isDead;
	Bud* allignedBud, *secondaryAllignedBud, *parentBud;
	FVector position, axis, originalAxis;
	float diameter;
	float resource;
	float lenght;
	uint32 weight;
};
