// Fill out your copyright notice in the Description page of Project Settings.

#include "TreeGenerator.h"
#include "TreeRoot.h"

ATreeRoot::ATreeRoot(const FObjectInitializer& ObjectInitializer) : AStaticMeshActor(ObjectInitializer){
	//mesh = StaticMesh'/Game/Meshes/treeIcon.treeIcon'
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("StaticMesh'/Game/Meshes/treeIcon.treeIcon'"));
	if (Mesh.Succeeded() && Mesh.Object)
		GetStaticMeshComponent()->Mobility = EComponentMobility::Movable;
		GetStaticMeshComponent()->SetStaticMesh(Mesh.Object);
}

void ATreeRoot::BeginPlay(){
	StaticMeshComponent->SetStaticMesh(NULL);
}


