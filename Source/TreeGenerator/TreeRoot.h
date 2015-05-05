// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "TreeRoot.generated.h"

/**
 * 
 */
UCLASS()
class TREEGENERATOR_API ATreeRoot : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	ATreeRoot(const FObjectInitializer& ObjectInitializer);
	
	virtual void BeginPlay() override;
};
